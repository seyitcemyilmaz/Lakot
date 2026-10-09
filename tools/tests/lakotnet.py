import os, sys, shutil, socket, struct, threading, time, subprocess, random, string, pathlib

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE / "vendor"))
sys.path.insert(0, str(HERE / "generated"))

PROTOC_DEFAULT = r"installed\x64-windows\tools\protobuf\protoc.exe"
BUILD_ROOT = r"C:\Development\build"
BIN_DEBUG = BUILD_ROOT + r"\build-Lakot-Desktop_Qt_6_8_3_MSVC2022_64bit-Debug\bin"
BIN_RELEASE = BUILD_ROOT + r"\build-Lakot-Release\bin"
BIN_DEFAULT = BIN_DEBUG


def resolve_bin(pValue=None, pDefault=BIN_DEFAULT):
    tValue = pValue or os.environ.get("LAKOT_BIN") or os.environ.get("LAKOT_BIN_DIR") or pDefault
    return {"debug": BIN_DEBUG, "release": BIN_RELEASE}.get(tValue.lower(), tValue)
PORT = 12345


def generate_bindings():
    vcpkg = os.environ.get("VCPKG_ROOT", r"C:\vcpkg")
    protoc = os.path.join(vcpkg, PROTOC_DEFAULT)
    out = HERE / "generated"
    out.mkdir(exist_ok=True)
    proto_dir = ROOT / "src" / "connection" / "proto"
    files = [str(p) for p in proto_dir.rglob("*.proto")]
    subprocess.check_call([protoc, f"-I{proto_dir}", f"--python_out={out}"] + files)


def rand_name(prefix="t"):
    return prefix + "".join(random.choices(string.ascii_lowercase + string.digits, k=10))


class Server:
    def __init__(self, bin_dir=None):
        self.bin_dir = resolve_bin(bin_dir)
        self.proc = None
        self.log_lines = []

    def start(self, timeout=40):
        run_dir = HERE / ".run"
        shutil.rmtree(run_dir, ignore_errors=True)
        shutil.copytree(self.bin_dir, run_dir)
        self.run_dir = str(run_dir)
        self.proc = subprocess.Popen([os.path.join(self.run_dir, "LakotServer.exe")], cwd=self.run_dir,
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     text=True, errors="replace")
        threading.Thread(target=self._pump, daemon=True).start()
        end = time.time() + timeout
        while time.time() < end:
            if self.proc.poll() is not None:
                raise RuntimeError("server exited early:\n" + "\n".join(self.log_lines[-20:]))
            try:
                socket.create_connection(("127.0.0.1", PORT), timeout=0.5).close()
                return
            except OSError:
                time.sleep(0.3)
        raise RuntimeError("server did not open port\n" + "\n".join(self.log_lines[-20:]))

    def _pump(self):
        for line in self.proc.stdout:
            self.log_lines.append(line.rstrip())

    def stop(self):
        if not self.proc:
            return
        try:
            self.proc.stdin.close()
        except Exception:
            pass
        try:
            self.proc.wait(timeout=15)
        except subprocess.TimeoutExpired:
            self.proc.kill()


