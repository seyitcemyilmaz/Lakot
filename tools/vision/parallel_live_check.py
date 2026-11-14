import sys
import tempfile
import shutil
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import play_test as pt
from live_client import LiveClient


def drive(pClient, pUser, pPassword):
    pClient.start()
    pClient.run(["wait 4000"])
    pClient.run(["click 0.5 0.35", "text " + pUser, "click 0.5 0.44", "text " + pPassword, "click 0.5 0.525", "wait 2500"])
    pClient.run(["click 0.5 0.535", "wait 7000"])
    pClient.shot("world")
    pClient.run(["keyhold w 1000", "wait 300"])
    pClient.shot("walk")
    pClient.run(["key enter", "wait 400", "text hello", "key enter", "wait 1200"])
    pClient.shot("chat")
    pClient.run(["drag 0.4 0.5 0.6 0.5 600 right", "wait 300"])
    pClient.shot("orbit")
    return pClient.out_dir


def main():
    tCount = int(sys.argv[1]) if len(sys.argv) > 1 else 3
    tOut = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(tempfile.mkdtemp(prefix="lakot_par_out_"))
    tBin = Path(pt.lakotnet.resolve_bin())
    tRoot = Path(tempfile.mkdtemp(prefix="lakot_par_"))
    tServer = pt.start_server(tBin, tRoot)
    tClients = []
    try:
        tAccounts = [pt.make_account() for _ in range(tCount)]
        tClients = [LiveClient(f"c{tIndex}", tBin, tRoot, pt.PORT, "1280x720") for tIndex in range(tCount)]
        with ThreadPoolExecutor(tCount) as tPool:
            tFutures = [tPool.submit(drive, tClient, tAccount[0], tAccount[1]) for tClient, tAccount in zip(tClients, tAccounts)]
            for tFuture in tFutures:
                print("done", tFuture.result(), flush=True)
        for tClient in tClients:
            shutil.copytree(tClient.out_dir, tOut / tClient.name, dirs_exist_ok=True)
        print("out", tOut)
    finally:
        for tClient in tClients:
            tClient.stop()
        tServer.kill()
        shutil.rmtree(tRoot, ignore_errors=True)


if __name__ == "__main__":
    main()
