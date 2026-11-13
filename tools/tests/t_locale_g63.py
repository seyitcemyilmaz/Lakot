import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
DATA = ROOT / "data"
LOCALE = DATA / "locale"
GAME = ROOT / "src" / "game"
KEY = re.compile(r"^[a-z0-9_]+(\.[a-z0-9_]+)*$")
PLACEHOLDER = re.compile(r"\{([^{}]*)\}")
TURKISH = re.compile("[ğĞıİşŞ]")
ALLOWED_NAMES = {"Albastı"}
META = ("meta.language_name", "meta.decimal_separator")
FAILURES = []
NOTES = []


def fail(message):
    FAILURES.append(message)


def load_flat(path):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as error:
        fail(f"{path}: {error}")
        return {}
    if not isinstance(data, dict):
        fail(f"{path}: not an object")
        return {}
    for key, value in data.items():
        if not isinstance(value, str):
            fail(f"{path}: {key} is not a string")
    return {k: v for k, v in data.items() if isinstance(v, str)}


def load_language(code):
    texts = {}
    for name in ("ui.json", "server.json"):
        path = LOCALE / code / name
        if not path.exists():
            continue
        for key, value in load_flat(path).items():
            if key in texts:
                fail(f"{code}: duplicate key {key} across files ({name})")
            texts[key] = value
    content_path = LOCALE / code / "content.json"
    content = load_flat(content_path) if content_path.exists() else {}
    return texts, content


def check_en(texts):
    for meta in META:
        if meta not in texts:
            fail(f"en: missing {meta}")
    for key, value in texts.items():
        if not KEY.match(key):
            fail(f"en: bad key pattern {key}")
        if not value.strip():
            fail(f"en: empty value for {key}")
        if TURKISH.search(value):
            fail(f"en: Turkish letter in {key}: {value}")


def source_files(pattern):
    return sorted(p for p in GAME.rglob(pattern) if "backend" not in p.parts)


def check_used_keys(texts):
    call = re.compile(r"[lL]ocalization(?:\(\))?\s*(?:\.|->)\s*(text|format|formatCount)\(\s*(\"[^\"]*\"|[^,)\s][^,)]*)")
    keyed = re.compile(r"LoginScene>\(\*this,\s*\"([^\"]+)\"")
    token = re.compile(r"\[\[([^\]]*)\]\]")
    used = set()
    for path in source_files("*.cpp") + source_files("*.h"):
        source = path.read_text(encoding="utf-8", errors="replace")
        for match in call.finditer(source):
            argument = match.group(2).strip()
            if argument == "mNotice" and path.name == "LoginScene.cpp":
                continue
            if not argument.startswith('"'):
                fail(f"{path.relative_to(ROOT)}: non-literal key {argument}")
                continue
            used.add(argument.strip('"'))
        for match in keyed.finditer(source):
            used.add(match.group(1))
    for path in source_files("*.rml") + sorted((GAME / "assets" / "ui").glob("*.rml")):
        source = path.read_text(encoding="utf-8")
        source = re.sub(r"<!--.*?-->", "", source, flags=re.S)
        for match in token.finditer(source):
            key = match.group(1)
            used.add(key)
            if key in texts and PLACEHOLDER.search(texts[key]):
                fail(f"{path.name}: markup key {key} has placeholders")
    for key in sorted(used):
        base = key[:-4] if key.endswith(".one") else key
        if key not in texts and base not in texts:
            fail(f"used key missing in en: {key}")
    return used


def check_language(code, en_texts):
    texts, content = load_language(code)
    missing = 0
    for key, value in texts.items():
        if key in META:
            continue
        if key not in en_texts:
            if key.endswith(".one") and key[:-4] in texts and key[:-4] in en_texts:
                continue
            fail(f"{code}: orphan key {key}")
            continue
        if set(PLACEHOLDER.findall(value)) != set(PLACEHOLDER.findall(en_texts[key])):
            fail(f"{code}: placeholder mismatch in {key}")
    for key in en_texts:
        if key not in META and key not in texts:
            missing += 1
    NOTES.append(f"{code}: {len(texts)} keys, {missing} missing (falls back to English), {len(content)} content keys")
    return content


