#!/usr/bin/env python3
"""Static smoke gate for every SH2 UI surface hosted by SMF.

The bind-menu failure was paint-without-input: InvisibleButton drew, but the
guest input callback unlinked mouse events before SMF's TranslateInputEvent.
This check encodes that class of bug:

1. Every ImGui/MCP widget SH2 calls must resolve to an SMF export that
   install() fail-closes on.
2. The guest input callback must not capture cursor/key events merely because
   a blocking SH2 window is open (bind-capture is the only allowed consume).
3. HUD overlays that sit under those windows must pass mouse through.
4. Opening a blocking SH2 window must dismiss the host control panel.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLUGIN = ROOT / "skse_plugin"
SRC = PLUGIN / "src"
HEADER = PLUGIN / "third_party" / "skse-menu-framework" / "SKSEMenuFramework.h"
GUEST = SRC / "smf" / "smf_guest.cpp"
INPUT = SRC / "input" / "input.cpp"
RENDER = SRC / "rendering" / "render_manager.cpp"

# Widgets that take clicks, keys, or drag. Draw-only helpers (Text, Dummy,
# SameLine) still need exports or they crash, but missing *these* is how a
# window looks alive while doing nothing.
INTERACTIVE_METHODS = frozenset(
    {
        "Button",
        "SmallButton",
        "InvisibleButton",
        "Checkbox",
        "RadioButton",
        "SliderFloat",
        "SliderInt",
        "InputText",
        "InputTextWithHint",
        "Combo",
        "BeginCombo",
        "Selectable",
        "BeginPopupModal",
        "OpenPopup",
        "CloseCurrentPopup",
        "BeginTable",
        "TableHeadersRow",
        "BeginDragDropSource",
        "SetDragDropPayload",
        "BeginDragDropTarget",
        "AcceptDragDropPayload",
        "CollapsingHeader",
        "IsItemHovered",
        "BeginItemTooltip",
        "IsItemClicked",
        "GetIO",
        "SetItemKeyOwner",
    }
)

# HUD windows are drawn every frame, including while a blocking editor is
# open. Without NoInputs they sit in the ImGui stack and steal hits.
HUD_WINDOW_NAMES = ("SpellHotbar", "SpellHotbarHUD", "SpellHotbarOblivionHUD")

UI_SURFACES = (
    ("MCP Keybinds", SRC / "mcp" / "mcp_pages.cpp", "draw_keybinds"),
    ("MCP Spell Bind Menu", SRC / "mcp" / "mcp_pages.cpp", "draw_bind_menu"),
    ("MCP Settings", SRC / "mcp" / "mcp_pages.cpp", "draw_settings"),
    ("MCP Bars", SRC / "mcp" / "mcp_pages.cpp", "draw_bars"),
    ("MCP Perks", SRC / "mcp" / "mcp_pages.cpp", "draw_perks"),
    ("MCP Presets", SRC / "mcp" / "mcp_pages.cpp", "draw_presets"),
    ("MCP Spells", SRC / "mcp" / "mcp_pages.cpp", "draw_spells"),
    ("MCP Util", SRC / "mcp" / "mcp_pages.cpp", "draw_util"),
    ("Binding Menu", SRC / "rendering" / "advanced_bind_menu.cpp", "drawFrame"),
    ("Spell Editor", SRC / "rendering" / "spell_editor.cpp", "renderEditor"),
    ("Spell Edit Dialog", SRC / "rendering" / "spell_edit_dialog.cpp", "drawEditDialog"),
    ("Potion Editor", SRC / "rendering" / "potion_editor.cpp", "renderEditor"),
    ("Icon Edit Dialog", SRC / "rendering" / "icon_edit_dialog.cpp", "drawEditDialog"),
    ("Bar Drag Settings", SRC / "rendering" / "bar_dragging_config_window.cpp", "draw_window"),
    ("Bar Drag Menu", SRC / "rendering" / "render_manager.cpp", "draw_drag_menu"),
    ("Tab Buttons", SRC / "rendering" / "gui_tab_button.cpp", "draw"),
    ("HUD", SRC / "rendering" / "render_manager.cpp", "render_hud"),
)


def read(path: Path) -> str:
    if not path.is_file():
        raise AssertionError(f"missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def iter_src_files() -> list[Path]:
    files = [path for path in SRC.rglob("*") if path.suffix in {".cpp", ".h"}]
    files.append(PLUGIN / "src" / "smf" / "imgui_mcp_compat.h")
    return sorted({path.resolve() for path in files})


def used_imgui_methods() -> dict[str, set[str]]:
    """Map ImGui::Method -> relative files that call it."""
    calls: dict[str, set[str]] = {}
    pattern = re.compile(r"\bImGui::([A-Za-z_][A-Za-z0-9_]*)\s*\(")
    for path in iter_src_files():
        text = path.read_text(encoding="utf-8")
        relative = str(path.relative_to(ROOT)).replace("\\", "/")
        for match in pattern.finditer(text):
            calls.setdefault(match.group(1), set()).add(relative)
    return calls


def export_belongs_to_method(method: str, export: str) -> bool:
    """Keep cimgui exports for this wrapper name; drop Manager helpers."""
    if not export.startswith("ig"):
        return False
    rest = export[2:]
    return rest == method or rest.startswith(method + "_") or rest == method + "V"


def header_method_exports(header_text: str) -> dict[str, set[str]]:
    """Map ImGuiMCP wrapper name -> SMF export strings it GetProcAddresses."""
    mapping: dict[str, set[str]] = {}
    method_re = re.compile(
        r"inline\s+[^{;]+?\b([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*?\)\s*\{",
        flags=re.DOTALL,
    )
    export_re = re.compile(r'GetMenuFrameworkFunction<[^>]+>\("([^"]+)"\)')
    for match in method_re.finditer(header_text):
        name = match.group(1)
        start = match.end()
        depth = 1
        index = start
        while index < len(header_text) and depth:
            char = header_text[index]
            if char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
            index += 1
        body = header_text[start : index - 1]
        exports = {
            export
            for export in export_re.findall(body)
            if export_belongs_to_method(name, export)
        }
        if exports:
            mapping.setdefault(name, set()).update(exports)
    return mapping


def required_export_literals(guest_text: str) -> set[str]:
    block = re.search(
        r"constexpr std::array required_exports\{(.*?)\};",
        guest_text,
        flags=re.DOTALL,
    )
    if block is None:
        raise AssertionError("smf_guest.cpp is missing required_exports")
    return set(re.findall(r'"([^"]+)"', block.group(1)))


def capture_assignments_outside_bind_capture(input_text: str) -> list[int]:
    """Line numbers of captureEvent = true that are not bind-capture consumes."""
    armed_start = input_text.find("if (capture.armed())")
    if armed_start < 0:
        raise AssertionError("input.cpp is missing bind-capture armed() gate")
    rest = input_text[armed_start:]
    else_match = re.search(r"\n\s*\} else \{", rest)
    if else_match is None:
        raise AssertionError("input.cpp bind-capture armed() branch has no else")
    armed_end = armed_start + else_match.start()
    leftover: list[int] = []
    for match in re.finditer(r"captureEvent\s*=\s*true", input_text):
        if match.start() < armed_start or match.start() >= armed_end:
            leftover.append(input_text.count("\n", 0, match.start()) + 1)
    return leftover


def extract_function(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise AssertionError(f"missing {signature}")
    brace = source.find("{", start)
    depth = 0
    index = brace
    while index < len(source):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
        index += 1
    raise AssertionError(f"unclosed {signature}")


def hud_windows_pass_mouse(render_text: str) -> list[str]:
    hud = extract_function(render_text, "void RenderManager::render_hud()")
    offenders: list[str] = []
    for match in re.finditer(
        r"static constexpr ImGuiWindowFlags window_flag\s*=([^;]+);", hud
    ):
        if "ImGuiWindowFlags_NoInputs" not in match.group(1):
            offenders.append(match.group(1).strip())
    for name in HUD_WINDOW_NAMES:
        if not re.search(rf'ImGui::Begin\(\s*"{re.escape(name)}"', hud):
            offenders.append(f"missing Begin({name})")
    return offenders


def verify() -> None:
    header = read(HEADER)
    guest = read(GUEST)
    input_text = read(INPUT)
    render = read(RENDER)
    pages = read(SRC / "mcp" / "mcp_pages.cpp")

    used = used_imgui_methods()
    mapping = header_method_exports(header)
    required = required_export_literals(guest)

    missing_wrappers: list[str] = []
    missing_exports: dict[str, set[str]] = {}
    interactive_missing: dict[str, set[str]] = {}

    for method, files in sorted(used.items()):
        exports = mapping.get(method)
        if not exports:
            # Compat shims (GetIO/GetWindowDrawList) may wrap MCP helpers
            # rather than GetProcAddress directly; those still must exist.
            if method in INTERACTIVE_METHODS:
                missing_wrappers.append(f"{method} ({', '.join(sorted(files))})")
            continue
        if exports & required:
            continue
        missing_exports[method] = set(exports)
        if method in INTERACTIVE_METHODS:
            interactive_missing[method] = set(exports)

    if interactive_missing:
        detail = "; ".join(
            f"{method} -> {', '.join(sorted(exports))}"
            for method, exports in interactive_missing.items()
        )
        raise AssertionError(
            "interactive ImGui widgets used without fail-closed SMF exports: " + detail
        )

    leftover_captures = capture_assignments_outside_bind_capture(input_text)
    if leftover_captures and "if (!captureEvent && !smf_blocking)" not in input_text:
        raise AssertionError(
            "gameplay input capture remains but is not gated off SMF blocking windows"
        )
    if re.search(
        r"should_block_game_(?:cursor|key)_inputs\(\)[\s\S]{0,240}captureEvent\s*=\s*true",
        input_text,
    ):
        raise AssertionError(
            "guest input callback still strips events when an SMF window is open"
        )
    if "if (!captureEvent && !smf_blocking)" not in input_text:
        raise AssertionError(
            "gameplay input must stay off while an SMF blocking window is open"
        )

    hud_offenders = hud_windows_pass_mouse(render)
    if hud_offenders:
        raise AssertionError(
            "HUD windows must use ImGuiWindowFlags_NoInputs so they cannot steal "
            "clicks from blocking SH2 windows: " + ", ".join(sorted(set(hud_offenders)))
        )

    if "close_host_menu()" not in guest or "GetMainWindow" not in guest:
        raise AssertionError("opening a blocking SH2 window must dismiss the SMF control panel")

    missing_surfaces = [
        name for name, path, needle in UI_SURFACES if needle and needle not in read(path)
    ]
    if missing_surfaces:
        raise AssertionError("UI surface entry points missing: " + ", ".join(missing_surfaces))

    openers = (
        'ImGui::Button("Open Spell Bind Menu")',
        'ImGui::Button("Open Spell Editor")',
        'ImGui::Button("Open Potion Editor")',
        'ImGui::Button("Drag Main Bar")',
        'ImGui::Button("Drag Oblivion Mode Bar")',
    )
    missing_openers = [opener for opener in openers if opener not in pages]
    if missing_openers:
        raise AssertionError("MCP is missing live openers: " + ", ".join(missing_openers))

    if "ImGui::InvisibleButton" not in read(SRC / "rendering" / "gui_tab_button.cpp"):
        raise AssertionError("category tabs must remain InvisibleButton hit targets")
    if "BeginDragDropSource" not in read(SRC / "rendering" / "advanced_bind_menu.cpp"):
        raise AssertionError("bind menu lost drag-and-drop")
    if 'Button("Rebind")' not in pages or 'Button("Unmap")' not in pages:
        raise AssertionError("keybind rows lost Rebind/Unmap")

    # Non-interactive missing exports are still a crash at first use. Report
    # them as the same class of gate so a widget cannot ship ungated.
    if missing_exports:
        detail = "; ".join(
            f"{method} -> {', '.join(sorted(exports))}"
            for method, exports in sorted(missing_exports.items())
        )
        raise AssertionError(
            "ImGui wrappers used without fail-closed SMF exports: " + detail
        )

    if missing_wrappers:
        raise AssertionError(
            "interactive ImGui methods have no SMF header wrapper: "
            + ", ".join(missing_wrappers)
        )


if __name__ == "__main__":
    try:
        verify()
    except AssertionError as exc:
        print(f"SMF UI smoke: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
    print("SMF UI smoke: PASS")
