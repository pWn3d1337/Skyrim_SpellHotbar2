#include "mcp/bind_capture.h"
#include "mcp/mcp_preset_name.h"

#include <cstdlib>
#include <iostream>

namespace {
    void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
}

int main()
{
    using SpellHotbar::Mcp::BindCaptureState;
    using SpellHotbar::Mcp::CaptureApply;
    using SpellHotbar::Mcp::is_listed_preset;
    using SpellHotbar::Mcp::valid_preset_name;
    using SpellHotbar::Mcp::with_json_extension;

    BindCaptureState capture;
    require(!capture.armed(), "capture starts disarmed");
    require(!capture.arm(-1), "negative key IDs must be rejected");
    require(!capture.arm(23), "IDs above 22 must be rejected");
    require(capture.arm(0), "slot 0 must arm");
    require(capture.pending_id() == 0, "pending ID must be slot 0");
    require(capture.arm(22), "arming another ID replaces the previous pending capture");
    require(capture.pending_id() == 22, "only one pending capture may exist");

    require(capture.apply_down_edge(true) == CaptureApply::cancelled, "Escape must cancel without rebinding");
    require(!capture.armed(), "Escape must disarm capture");
    require(capture.apply_down_edge(false) == CaptureApply::ignored, "a disarmed capture must ignore input");

    require(capture.arm(7), "slot 7 must arm after cancel");
    require(capture.apply_down_edge(false) == CaptureApply::rebound, "a down edge must consume the pending ID");
    require(!capture.armed(), "a successful rebind must disarm capture");

    require(capture.arm(12), "menu next must arm");
    require(capture.consume_rebind(), "consume_rebind must succeed while armed");
    require(!capture.consume_escape(), "consume_escape must fail after disarm");

    require(valid_preset_name("controller"), "plain preset names are valid");
    require(valid_preset_name("auto_profile"), "first-run profile names remain valid filenames");
    require(!valid_preset_name(""), "empty names are invalid");
    require(!valid_preset_name("<cancel>"), "MCM cancel sentinels are not filenames");
    require(!valid_preset_name("foo/bar"), "path separators are invalid");
    require(!valid_preset_name("foo\\bar"), "Windows separators are invalid");
    require(!valid_preset_name("foo..bar"), "parent-directory tokens are invalid");
    require(!valid_preset_name("foo:bar"), "reserved filename characters are invalid");
    require(!is_listed_preset("<cancel>"), "cancel sentinels must not be listed");
    require(is_listed_preset("simple.json"), "real preset files must be listed");
    require(with_json_extension("simple") == "simple.json", "save names gain a .json suffix");
    require(with_json_extension("simple.json") == "simple.json", "existing .json suffixes must not double");

    return EXIT_SUCCESS;
}
