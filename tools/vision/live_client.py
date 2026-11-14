import os
import shutil
import subprocess
import time
from pathlib import Path


class LiveClient:
    def __init__(self, pName, pBinDir, pRunRoot, pPort, pSize="1280x720"):
        self.name = pName
        self.live_dir = pRunRoot / f"live_{pName}"
        self.out_dir = pRunRoot / f"out_{pName}"
        self.game_dir = pRunRoot / f"game_{pName}"
        self.log_path = pRunRoot / f"game_{pName}.log"
        self.index = 0
        self.process = None
        self.bin_dir = pBinDir
        self.port = pPort
        self.size = pSize

    def start(self):
        self.live_dir.mkdir(parents=True, exist_ok=True)
        self.out_dir.mkdir(parents=True, exist_ok=True)
        shutil.copytree(self.bin_dir, self.game_dir)
        tEnv = {**os.environ, "LAKOT_SERVER_HOST": "127.0.0.1", "LAKOT_SERVER_PORT": str(self.port), "LAKOT_SNAPSHOT_LIVE": str(self.live_dir),
                "LAKOT_SNAPSHOT_OUT": str(self.out_dir), "LAKOT_SNAPSHOT_SIZE": self.size}
        tEnv.pop("LAKOT_SNAPSHOT_SCRIPT", None)
        self.process = subprocess.Popen([str(self.game_dir / "LakotGame.exe")], cwd=self.game_dir, env=tEnv,
                                        stdout=open(self.log_path, "w"), stderr=subprocess.STDOUT)

    def is_alive(self):
        return self.process is not None and self.process.poll() is None

    def run(self, pCommands, pTimeout=60.0):
        self.index += 1
        tStem = f"{self.index:04d}"
        tTemp = self.live_dir / f"tmp-{tStem}.txt"
        tTemp.write_text("\n".join(pCommands) + "\n", encoding="utf-8")
        tTemp.replace(self.live_dir / f"cmd-{tStem}.txt")
        tDone = self.live_dir / f"done-{tStem}.txt"
        tEnd = time.time() + pTimeout
        while time.time() < tEnd:
            if tDone.exists():
                return tDone.read_text().strip()
            if not self.is_alive():
                raise RuntimeError(f"{self.name}: game exited")
            time.sleep(0.05)
        raise RuntimeError(f"{self.name}: command {tStem} timed out")

    def shot(self, pName, pTimeout=60.0):
        self.run([f"shot {pName}"], pTimeout)
        return self.out_dir / f"{pName}.png"

    def stop(self):
        if self.is_alive():
            try:
                self.run(["quit"], 10.0)
                self.process.wait(timeout=10)
            except Exception:
                self.process.kill()
