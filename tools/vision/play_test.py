import base64
import ctypes
import ctypes.wintypes as wt
import io
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
from pathlib import Path

from PIL import ImageGrab

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "tests"))
import lakotnet

MODEL = os.environ.get("LAKOT_VISION_MODEL", "gemini-3.5-flash-lite")
URL = "https://generativelanguage.googleapis.com/v1beta/models/{}:generateContent"
PORT = int(os.environ.get("LAKOT_VISION_PORT") or 12347)
SHOT_WIDTH = 1024

user32 = ctypes.WinDLL("user32", use_last_error=True)
user32.SetProcessDPIAware()

KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_SCANCODE = 0x0008
KEYEVENTF_UNICODE = 0x0004
MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP = 0x0002, 0x0004
MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP = 0x0008, 0x0010

SCANCODES = {
    "esc": 0x01, "1": 0x02, "2": 0x03, "3": 0x04, "4": 0x05, "5": 0x06, "backspace": 0x0E, "tab": 0x0F,
    "q": 0x10, "w": 0x11, "e": 0x12, "r": 0x13, "t": 0x14, "y": 0x15, "u": 0x16, "i": 0x17, "o": 0x18, "p": 0x19,
    "enter": 0x1C, "a": 0x1E, "s": 0x1F, "d": 0x20, "f": 0x21, "g": 0x22, "h": 0x23, "j": 0x24, "k": 0x25, "l": 0x26,
    "z": 0x2C, "x": 0x2D, "c": 0x2E, "v": 0x2F, "b": 0x30, "n": 0x31, "m": 0x32, "space": 0x39,
    "f1": 0x3B, "f2": 0x3C, "f3": 0x3D, "f4": 0x3E, "f5": 0x3F, "f10": 0x44,
}


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wt.WORD), ("wScan", wt.WORD), ("dwFlags", wt.DWORD), ("time", wt.DWORD), ("dwExtraInfo", ctypes.c_size_t)]


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [("dx", wt.LONG), ("dy", wt.LONG), ("mouseData", wt.DWORD), ("dwFlags", wt.DWORD), ("time", wt.DWORD), ("dwExtraInfo", ctypes.c_size_t)]


class INPUTUNION(ctypes.Union):
    _fields_ = [("ki", KEYBDINPUT), ("mi", MOUSEINPUT)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wt.DWORD), ("u", INPUTUNION)]


def send_inputs(pInputs):
    tArray = (INPUT * len(pInputs))(*pInputs)
    user32.SendInput(len(pInputs), tArray, ctypes.sizeof(INPUT))


def key_input(pScan, pUp=False, pUnicode=False):
    tFlags = (KEYEVENTF_UNICODE if pUnicode else KEYEVENTF_SCANCODE) | (KEYEVENTF_KEYUP if pUp else 0)
    return INPUT(type=1, u=INPUTUNION(ki=KEYBDINPUT(0, pScan, tFlags, 0, 0)))


def press(pKey, pHold):
    tScan = SCANCODES[pKey]
    send_inputs([key_input(tScan)])
    time.sleep(max(0.05, min(pHold, 5.0)))
    send_inputs([key_input(tScan, True)])


def type_text(pText):
    for tChar in pText:
        send_inputs([key_input(ord(tChar), False, True), key_input(ord(tChar), True, True)])
        time.sleep(0.03)


def click(pX, pY, pButton):
    user32.SetCursorPos(int(pX), int(pY))
    time.sleep(0.05)
    tDown, tUp = (MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP) if pButton == "right" else (MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP)
    send_inputs([INPUT(type=0, u=INPUTUNION(mi=MOUSEINPUT(0, 0, 0, tDown, 0, 0)))])
    time.sleep(0.05)
    send_inputs([INPUT(type=0, u=INPUTUNION(mi=MOUSEINPUT(0, 0, 0, tUp, 0, 0)))])


