#!/usr/bin/env python3
import argparse
import glob
import os
import re
import sys
import time
from pathlib import Path

CRASH_RE = re.compile(r"^(CRASH|ERROR) (\S+) (\S+) (.*) at (\S+):(\d+) in (.+) \(thread ([^:()]+):(\d+)\)$")

REPO = Path(__file__).resolve().parent.parent.parent


def default_roots():
    tRoots = []
    if os.environ.get("LAKOT_CRASH_DIR"):
        tRoots.append(Path(os.environ["LAKOT_CRASH_DIR"]))
    if os.environ.get("LAKOT_TEST_LOG_DIR"):
        tRoots.append(Path(os.environ["LAKOT_TEST_LOG_DIR"]) / "crashes")
    tRoots.append(REPO / "tools" / "tests" / ".run-crashes")
    if os.environ.get("LAKOT_BIN"):
        tRoots.append(Path(os.environ["LAKOT_BIN"]) / "crashes")
    tRoots += [Path(p) for p in glob.glob(str(REPO.parent / "build" / "*" / "bin" / "crashes"))]
    if os.environ.get("LOCALAPPDATA"):
        tRoots.append(Path(os.environ["LOCALAPPDATA"]) / "Lakot" / "crashes")
    return tRoots


def first_line(pDirectory):
    try:
        with open(Path(pDirectory) / "summary.txt", encoding="utf-8", errors="replace") as tFile:
            return tFile.readline().rstrip()
    except OSError:
        return ""


def find_crash_dirs(pRoots, pSince=None):
    tFound = {}
    for tRoot in pRoots:
        tRoot = Path(tRoot)
        if not tRoot.is_dir():
            continue
        tCandidates = [tRoot] if (tRoot / "summary.txt").is_file() else []
        if not tCandidates:
            tScan = tRoot / "crashes" if (tRoot / "crashes").is_dir() else tRoot
            tCandidates = [d for d in tScan.iterdir() if (d / "summary.txt").is_file()]
        for tDir in tCandidates:
            tTime = (tDir / "summary.txt").stat().st_mtime
            if pSince is None or tTime >= pSince:
                tFound[str(tDir.resolve())] = tTime
    return [Path(d) for d, _ in sorted(tFound.items(), key=lambda kv: kv[1], reverse=True)]


def scan_logs(pRoots):
    tDirectories = set()
    for tRoot in pRoots:
        tRoot = Path(tRoot)
        if tRoot.name == "crashes":
            tRoot = tRoot.parent
        if tRoot.is_dir():
            tDirectories.add(tRoot)
    tLines = []
    for tDirectory in tDirectories:
        if (tDirectory / "crashes").is_dir() and any((tDirectory / "crashes").iterdir()):
            continue
        for tLog in tDirectory.glob("*.log"):
            try:
                with open(tLog, encoding="utf-8", errors="replace") as tFile:
                    for tLine in tFile:
                        if tLine.startswith("CRASH "):
                            tLines.append((os.path.getmtime(tLog), tLine.rstrip(), str(tLog)))
                            break
            except OSError:
                pass
    return sorted(tLines, reverse=True)


def main():
    tParser = argparse.ArgumentParser(description="List the newest Lakot crash reports")
    tParser.add_argument("paths", nargs="*", help="crash dir, a dir of crash dirs, or a test log dir")
    tParser.add_argument("-n", type=int, default=5, help="how many reports to show")
    tParser.add_argument("--full", action="store_true", help="print the whole summary.txt")
    tParser.add_argument("--quiet", action="store_true", help="print nothing when there is no crash")
    tArgs = tParser.parse_args()

    tRoots = [Path(p) for p in tArgs.paths] if tArgs.paths else default_roots()
    tDirs = find_crash_dirs(tRoots)[: tArgs.n]
    tOrphans = scan_logs(tRoots)[: tArgs.n]

    if not tDirs and not tOrphans:
        if not tArgs.quiet:
            print("no crash reports found")
        return 0

    for tDir in tDirs:
        if tArgs.full:
            print((tDir / "summary.txt").read_text(encoding="utf-8", errors="replace").rstrip())
            print(f"  {tDir}\n")
        else:
            print(first_line(tDir))
            print(f"  {tDir}")
    for tTime, tLine, tLog in tOrphans:
        print(tLine)
        print(f"  {tLog} ({time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(tTime))})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
