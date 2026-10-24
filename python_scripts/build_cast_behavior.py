"""
Builds the custom cast behavior for Spell Hotbar 2 and the Nemesis/Pandora patch that hooks it in.

Output (written to the dev mod folder):
  meshes/actors/character/behaviors/SpellHotbarCast.hkx   fork of the vanilla shout behavior
  Nemesis_Engine/mod/shbc/                               patch (Nemesis format, also read by Pandora)

The vanilla behaviors are read from the Nemesis cache and converted with hkxcmd, nothing vanilla is stored in the repo.
"""
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path

mod_root_path = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2")
vanilla_behavior_path = Path(r"F:\Skyrim Dev\ADT\mods\Project New Reign - Nemesis Unlimited Behavior Engine\Nemesis_Engine\cached_behaviors")
hkxcmd_path = Path(r"F:\Skyrim Dev\TOOLS\SBT\hkxcmd.exe")
anim_set_header_path = Path(__file__).parent.parent / "skse_plugin" / "src" / "casts" / "cast_behavior_anims.h"

mod_code = "shbc"
behavior_name = "SpellHotbarCast"

event_cast_start = "SHB_CastStart"
event_cast_release = "SHB_CastRelease"
event_cast_stop = "SHB_CastStop"
# raised by the graph itself when the charge start clip ends
event_charge_loop = "SHB_ChargeLoop"
# raised when the release clip of a set with release and hold ends, the plugin fires the spell then
event_cast_hold = "SHB_CastHold"
var_casting = "SHB_bCasting"
var_anim = "SHB_iAnim"
# set by the plugin during an aimed hotbar cast on horseback, enables the torso aim modifier in 0_master
var_mounted_aim = "SHB_bMountedAim"
# turn limit of the torso aim in degrees, the plugin lowers it to 0 at the end of the cast so the torso turns back smoothly
var_mounted_aim_limit = "SHB_fMountedAimLimit"
# For conditions of animation replacers (OAR CompareValues with a graph variable), set by the plugin for the whole hotbar
# spell cast and 0 otherwise: the local form id of the spell (as in its plugin, ESL aware), a 24 bit hash of the plugin's
# file name and the casting hand (1 left, 2 right, 3 both). The spell edit dialog shows the values of a spell.
var_cast_spell = "SHB_iCastSpell"
var_cast_plugin = "SHB_iCastPlugin"
var_cast_hand = "SHB_iCastHand"

# character property with all bones a cast clip may drive: everything but the root and the camera / look nodes
full_body_property = "ExcludeRoot+Cam+CamCtrl+Look"

# weight variable of each overlay -> (character properties with the bones it takes over, properties whose bones are left out)
# The plugin keeps one of the first three active for the whole cast. A body overlay is faded in on top while the player stands still:
# the whole pose comes from the cast clip then, weapon stances like two handed or dual wield stand turned to the side.
# Overlays that are active together must not share bones, each one is an own instance of the cast behavior.
overlay_weights = {
    "SHB_fWeightLeft": (["LeftArm"], []),
    "SHB_fWeightRight": (["RightArm"], []),
    # dual and ritual casts move the torso, vanilla layers its cast states with UpperBody as well
    "SHB_fWeightBoth": (["UpperBody"], []),
    # with a single hand cast: the weapon arm stays with the stance
    "SHB_fWeightBodySingle": ([full_body_property], ["Arms"]),
    "SHB_fWeightBodyBoth": ([full_body_property], ["UpperBody"]),
}

new_events = [event_cast_start, event_cast_release, event_cast_stop, event_charge_loop, event_cast_hold]
# name, havok type, initial word value
cast_variables = [(var_casting, "VARIABLE_TYPE_BOOL", 0), (var_anim, "VARIABLE_TYPE_INT32", 0)]
new_variables = cast_variables + [(name, "VARIABLE_TYPE_REAL", 0) for name in overlay_weights] + [(var_mounted_aim, "VARIABLE_TYPE_BOOL", 0), (var_mounted_aim_limit, "VARIABLE_TYPE_REAL", 0)] + [(name, "VARIABLE_TYPE_INT32", 0) for name in (var_cast_spell, var_cast_plugin, var_cast_hand)]

# Turns the rider's torso toward the camera during a mounted hotbar cast, a copy of the modifier of mounted bow aiming
mounted_aim_template = "MC_BSDirectAtModifier_Bow"
# member of the modifier -> variable of 0_master
mounted_aim_bindings = [
    ("onGain", "AimGainOnMounted"),
    ("offGain", "AimGainOff"),
    ("directAtCameraX", "camerafromx"),
    ("directAtCameraY", "camerafromy"),
    ("directAtCameraZ", "camerafromz"),
    ("active", "bAimActive"),
    ("currentHeadingOffset", "AimHeadingCurrent"),
    ("currentPitchOffset", "AimPitchCurrent"),
    ("enable", var_mounted_aim),
    ("limitHeadingDegrees", var_mounted_aim_limit),
    ("limitPitchDegrees", var_mounted_aim_limit),
]

# seconds, blend between the states of the cast behavior (charge start -> charge loop -> release)
transition_blend_time = 0.1

# sub folders of the character with an own behavior project, third and first person
projects = ["", "_1stperson"]


@dataclass
class AnimSet:
    """Clips of one cast animation, file names in Animations/ without extension"""
    name: str
    # played once when the cast starts
    charge_start: str
    # looped after charge_start until the release, optional
    charge_loop: str | None = None
    # played once on release, the cast state is left when it ends (unless there is a hold clip)
    release: str | None = None
    # looped on release until the cast is stopped (concentration), after the release clip if there is one
    hold: str | None = None


