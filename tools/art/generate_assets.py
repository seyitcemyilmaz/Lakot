"""Builds the warrior concept art and the rigged 3D models (Meshy) listed in asset_manifest.json; tools/art/mixamo_rig.py moves them onto the Mixamo skeleton."""

import argparse
import base64
import io
import json
import os
import shutil
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ART = os.path.join(ROOT, "art")
CONCEPTS = os.path.join(ART, "concepts")
VIEWS = os.path.join(ART, "views")
MODELS = os.path.join(ART, "models")
MESHES = os.path.join(ART, "meshes")
TASKS = os.path.join(ART, ".tasks.json")
KEY_DIR = os.path.join(os.path.expanduser("~"), ".lakot")

GEMINI_URL = "https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent"
MESHY_URL = "https://api.meshy.ai/openapi/v1"

POLYCOUNT = {"body": 20000, "helmet": 5000, "sword": 3000}


def load_key(name):
    value = os.environ.get(name.upper() + "_API_KEY")
    if value:
        return value.strip()
    path = os.path.join(KEY_DIR, name + "_api_key")
    if os.path.exists(path):
        return open(path, encoding="utf-8").read().strip()
    sys.exit(f"missing {name} API key: set {name.upper()}_API_KEY or write it to {path}")


def load_manifest():
    with open(os.path.join(HERE, "asset_manifest.json"), encoding="utf-8") as file:
        manifest = json.load(file)
    manifest["by_id"] = {entry["id"]: entry for entry in manifest["entries"]}
    return manifest


def load_tasks():
    if os.path.exists(TASKS):
        with open(TASKS, encoding="utf-8") as file:
            return json.load(file)
    return {}


def save_tasks(tasks):
    os.makedirs(ART, exist_ok=True)
    with open(TASKS, "w", encoding="utf-8") as file:
        json.dump(tasks, file, indent=2)


def http_json(method, url, headers, body=None, timeout=300):
    data = json.dumps(body).encode() if body is not None else None
    request = urllib.request.Request(url, data=data, method=method, headers={"Content-Type": "application/json", **headers})
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            return json.loads(response.read())
    except urllib.error.HTTPError as error:
        raise RuntimeError(f"{method} {url.split('?')[0]} -> {error.code}: {error.read().decode(errors='replace')[:800]}")


