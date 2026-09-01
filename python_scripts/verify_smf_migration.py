#!/usr/bin/env python3
"""Static acceptance gate for the atomic SKSE Menu Framework migration.

This check intentionally focuses on architectural invariants that are easy to
regress during review.  It complements (and never replaces) a release build and
in-game acceptance.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

from plugin_provenance import verify_provenance
from verify_smf_ui_smoke import verify as verify_ui_smoke


ROOT = Path(__file__).resolve().parents[1]
PLUGIN = ROOT / "skse_plugin"

RETIRED_SCRIPT_NAMES = (
    "SpellHotbarMCM.psc",
    "SpellHotbarMCM.pex",
    "SpellHotbarInitQuestScript.psc",
    "SpellHotbarInitQuestScript.psc.off",
    "SpellHotbarInitQuestScript.pex",
    "SpellHotbarToggleDualCastingEffect.psc",
    "SpellHotbarToggleDualCastingEffect.pex",
    "SpellHotbarOpenBattleMagePerkTree.psc",
    "SpellHotbarOpenBattleMagePerkTree.pex",
    "SpellHotbarBattleMageInitQuestScript.psc",
    "SpellHotbarBattleMageInitQuestScript.pex",
)

REQUIRED_HOST_FILES = (
    PLUGIN / "third_party" / "skse-menu-framework" / "SKSEMenuFramework.h",
    PLUGIN / "src" / "smf" / "smf_guest.h",
    PLUGIN / "src" / "smf" / "smf_guest.cpp",
)

MCP_FILES = (
    PLUGIN / "src" / "mcp" / "mcp_pages.h",
    PLUGIN / "src" / "mcp" / "mcp_pages.cpp",
)


@dataclass(frozen=True)
class CheckResult:
    name: str
    ok: bool
    detail: str


def read(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except FileNotFoundError:
        return ""


def git_output(args: list[str]) -> list[str]:
    completed = subprocess.run(
        ["git", *args], cwd=ROOT, check=False, capture_output=True, text=True
    )
    if completed.returncode != 0:
        raise RuntimeError(completed.stderr.strip() or "git command failed")
    return [line.strip() for line in completed.stdout.splitlines() if line.strip()]


def required_host_files() -> CheckResult:
    missing = [str(path.relative_to(ROOT)) for path in REQUIRED_HOST_FILES if not path.is_file()]
    return CheckResult(
        "required SMF host source",
        not missing,
        "present" if not missing else "missing: " + ", ".join(missing),
    )


def mcp_pages() -> CheckResult:
    missing = [str(path.relative_to(ROOT)) for path in MCP_FILES if not path.is_file()]
    pages = read(PLUGIN / "src" / "mcp" / "mcp_pages.cpp")
    guest = read(PLUGIN / "src" / "smf" / "smf_guest.cpp")
    required_items = (
        "Keybinds",
        "Spell Bind Menu",
        "Settings",
        "Bars",
        "Perks",
        "Presets",
        "Spells",
        "Util",
    )
    missing_items = [
        name for name in required_items if f'AddSectionItem("{name}"' not in pages
    ]
    section_ok = 'SetSection("Spell Hotbar 2")' in pages
    registered = "Mcp::register_pages()" in guest
    addon_tokens = (
        "Ability",
        "Weapon Art",
        "castSlot",
        "save_format = 6",
    )
    leaked = [token for token in addon_tokens if token in pages]
    ok = not missing and section_ok and registered and not missing_items and not leaked
    if missing:
        detail = "missing: " + ", ".join(missing)
    elif missing_items:
        detail = "missing page registrations: " + ", ".join(missing_items)
    elif not section_ok:
        detail = "SetSection(\"Spell Hotbar 2\") is missing"
    elif not registered:
        detail = "SmfGuest::install does not call Mcp::register_pages()"
    elif leaked:
        detail = "addon-only tokens in MCP pages: " + ", ".join(leaked)
    else:
        detail = "eight MCP pages registered under Spell Hotbar 2"
    return CheckResult("MCP pages", ok, detail)


def ui_smoke() -> CheckResult:
    try:
        verify_ui_smoke()
    except AssertionError as exc:
        return CheckResult("SMF UI smoke", False, str(exc))
    return CheckResult(
        "SMF UI smoke",
        True,
        "interactive widgets gated; guest input does not starve ImGui",
    )


def no_private_imgui_dependencies() -> CheckResult:
    cmake = read(PLUGIN / "CMakeLists.txt")
    vcpkg = read(PLUGIN / "vcpkg.json")
    offenders = []
    for label, text in (("CMakeLists.txt", cmake), ("vcpkg.json", vcpkg)):
        for dependency in ("imgui", "directxtk"):
            if re.search(rf"(?i)\b{dependency}\b", text):
                offenders.append(f"{label}:{dependency}")
    return CheckResult(
        "SMF owns ImGui and textures",
        not offenders,
        "no private dependencies" if not offenders else "found " + ", ".join(offenders),
    )


def plugin_source_invariants() -> CheckResult:
    cmake = read(PLUGIN / "CMakeLists.txt")
    required = (
        (r"src/lifecycle/lifecycle\.cpp", "lifecycle source"),
        (r"src/storage/runtime_state_reset\.h", "runtime reset header"),
        (r"src/input/input_event_adapter\.h", "input adapter"),
        (r"src/mcp/mcp_pages\.cpp", "MCP pages"),
        (r"src/mcp/bind_capture\.cpp", "bind capture"),
        (r"set\(OUTPUT_FOLDER\s+\"\"\s+CACHE\s+PATH", "opt-in OUTPUT_FOLDER"),
        (r"add_subdirectory\(tests\)", "unit tests"),
    )
    missing = [label for pattern, label in required if re.search(pattern, cmake) is None]
    return CheckResult(
        "plugin source/build invariants",
        not missing,
        "present" if not missing else "missing: " + ", ".join(missing),
    )


def no_legacy_host_hooks() -> CheckResult:
    forbidden = (
        "ImGui::CreateContext",
        "ImGui_ImplWin32_Init",
        "ImGui_ImplDX11_Init",
        "D3D11CreateDeviceAndSwapChain",
        "SetWindowLongPtrA",
        "Input::install_hook",
        "RenderManager::install",
    )
    hits: list[str] = []
    for path in (PLUGIN / "src").rglob("*.cpp"):
        text = read(path)
        for token in forbidden:
            if token in text:
                hits.append(f"{path.relative_to(ROOT)}:{token}")
    return CheckResult(
        "single UI/input host",
        not hits,
        "legacy hooks absent" if not hits else "found " + ", ".join(hits),
    )


def serialization_formats_unchanged() -> CheckResult:
    storage = read(PLUGIN / "src" / "storage" / "storage.h")
    presets = read(PLUGIN / "src" / "storage" / "user_data_io.h")
    save_ok = bool(re.search(r"save_format\s*=\s*5(?:U)?\s*;", storage))
    preset_ok = bool(re.search(r"preset_save_version\s*=\s*2\s*;", presets))
    return CheckResult(
        "persistence compatibility",
        save_ok and preset_ok,
        f"co-save format 5={save_ok}; preset format 2={preset_ok}",
    )


def mcm_retired() -> CheckResult:
    packaging_text = "\n".join(
        read(path)
        for path in (
            ROOT / "python_scripts" / "build_release_package.py",
            ROOT / "python_scripts" / "create_fomod_installer.py",
            ROOT / "papyrus" / "skyrimse.ppj",
        )
    )
    stale = []
    for name in RETIRED_SCRIPT_NAMES:
        if (ROOT / "papyrus" / "Scripts" / "Source" / name).exists() or (
            ROOT / "papyrus" / "Scripts" / name
        ).exists():
            stale.append(name)
        if name in packaging_text:
            stale.append(f"package/project references {name}")
    return CheckResult(
        "legacy MCM/init/opener scripts retired",
        not stale,
        "no retired script/PEX names" if not stale else "; ".join(stale),
    )


def packaging_smf_and_fonts() -> CheckResult:
    fomod = read(ROOT / "python_scripts" / "create_fomod_installer.py")
    release = read(ROOT / "python_scripts" / "build_release_package.py")
    packaging = fomod + "\n" + release
    missing = []
    if 'fileDependency file="SKSEMenuFramework.dll"' not in fomod:
        missing.append("FOMOD SMF fileDependency")
    if "build_plugins.py" not in fomod or "build_plugins.py" not in release:
        missing.append("build_plugins.py producer")
    if "SKSE/Plugins/Fonts" not in packaging:
        missing.append("SMF Fonts install path")
    leaked_host = []
    for label, text in (("create_fomod_installer.py", fomod), ("build_release_package.py", release)):
        if re.search(r"SKSEMenuFramework\.dll['\"]\s*,", text) or "SKSEMenuFramework.dll)" in text:
            leaked_host.append(label)
    stale_font_dest = "SKSE/Plugins/SpellHotbar/fonts" in fomod and 'destination="SKSE/Plugins/SpellHotbar/fonts' in fomod
    ok = not missing and not leaked_host and not stale_font_dest
    if leaked_host:
        detail = "packages SMF host DLL from " + ", ".join(leaked_host)
    elif stale_font_dest:
        detail = "FOMOD still installs fonts under SpellHotbar/fonts"
    elif missing:
        detail = "missing: " + ", ".join(missing)
    else:
        detail = "SMF dependency, Fonts path, and ESP producer are present; host DLL is not shipped"
    return CheckResult("package SMF/fonts/ESP producer", ok, detail)


def plugin_provenance() -> CheckResult:
    try:
        verify_provenance(ROOT)
    except (OSError, RuntimeError, ValueError) as error:
        return CheckResult("plugin provenance", False, str(error))
    return CheckResult("plugin provenance", True, "YAML FormIDs and five VMAD removals match the 0.0.14 manifest")


def working_tree_names() -> set[str]:
    names = set(git_output(["diff", "--name-only", "--cached"]))
    names.update(git_output(["diff", "--name-only"]))
    names.update(git_output(["ls-files", "--others", "--exclude-standard"]))
    return names


def upstream_only_diff(base: str) -> CheckResult:
    try:
        changed = set(git_output(["diff", "--name-only", f"{base}...HEAD"]))
        changed.update(working_tree_names())
    except RuntimeError as error:
        return CheckResult("upstream-only scope", False, str(error))

    addon_markers = (
        ".scratch/",
        "addon",
        "ability",
        "weapon_art",
        "skse_plugin/tests/fixtures/acceptance-matrix",
    )
    leaked = [
        path
        for path in sorted(changed)
        if any(marker in path.casefold() for marker in addon_markers)
    ]
    return CheckResult(
        "upstream-only scope",
        not leaked,
        "no addon paths in committed/staged/unstaged/untracked names"
        if not leaked
        else "leaked paths: " + ", ".join(leaked),
    )


def run(base: str) -> int:
    checks = (
        required_host_files(),
        no_private_imgui_dependencies(),
        plugin_source_invariants(),
        no_legacy_host_hooks(),
        serialization_formats_unchanged(),
        mcm_retired(),
        plugin_provenance(),
        packaging_smf_and_fonts(),
        upstream_only_diff(base),
        mcp_pages(),
        ui_smoke(),
    )

    for result in checks:
        status = "PASS" if result.ok else "FAIL"
        print(f"[{status}] {result.name}: {result.detail}")

    blocking = [result for result in checks if not result.ok]
    passed = sum(result.ok for result in checks)
    print(f"\n{passed}/{len(checks)} static migration checks passed")
    if blocking:
        print("Static acceptance is incomplete; build and runtime acceptance are separate gates.")
        return 1
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--base",
        default="upstream/master",
        help="Git base used to detect addon-only paths (default: upstream/master)",
    )
    args = parser.parse_args()
    return run(args.base)


if __name__ == "__main__":
    sys.exit(main())