# SHB_iAnim is the index into this list, the plugin gets the same order through the generated header
anim_sets = [
    AnimSet("aimed_left", "MLh_PreCharge", "MLh_ChargeLoop", "MLh_Release"),
    AnimSet("aimed_right", "MRh_PreCharge", "MRh_ChargeLoop", "MRh_Release"),
    # vanilla has no MLh_SelfPreCharge
    AnimSet("self_left", "MLh_PreCharge", "MLh_SelfChargeLoop", "MLh_SelfRelease"),
    AnimSet("self_right", "MRh_SelfPreCharge", "MRh_SelfChargeLoop", "MRh_SelfRelease"),
    AnimSet("aimed_conc_left", "MLh_PreAimedCon", hold="MLh_AimedConcentration"),
    AnimSet("aimed_conc_right", "MRh_PreAimedCon", hold="MRh_AimedConcentration"),
    AnimSet("self_conc_left", "MLh_PreSelfCon", hold="MLh_SelfConcentration"),
    AnimSet("self_conc_right", "MRh_PreSelfCon", hold="MRh_SelfConcentration"),
    AnimSet("ward_left", "MLh_PreWardLoop", hold="MLh_WardLoop"),
    AnimSet("ward_right", "MRh_PreWardLoop", hold="MRh_WardLoop"),
    AnimSet("staff_left", "StfMagic_PreCharge", "StfMagic_ChargeLoop", "StfMagic_Release"),
    AnimSet("staff_right", "StfMagicRight_PreCharge", "StfMagicRight_ChargeLoop", "StfMagicRight_Release"),
    AnimSet("staff_conc_left", "StfMagic_PreAimCon", hold="StfMagic_AimConLoop"),
    AnimSet("staff_conc_right", "StfMagicRight_PreAimCon", hold="StfMagicRight_AimConLoop"),
    AnimSet("dual_aimed", "DMagPreAimConCharge", "DMagAimConCharge", "DMagAimRelease"),
    AnimSet("dual_self", "DMagPreSelfConCharge", "DMagSelfConCharge", "DMagSelfRelease"),
    AnimSet("dual_aimed_conc", "DMagPreAimCon", hold="DMagAimConLoop"),
    AnimSet("dual_self_conc", "DMagPreSelfCon", hold="DMagSelfConLoop"),
    AnimSet("ritual_aimed", "RitualSpell_Charge", "RitualSpell_Ready", "RitualSpell_AimRelease"),
    AnimSet("ritual_self", "RitualSpell_Charge", "RitualSpell_Ready", "RitualSpell_Release"),
    # as vanilla RitualSpellAimConcentrationStart (Lightning Storm): ritual charge, then the dual concentration loop
    AnimSet("ritual_aimed_conc", "RitualSpell_Charge", "RitualSpell_Ready", "RitualSpell_AimRelease", "DMagAimConLoop"),
]


def _run_hkxcmd(src: Path, dst: Path, fmt: str):
    subprocess.run([str(hkxcmd_path), "convert", "-i", str(src), "-o", str(dst), f"-v:{fmt}"], check=True, capture_output=True)
    if not dst.exists():
        raise RuntimeError(f"hkxcmd did not write {dst}")


def load_vanilla_xml(name: str, tmp: Path, project: str = "") -> list[str]:
    """project is the sub folder of the character, empty for third person"""
    out = tmp / project / f"{name}.xml"
    out.parent.mkdir(exist_ok=True)
    _run_hkxcmd(vanilla_behavior_path / project / f"{name}.hkx", out, "XML")
    return out.read_text(encoding="ascii").splitlines()


def node_lines(xml: list[str], node_id: str) -> list[str]:
    """Raw lines of one top level hkobject"""
    start = next(i for i, line in enumerate(xml) if line.startswith(f'\t\t<hkobject name="{node_id}"'))
    end = next(i for i in range(start, len(xml)) if xml[i] == "\t\t</hkobject>")
    return xml[start:end + 1]


def nemesis_format(lines: list[str]) -> list[str]:
    """hkxcmd xml -> Nemesis node file layout: no ignored member comments, one array element per line"""
    out = []
    for line in lines:
        if "SERIALIZE_IGNORED" in line:
            continue
        stripped = line.strip()
        if stripped and not stripped.startswith("<") and " " in stripped:
            indent = line[:len(line) - len(line.lstrip())]
            # vectors lose their brackets as well
            out.extend(indent + v for v in stripped.replace("(", "").replace(")", "").split())
        else:
            out.append(line)
    return out


def insert_before_param_end(lines: list[str], param: str, new_lines: list[str]) -> list[str]:
    """Add a MOD_CODE block at the end of the given top level array hkparam of a node"""
    start = next(i for i, line in enumerate(lines) if line.startswith(f'\t\t\t<hkparam name="{param}"'))
    if lines[start].endswith("</hkparam>"):
        raise RuntimeError(f"array {param} is empty, appending to an empty array is not supported")
    end = next(i for i in range(start, len(lines)) if lines[i] == "\t\t\t</hkparam>")
    return lines[:end] + [f"<!-- MOD_CODE ~{mod_code}~ OPEN -->"] + new_lines + ["<!-- CLOSE -->"] + lines[end:]


def write_node(folder: Path, node_id: str, lines: list[str]):
    folder.mkdir(parents=True, exist_ok=True)
    (folder / f"{node_id}.txt").write_text("\r\n".join(lines) + "\r\n", encoding="ascii", newline="")


def _find_data_nodes(xml: list[str]) -> dict[str, str]:
    ids = {}
    for line in xml:
        m = re.match(r'\t\t<hkobject name="(#\d+)" class="(hkbBehaviorGraphStringData|hkbVariableValueSet|hkbBehaviorGraphData)"', line)
        if m:
            ids[m.group(2)] = m.group(1)
    return ids


def _indent(lines: list[str], tabs: int) -> list[str]:
    return ["\t" * tabs + line for line in lines]