class Client:
    def __init__(self):
        from connection_pb2 import Message
        self.Message = Message
        self.sock = socket.create_connection(("127.0.0.1", PORT), timeout=5)
        self.sock.settimeout(None)
        self.log = []
        self.cv = threading.Condition()
        self.next_id = 1
        self.closed = False
        self.t = threading.Thread(target=self._read, daemon=True)
        self.t.start()

    def _recv(self, n):
        buf = b""
        while len(buf) < n:
            chunk = self.sock.recv(n - len(buf))
            if not chunk:
                raise EOFError
            buf += chunk
        return buf

    def _read(self):
        try:
            while True:
                n = struct.unpack(">I", self._recv(4))[0]
                m = self.Message()
                m.ParseFromString(self._recv(n))
                with self.cv:
                    self.log.append((time.time(), m))
                    self.cv.notify_all()
        except Exception:
            pass
        with self.cv:
            self.closed = True
            self.cv.notify_all()

    def close(self):
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except Exception:
            pass
        self.sock.close()

    def send(self, fill):
        m = self.Message()
        req = m.request
        req.header.id = self.next_id
        self.next_id += 1
        fill(req)
        data = m.SerializeToString()
        self.sock.sendall(struct.pack(">I", len(data)) + data)
        return req.header.id

    def mark(self):
        with self.cv:
            return len(self.log)

    def wait(self, pred, timeout=5.0, since=0):
        end = time.time() + timeout
        idx = since
        with self.cv:
            while True:
                while idx < len(self.log):
                    r = self.log[idx][1].response
                    idx += 1
                    if pred(r):
                        return r
                left = end - time.time()
                if left <= 0 or self.closed:
                    return None
                self.cv.wait(min(left, 0.2))

    def call(self, fill, timeout=8.0):
        since = self.mark()
        rid = self.send(fill)
        return self.wait(lambda r: r.header.reply_to == rid, timeout, since)

    def collect(self, pred, seconds, since=None):
        since = self.mark() if since is None else since
        time.sleep(seconds)
        with self.cv:
            return [m.response for _, m in self.log[since:] if pred(m.response)]

    def register(self, user, pw="pw12345", email=None):
        return self.call(lambda q: (setattr(q.register_request, "username", user),
                                    setattr(q.register_request, "password", pw),
                                    setattr(q.register_request, "email", email or user + "@test.local")))

    def login(self, user, pw="pw12345"):
        return self.call(lambda q: (setattr(q.login_request, "username", user),
                                    setattr(q.login_request, "password", pw)))

    def char_list(self):
        return self.call(lambda q: q.character_list_request.SetInParent())

    def create_char(self, name, kingdom=1):
        def f(q):
            q.character_create_request.name = name
            q.character_create_request.kingdom = kingdom
        return self.call(f)

    def enter(self, cid):
        def f(q):
            q.enter_world_request.character_id = cid
        return self.call(f)

    def move(self, x, y, z, yaw=0.0):
        def f(q):
            p = q.player_state_update
            p.position.x, p.position.y, p.position.z, p.yaw = x, y, z, yaw
        self.send(f)

    def attack(self, yaw, combo=0):
        def f(q):
            q.attack_request.yaw = yaw
            q.attack_request.combo = combo
        self.send(f)

    def chat(self, text, is_global=False):
        def f(q):
            q.chat_message_request.text = text
            q.chat_message_request.is_global = is_global
        return self.call(f)

    def whisper(self, to, text):
        def f(q):
            q.direct_message_request.message_to = to
            q.direct_message_request.text = text
        return self.call(f)

    def pickup(self, entity_id, timeout=8.0):
        def f(q):
            q.pickup_item_request.entity_id = entity_id
        r = self.call(f, timeout)
        return r.pickup_item_response.result if r is not None and r.HasField("pickup_item_response") else None

    def snapshots(self, since=0):
        with self.cv:
            return [m.response.world_snapshot for _, m in self.log[since:] if m.response.HasField("world_snapshot")]


def make_player(kingdom=1, prefix="p"):
    from common_pb2 import STATUS_OK
    c = Client()
    user = rand_name(prefix)
    r = c.register(user)
    assert r is not None and r.register_response.register_status == 0, f"register failed: {r}"
    r = c.login(user)
    assert r is not None and r.header.status.code == STATUS_OK and r.login_response.token, f"login failed: {r}"
    name = rand_name("c")
    r = c.create_char(name, kingdom)
    assert r is not None and r.character_create_response.status == 0, f"create failed: {r}"
    cid = r.character_create_response.character.character_id
    c.user, c.name, c.cid = user, name, cid
    return c


def enter_world(c):
    r = c.enter(c.cid)
    assert r is not None and r.enter_world_response.status == 0, f"enter failed: {r}"
    c.pos = (r.enter_world_response.pos_x, r.enter_world_response.pos_y, r.enter_world_response.pos_z)
    return r.enter_world_response
