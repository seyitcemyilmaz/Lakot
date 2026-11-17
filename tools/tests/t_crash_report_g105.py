import io, os, re, shutil, subprocess, sys, tempfile, threading, time, queue, contextlib, pathlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lakotnet

SRC = lakotnet.ROOT / "src" / "utils" / "CrashTrigger.cpp"
LINES = SRC.read_text(encoding="utf-8").splitlines()
LINE_RE = re.compile(r"^(CRASH|ERROR) (\S+) (\S+) (.*) at (\S+):(\d+) in (.+) \(thread ([^:()]+):(\d+)\)$")
FAILURES = []


def fail(pMessage):
    FAILURES.append(pMessage)
    print("FAIL", pMessage, flush=True)


def find_line(pText):
    tHits = [i + 1 for i, l in enumerate(LINES) if pText in l]
    if len(tHits) != 1:
        raise RuntimeError(f"source anchor {pText!r} matches {len(tHits)} lines")
    return tHits[0]


def function_range(pSignature):
    tStart = find_line(pSignature)
    tDepth = 0
    for i in range(tStart - 1, len(LINES)):
        tDepth += LINES[i].count("{") - LINES[i].count("}")
        if tDepth == 0 and "}" in LINES[i]:
            return tStart, i + 1
    raise RuntimeError("function end not found")


RECURSE = function_range("int recurse(int pDepth)")
THROW_RT = 'throw std::runtime_error("LAKOT_CRASH_TEST throw");'
CASES = [
    ("null", ("access-violation",), "*tTarget = 1;"),
    ("divzero", ("divide-by-zero",), "10 / tDivisor"),
    ("stackoverflow", ("stack-overflow",), None),
    ("throw", ("uncaught-exception",), THROW_RT),
    ("throw@thread", ("uncaught-exception",), THROW_RT),
    ("terminate", ("terminate",), "std::terminate();"),
    ("abort", ("abort",), "std::abort();"),
    ("purecall", ("pure-virtual-call",), "tSelf->callPure();"),
    ("invalidparam", ("invalid-parameter", "crt-assert"), "strcpy_s(tSmall"),
    ("assert", ("assert",), "assert(sKeepRecursing"),
]


def server_env(pCrashDir, pKind):
    tEnv = {**os.environ, "LAKOT_CRASH_DIR": pCrashDir, "LAKOT_CRASH_TEST": pKind,
            "LAKOT_SERVER_PORT": str(lakotnet.PORT), "LAKOT_SERVER_BIND": "127.0.0.1"}
    tEnv.setdefault("LAKOT_DB_PASSWORD", "1")
    tEnv.pop("LAKOT_TEST_LOG_DIR", None)
    return tEnv


def parse(pLine, pLabel):
    tMatch = LINE_RE.match(pLine or "")
    if not tMatch:
        fail(f"{pLabel}: line 1 does not match format: {pLine!r}")
    return tMatch


def run_crash(pBin, pKind, pExpectedKinds, pAnchor):
    tDir = tempfile.mkdtemp(prefix="lakot-crash-")
    try:
        tStart = time.time()
        tProc = subprocess.run([os.path.join(pBin, "LakotServer.exe")], cwd=pBin, env=server_env(tDir, pKind),
                               stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, errors="replace", timeout=120)
        tWall = time.time() - tStart
        if tProc.returncode == 0:
            fail(f"{pKind}: exit code is 0")
        tDirs = [d for d in pathlib.Path(tDir).iterdir() if d.is_dir()]
        if len(tDirs) != 1:
            fail(f"{pKind}: expected exactly one crash dir, found {len(tDirs)}\n{tProc.stdout[-1500:]}")
            return
        tCrash = tDirs[0]
        tSummaryPath = tCrash / "summary.txt"
        if not tSummaryPath.is_file():
            fail(f"{pKind}: summary.txt missing")
            return
        tFirst = tSummaryPath.read_text(encoding="utf-8", errors="replace").splitlines()[0]
        tMatch = parse(tFirst, pKind)
        if tMatch:
            if tMatch.group(1) != "CRASH" or tMatch.group(2) != "LakotServer":
                fail(f"{pKind}: wrong prefix/exe: {tFirst}")
            if tMatch.group(3) not in pExpectedKinds:
                fail(f"{pKind}: kind {tMatch.group(3)!r} not in {pExpectedKinds}: {tFirst}")
            tFile, tLine = tMatch.group(5), int(tMatch.group(6))
            if tFile != "src/utils/CrashTrigger.cpp":
                fail(f"{pKind}: file is {tFile!r}, expected src/utils/CrashTrigger.cpp: {tFirst}")
            if pAnchor:
                tExpected = find_line(pAnchor)
                if tLine != tExpected:
                    fail(f"{pKind}: line {tLine}, expected {tExpected}: {tFirst}")
            else:
                if "recurse" not in tMatch.group(7):
                    fail(f"{pKind}: function {tMatch.group(7)!r} does not contain recurse")
                if not (RECURSE[0] <= tLine <= RECURSE[1]):
                    fail(f"{pKind}: line {tLine} outside recurse {RECURSE}")
            tWantThread = "crash-test" if pKind.endswith("@thread") else "main"
            if tMatch.group(8) != tWantThread:
                fail(f"{pKind}: thread {tMatch.group(8)!r}, expected {tWantThread!r}")
            if pKind.startswith("throw") and "std::runtime_error" not in tMatch.group(4):
                fail(f"{pKind}: description lacks std::runtime_error: {tMatch.group(4)!r}")
        tReport = tCrash / "report.txt"
        if not tReport.is_file():
            fail(f"{pKind}: report.txt missing")
        else:
            tText = tReport.read_text(encoding="utf-8", errors="replace")
            for tSection in ("== stack", "== modules ==", "== log"):
                if tSection not in tText:
                    fail(f"{pKind}: report.txt lacks section {tSection!r}")
        tDump = tCrash / "minidump.dmp"
        if not tDump.is_file():
            fail(f"{pKind}: minidump.dmp missing")
        elif tDump.stat().st_size <= 10 * 1024:
            fail(f"{pKind}: minidump.dmp too small ({tDump.stat().st_size})")
        if tFirst not in tProc.stdout.splitlines():
            fail(f"{pKind}: stdout does not contain the summary line 1")
        if tWall > 60:
            fail(f"{pKind}: crash took {tWall:.0f}s")
        print(f"ok {pKind} ({tWall:.1f}s): {tFirst}", flush=True)
    finally:
        shutil.rmtree(tDir, ignore_errors=True)