def patch_data_nodes(xml: list[str], out_folder: Path, events: list[str], variables: list[tuple[str, str, int]]):
    """Register new events and variables in the string data / value set / graph data of a behavior file"""
    ids = _find_data_nodes(xml)

    string_data = nemesis_format(node_lines(xml, ids["hkbBehaviorGraphStringData"]))
    string_data = insert_before_param_end(string_data, "eventNames", _indent([f"<hkcstring>{e}</hkcstring>" for e in events], 4))
    if variables:
        string_data = insert_before_param_end(string_data, "variableNames", _indent([f"<hkcstring>{v[0]}</hkcstring>" for v in variables], 4))
    write_node(out_folder, ids["hkbBehaviorGraphStringData"], string_data)

    graph_data = nemesis_format(node_lines(xml, ids["hkbBehaviorGraphData"]))
    event_infos = []
    for _ in events:
        event_infos += ["<hkobject>", '\t<hkparam name="flags">0</hkparam>', "</hkobject>"]
    graph_data = insert_before_param_end(graph_data, "eventInfos", _indent(event_infos, 4))
    if variables:
        var_infos = []
        var_values = []
        for _, var_type, value in variables:
            var_infos += [
                "<hkobject>",
                '\t<hkparam name="role">',
                "\t\t<hkobject>",
                '\t\t\t<hkparam name="role">ROLE_DEFAULT</hkparam>',
                '\t\t\t<hkparam name="flags">0</hkparam>',
                "\t\t</hkobject>",
                "\t</hkparam>",
                f'\t<hkparam name="type">{var_type}</hkparam>',
                "</hkobject>",
            ]
            var_values += ["<hkobject>", f'\t<hkparam name="value">{value}</hkparam>', "</hkobject>"]
        graph_data = insert_before_param_end(graph_data, "variableInfos", _indent(var_infos, 4))

        value_set = nemesis_format(node_lines(xml, ids["hkbVariableValueSet"]))
        value_set = insert_before_param_end(value_set, "wordVariableValues", _indent(var_values, 4))
        write_node(out_folder, ids["hkbVariableValueSet"], value_set)
    write_node(out_folder, ids["hkbBehaviorGraphData"], graph_data)


def character_bone_weights(character_xml: list[str], names: list[str]) -> dict[str, list[float]]:
    """Bone weight arrays of the given character properties (LeftArm, ...) from a character file"""
    def array_values(lines: list[str], param: str) -> list[str]:
        start = next(i for i, line in enumerate(lines) if f'<hkparam name="{param}" numelements="' in line)
        values = []
        for line in lines[start + 1:]:
            if "</hkparam>" in line and "<hkparam" not in line:
                break
            values.append(line.strip())
        return values

    string_data = node_lines(character_xml, next(re.match(r'\t\t<hkobject name="(#\d+)"', line).group(1) for line in character_xml if 'class="hkbCharacterStringData"' in line))
    properties = [re.search(r"<hkcstring>(.*)</hkcstring>", v).group(1) for v in array_values(string_data, "characterPropertyNames")]

    character_data = node_lines(character_xml, next(re.match(r'\t\t<hkobject name="(#\d+)"', line).group(1) for line in character_xml if 'class="hkbCharacterData"' in line))
    value_set_id = next(re.search(r">(#\d+)<", line).group(1) for line in character_data if '<hkparam name="characterPropertyValues">' in line)
    value_set = node_lines(character_xml, value_set_id)
    # the word value of a pointer property is the index into the variant values
    words = [int(re.search(r">(-?\d+)<", line).group(1)) for line in array_values(value_set, "wordVariableValues") if '<hkparam name="value">' in line]
    variants = " ".join(array_values(value_set, "variantVariableValues")).split()

    result = {}
    for name in names:
        array = node_lines(character_xml, variants[words[properties.index(name)]])
        if 'class="hkbBoneWeightArray"' not in array[0]:
            raise RuntimeError(f"character property {name} is no bone weight array")
        result[name] = [float(v) for v in " ".join(array_values(array, "boneWeights")).split()]
    return result