def find_window(pPid):
    tFound = []
    tProc = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

    def tCallback(pHwnd, pParam):
        tWindowPid = wt.DWORD()
        user32.GetWindowThreadProcessId(pHwnd, ctypes.byref(tWindowPid))
        if tWindowPid.value == pPid and user32.IsWindowVisible(pHwnd):
            tFound.append(pHwnd)
        return True

    user32.EnumWindows(tProc(tCallback), 0)
    return tFound[0] if tFound else None


def focus(pHwnd):
    if user32.GetForegroundWindow() == pHwnd:
        return
    user32.ShowWindow(pHwnd, 9)
    send_inputs([key_input(0x38), key_input(0x38, True)])
    user32.SetForegroundWindow(pHwnd)
    time.sleep(0.3)


def client_rect(pHwnd):
    tRect = wt.RECT()
    user32.GetClientRect(pHwnd, ctypes.byref(tRect))
    tPoint = wt.POINT(0, 0)
    user32.ClientToScreen(pHwnd, ctypes.byref(tPoint))
    return tPoint.x, tPoint.y, tRect.right, tRect.bottom


def screenshot(pHwnd, pPath):
    tX, tY, tW, tH = client_rect(pHwnd)
    tImage = ImageGrab.grab(bbox=(tX, tY, tX + tW, tY + tH), all_screens=True)
    tScale = SHOT_WIDTH / tImage.width
    tImage = tImage.resize((SHOT_WIDTH, int(tImage.height * tScale)))
    tImage.save(pPath, "JPEG", quality=75)
    tBuffer = io.BytesIO()
    tImage.save(tBuffer, "JPEG", quality=75)
    return base64.b64encode(tBuffer.getvalue()).decode(), (tX, tY, 1.0 / tScale)


PROVIDER = os.environ.get("LAKOT_VISION_PROVIDER", "gemini")
OPENCODE_MODEL = "opencode/nemotron-3.5-super-vl-free"


def ask(pImages, pText, pJson, pThinking):
    if PROVIDER == "gemini":
        tParts = [{"inline_data": {"mime_type": "image/jpeg", "data": base64.b64encode(Path(tImage).read_bytes()).decode()}} for tImage in pImages]
        tParts.append({"text": pText})
        return ask_gemini(tParts, pJson, pThinking)
    tFd, tPromptName = tempfile.mkstemp(suffix=".txt", prefix="lakot_prompt_")
    os.close(tFd)
    tPromptFile = Path(tPromptName)
    tPromptFile.write_text(pText, encoding="utf-8")
    tArgs = [shutil.which("opencode") or "opencode", "run", "--standalone", "-m", OPENCODE_MODEL, "-f", str(tPromptFile)]
    for tImage in pImages:
        tArgs += ["-f", str(tImage)]
    tArgs.append("Follow the instructions in the attached text file exactly; the images are screenshots of the game.")
    tEnv = {**os.environ, "OPENCODE_CONFIG_CONTENT": json.dumps({"permission": {"*": "deny"}})}
    tResult = subprocess.run(tArgs, stdin=subprocess.DEVNULL, capture_output=True, text=True, encoding="utf-8", errors="replace", env=tEnv, timeout=300)
    tPromptFile.unlink(missing_ok=True)
    if tResult.returncode != 0:
        raise RuntimeError("opencode failed: " + tResult.stderr[-300:])
    tOut = tResult.stdout
    if pJson:
        tStart, tEnd = tOut.find("{"), tOut.rfind("}")
        return tOut[tStart:tEnd + 1] if tStart >= 0 and tEnd > tStart else tOut
    return tOut


def ask_gemini(pParts, pJson, pThinking):
    tKey = os.environ.get("GEMINI_API_KEY") or open(os.path.expanduser("~/.lakot/gemini_api_key")).read().strip()
    tConfig = {}
    if pJson:
        tConfig["responseMimeType"] = "application/json"
    if pThinking is not None:
        tConfig["thinkingConfig"] = {"thinkingBudget": pThinking}
    tBody = json.dumps({"contents": [{"parts": pParts}], "generationConfig": tConfig}).encode()
    for tAttempt in range(5):
        tRequest = urllib.request.Request(URL.format(MODEL), data=tBody, headers={"x-goog-api-key": tKey, "Content-Type": "application/json"})
        try:
            tResponse = json.load(urllib.request.urlopen(tRequest, timeout=300))
            tParts = tResponse["candidates"][0]["content"]["parts"]
            return "".join(p.get("text", "") for p in tParts if not p.get("thought"))
        except urllib.error.HTTPError as e:
            tDetail = e.read().decode(errors="replace")[:300]
            if e.code == 400 and pThinking is not None and "thinking" in tDetail.lower():
                return ask_gemini(pParts, pJson, None)
            if e.code not in (429, 500, 503) or tAttempt == 4:
                raise RuntimeError(f"HTTP {e.code}: {tDetail}")
            time.sleep(15 + tAttempt * 15)
    raise RuntimeError("no response")