def download(url, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with urllib.request.urlopen(url, timeout=600) as response, open(path + ".part", "wb") as file:
        shutil.copyfileobj(response, file)
    os.replace(path + ".part", path)


def ordered(manifest, only):
    done, result = set(), []

    def visit(entry_id):
        if entry_id in done:
            return
        done.add(entry_id)
        for ref in manifest["by_id"][entry_id]["refs"]:
            visit(ref)
        result.append(manifest["by_id"][entry_id])

    for entry in manifest["entries"]:
        visit(entry["id"])
    return [entry for entry in result if not only or entry["id"] in only]


def build_prompt(manifest, entry):
    return " ".join([manifest["style"], manifest["preambles"][entry["preamble"]], entry["description"]])


def png_part(path, max_side=1536):
    image = Image.open(path).convert("RGB")
    image.thumbnail((max_side, max_side))
    buffer = io.BytesIO()
    image.save(buffer, "PNG")
    return base64.b64encode(buffer.getvalue()).decode()


def gemini_generate(key, model, prompt, reference_paths, aspect):
    parts = [{"text": prompt}]
    parts += [{"inline_data": {"mime_type": "image/png", "data": png_part(path)}} for path in reference_paths]
    image_options = {"aspectRatio": aspect, "imageSize": "2K"}
    config = {"responseModalities": ["TEXT", "IMAGE"], "imageConfig": image_options}
    try:
        response = http_json("POST", GEMINI_URL.format(model=model), {"x-goog-api-key": key},
                             {"contents": [{"parts": parts}], "generationConfig": config})
    except RuntimeError as error:
        if "-> 400" not in str(error):
            raise
        config = {"responseModalities": ["TEXT", "IMAGE"], "responseFormat": {"image": image_options}}
        response = http_json("POST", GEMINI_URL.format(model=model), {"x-goog-api-key": key},
                             {"contents": [{"parts": parts}], "generationConfig": config})
    for candidate in response.get("candidates", []):
        for part in candidate.get("content", {}).get("parts", []):
            inline = part.get("inline_data") or part.get("inlineData")
            if inline:
                return base64.b64decode(inline["data"])
    raise RuntimeError("no image in response: " + json.dumps(response)[:800])


def optional_key(name):
    value = os.environ.get(name.upper())
    path = os.path.join(KEY_DIR, name)
    if not value and os.path.exists(path):
        value = open(path, encoding="utf-8").read().strip()
    return value


def gradio_headers():
    token = optional_key("hf_token")
    return {"Authorization": f"Bearer {token}"} if token else {}


def gradio_upload(space, path):
    boundary = "lakotboundary" + str(int(time.time() * 1000))
    data = open(path, "rb").read()
    body = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"files\"; filename=\"{os.path.basename(path)}\"\r\n"
            f"Content-Type: image/png\r\n\r\n").encode() + data + f"\r\n--{boundary}--\r\n".encode()
    request = urllib.request.Request(f"https://{space}.hf.space/gradio_api/upload", data=body, method="POST",
                                     headers={"Content-Type": f"multipart/form-data; boundary={boundary}", **gradio_headers()})
    with urllib.request.urlopen(request, timeout=300) as response:
        return json.loads(response.read())[0]


def gradio_call(space, endpoint, data):
    base = f"https://{space}.hf.space/gradio_api/call/{endpoint}"
    event_id = http_json("POST", base, gradio_headers(), {"data": data})["event_id"]
    request = urllib.request.Request(f"{base}/{event_id}", headers=gradio_headers())
    event = None
    with urllib.request.urlopen(request, timeout=900) as response:
        for raw in response:
            line = raw.decode(errors="replace").strip()
            if line.startswith("event:"):
                event = line[6:].strip()
            elif line.startswith("data:") and event in ("complete", "error"):
                payload = line[5:].strip()
                if event == "error":
                    raise RuntimeError(f"{space} error: {payload[:600]}")
                return json.loads(payload)
    raise RuntimeError(f"{space}: stream ended without a result")


def gradio_image(result):
    item = result[0]
    url = item.get("url") if isinstance(item, dict) else None
    with urllib.request.urlopen(urllib.request.Request(url, headers=gradio_headers()), timeout=300) as response:
        return response.read()


class GradioSession:
    def __init__(self, space):
        self.space = space
        self.base = f"https://{space}.hf.space/gradio_api"
        self.session = "lakot" + str(int(time.time() * 1000))

    def run(self, fn_index, data):
        http_json("POST", f"{self.base}/queue/join", gradio_headers(),
                  {"data": data, "event_data": None, "fn_index": fn_index, "trigger_id": None, "session_hash": self.session})
        request = urllib.request.Request(f"{self.base}/queue/data?session_hash={self.session}", headers=gradio_headers())
        with urllib.request.urlopen(request, timeout=1800) as response:
            for raw in response:
                line = raw.decode(errors="replace").strip()
                if not line.startswith("data:"):
                    continue
                message = json.loads(line[5:])
                kind = message.get("msg")
                if kind == "process_completed":
                    if not message.get("success", False):
                        raise RuntimeError(f"{self.space} fn {fn_index} failed: {json.dumps(message.get('output'))[:600]}")
                    return message["output"]["data"]
                if kind in ("estimation", "process_starts") and message.get("rank"):
                    print(f"  {self.space} queue rank {message.get('rank')}", flush=True)
        raise RuntimeError(f"{self.space}: stream ended without a result")