def content_targets():
    targets = set()
    skills = json.loads((DATA / "skills.json").read_text(encoding="utf-8"))["skills"]
    for skill in skills:
        targets.add(f"skill.{skill['key']}.name")
        targets.add(f"skill.{skill['key']}.desc")
    for monster in json.loads((DATA / "monsters.json").read_text(encoding="utf-8"))["monsters"]:
        targets.add(f"monster.{monster['id']}.name")
    for klass in json.loads((DATA / "classes.json").read_text(encoding="utf-8"))["classes"]:
        targets.add(f"class.{klass['key']}.name")
        targets.add(f"class.{klass['key']}.desc")
        for path in klass.get("paths", []):
            targets.add(f"path.{klass['key']}.{path['key']}.name")
    for kingdom in json.loads((DATA / "kingdoms.json").read_text(encoding="utf-8"))["kingdoms"]:
        targets.add(f"kingdom.{kingdom['id']}.name")
        targets.add(f"kingdom.{kingdom['id']}.desc")
    for item in json.loads((DATA / "items.json").read_text(encoding="utf-8"))["items"]:
        targets.add(f"item.{item['id']}.name")
        targets.add(f"item.{item['id']}.desc")
    for map_file in (DATA / "maps").glob("*/map.json"):
        targets.add(f"map.{json.loads(map_file.read_text(encoding='utf-8'))['id']}.name")
    return targets


def check_base_content():
    skills = json.loads((DATA / "skills.json").read_text(encoding="utf-8"))["skills"]
    for skill in skills:
        for field in ("name", "description"):
            if not str(skill.get(field, "")).strip():
                fail(f"skill {skill['key']}: empty {field}")
            elif TURKISH.search(skill[field]):
                fail(f"skill {skill['key']}: Turkish letter in {field}")
    for monster in json.loads((DATA / "monsters.json").read_text(encoding="utf-8"))["monsters"]:
        if not str(monster.get("name", "")).strip() or (TURKISH.search(monster["name"]) and monster["name"] not in ALLOWED_NAMES):
            fail(f"monster {monster['id']}: bad English name")
    for kingdom in json.loads((DATA / "kingdoms.json").read_text(encoding="utf-8"))["kingdoms"]:
        for field in ("name", "description"):
            if not str(kingdom.get(field, "")).strip() or TURKISH.search(kingdom[field]):
                fail(f"kingdom {kingdom['id']}: bad {field}")
    for item in json.loads((DATA / "items.json").read_text(encoding="utf-8"))["items"]:
        if not str(item.get("name", "")).strip() or TURKISH.search(item["name"]):
            fail(f"item {item['id']}: bad name")
    for klass in json.loads((DATA / "classes.json").read_text(encoding="utf-8"))["classes"]:
        if not str(klass.get("name", "")).strip():
            fail(f"class {klass['key']}: empty name")
        for path in klass.get("paths", []):
            if not str(path.get("name", "")).strip():
                fail(f"path {klass['key']}.{path['key']}: empty name")
    NOTES.append("class/path names are not Turkish-checked (pending approval)")


def main():
    en_texts, en_content = load_language("en")
    check_en(en_texts)
    check_used_keys(en_texts)
    check_base_content()
    targets = content_targets()
    for key in en_content:
        if key not in targets:
            fail(f"en content: key {key} points to nothing")
    for directory in sorted(p for p in LOCALE.iterdir() if p.is_dir() and p.name != "en"):
        content = check_language(directory.name, en_texts)
        for key in content:
            if key not in targets:
                fail(f"{directory.name} content: key {key} points to nothing")
    for note in NOTES:
        print(note)
    if FAILURES:
        for message in FAILURES:
            print("FAIL", message)
        print(f"{len(FAILURES)} failure(s)")
        return 1
    print("locale checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