def patch_master(xml: list[str], out_folder: Path, overlay_bones: dict[str, list[float]], horse_xml: list[str] | None = None):
    """
    Layers the cast behavior over everything below the root modifier generator, the way vanilla layers shield and spell:
    a blender whose base child nearly gives up the arm bones (weight 0.001) and one overlay child per hand with a variable bound weight.
    As long as the weight variables are 0 the overlays are inactive and the base owns all bones.
    """
    patch_data_nodes(xml, out_folder, new_events, new_variables)

    root_id = next(re.match(r'\t\t<hkobject name="(#\d+)" class="hkbModifierGenerator"', line).group(1) for i, line in enumerate(xml)
                   if 'class="hkbModifierGenerator"' in line and '\t\t\t<hkparam name="name">RootModifierGenerator</hkparam>' in node_lines(xml, re.match(r'\t\t<hkobject name="(#\d+)"', line).group(1)))
    root = nemesis_format(node_lines(xml, root_id))
    index = next(i for i, line in enumerate(root) if line.startswith('\t\t\t<hkparam name="generator">'))
    base_generator = re.search(r">(#\d+)<", root[index]).group(1)

    new_index = 0

    def new_node(node_class: str, signature: str, params: list[str]) -> str:
        nonlocal new_index
        node_id = f"#{mod_code}${new_index}"
        new_index += 1
        write_node(out_folder, node_id, [f'\t\t<hkobject name="{node_id}" class="{node_class}" signature="{signature}">'] + _indent(params, 3) + ["\t\t</hkobject>"])
        return node_id

    def bone_weights(values: list[float]) -> str:
        return new_node("hkbBoneWeightArray", "0xcd902b77", [
            '<hkparam name="variableBindingSet">null</hkparam>',
            f'<hkparam name="boneWeights" numelements="{len(values)}">',
            *[f"\t{v:.6f}" for v in values],
            "</hkparam>",
        ])

    def child(generator: str, weights: str, weight: float, world_from_model: float, binding: str = "null") -> str:
        return new_node("hkbBlenderGeneratorChild", "0xe2b384b0", [
            f'<hkparam name="variableBindingSet">{binding}</hkparam>',
            f'<hkparam name="generator">{generator}</hkparam>',
            f'<hkparam name="boneWeights">{weights}</hkparam>',
            f'<hkparam name="weight">{weight:.6f}</hkparam>',
            f'<hkparam name="worldFromModelWeight">{world_from_model:.6f}</hkparam>',
        ])

    # the base nearly gives up every bone one of the overlays can take, without an active overlay on a bone it still owns it alone
    any_overlay = [max(values) for values in zip(*overlay_bones.values())]
    children = [child(base_generator, bone_weights([0.001 if v > 0.0 else 1.0 for v in any_overlay]), 1.0, 1.0)]
    for variable, weights in overlay_bones.items():
        reference = new_node("hkbBehaviorReferenceGenerator", "0xfcb5423", [
            '<hkparam name="variableBindingSet">null</hkparam>',
            '<hkparam name="userData">0</hkparam>',
            f'<hkparam name="name">{behavior_name}_{variable}_BFR</hkparam>',
            f'<hkparam name="behaviorName">Behaviors\\{behavior_name}.hkx</hkparam>',
        ])
        binding = new_node("hkbVariableBindingSet", "0x338ad4ff", [
            '<hkparam name="bindings" numelements="1">',
            "\t<hkobject>",
            '\t\t<hkparam name="memberPath">weight</hkparam>',
            f'\t\t<hkparam name="variableIndex">$variableID[{variable}]$</hkparam>',
            '\t\t<hkparam name="bitIndex">-1</hkparam>',
            '\t\t<hkparam name="bindingType">BINDING_TYPE_VARIABLE</hkparam>',
            "\t</hkobject>",
            "</hkparam>",
            '<hkparam name="indexOfBindingToEnable">-1</hkparam>',
        ])
        children.append(child(reference, bone_weights(weights), 0.0, 0.0, binding))

    blender = new_node("hkbBlenderGenerator", "0x22df7147", [
        '<hkparam name="variableBindingSet">null</hkparam>',
        '<hkparam name="userData">0</hkparam>',
        f'<hkparam name="name">{behavior_name}_Blend</hkparam>',
        '<hkparam name="referencePoseWeightThreshold">0.000000</hkparam>',
        '<hkparam name="blendParameter">1.000000</hkparam>',
        '<hkparam name="minCyclicBlendParameter">0.000000</hkparam>',
        '<hkparam name="maxCyclicBlendParameter">1.000000</hkparam>',
        # the base (first child) has to stay the sync master, otherwise a cast clip takes over and the locomotion cycle freezes
        '<hkparam name="indexOfSyncMasterChild">0</hkparam>',
        '<hkparam name="flags">0</hkparam>',
        '<hkparam name="subtractLastChild">false</hkparam>',
        f'<hkparam name="children" numelements="{len(children)}">',
        *["\t" + c for c in children],
        "</hkparam>",
    ])

    root[index:index + 1] = [
        f"<!-- MOD_CODE ~{mod_code}~ OPEN -->",
        f'\t\t\t<hkparam name="generator">{blender}</hkparam>',
        "<!-- ORIGINAL -->",
        root[index],
        "<!-- CLOSE -->",
    ]
    write_node(out_folder, root_id, root)
    print(f"  blender between {root_id} RootModifierGenerator and {base_generator}")

    if horse_xml is not None:
        # The torso aim: a copy of the mounted bow aim modifier of horsebehavior, appended to the root modifier list so it
        # turns the spine of the final pose. Enabled by the plugin only for an aimed cast on horseback.
        template_id = next(re.match(r'\t\t<hkobject name="(#\d+)" class="BSDirectAtModifier"', line).group(1) for line in horse_xml
                           if 'class="BSDirectAtModifier"' in line
                           and f'\t\t\t<hkparam name="name">{mounted_aim_template}</hkparam>' in node_lines(horse_xml, re.match(r'\t\t<hkobject name="(#\d+)"', line).group(1)))
        binding = new_node("hkbVariableBindingSet", "0x338ad4ff", [
            f'<hkparam name="bindings" numelements="{len(mounted_aim_bindings)}">',
            *[line for member, variable in mounted_aim_bindings for line in (
                "\t<hkobject>",
                f'\t\t<hkparam name="memberPath">{member}</hkparam>',
                f'\t\t<hkparam name="variableIndex">$variableID[{variable}]$</hkparam>',
                '\t\t<hkparam name="bitIndex">-1</hkparam>',
                '\t\t<hkparam name="bindingType">BINDING_TYPE_VARIABLE</hkparam>',
                "\t</hkobject>",
            )],
            "</hkparam>",
            '<hkparam name="indexOfBindingToEnable">-1</hkparam>',
        ])
        template = nemesis_format(node_lines(horse_xml, template_id))
        params = [line[3:] if line.startswith("\t\t\t") else line for line in template[1:-1]]
        # an inline vector is one value per line in the node files (targetLocation)
        expanded = []
        for line in params:
            m = re.match(r'(	*)(<hkparam name="\w+">)\(([^)]*)\)</hkparam>$', line)
            if m:
                expanded += [m.group(1) + m.group(2), *[m.group(1) + "	" + v for v in m.group(3).split()], m.group(1) + "</hkparam>"]
            else:
                expanded.append(line)
        params = expanded
        for i, line in enumerate(params):
            if line.startswith('<hkparam name="variableBindingSet">'):
                params[i] = f'<hkparam name="variableBindingSet">{binding}</hkparam>'
            elif line.startswith('<hkparam name="name">'):
                params[i] = '<hkparam name="name">SHB_MountedAim_DAM</hkparam>'
        signature = re.search(r'signature="(0x[0-9a-f]+)"', template[0]).group(1)
        aim = new_node("BSDirectAtModifier", signature, params)

        modifier_list_id = re.search(r">(#\d+)<", next(line for line in root if line.startswith('\t\t\t<hkparam name="modifier">'))).group(1)
        modifier_list = nemesis_format(node_lines(xml, modifier_list_id))
        if 'class="hkbModifierList"' not in modifier_list[0]:
            raise RuntimeError(f"root modifier {modifier_list_id} is no modifier list")
        write_node(out_folder, modifier_list_id, insert_before_param_end(modifier_list, "modifiers", [f"\t\t\t\t{aim}"]))
        print(f"  mounted torso aim {aim} (copy of {template_id} {mounted_aim_template}) in {modifier_list_id}")