def run_error(pBin):
    tDir = tempfile.mkdtemp(prefix="lakot-crash-")
    tProc = subprocess.Popen([os.path.join(pBin, "LakotServer.exe")], cwd=pBin, env=server_env(tDir, "error"),
                             stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             text=True, errors="replace")
    tQueue = queue.Queue()

    def pump():
        for tLine in tProc.stdout:
            tQueue.put(tLine.rstrip())
        tQueue.put(None)

    threading.Thread(target=pump, daemon=True).start()
    tFound = None
    tDeadline = time.time() + 60
    try:
        while time.time() < tDeadline:
            try:
                tLine = tQueue.get(timeout=1)
            except queue.Empty:
                continue
            if tLine is None:
                break
            if tLine.startswith("ERROR "):
                tFound = tLine
                break
        if not tFound:
            fail("error: no ERROR line within 60 s")
            return
        tMatch = parse(tFound, "error")
        if tMatch:
            if tMatch.group(3) != "caught-error":
                fail(f"error: kind {tMatch.group(3)!r}: {tFound}")
            if "std::out_of_range" not in tFound:
                fail(f"error: type std::out_of_range missing: {tFound}")
            tThrowLine = find_line('throw std::out_of_range("LAKOT_CRASH_TEST error");')
            if tMatch.group(5) != "src/utils/CrashTrigger.cpp" or int(tMatch.group(6)) != tThrowLine:
                fail(f"error: throw site wrong (expected line {tThrowLine}): {tFound}")
            if not re.search(r"caught at \S*CrashTrigger\.cpp:%d\b" % find_line("ErrorReport::report("), tFound):
                fail(f"error: caught-at site wrong: {tFound}")
            if " thrown at " not in tFound:
                fail(f"error: 'thrown at' label missing: {tFound}")
        print(f"ok error: {tFound}", flush=True)
    finally:
        try:
            tProc.kill()
            tProc.wait(timeout=10)
        except Exception:
            pass
        shutil.rmtree(tDir, ignore_errors=True)


def run_harness(pBin):
    tDir = tempfile.mkdtemp(prefix="lakot-crash-")
    tOld = os.environ.pop("LAKOT_TEST_LOG_DIR", None)
    try:
        tServer = lakotnet.Server(pBin)
        tServer.proc = subprocess.Popen([os.path.join(pBin, "LakotServer.exe")], cwd=pBin, env=server_env(tDir, "null"),
                                        stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                        text=True, errors="replace")
        tBuffer = io.StringIO()
        with contextlib.redirect_stdout(tBuffer):
            tServer._pump()
        tCrashLines = [l for l in tBuffer.getvalue().splitlines() if l.startswith("CRASH ")]
        if len(tCrashLines) != 1:
            fail(f"harness: expected one CRASH line in test output, got {tCrashLines}")
        elif not LINE_RE.match(tCrashLines[0]) or "CrashTrigger.cpp" not in tCrashLines[0]:
            fail(f"harness: CRASH line malformed: {tCrashLines[0]!r}")
        else:
            print(f"ok harness prints: {tCrashLines[0]}", flush=True)
    finally:
        if tOld is not None:
            os.environ["LAKOT_TEST_LOG_DIR"] = tOld
        shutil.rmtree(tDir, ignore_errors=True)


def main():
    tBin = lakotnet.resolve_bin(None)
    if not os.path.isfile(os.path.join(tBin, "LakotServer.exe")):
        print("LakotServer.exe not found in", tBin)
        return 1
    tDebug = "debug" in tBin.lower() or "testruns" in tBin.lower()
    for tKind, tExpected, tAnchor in CASES:
        if tKind == "assert" and not tDebug:
            continue
        try:
            run_crash(tBin, tKind, tExpected, tAnchor)
        except Exception as tError:
            fail(f"{tKind}: test error {tError!r}")
    for tFn in (run_error, run_harness):
        try:
            tFn(tBin)
        except Exception as tError:
            fail(f"{tFn.__name__}: test error {tError!r}")
    print(f"{len(FAILURES)} failure(s)" if FAILURES else "crash report checks passed")
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(main())