def file_data(space, path):
    return {"path": gradio_upload(space, path), "meta": {"_type": "gradio.FileData"}}


def fetch_file(item):
    url = item.get("url") if isinstance(item, dict) else None
    if not url and isinstance(item, dict) and "value" in item:
        url = item["value"].get("url")
    with urllib.request.urlopen(urllib.request.Request(url, headers=gradio_headers()), timeout=600) as response:
        return response.read()


TRELLIS = "microsoft-trellis-2"
FACES = {"body": 100000, "helmet": 100000, "sword": 100000}


def trellis_model(entry, image_path, target):
    session = GradioSession(TRELLIS)
    session.run(2, [])
    image = session.run(4, [file_data(TRELLIS, image_path)])[0]
    session.run(7, [image, 0, "1024", 7.5, 0.7, 12, 5.0, 7.5, 0.5, 12, 3.0, 1.0, 0.0, 12, 3.0])
    outputs = session.run(9, [None, FACES[entry["kind"]], 2048])
    os.makedirs(os.path.dirname(target), exist_ok=True)
    with open(target + ".part", "wb") as file:
        file.write(fetch_file(outputs[0]))
    os.replace(target + ".part", target)


def cmd_meshes(args, manifest):
    os.makedirs(MESHES, exist_ok=True)
    for entry in ordered(manifest, args.only):
        target = os.path.join(MESHES, entry["id"] + ".glb")
        view = os.path.join(VIEWS, entry["id"] + "_front.png")
        if (os.path.exists(target) and not args.force) or not os.path.exists(view):
            continue
        for attempt in range(3):
            try:
                trellis_model(entry, view, target)
                print("mesh", os.path.relpath(target, ROOT), flush=True)
                break
            except Exception as error:
                print(f"mesh {entry['id']} attempt {attempt + 1} failed: {error}", flush=True)
                time.sleep(30 * (attempt + 1))


FLUX_SIZES = {"16:9": (1344, 768), "4:3": (1152, 864), "1:1": (1024, 1024)}


def flux_generate(manifest, entry, references):
    if references and entry["preamble"] == "armor":
        uploaded = gradio_upload("black-forest-labs-flux-1-kontext-dev", references[0])
        prompt = ("Dress the character in all three views in " + entry["description"] +
                  " Keep exactly the same face, body, proportions, A-pose, three-view layout, plain light-grey background and art style."
                  " No helmet, no weapon, hands empty.")
        result = gradio_call("black-forest-labs-flux-1-kontext-dev", "infer",
                             [{"path": uploaded, "meta": {"_type": "gradio.FileData"}}, prompt, 0, True, 2.5, 28])
        return gradio_image(result)
    width, height = FLUX_SIZES[entry["aspect"]]
    result = gradio_call("black-forest-labs-flux-1-dev", "infer",
                         [build_prompt(manifest, entry), 0, True, width, height, 3.5, 28])
    return gradio_image(result)


def meshy_image(manifest, entry, references):
    key = load_key("meshy")
    task_id = meshy_start(key, "image-to-image", {
        "ai_model": "nano-banana-2",
        "prompt": build_prompt(manifest, entry),
        "reference_image_urls": ["data:image/png;base64," + png_part(path, 1536) for path in references],
        "aspect_ratio": entry["aspect"],
    })
    task = meshy_wait(key, "image-to-image", task_id, entry["id"])
    with urllib.request.urlopen(task["image_urls"][0], timeout=300) as response:
        return response.read()