def set_param(lines: list[str], name: str, value: str):
    """Set a single line top level hkparam of a node"""
    prefix = f'\t\t\t<hkparam name="{name}">'
    index = next(i for i, line in enumerate(lines) if line.startswith(prefix) and line.endswith("</hkparam>"))
    lines[index] = f"{prefix}{value}</hkparam>"


def _array_range(lines: list[str], name: str) -> tuple[int, int, int]:
    start = next(i for i, line in enumerate(lines) if line.startswith(f'\t\t\t<hkparam name="{name}" numelements="'))
    count = int(re.search(r'numelements="(\d+)"', lines[start]).group(1))
    end = start if lines[start].endswith("</hkparam>") else next(i for i in range(start, len(lines)) if lines[i] == "\t\t\t</hkparam>")
    return start, end, count


def set_array(lines: list[str], name: str, count: int, body: list[str]):
    """Replace a top level array hkparam of a node"""
    start, end, _ = _array_range(lines, name)
    lines[start:end + 1] = [f'\t\t\t<hkparam name="{name}" numelements="{count}">'] + body + ["\t\t\t</hkparam>"]


def append_array(lines: list[str], name: str, count: int, body: list[str]):
    """Add elements to the end of a top level array hkparam of a node"""
    start, end, old_count = _array_range(lines, name)
    set_array(lines, name, old_count + count, lines[start + 1:end] + body)


class BehaviorXml:
    """hkxcmd xml of a behavior file with helpers to add nodes cloned from existing ones"""

    def __init__(self, lines: list[str]):
        self.lines = list(lines)
        self._next_id = 9000
        # id of the node new nodes are inserted in front of
        self.anchor = ""

    def _range(self, node_id: str) -> tuple[int, int]:
        start = next(i for i, line in enumerate(self.lines) if line.startswith(f'\t\t<hkobject name="{node_id}"'))
        end = next(i for i in range(start, len(self.lines)) if self.lines[i] == "\t\t</hkobject>")
        return start, end

    def get(self, node_id: str) -> list[str]:
        start, end = self._range(node_id)
        return self.lines[start:end + 1]

    def replace(self, node_id: str, lines: list[str]):
        start, end = self._range(node_id)
        self.lines[start:end + 1] = lines

    def add(self, lines: list[str]):
        # the file lists nodes before the nodes referencing them, new nodes go in front of the anchor that will use them
        position, _ = self._range(self.anchor)
        self.lines[position:position] = lines + [""]

    def clone(self, template_id: str) -> tuple[str, list[str]]:
        """Copy of a node under a new id, has to be passed to add() after editing"""
        new_id = f"#{self._next_id}"
        self._next_id += 1
        lines = self.get(template_id)
        lines[0] = lines[0].replace(f'name="{template_id}"', f'name="{new_id}"')
        return new_id, lines

    def find(self, node_class: str, name: str) -> str:
        """Id of the node with the given class and name hkparam"""
        for i, line in enumerate(self.lines):
            m = re.match(rf'\t\t<hkobject name="(#\d+)" class="{node_class}"', line)
            if m and f'\t\t\t<hkparam name="name">{name}</hkparam>' in self.get(m.group(1)):
                return m.group(1)
        raise RuntimeError(f"no {node_class} named {name}")

    def param(self, node_id: str, name: str) -> str:
        prefix = f'\t\t\t<hkparam name="{name}">'
        line = next(line for line in self.get(node_id) if line.startswith(prefix))
        return line[len(prefix):-len("</hkparam>")]

    def data_node(self, node_class: str) -> str:
        return next(re.match(r'\t\t<hkobject name="(#\d+)"', line).group(1) for line in self.lines if f'class="{node_class}"' in line)

    def names(self, array: str) -> list[str]:
        lines = self.get(self.data_node("hkbBehaviorGraphStringData"))
        start, end, _ = _array_range(lines, array)
        return [re.search(r"<hkcstring>(.*)</hkcstring>", line).group(1) for line in lines[start + 1:end]]

    def add_event(self, name: str):
        string_data = self.data_node("hkbBehaviorGraphStringData")
        lines = self.get(string_data)
        append_array(lines, "eventNames", 1, [f"\t\t\t\t<hkcstring>{name}</hkcstring>"])
        self.replace(string_data, lines)
        graph_data = self.data_node("hkbBehaviorGraphData")
        lines = self.get(graph_data)
        append_array(lines, "eventInfos", 1, _indent(["<hkobject>", '\t<hkparam name="flags">0</hkparam>', "</hkobject>"], 4))
        self.replace(graph_data, lines)

    def add_variable(self, name: str, var_type: str, value: int):
        string_data = self.data_node("hkbBehaviorGraphStringData")
        lines = self.get(string_data)
        append_array(lines, "variableNames", 1, [f"\t\t\t\t<hkcstring>{name}</hkcstring>"])
        self.replace(string_data, lines)
        graph_data = self.data_node("hkbBehaviorGraphData")
        lines = self.get(graph_data)
        append_array(lines, "variableInfos", 1, _indent([
            "<hkobject>",
            '\t<hkparam name="role">',
            "\t\t<hkobject>",
            '\t\t\t<hkparam name="role">ROLE_DEFAULT</hkparam>',
            '\t\t\t<hkparam name="flags">0</hkparam>',
            "\t\t</hkobject>",
            "\t</hkparam>",
            f'\t<hkparam name="type">{var_type}</hkparam>',
            "</hkobject>",
        ], 4))
        self.replace(graph_data, lines)
        value_set = self.data_node("hkbVariableValueSet")
        lines = self.get(value_set)
        append_array(lines, "wordVariableValues", 1, _indent(["<hkobject>", f'\t<hkparam name="value">{value}</hkparam>', "</hkobject>"], 4))
        self.replace(value_set, lines)

    def clear_events_and_variables(self):
        """Empty event / variable tables, everything still referencing the old indices has to be rebuilt or removed afterwards"""
        for node_class, arrays in (("hkbBehaviorGraphStringData", ("eventNames", "variableNames")),
                                   ("hkbBehaviorGraphData", ("variableInfos", "eventInfos")),
                                   ("hkbVariableValueSet", ("wordVariableValues", "quadVariableValues", "variantVariableValues"))):
            node_id = self.data_node(node_class)
            lines = self.get(node_id)
            for array in arrays:
                if array in ("quadVariableValues", "variantVariableValues"):
                    if _array_range(lines, array)[2] != 0:
                        raise RuntimeError(f"{array} is not empty")
                else:
                    set_array(lines, array, 0, [])
            self.replace(node_id, lines)

    def remove_unreachable(self) -> int:
        """Drop all nodes that are not referenced from the top level object anymore"""
        top = re.search(r'toplevelobject="(#\d+)"', "\n".join(self.lines[:5])).group(1)
        ids = [re.match(r'\t\t<hkobject name="(#\d+)"', line).group(1) for line in self.lines if line.startswith('\t\t<hkobject name="#')]
        # anything that looks like an id but is none (character entities) is filtered by the known ids
        refs = {node_id: set(re.findall(r"#\d+", "\n".join(self.get(node_id)[1:]))).intersection(ids) for node_id in ids}
        reachable = set()
        todo = [top]
        while todo:
            node_id = todo.pop()
            if node_id not in reachable:
                reachable.add(node_id)
                todo.extend(refs[node_id])
        removed = [node_id for node_id in ids if node_id not in reachable]
        for node_id in removed:
            start, end = self._range(node_id)
            # also the empty line after the node
            del self.lines[start:end + 2]
        return len(removed)