def start_server(pBinDir, pRunRoot):
    tRunDir = pRunRoot / "server"
    shutil.copytree(pBinDir, tRunDir)
    tLog = open(pRunRoot / "server.log", "w", encoding="utf-8", errors="replace")
    tProc = subprocess.Popen([str(tRunDir / "LakotServer.exe")], cwd=tRunDir, env={**os.environ, "LAKOT_SERVER_PORT": str(PORT), "LAKOT_SERVER_BIND": "127.0.0.1"},
                             stdin=subprocess.PIPE, stdout=tLog, stderr=subprocess.STDOUT)
    tEnd = time.time() + 40
    while time.time() < tEnd:
        if tProc.poll() is not None:
            raise RuntimeError("server exited early, see server.log")
        try:
            socket.create_connection(("127.0.0.1", PORT), timeout=0.5).close()
            return tProc
        except OSError:
            time.sleep(0.3)
    tProc.kill()
    raise RuntimeError("server did not open its port")


def make_account():
    lakotnet.PORT = PORT
    lakotnet.generate_bindings()
    tClient = lakotnet.make_player(prefix="vis")
    tClient.close()
    return tClient.user, "pw12345", tClient.name


def build_prompt(pGoal, pUser, pPassword, pHistory, pStep, pMaxSteps):
    tLog = "\n".join(pHistory[-25:]) or "(nothing yet)"
    return f"""You are play-testing Lakot, a 3D online RPG, through screenshots of its window. You control it with keyboard and mouse.
Goal for this session: {pGoal}
Test account (local test server): username "{pUser}", password "{pPassword}". The account already has one character.
Known controls: W/A/S/D move, Space attack, Z pick up nearest loot, I inventory, C character, K skills, M world map, Enter opens chat and sends, Esc closes windows, 1-5 skill bar, left click selects / uses UI, right click drag rotates the camera.
Give click coordinates normalized to 0-1000 on both axes (0,0 top-left, 1000,1000 bottom-right of the screenshot). Click a text field before typing into it.
Step {pStep} of {pMaxSteps}. Your earlier actions and notes:
{tLog}
Reply with ONE JSON object only:
{{"observation": "what you see now, short", "action": "click|right_click|key|type|wait|done", "x": 0, "y": 0, "key": "w", "hold": 0.2, "text": "", "seconds": 1, "note": "anything that looks broken, ugly or confusing, or empty"}}
"key" must be one of: {", ".join(SCANCODES)}. "type" types text into the focused field. Use "done" when the goal is reached or you are stuck."""


