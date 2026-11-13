import ast
import os
import queue
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
SCRIPTS = next(ast.literal_eval(n.value) for n in ast.parse((HERE / "run_all.py").read_text(encoding="utf-8")).body
               if isinstance(n, ast.Assign) and getattr(n.targets[0], "id", "") == "SCRIPTS")

SERIAL = ["run_all", "t_login_pipeline_g54", "t_login_limiter_g55", "t_register_limit_g58", "t_speed_y1", "t_save_retry_y3"]
BASE_PORT = 12400
WORKERS = int(os.environ.get("LAKOT_TEST_WORKERS") or 6)
LOG_DIR = Path(os.environ.get("LAKOT_TEST_LOG_DIR") or tempfile.mkdtemp(prefix="lakot_tests_"))


SLOTS = queue.Queue()


def run_pooled(pName):
    tSlot = SLOTS.get()
    try:
        return run(pName, tSlot)
    finally:
        SLOTS.put(tSlot)


def run(pName, pSlot):
    tEnv = {**os.environ, "LAKOT_TEST_PORT": str(BASE_PORT + pSlot), "LAKOT_TEST_RUN_DIR": str(HERE / f".run_p{pSlot}")}
    if pName == "run_all":
        tEnv["LAKOT_CORE_ONLY"] = "1"
    tLog = LOG_DIR / f"{pName}.log"
    tStart = time.time()
    with open(tLog, "w", encoding="utf-8", errors="replace") as tFile:
        tCode = subprocess.call([sys.executable, str(HERE / f"{pName}.py")], cwd=HERE.parent.parent, env=tEnv, stdout=tFile, stderr=subprocess.STDOUT)
    tSeconds = time.time() - tStart
    print(f"{'PASS' if tCode == 0 else 'FAIL'}  {pName}  ({tSeconds:.0f}s)", flush=True)
    return pName, tCode, tSeconds


def main():
    tParallel = [n for n in SCRIPTS if n not in SERIAL]
    tStart = time.time()
    tResults = []
    for tSlot in range(WORKERS):
        SLOTS.put(tSlot)
    with ThreadPoolExecutor(WORKERS) as tPool:
        tFutures = [tPool.submit(run_pooled, n) for n in tParallel]
        tResults += [f.result() for f in tFutures]
    for tName in SERIAL:
        tResults.append(run(tName, 0))
    tFailed = [r for r in tResults if r[1] != 0]
    for tName, _, _ in tFailed:
        print(f"\n--- {tName} (tail) ---")
        print("\n".join((LOG_DIR / f"{tName}.log").read_text(encoding="utf-8", errors="replace").splitlines()[-30:]))
    print(f"\n{len(tResults) - len(tFailed)}/{len(tResults)} scripts passed in {time.time() - tStart:.0f}s, logs in {LOG_DIR}")
    sys.exit(1 if tFailed else 0)


if __name__ == "__main__":
    main()
