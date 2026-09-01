#!/usr/bin/env python3
"""Prove the Spriggit YAML trees match the sanctioned 0.0.14 VMAD cut."""

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = ROOT / "plugin-src" / "provenance.json"
FORMKEY_RE = re.compile(r"^FormKey:\s*([0-9A-Fa-f]{6}):(\S+)\s*$", re.MULTILINE)
EDITOR_RE = re.compile(r"^EditorID:\s*(\S+)\s*$", re.MULTILINE)
EMPTY_OBJECT_VMAD_RE = re.compile(r"^VirtualMachineAdapter:\s*\{\s*\}\s*$", re.MULTILINE)
EMPTY_FILENAME_VMAD_RE = re.compile(
    r"^VirtualMachineAdapter:\s*\n[ \t]+FileName:\s*''\s*$", re.MULTILINE
)
LIVE_VMAD_RE = re.compile(
    r"^VirtualMachineAdapter:(?:\s*\{[^}]*Name:|\s*\n(?:[ \t]+.*\n)*?[ \t]+(?:Scripts:|Name:))",
    re.MULTILINE,
)
START_GAME_ENABLED_RE = re.compile(r"^[ \t]*- StartGameEnabled\s*$", re.MULTILINE)


def load_manifest() -> dict:
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def record_yaml_files(source: Path) -> list[Path]:
    return sorted(
        path
        for path in source.rglob("*.yaml")
        if path.name not in {"RecordData.yaml"}
    )


def vmad_is_empty(text: str) -> bool:
    if "VirtualMachineAdapter:" not in text:
        return True
    return bool(EMPTY_OBJECT_VMAD_RE.search(text) or EMPTY_FILENAME_VMAD_RE.search(text))


def has_live_vmad(text: str) -> bool:
    if "VirtualMachineAdapter:" not in text:
        return False
    if vmad_is_empty(text):
        return False
    return bool(LIVE_VMAD_RE.search(text) or re.search(r"FileName:\s*'.+'", text))


def inspect_record(path: Path) -> dict:
    text = path.read_text(encoding="utf-8")
    formkey = FORMKEY_RE.search(text)
    editor = EDITOR_RE.search(text)
    if formkey is None:
        raise RuntimeError(f"{path} has no FormKey")
    return {
        "path": path,
        "form": formkey.group(1).upper(),
        "plugin": formkey.group(2),
        "editor_id": editor.group(1) if editor else "",
        "text": text,
    }


def verify_provenance(root: Path = ROOT) -> None:
    manifest = json.loads((root / "plugin-src" / "provenance.json").read_text(encoding="utf-8"))
    if manifest.get("upstream_version") != "0.0.14":
        raise RuntimeError("provenance must pin upstream 0.0.14")
    if manifest.get("upstream_commit") != "f203cd26747e875336caed91f7d1453ca9a8a808":
        raise RuntimeError("provenance must pin upstream commit f203cd2")
    spriggit = manifest.get("spriggit") or {}
    if spriggit.get("package") != "Spriggit.Yaml.Skyrim" or spriggit.get("version") != "0.40.1":
        raise RuntimeError("provenance must pin Spriggit.Yaml.Skyrim 0.40.1")

    allowed = {
        (item["plugin"], item["form"].upper()): item
        for item in manifest["allowed_record_changes"]
    }
    if len(allowed) != 5:
        raise RuntimeError("provenance must list exactly five sanctioned VMAD removals")

    for plugin in manifest["plugins"]:
        source = root / plugin["source"]
        records = [inspect_record(path) for path in record_yaml_files(source)]
        if len(records) != plugin["record_count"]:
            raise RuntimeError(
                f"{plugin['filename']} record count {len(records)} != {plugin['record_count']}"
            )

        seen: set[str] = set()
        for record in records:
            if record["plugin"] != plugin["filename"]:
                raise RuntimeError(f"{record['path']} FormKey plugin mismatch")
            if record["form"] in seen:
                raise RuntimeError(f"{plugin['filename']} duplicate FormID {record['form']}")
            seen.add(record["form"])

            key = (plugin["filename"], record["form"])
            sanctioned = allowed.get(key)
            if sanctioned:
                if record["editor_id"] != sanctioned["editor_id"]:
                    raise RuntimeError(
                        f"{key} EditorID {record['editor_id']!r} != {sanctioned['editor_id']!r}"
                    )
                if "vmad_removed_or_empty" in sanctioned["changes"] and not vmad_is_empty(record["text"]):
                    raise RuntimeError(f"{key} still has a live VMAD")
                if "start_game_enabled_removed" in sanctioned["changes"] and START_GAME_ENABLED_RE.search(
                    record["text"]
                ):
                    raise RuntimeError(f"{key} still has StartGameEnabled")
            else:
                if has_live_vmad(record["text"]):
                    raise RuntimeError(f"{record['path']} has an unsanctioned live VMAD")
                if START_GAME_ENABLED_RE.search(record["text"]):
                    raise RuntimeError(f"{record['path']} has unsanctioned StartGameEnabled")

    print("plugin provenance: PASS")
