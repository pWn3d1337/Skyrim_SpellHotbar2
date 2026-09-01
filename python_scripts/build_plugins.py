#!/usr/bin/env python3
"""Build and round-trip verify Spell Hotbar's repository-owned plugins."""

from __future__ import annotations

import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path


from plugin_provenance import verify_provenance


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "build" / "plugins"
PACKAGE = "Spriggit.Yaml.Skyrim"
VERSION = "0.40.1"
OFFICIAL_NATIVE_PEX_SHA256 = (
    "03DD7538E14E51A6E97317C7AFBBD833F8FD2621E2DB439CB63133AB4CB42FA8"
)
PLUGINS = (
    (ROOT / "plugin-src" / "SpellHotbar", "SpellHotbar.esp"),
    (ROOT / "plugin-src" / "SpellHotbar_BattleMage", "SpellHotbar_BattleMage.esp"),
)


def run_spriggit(*args: str) -> None:
    subprocess.run(
        ["dotnet", "tool", "run", "spriggit", "--", *args],
        cwd=ROOT,
        check=True,
    )


def files_under(root: Path) -> dict[str, bytes]:
    return {
        str(path.relative_to(root)): path.read_bytes().replace(b"\r\n", b"\n")
        for path in root.rglob("*")
        if path.is_file()
    }


def verify_native_pex() -> None:
    path = ROOT / "papyrus" / "Scripts" / "SpellHotbar.pex"
    actual = hashlib.sha256(path.read_bytes()).hexdigest().upper()
    if actual != OFFICIAL_NATIVE_PEX_SHA256:
        raise RuntimeError(
            "SpellHotbar.pex is not the upstream 0.0.14 artifact: "
            f"expected {OFFICIAL_NATIVE_PEX_SHA256}, got {actual}"
        )


def build_and_verify(source: Path, filename: str) -> None:
    destination = OUTPUT / filename
    if destination.exists():
        destination.unlink()

    run_spriggit(
        "deserialize",
        "-i",
        str(source),
        "-o",
        str(destination),
        "-p",
        PACKAGE,
        "-v",
        VERSION,
    )
    if not destination.is_file():
        raise RuntimeError(f"Spriggit did not create {destination}")

    with tempfile.TemporaryDirectory(prefix=f"{destination.stem}-roundtrip-") as temp:
        roundtrip = Path(temp)
        run_spriggit(
            "serialize",
            "-i",
            str(destination),
            "-o",
            str(roundtrip),
            "-g",
            "SkyrimSE",
            "-p",
            PACKAGE,
            "-v",
            VERSION,
            "-c",
            "-u",
        )
        expected = files_under(source)
        actual = files_under(roundtrip)
        if expected != actual:
            missing = sorted(expected.keys() - actual.keys())
            extra = sorted(actual.keys() - expected.keys())
            changed = sorted(
                path for path in expected.keys() & actual.keys() if expected[path] != actual[path]
            )
            raise RuntimeError(
                f"{filename} failed source round-trip: "
                f"missing={missing}, extra={extra}, changed={changed}"
            )

    print(f"built and round-trip verified: {destination.relative_to(ROOT)}")


def main() -> int:
    subprocess.run(["dotnet", "tool", "restore"], cwd=ROOT, check=True)
    verify_native_pex()
    verify_provenance(ROOT)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for source, filename in PLUGINS:
        build_and_verify(source, filename)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"plugin build failed: {error}", file=sys.stderr)
        sys.exit(1)