def cmd_concepts(args, manifest):
    key = load_key("gemini") if args.backend == "gemini" else None
    os.makedirs(CONCEPTS, exist_ok=True)
    for entry in ordered(manifest, args.only):
        target = os.path.join(CONCEPTS, entry["id"] + ".png")
        if os.path.exists(target) and not args.force and args.candidates == 1:
            continue
        references = [os.path.join(CONCEPTS, ref + ".png") for ref in entry["refs"]]
        missing = [path for path in references if not os.path.exists(path)]
        if missing:
            print(f"skip {entry['id']}: reference missing {missing}", flush=True)
            continue
        outputs = [target] if args.candidates == 1 else [os.path.join(CONCEPTS, f"{entry['id']}_c{i + 1}.png") for i in range(args.candidates)]
        for output in outputs:
            for attempt in range(3):
                try:
                    if args.backend == "meshy":
                        data = meshy_image(manifest, entry, references)
                    elif args.backend == "gemini":
                        data = gemini_generate(key, args.model, build_prompt(manifest, entry), references, entry["aspect"])
                    else:
                        data = flux_generate(manifest, entry, references)
                    Image.open(io.BytesIO(data)).save(output, "PNG")
                    print("concept", os.path.relpath(output, ROOT), flush=True)
                    break
                except Exception as error:
                    print(f"concept {entry['id']} attempt {attempt + 1} failed: {error}", flush=True)
                    time.sleep(20 * (attempt + 1))


