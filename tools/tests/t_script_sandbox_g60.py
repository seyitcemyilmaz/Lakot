import sys, os, time, shutil, subprocess, threading, socket
from lakotnet import *

generate_bindings()

RUN = HERE / ".run_g60"

HOSTILE = {
    "aa_dofile.lua": 'lakot.on("enter_world", function(c) dofile("x.lua") end)\n',
    "aa_loop.lua": 'lakot.on("enter_world", function(c) while true do end end)\n',
    "aa_load.lua": 'lakot.on("enter_world", function(c) load("return 1")() end)\n',
}


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G60Server(Server):
    def start(self, timeout=40):
        if not os.environ.get("LAKOT_DB_PASSWORD"):
            raise RuntimeError("LAKOT_DB_PASSWORD is not set")
        shutil.rmtree(RUN, ignore_errors=True)
        shutil.copytree(self.bin_dir, RUN)
        self.run_dir = str(RUN)
        self.log_lines = []
        for tName, tBody in HOSTILE.items():
            (RUN / "scripts" / tName).write_text(tBody)
        self.proc = subprocess.Popen([os.path.join(self.run_dir, "LakotServer.exe")], cwd=self.run_dir,
                                     env={**os.environ, "LAKOT_SERVER_PORT": str(PORT)},
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     text=True, errors="replace")
        threading.Thread(target=self._pump, daemon=True).start()
        tEnd = time.time() + timeout
        while time.time() < tEnd:
            if self.proc.poll() is not None:
                raise RuntimeError("server exited early:\n" + "\n".join(self.log_lines[-20:]))
            try:
                socket.create_connection(("127.0.0.1", PORT), timeout=0.5).close()
                return
            except OSError:
                time.sleep(0.3)
        raise RuntimeError("server did not open port")


def main():
    tServer = G60Server()
    tServer.start()
    try:
        c = make_player()
        tStart = time.time()
        enter_world(c)
        tUpdate = c.wait(lambda x: x.HasField("inventory_update"), 8)
        check(tUpdate is not None, "no inventory update after enter")
        check(time.time() - tStart < 5, "enter took too long")
        tMark = c.mark()
        tSnap = c.wait(lambda x: x.HasField("world_snapshot"), 5, tMark)
        check(tSnap is not None, "zone tick stopped: no snapshot after enter")
        time.sleep(1.0)
        tLog = "\n".join(tServer.log_lines)
        check("global 'dofile'" in tLog, "dofile error missing from log")
        check("global 'load'" in tLog, "load error missing from log")
        check("instruction budget" in tLog, "budget error missing from log")
        check("received starter equipment" in tLog, "starter_equipment did not run")
        c.close()
        print("PASS g60")
    finally:
        tServer.stop()


if __name__ == "__main__":
    main()