class CastGraphBuilder:
    """Builds the charge / release state machine that replaces the inhale / exhale part of the shout behavior"""

    def __init__(self, xml: BehaviorXml):
        self.xml = xml
        self.events = xml.names("eventNames")
        self.variables = xml.names("variableNames")
        self._blend_effect = ""

        # the root state machine becomes the cast state machine, its isActive binding is the SHB_bCasting output
        self.root_id = xml.find("hkbStateMachine", "Shout_Behavior")
        xml.anchor = self.root_id
        # nodes of the shout behavior used as templates
        self.machine_id = xml.find("hkbStateMachine", "ShoutBehavior")
        self.state_template = xml.find("hkbStateMachineStateInfo", "Shout_Inhale")
        self.selector_template = xml.find("hkbManualSelectorGenerator", "Shout_Inhale_MSG")
        self.binding_template = xml.param(self.selector_template, "variableBindingSet")
        self.clip_template = xml.find("hkbClipGenerator", "MT_Shout_Exhale")
        self.trigger_template = xml.param(self.clip_template, "triggers")
        transitions = xml.get(xml.param(self.state_template, "transitions"))
        self.transition_template = xml.param(self.state_template, "transitions")
        start = transitions.index("\t\t\t\t<hkobject>")
        end = transitions.index("\t\t\t\t</hkobject>")
        self.transition_object = transitions[start:end + 1]
        if not any("FLAG_DISABLE_CONDITION</hkparam>" in line for line in self.transition_object):
            raise RuntimeError("unexpected transition template")

    def clip(self, name: str, animation: str, looping: bool, end_event: str | None = None) -> str:
        triggers = "null"
        if end_event:
            triggers, lines = self.xml.clone(self.trigger_template)
            set_array(lines, "triggers", 1, _indent([
                "<hkobject>",
                '\t<hkparam name="localTime">0.000000</hkparam>',
                '\t<hkparam name="event">',
                "\t\t<hkobject>",
                f'\t\t\t<hkparam name="id">{self.events.index(end_event)}</hkparam>',
                '\t\t\t<hkparam name="payload">null</hkparam>',
                "\t\t</hkobject>",
                "\t</hkparam>",
                '\t<hkparam name="relativeToEndOfClip">true</hkparam>',
                '\t<hkparam name="acyclic">false</hkparam>',
                '\t<hkparam name="isAnnotation">false</hkparam>',
                "</hkobject>",
            ], 4))
            self.xml.add(lines)
        clip_id, lines = self.xml.clone(self.clip_template)
        set_param(lines, "name", name)
        set_param(lines, "animationName", f"Animations\\{animation}.hkx")
        set_param(lines, "triggers", triggers)
        set_param(lines, "mode", "MODE_LOOPING" if looping else "MODE_SINGLE_PLAY")
        self.xml.add(lines)
        return clip_id

    def blend_effect(self) -> str:
        """Blend used by all transitions. The vanilla effects take their duration from a vanilla variable, ours is fixed"""
        if not self._blend_effect:
            template = next(re.search(r">(#\d+)<", line).group(1) for line in self.transition_object if '<hkparam name="transition">' in line)
            self._blend_effect, lines = self.xml.clone(template)
            set_param(lines, "variableBindingSet", "null")
            set_param(lines, "name", "SHB_BlendTransition")
            set_param(lines, "duration", f"{transition_blend_time:.6f}")
            self.xml.add(lines)
        return self._blend_effect

    def transitions(self, entries: list[tuple[str, int]]) -> str:
        """Transition array from (event, target state id)"""
        if not entries:
            return "null"
        effect = self.blend_effect()
        array_id, lines = self.xml.clone(self.transition_template)
        body = []
        for event, state in entries:
            for line in self.transition_object:
                if line.startswith('\t\t\t\t\t<hkparam name="eventId">'):
                    line = f'\t\t\t\t\t<hkparam name="eventId">{self.events.index(event)}</hkparam>'
                elif line.startswith('\t\t\t\t\t<hkparam name="toStateId">'):
                    line = f'\t\t\t\t\t<hkparam name="toStateId">{state}</hkparam>'
                elif line.startswith('\t\t\t\t\t<hkparam name="transition">'):
                    line = f'\t\t\t\t\t<hkparam name="transition">{effect}</hkparam>'
                body.append(line)
        set_array(lines, "transitions", len(entries), body)
        self.xml.add(lines)
        return array_id

    def state(self, name: str, state_id: int, generator: str, transitions: list[tuple[str, int]]) -> str:
        new_id, lines = self.xml.clone(self.state_template)
        set_param(lines, "transitions", self.transitions(transitions))
        set_param(lines, "generator", generator)
        set_param(lines, "name", name)
        set_param(lines, "stateId", str(state_id))
        self.xml.add(lines)
        return new_id

    def _machine_lines(self, lines: list[str], name: str, states: list[str]):
        set_param(lines, "name", name)
        set_param(lines, "startStateId", "0")
        set_array(lines, "states", len(states), ["\t\t\t\t" + " ".join(states)])

    def machine(self, name: str, states: list[str]) -> str:
        """State machine starting in state id 0"""
        new_id, lines = self.xml.clone(self.machine_id)
        set_param(lines, "variableBindingSet", "null")
        self._machine_lines(lines, name, states)
        self.xml.add(lines)
        return new_id

    def selector(self, name: str, generators: list[str], variable: str) -> str:
        binding_id, lines = self.xml.clone(self.binding_template)
        index = next(i for i, line in enumerate(lines) if '<hkparam name="variableIndex">' in line)
        lines[index] = re.sub(r">\d+<", f">{self.variables.index(variable)}<", lines[index])
        self.xml.add(lines)
        new_id, lines = self.xml.clone(self.selector_template)
        set_param(lines, "variableBindingSet", binding_id)
        set_param(lines, "name", name)
        set_array(lines, "generators", len(generators), ["\t\t\t\t" + " ".join(generators)])
        self.xml.add(lines)
        return new_id

    def build(self, sets: list[AnimSet]):
        charge = []
        release = []
        for s in sets:
            if s.charge_loop:
                start = self.clip(f"SHB_{s.name}_ChargeStart", s.charge_start, False, event_charge_loop)
                loop = self.clip(f"SHB_{s.name}_ChargeLoop", s.charge_loop, True)
                charge.append(self.machine(f"SHB_{s.name}_Charge", [
                    self.state(f"SHB_{s.name}_ChargeStartState", 0, start, [(event_charge_loop, 1)]),
                    self.state(f"SHB_{s.name}_ChargeLoopState", 1, loop, []),
                ]))
            else:
                charge.append(self.clip(f"SHB_{s.name}_ChargeStart", s.charge_start, False))
            if s.hold and s.release:
                once = self.clip(f"SHB_{s.name}_Release", s.release, False, event_cast_hold)
                hold = self.clip(f"SHB_{s.name}_Hold", s.hold, True)
                release.append(self.machine(f"SHB_{s.name}_ReleaseHold", [
                    self.state(f"SHB_{s.name}_ReleaseState", 0, once, [(event_cast_hold, 1)]),
                    self.state(f"SHB_{s.name}_HoldState", 1, hold, []),
                ]))
            elif s.hold:
                release.append(self.clip(f"SHB_{s.name}_Hold", s.hold, True))
            else:
                release.append(self.clip(f"SHB_{s.name}_Release", s.release, False, event_cast_stop))

        states = [
            self.state("SHB_Charge", 0, self.selector("SHB_Charge_MSG", charge, var_anim), [(event_cast_release, 1)]),
            # a new cast can start while the overlay of the previous one is still fading out
            self.state("SHB_Release", 1, self.selector("SHB_Release_MSG", release, var_anim), [(event_cast_start, 0)]),
        ]
        lines = self.xml.get(self.root_id)
        self._machine_lines(lines, "SHB_CastBehavior", states)
        set_param(lines, "wildcardTransitions", "null")
        self.xml.replace(self.root_id, lines)

        # isActive output of the root state machine, was IsShouting
        binding_id = self.xml.param(self.root_id, "variableBindingSet")
        lines = self.xml.get(binding_id)
        if sum('<hkparam name="memberPath">' in line for line in lines) != 1 or not any('<hkparam name="memberPath">isActive</hkparam>' in line for line in lines):
            raise RuntimeError("unexpected binding on the shout root state machine")
        index = next(i for i, line in enumerate(lines) if '<hkparam name="variableIndex">' in line)
        lines[index] = re.sub(r">\d+<", f">{self.variables.index(var_casting)}<", lines[index])
        self.xml.replace(binding_id, lines)