def find_views(image, count):
    rgb = image.convert("RGB")
    width, height = rgb.size
    pixels = rgb.load()
    row_background = []
    for y in range(height):
        left, right = pixels[3, y], pixels[width - 4, y]
        row_background.append(tuple((left[i] + right[i]) // 2 for i in range(3)))

    def is_content(x, y):
        pixel, background = pixels[x, y], row_background[y]
        return sum(abs(pixel[i] - background[i]) for i in range(3)) > 60

    step = 2
    min_count = height // step // 30
    occupied = []
    for x in range(width):
        occupied.append(sum(1 for y in range(0, height, step) if is_content(x, y)) > min_count)

    runs, start = [], None
    for x, filled in enumerate(occupied + [False]):
        if filled and start is None:
            start = x
        elif not filled and start is not None:
            runs.append([start, x])
            start = None

    min_gap = width // 60
    merged = []
    for run in runs:
        if merged and run[0] - merged[-1][1] < min_gap:
            merged[-1][1] = run[1]
        else:
            merged.append(run)
    merged = sorted(merged, key=lambda r: r[1] - r[0], reverse=True)[:count]
    merged.sort()

    boxes = []
    for left, right in merged:
        rows = [y for y in range(height) if any(is_content(x, y) for x in range(left, right, step))]
        if rows:
            boxes.append((left, rows[0], right, rows[-1] + 1))
    return boxes


def pad_square(image, background):
    side = int(max(image.size) * 1.1)
    canvas = Image.new("RGB", (side, side), background)
    canvas.paste(image, ((side - image.width) // 2, (side - image.height) // 2))
    return canvas


def cmd_split(args, manifest):
    os.makedirs(VIEWS, exist_ok=True)
    for entry in ordered(manifest, args.only):
        source = os.path.join(CONCEPTS, entry["id"] + ".png")
        if not os.path.exists(source):
            continue
        image = Image.open(source).convert("RGB")
        boxes = find_views(image, entry["views"])
        status = "ok" if len(boxes) == entry["views"] else f"EXPECTED {entry['views']}"
        names = ["front", "side", "back"][:len(boxes)]
        background = image.getpixel((3, 3))
        for name, box in zip(names, boxes):
            pad_square(image.crop(box), background).save(os.path.join(VIEWS, f"{entry['id']}_{name}.png"))
        print(f"split {entry['id']}: {len(boxes)} views {status}", flush=True)


def meshy_wait(key, kind, task_id, label):
    while True:
        task = http_json("GET", f"{MESHY_URL}/{kind}/{task_id}", {"Authorization": f"Bearer {key}"})
        status = task.get("status")
        if status == "SUCCEEDED":
            return task
        if status in ("FAILED", "CANCELED"):
            raise RuntimeError(f"{label} {kind} {status}: {task.get('task_error')}")
        print(f"  {label} {kind} {status} {task.get('progress', 0)}%", flush=True)
        time.sleep(15)


def meshy_start(key, kind, body):
    return http_json("POST", f"{MESHY_URL}/{kind}", {"Authorization": f"Bearer {key}"}, body)["result"]


def cmd_models(args, manifest):
    key = load_key("meshy")
    tasks = load_tasks()
    os.makedirs(MODELS, exist_ok=True)
    for entry in ordered(manifest, args.only):
        target = os.path.join(MODELS, entry["id"] + ".glb")
        if os.path.exists(target) and not args.force:
            continue
        views = [os.path.join(VIEWS, f"{entry['id']}_{name}.png") for name in ["front", "side", "back"][:entry["views"]]]
        views = [path for path in views if os.path.exists(path)]
        if not views:
            print(f"skip {entry['id']}: no views", flush=True)
            continue
        if args.force:
            tasks[entry["id"]] = {}
        state = tasks.setdefault(entry["id"], {})
        try:
            if "mesh" not in state:
                state["remeshed"] = True
                state["mesh"] = meshy_start(key, "multi-image-to-3d", {
                    "image_urls": ["data:image/png;base64," + png_part(path, 1024) for path in views],
                    "ai_model": "latest",
                    "should_texture": True,
                    "enable_pbr": False,
                    "topology": "triangle",
                    "target_polycount": POLYCOUNT[entry["kind"]],
                    "should_remesh": True,
                    "target_formats": ["glb"],
                })
                save_tasks(tasks)
            mesh = meshy_wait(key, "multi-image-to-3d", state["mesh"], entry["id"])
            state["thumbnail"] = mesh.get("thumbnail_url")
            source_id = state["mesh"]
            if not state.get("remeshed"):
                if "remesh" not in state:
                    state["remesh"] = meshy_start(key, "remesh", {"input_task_id": state["mesh"], "target_polycount": POLYCOUNT[entry["kind"]],
                                                                  "topology": "triangle", "target_formats": ["glb"]})
                    save_tasks(tasks)
                mesh = meshy_wait(key, "remesh", state["remesh"], entry["id"])
                source_id = state["remesh"]

            if entry["kind"] != "body":
                download(mesh["model_urls"]["glb"], target)
                save_tasks(tasks)
                print("model", os.path.relpath(target, ROOT), flush=True)
                continue

            if "rig" not in state:
                state["rig"] = meshy_start(key, "rigging", {"input_task_id": source_id, "height_meters": 1.8})
                save_tasks(tasks)
            rig = meshy_wait(key, "rigging", state["rig"], entry["id"])
            download(rig["result"]["rigged_character_glb_url"] if "result" in rig else rig["rigged_character_glb_url"], target)
            save_tasks(tasks)
            print("model", os.path.relpath(target, ROOT), flush=True)
        except Exception as error:
            print(f"model {entry['id']} failed: {error}", flush=True)


def cmd_pick(args, manifest):
    source = os.path.join(CONCEPTS, f"{args.id}_c{args.candidate}.png")
    shutil.copyfile(source, os.path.join(CONCEPTS, args.id + ".png"))
    print("picked", source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("concepts", "split", "meshes", "models"):
        command = sub.add_parser(name)
        command.add_argument("--only", nargs="*", default=[])
        command.add_argument("--force", action="store_true")
        command.add_argument("--candidates", type=int, default=1)
        command.add_argument("--model", default="gemini-3-pro-image")
        command.add_argument("--backend", choices=("hf", "gemini", "meshy"), default="hf")
    pick = sub.add_parser("pick")
    pick.add_argument("id")
    pick.add_argument("candidate", type=int)

    args = parser.parse_args()
    manifest = load_manifest()
    {"concepts": cmd_concepts, "split": cmd_split, "meshes": cmd_meshes, "models": cmd_models, "pick": cmd_pick}[args.command](args, manifest)


if __name__ == "__main__":
    main()
