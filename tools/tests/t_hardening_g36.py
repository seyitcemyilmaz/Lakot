import os
import socket
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lakotnet import *

generate_bindings()


def is_closed(pSock, pWait):
    pSock.settimeout(pWait)
    try:
        while True:
            tData = pSock.recv(4096)
            if not tData:
                return True
    except socket.timeout:
        return False
    except OSError:
        return True


def raw():
    return socket.create_connection(("127.0.0.1", PORT), timeout=5)


def t_zero_length_frame_closes():
    s = raw()
    s.sendall(struct.pack(">I", 0))
    return is_closed(s, 3.0)


def t_unauthenticated_idle_closes():
    s = raw()
    tStart = time.time()
    tClosed = is_closed(s, 70.0)
    tElapsed = time.time() - tStart
    print(f"  idle connection closed={tClosed} after {tElapsed:.1f}s")
    return tClosed and 58.0 <= tElapsed <= 66.0


def t_logged_in_idle_stays():
    c = make_player(prefix="g36")
    time.sleep(62.0)
    r = c.char_list()
    c.close()
    return r is not None


def t_flood_closes():
    from connection_pb2 import Message
    s = raw()
    tMessage = Message()
    tMessage.request.character_list_request.SetInParent()
    tData = tMessage.SerializeToString()
    tFrame = struct.pack(">I", len(tData)) + tData
    try:
        s.sendall(tFrame * 1000)
    except OSError:
        return True
    return is_closed(s, 5.0)


def t_normal_rate_survives():
    c = make_player(prefix="g36")
    tOk = True
    for _ in range(30):
        if c.char_list() is None:
            tOk = False
            break
        time.sleep(0.05)
    c.close()
    return tOk


TESTS = [t_zero_length_frame_closes, t_flood_closes, t_normal_rate_survives, t_unauthenticated_idle_closes, t_logged_in_idle_stays]


def main():
    s = Server()
    s.start()
    tFailed = []
    try:
        for t in TESTS:
            try:
                tOk = t()
            except Exception as e:
                print(f"  {t.__name__}: exception {e}")
                tOk = False
            print(f"{'PASS' if tOk else 'FAIL'} {t.__name__}")
            if not tOk:
                tFailed.append(t.__name__)
    finally:
        s.stop()
    print(f"{len(TESTS) - len(tFailed)}/{len(TESTS)} passed")
    sys.exit(1 if tFailed else 0)


main()