def known_animations(xml: list[str]) -> set[str]:
    """Lower case animation paths listed in a character file or used by the clips of a behavior file"""
    matches = (re.search(r"<hkcstring>(Animations\\.*)</hkcstring>|<hkparam name=\"animationName\">(Animations\\.*)</hkparam>", line) for line in xml)
    return {(m.group(1) or m.group(2)).lower() for m in matches if m}


def check_anim_sets(known: set[str]):
    """All clips have to be animations the character project already knows"""
    for s in anim_sets:
        if not s.release and not s.hold:
            raise RuntimeError(f"{s.name}: needs a release or a hold clip")
        for clip in (s.charge_start, s.charge_loop, s.release, s.hold):
            if clip and f"animations\\{clip}.hkx".lower() not in known:
                raise RuntimeError(f"{s.name}: Animations\\{clip}.hkx is not in the character animation list")


def write_anim_set_header():
    lines = [
        "#pragma once",
        "",
        "//Generated by python_scripts/build_cast_behavior.py, do not edit.",
        "namespace SpellHotbar::casts {",
        "",
        "\t//Value of the SHB_iAnim graph variable, selects the clips played by the cast behavior",
        "\tenum class CastAnimSet : int {",
    ]
    lines += [f"\t\t{s.name} = {i}," for i, s in enumerate(anim_sets)]
    lines += ["\t};", "}", ""]
    text = "\n".join(lines)
    if not anim_set_header_path.exists() or anim_set_header_path.read_text(encoding="utf-8") != text:
        anim_set_header_path.write_text(text, encoding="utf-8", newline="\n")
        print(f"  updated {anim_set_header_path.name}")