def main():
    tGoal = " ".join(sys.argv[1:]) or "Log in, enter the world with the character, walk around, open the inventory and chat, and judge how the game looks and feels."
    tMaxSteps = int(os.environ.get("LAKOT_VISION_STEPS") or 40)
    tOutDir = Path(os.environ.get("LAKOT_VISION_OUT") or tempfile.mkdtemp(prefix="lakot_vision_"))
    tOutDir.mkdir(parents=True, exist_ok=True)
    tBinDir = Path(lakotnet.resolve_bin())
    tRunRoot = Path(tempfile.mkdtemp(prefix="lakot_vision_run_"))

    tServer = start_server(tBinDir, tRunRoot)
    tGame = None
    tHistory, tNotes = [], []
    try:
        tUser, tPassword, tCharacter = make_account()
        tGameDir = tRunRoot / "game"
        shutil.copytree(tBinDir, tGameDir)
        tGame = subprocess.Popen([str(tGameDir / "LakotGame.exe")], cwd=tGameDir,
                                 env={**os.environ, "LAKOT_SERVER_HOST": "127.0.0.1", "LAKOT_SERVER_PORT": str(PORT)},
                                 stdout=open(tRunRoot / "game.log", "w"), stderr=subprocess.STDOUT)
        tHwnd = None
        tEnd = time.time() + 60
        while time.time() < tEnd and not tHwnd:
            time.sleep(1.0)
            tHwnd = find_window(tGame.pid)
        if not tHwnd:
            raise RuntimeError("game window did not appear")
        time.sleep(4.0)

        for tStep in range(1, tMaxSteps + 1):
            focus(tHwnd)
            tShotPath = tOutDir / f"step{tStep:02d}.jpg"
            _, (tOriginX, tOriginY, tScale) = screenshot(tHwnd, tShotPath)
            tReply = ask([tShotPath], build_prompt(tGoal, tUser, tPassword, tHistory, tStep, tMaxSteps), True, 512)
            try:
                tAction = json.loads(tReply)
                if isinstance(tAction, list):
                    tAction = tAction[0]
            except (json.JSONDecodeError, IndexError):
                tHistory.append(f"{tStep}: (unparseable reply)")
                continue
            tKind = tAction.get("action", "wait")
            tFields = {"click": ("x", "y"), "right_click": ("x", "y"), "key": ("key", "hold"), "type": ("text",), "wait": ("seconds",)}.get(tKind, ())
            tSummary = f"{tStep}: saw: {tAction.get('observation', '')} -> {tKind} {json.dumps({k: tAction.get(k) for k in tFields})}"
            tHistory.append(tSummary)
            if tAction.get("note"):
                tNotes.append(f"step {tStep}: {tAction['note']}")
            print(tSummary, flush=True)
            if tKind == "done":
                break
            focus(tHwnd)
            if tKind in ("click", "right_click"):
                tWidth, tHeight = client_rect(tHwnd)[2:]
                click(tOriginX + float(tAction.get("x", 0)) / 1000.0 * tWidth, tOriginY + float(tAction.get("y", 0)) / 1000.0 * tHeight, "right" if tKind == "right_click" else "left")
            elif tKind == "key" and tAction.get("key") in SCANCODES:
                press(tAction["key"], float(tAction.get("hold", 0.15)))
            elif tKind == "type":
                type_text(str(tAction.get("text", ""))[:64])
            else:
                time.sleep(max(0.2, min(float(tAction.get("seconds", 1)), 5.0)))
            time.sleep(0.8)

        tShots = sorted(tOutDir.glob("step*.jpg"))
        tPick = tShots[:: max(1, len(tShots) // 6)][:6]
        tReport = ask(tPick, ("These are screenshots from a play-test session of Lakot, an early-stage 3D online RPG. Action log:\n" + "\n".join(tHistory)
                       + "\nNotes taken while playing:\n" + ("\n".join(tNotes) or "(none)")
                       + "\nWrite a short play-test report in Turkish: what worked, what looked broken or buggy (be concrete, cite step numbers), UI/visual quality, and the 5 most valuable improvements. Max 40 lines."), False, None)
        (tOutDir / "report.md").write_text(tReport, encoding="utf-8")
        (tOutDir / "actions.log").write_text("\n".join(tHistory) + "\n\nNOTES\n" + "\n".join(tNotes), encoding="utf-8")
        print(f"report: {tOutDir / 'report.md'}")
    finally:
        if tGame and tGame.poll() is None:
            tGame.kill()
        if tServer.poll() is None:
            try:
                tServer.stdin.write(b"quit\n")
                tServer.stdin.flush()
                tServer.wait(timeout=10)
            except Exception:
                tServer.kill()
        for tLog in ("server.log", "game.log"):
            if (tRunRoot / tLog).exists():
                shutil.copy(tRunRoot / tLog, tOutDir / tLog)
        shutil.rmtree(tRunRoot, ignore_errors=True)


if __name__ == "__main__":
    main()