def check_bindings(xml: BehaviorXml):
    """
    The variable table was rebuilt, a binding left over from the shout behavior would point at one of our variables by index.
    Only the bindings the builder sets may remain.
    """
    variables = xml.names("variableNames")
    expected = {"isActive": var_casting, "selectedGeneratorIndex": var_anim}
    path = None
    for line in xml.lines:
        m = re.search(r'<hkparam name="memberPath">(.*)</hkparam>', line)
        if m:
            path = m.group(1)
        m = re.search(r'<hkparam name="variableIndex">(\d+)</hkparam>', line)
        if m:
            index = int(m.group(1))
            if path not in expected or index >= len(variables) or variables[index] != expected[path]:
                raise RuntimeError(f"unexpected variable binding: {path} <- index {index}")
    if any('<hkparam name="condition">#' in line for line in xml.lines):
        raise RuntimeError("transition condition left over from the shout behavior")


def build_cast_behavior(shout_xml: list[str], tmp: Path, out_hkx: Path):
    """Fork the shout behavior: own events / variables and a cast state machine instead of inhale / exhale"""
    text = "\n".join(shout_xml)
    text = text.replace('<hkparam name="name">Shout_Behavior.hkb</hkparam>', f'<hkparam name="name">{behavior_name}.hkb</hkparam>')

    xml = BehaviorXml(text.split("\n"))
    # the cast behavior runs next to the rest of the character graph, it must not declare any of the vanilla variables / events
    xml.clear_events_and_variables()
    for event in new_events:
        xml.add_event(event)
    for name, var_type, value in cast_variables:
        xml.add_variable(name, var_type, value)

    CastGraphBuilder(xml).build(anim_sets)
    print(f"  {len(anim_sets)} anim sets, removed {xml.remove_unreachable()} unused shout nodes")
    check_bindings(xml)

    xml_path = tmp / f"{behavior_name}.xml"
    xml_path.write_text("\n".join(xml.lines) + "\n", encoding="ascii")
    out_hkx.parent.mkdir(parents=True, exist_ok=True)
    if out_hkx.exists():
        out_hkx.unlink()
    _run_hkxcmd(xml_path, out_hkx, "AMD64")


def build():
    patch_root = mod_root_path / "Nemesis_Engine" / "mod" / mod_code

    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp = Path(tmp_dir)

        write_anim_set_header()

        # generated folder, node files of a previous run would be stale
        if patch_root.exists():
            shutil.rmtree(patch_root)
        patch_root.mkdir(parents=True)
        (patch_root / "info.ini").write_text("name=Spell Hotbar 2 - Cast Behavior\r\nauthor=pWn3d\r\nsite=null\r\nauto=SpellHotbar.esp\r\n", encoding="ascii", newline="")

        character_xml = load_vanilla_xml("defaultmale", tmp)
        # the first person skeleton uses the same bone order (its 0_master contains the same LeftArm / RightArm arrays),
        # its character file is not in the cache, so the third person bone sets are used for both projects
        properties = sorted({p for include, exclude in overlay_weights.values() for p in include + exclude})
        property_bones = character_bone_weights(character_xml, properties)
        overlay_bones = {}
        for variable, (include, exclude) in overlay_weights.items():
            bones = [max(values) for values in zip(*(property_bones[p] for p in include))]
            for p in exclude:
                bones = [0.0 if e > 0.0 else b for b, e in zip(bones, property_bones[p])]
            overlay_bones[variable] = bones
            print(f"  {variable}: {sum(1 for b in bones if b > 0.0)} bones")

        for project in projects:
            print(f"cast behavior {project or 'third person'}")
            master_xml = load_vanilla_xml("0_master", tmp, project)
            if project:
                # no character file in the cache, the clips have to be used somewhere in the vanilla behaviors of the project
                known = known_animations(master_xml) | known_animations(load_vanilla_xml("magicbehavior", tmp, project))
            else:
                known = known_animations(character_xml)
            check_anim_sets(known)
            build_cast_behavior(load_vanilla_xml("shout_behavior", tmp, project), tmp / project, mod_root_path / "meshes" / "actors" / "character" / project / "behaviors" / f"{behavior_name}.hkx")
            # the torso aim only matters for the body that is seen, third person
            horse_xml = None if project else load_vanilla_xml("horsebehavior", tmp, project)
            patch_master(master_xml, patch_root / project / "0_master", overlay_bones, horse_xml)

            # every behavior file of the project has to know the events: a file that does not gets them as invalid id,
            # its state machines react to that and the locomotion drops back to its default state
            others = sorted(p.stem for p in (vanilla_behavior_path / project).glob("*.hkx") if p.stem != "0_master" and not p.stem.startswith("default"))
            for name in others:
                patch_data_nodes(load_vanilla_xml(name, tmp, project), patch_root / project / name, new_events, [])
            print(f"  events registered in {len(others)} more behavior files")

    print(f"done: {patch_root}")


if __name__ == "__main__":
    build()
