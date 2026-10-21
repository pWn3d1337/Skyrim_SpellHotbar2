#pragma once
#include <imgui.h>

#define FUCK_API_VERSION 7

// Optional extras:
//   FUCK_ColorPalette.h - Recent/fixed colour swatch grid (v6+)
//   FUCK_Camera.h       - Camera sessions, shared camera controls and applets (v7+)

// ==================================================
// [ OPTIONAL ] SIMPLEINI HELPERS
// ==================================================
//
// To use the FUCK::PluginSettings and FUCK::INI helpers,
// #include SimpleIni (via clibutil or vcpkg) and add the
// enabler macro before including this header:
//
// #define FUCK_API_ENABLE_SIMPLEINI
// #include "FUCK_API.h"
//
// ==================================================

#ifndef FUCK_API_ENABLE_SIMPLEINI
struct FUCK_SimpleIni_Opaque;
#	define FUCK_SIMPLEINI_TYPE FUCK_SimpleIni_Opaque
#else
#	define FUCK_SIMPLEINI_TYPE CSimpleIniA
#endif

// ==================================================
// [ SECTION 1 ] TYPES & INTERFACES
// ==================================================

namespace FUCK
{
	inline const char* g_pluginName = "UnknownPlugin";

	// --- Core Enums ---
	enum class Font
	{
		kRegular,
		kLarge
	};

	enum class Overlay
	{
		kGrid,
		kCrosshair,
		kGoldenSpiral,
		kGoldenRatio,
		kTriangle
	};

	enum class InputDevice
	{
		kMouseKeyboard,
		kGamepad
	};

	enum class Modifier
	{
		kShift = 0,
		kCtrl  = 1,
		kAlt   = 2
	};

	// --- Key Codes ---
	// What every key function takes (IsInputDown, GetIconForKey, key badges, hotkeys).
	// The keyboard's are its scan codes (RE::BSWin32KeyboardDevice::Key) and the mouse's are SKSE's,
	// but a gamepad button is SKSE's code plus kGPBase again, so use these for the gamepad rather than SKSE's own
	namespace Keys
	{
		inline constexpr std::uint32_t kMBBase = static_cast<std::uint32_t>(SKSE::InputMap::kMacro_MouseButtonOffset);  // 256
		inline constexpr std::uint32_t kGPBase = static_cast<std::uint32_t>(SKSE::InputMap::kMacro_GamepadOffset);      // 266

		/// @brief An SKSE key code as FUCK's, so a gamepad button from an SKSE event or an MCM setting can be passed on. Keyboard and mouse codes come back as they are.
		constexpr std::uint32_t FromSKSE(std::uint32_t a_code) { return a_code >= kGPBase ? a_code + kGPBase : a_code; }

		// Mouse
		inline constexpr std::uint32_t kMouse_Left      = kMBBase + 0;
		inline constexpr std::uint32_t kMouse_Right     = kMBBase + 1;
		inline constexpr std::uint32_t kMouse_Middle    = kMBBase + 2;
		inline constexpr std::uint32_t kMouse_WheelUp   = kMBBase + 8;
		inline constexpr std::uint32_t kMouse_WheelDown = kMBBase + 9;

		// Gamepad buttons
		inline constexpr std::uint32_t kGP_Up    = kGPBase + SKSE::InputMap::kGamepadButtonOffset_DPAD_UP;
		inline constexpr std::uint32_t kGP_Down  = kGPBase + SKSE::InputMap::kGamepadButtonOffset_DPAD_DOWN;
		inline constexpr std::uint32_t kGP_Left  = kGPBase + SKSE::InputMap::kGamepadButtonOffset_DPAD_LEFT;
		inline constexpr std::uint32_t kGP_Right = kGPBase + SKSE::InputMap::kGamepadButtonOffset_DPAD_RIGHT;
		inline constexpr std::uint32_t kGP_Start = kGPBase + SKSE::InputMap::kGamepadButtonOffset_START;
		inline constexpr std::uint32_t kGP_Back  = kGPBase + SKSE::InputMap::kGamepadButtonOffset_BACK;
		inline constexpr std::uint32_t kGP_L3    = kGPBase + SKSE::InputMap::kGamepadButtonOffset_LEFT_THUMB;
		inline constexpr std::uint32_t kGP_R3    = kGPBase + SKSE::InputMap::kGamepadButtonOffset_RIGHT_THUMB;
		inline constexpr std::uint32_t kGP_LB    = kGPBase + SKSE::InputMap::kGamepadButtonOffset_LEFT_SHOULDER;
		inline constexpr std::uint32_t kGP_RB    = kGPBase + SKSE::InputMap::kGamepadButtonOffset_RIGHT_SHOULDER;
		inline constexpr std::uint32_t kGP_A     = kGPBase + SKSE::InputMap::kGamepadButtonOffset_A;
		inline constexpr std::uint32_t kGP_B     = kGPBase + SKSE::InputMap::kGamepadButtonOffset_B;
		inline constexpr std::uint32_t kGP_X     = kGPBase + SKSE::InputMap::kGamepadButtonOffset_X;
		inline constexpr std::uint32_t kGP_Y     = kGPBase + SKSE::InputMap::kGamepadButtonOffset_Y;
		inline constexpr std::uint32_t kGP_LT    = kGPBase + SKSE::InputMap::kGamepadButtonOffset_LT;
		inline constexpr std::uint32_t kGP_RT    = kGPBase + SKSE::InputMap::kGamepadButtonOffset_RT;

		// Thumbstick axes, read with GetAnalogInput()
		inline constexpr std::uint32_t kGP_LSX = kGPBase + 32;
		inline constexpr std::uint32_t kGP_LSY = kGPBase + 33;
		inline constexpr std::uint32_t kGP_RSX = kGPBase + 34;
		inline constexpr std::uint32_t kGP_RSY = kGPBase + 35;

		// Icons with no key of their own, for GetIconForKey() and key badges. Nothing ever presses these
		inline constexpr std::uint32_t kIconBase       = 0x1000;
		inline constexpr std::uint32_t kIcon_MouseMove = kIconBase + 0;  // a mouse being dragged
		inline constexpr std::uint32_t kIcon_Wheel     = kIconBase + 1;  // the wheel either way
		inline constexpr std::uint32_t kIcon_Triggers  = kIconBase + 2;  // LT and RT together, drawn for the gamepad in use
	}

	enum class BindResult
	{
		kNone,
		kBound,
		kCancelled,
	};

	enum class EditorBoundsState
	{
		kLocked,    // Grey
		kNormal,    // Purple
		kSelected,  // Green
		kHovered    // Bright Green
	};

	enum class TableBgTarget
	{
		kNone   = 0,
		kRowBg0 = 1,
		kRowBg1 = 2,
		kCellBg = 3
	};

	// --- Bitflags ---
	enum class WindowFlags
	{
		kNone                = 0,
		kPauseHard           = 1 << 0,   // Fully suspends the game engine
		kPauseSoft           = 1 << 1,   // Freezes game time
		kBlockVanity         = 1 << 2,   // Prevents idle vanity camera
		kHideHUD             = 1 << 3,   // Hides the in-game HUD
		kBlurBackground      = 1 << 4,   // Blurs the game world
		kPassInputToGame     = 1 << 5,   // Allows player control while open
		kCloseOnEsc          = 1 << 6,   // Closes when Escape is pressed
		kCloseOnGameMenu     = 1 << 7,   // Hides when native game menus open
		kNoDecoration        = 1 << 8,   // Removes title bar and controls
		kNoBackground        = 1 << 9,   // Makes window background transparent
		kExtendBorder        = 1 << 10,  // Draws border outside window bounds
		kNoResize            = 1 << 11,  // Prevents manual resizing by the user
		kNoMove              = 1 << 12,  // Prevents manual dragging by the user
		kAutoResize          = 1 << 13,  // Sizes automatically to contents
		kIgnoreUserScale     = 1 << 14,  // Ignores global UI scaling slider
		kCustomPosition      = 1 << 15,  // Opts out of Host-managed pos saving/loading
		kRenderDuringTM      = 1 << 16,  // Renders when 'tm' (Toggle Menus) is set
		kCloseOnClickOutside = 1 << 17,  // Closes the window when the user clicks anywhere outside of its bounds
		kEditable            = 1 << 18,  // Host-managed moving and saving while the FUCK menu is open
		kScalable            = 1 << 19,  // With kEditable: corner handles scale the window. Read it with GetWindowScale()
		kAnchorRight         = 1 << 20,  // With kEditable: keeps the right edge fixed. GetDefaultPos() is then the top-right corner
		kRotatable           = 1 << 21,  // With kEditable: a handle rotates the window's drawn content. Read it with GetWindowRotation()
		kHideWithHUD         = 1 << 22   // A HUD widget: hides while the game hides its own HUD (intro, cutscenes, dialogue), and HUD managers may fade it. See IsGameHUDHidden(), EnumerateHUDWidgets()
	};

	enum class TableFlags
	{
		kNone              = 0,
		kResizable         = 1 << 0,
		kReorderable       = 1 << 1,
		kHideable          = 1 << 2,
		kSortable          = 1 << 3,
		kNoSavedSettings   = 1 << 4,
		kContextMenuInBody = 1 << 5,
		kRowBg             = 1 << 6,
		kBordersInnerH     = 1 << 7,
		kBordersOuterH     = 1 << 8,
		kBordersInnerV     = 1 << 9,
		kBordersOuterV     = 1 << 10,
		kBordersH          = kBordersInnerH | kBordersOuterH,
		kBordersV          = kBordersInnerV | kBordersOuterV,
		kBorders           = kBordersInnerH | kBordersOuterH | kBordersInnerV | kBordersOuterV,
		kSizingFixedFit    = 1 << 13,
		kSizingFixedSame   = 2 << 13,
		kSizingStretchProp = 3 << 13,
		kSizingStretchSame = 4 << 13,
	};

	enum class TableColumnFlags
	{
		kNone                 = 0,
		kDisabled             = 1 << 0,
		kDefaultHide          = 1 << 1,
		kDefaultSort          = 1 << 2,
		kWidthStretch         = 1 << 3,
		kWidthFixed           = 1 << 4,
		kNoResize             = 1 << 5,
		kNoReorder            = 1 << 6,
		kNoHide               = 1 << 7,
		kNoClip               = 1 << 8,
		kNoSort               = 1 << 9,
		kNoSortAscending      = 1 << 10,
		kNoSortDescending     = 1 << 11,
		kNoHeaderLabel        = 1 << 12,
		kNoHeaderWidth        = 1 << 13,
		kPreferSortAscending  = 1 << 14,
		kPreferSortDescending = 1 << 15,
		kIndentEnable         = 1 << 16,
		kIndentDisable        = 1 << 17
	};

	enum class DragDropFlags
	{
		kNone                     = 0,
		kSourceNoPreviewTooltip   = 1 << 0,
		kSourceNoDisableHover     = 1 << 1,
		kSourceNoHoldToOpenOthers = 1 << 2,
		kSourceAllowNullID        = 1 << 3,
		kSourceExtern             = 1 << 4,
		kSourceAutoExpirePayload  = 1 << 5,
		kAcceptBeforeDelivery     = 1 << 10,
		kAcceptNoDrawDefaultRect  = 1 << 11,
		kAcceptNoPreviewTooltip   = 1 << 12,
		kAcceptPeekOnly           = kAcceptBeforeDelivery | kAcceptNoDrawDefaultRect
	};

	enum class ItemFlags
	{
		kNone         = 0,
		kNoTabStop    = 1 << 0,
		kButtonRepeat = 1 << 1,
		kDisabled     = 1 << 2,
		kNoNav        = 1 << 3
	};

	enum class PopupFlags
	{
		kNone          = 0,
		kAnyPopupId    = 1 << 10,
		kAnyPopupLevel = 1 << 11,
		kAnyPopup      = kAnyPopupId | kAnyPopupLevel
	};

	enum class ColorEditFlags
	{
		kNone             = 0,
		kNoAlpha          = 1 << 1,
		kNoPicker         = 1 << 2,
		kNoOptions        = 1 << 3,
		kNoSmallPreview   = 1 << 4,
		kNoInputs         = 1 << 5,
		kNoTooltip        = 1 << 6,
		kNoLabel          = 1 << 7,
		kNoSidePreview    = 1 << 8,
		kNoDragDrop       = 1 << 9,
		kNoBorder         = 1 << 10,  // ColorButton: also hides the FUCK border
		kAlphaOpaque      = 1 << 12,
		kAlphaNoBg        = 1 << 13,
		kAlphaPreviewHalf = 1 << 14,
		kHDR              = 1 << 19,
		kDisplayRGB       = 1 << 20,
		kDisplayHSV       = 1 << 21,
		kDisplayHex       = 1 << 22,
		kUint8            = 1 << 23,
		kFloat            = 1 << 24,
		kPickerHueBar     = 1 << 25,
		kPickerHueWheel   = 1 << 26,
		kInputRGB         = 1 << 27,
		kInputHSV         = 1 << 28
	};

	enum class HotkeyFlags : int
	{
		kNone            = 0,
		kAlignNear       = 1 << 0,
		kLabelRight      = 1 << 1,
		kCtrlToRebind    = 1 << 2,
		kAlwaysHighlight = 1 << 3,
		kNoModifiers     = 1 << 4
	};

	/// @brief What FUCK is taking input for, from GetInputCaptureState().
	enum class InputCaptureState : int
	{
		kNone      = 0,
		kMenu      = 1 << 0,  // FUCK's main menu is open
		kGameInput = 1 << 1,  // FUCK is keeping input from the game: the menu or a window without kPassInputToGame is open, the mouse is on a window's frame, or a camera session holds the controls
		kKeyboard  = 1 << 2,  // a FUCK window wants the keyboard, e.g. a focused widget
		kMouse     = 1 << 3,  // a FUCK window wants the mouse, e.g. it's under the cursor
		kTextInput = 1 << 4   // a FUCK text field is being typed in
	};

// --- Bitwise Operator Macro ---
#define FUCK_DEFINE_ENUM_BITWISE_OPERATORS(Type)                                                                    \
	inline Type  operator|(Type a, Type b) { return static_cast<Type>(static_cast<int>(a) | static_cast<int>(b)); } \
	inline bool  operator&(Type a, Type b) { return (static_cast<int>(a) & static_cast<int>(b)) != 0; }             \
	inline Type& operator|=(Type& a, Type b)                                                                        \
	{                                                                                                               \
		a = static_cast<Type>(static_cast<int>(a) | static_cast<int>(b));                                           \
		return a;                                                                                                   \
	}                                                                                                               \
	inline Type& operator&=(Type& a, Type b)                                                                        \
	{                                                                                                               \
		a = static_cast<Type>(static_cast<int>(a) & static_cast<int>(b));                                           \
		return a;                                                                                                   \
	}

	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(WindowFlags)
	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(TableFlags)
	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(TableColumnFlags)
	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(DragDropFlags)
	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(ItemFlags)
	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(PopupFlags)
	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(HotkeyFlags)
	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(ColorEditFlags)
	FUCK_DEFINE_ENUM_BITWISE_OPERATORS(InputCaptureState)

#undef FUCK_DEFINE_ENUM_BITWISE_OPERATORS

	struct ManagedHotkey
	{
		std::uint32_t kKey = 0, gKey = 0;
		std::int32_t  kMod1 = -1, gMod1 = -1;
		std::int32_t  kMod2 = -1, gMod2 = -1;
		bool          isBinding         = false;
		bool          wasTriggered      = false;
		bool          waitForRelease    = false;
		bool          disallowModifiers = false;  // Single-key binds only, modifier keys treated as bindable.

		void Clear()
		{
			kKey = gKey = 0;
			kMod1 = gMod1 = kMod2 = gMod2 = -1;
			isBinding                     = false;
			wasTriggered                  = false;
			waitForRelease                = false;
		}
	};

	// --- Images ---
	/// @brief Options for RequestImage().
	struct ImageRequest
	{
		std::uint32_t size       = sizeof(ImageRequest);  // Lets later versions add options, leave as is
		const char*   path       = nullptr;               // The image file, as for LoadImage(). Copied
		float         scale      = 1.0f;                  // e.g. 0.25 for a thumbnail
		const char*   cachePath  = nullptr;               // Optional. A copy saved here is loaded instead, and the scaled image is saved here when there's none. Copied
		bool          background = true;                  // Loads on a background thread. false loads before returning
		bool          urgent     = false;                 // Background only: goes ahead of other waiting loads, e.g. the image being viewed
	};

	enum class ImageState
	{
		kNone,     // no image
		kLoading,  // waiting or loading in the background
		kReady,    // drawn by FUCK's image functions
		kFailed    // the file couldn't be loaded
	};

	// --- Optional Header Types ---
	// Defined in FUCK_Camera.h
	struct CameraSessionOptions;
	struct CameraScreenRect;
	struct CameraMove;
	struct CameraOffsets;
	struct CameraPose;
	class ICameraClient;

	// --- Extension Interfaces ---

	/// @brief Implement this to add a new Tool to the FUCK Sidebar.
	class ITool
	{
	public:
		virtual ~ITool() = default;

		/// @brief The exact name of your SKSE plugin.
		/// @note Defaults to the name passed into FUCK::Connect().
		virtual const char* PluginName() const { return FUCK::g_pluginName; }

		/// @brief The tool's name in the sidebar.
		/// @note With PluginName(), it's what the user's settings for the tool are saved under (its group, place, name,
		/// favourite and hidden state), so keep it the same between sessions or they won't find it again.
		virtual const char* Name() const = 0;

		/// @brief The sidebar group this tool sits in, or nullptr for the top level.
		/// @note "::" nests groups, up to 4 deep, e.g. "Camera::Presets". Tools naming the same group share it,
		/// in registration order. The string must stay alive, so build it once if it's put together at runtime.
		/// @note Read when the tool registers. Call FUCK::RefreshTools() if it changes. Where the user has moved it wins.
		virtual const char* Group() const { return nullptr; }
		virtual void        Draw() = 0;

		/// @brief Called every frame the host draws, for every tool, not just the open one. Check your own state before drawing.
		virtual void        RenderOverlay() {}

		/// @brief Called when the tool is selected or deselected, and when the menu opens or closes while it's selected.
		virtual void        OnOpen() {}
		virtual void        OnClose() {}

		/// @brief The event is a const RE::InputEvent* const*. Called as the game dispatches input, outside the frame, so
		/// don't draw here. Every tool gets it, the open one first. Return true to consume it and stop later handlers.
		virtual bool        OnAsyncInput(const void*) { return false; }

		/// @brief Whether the tool is listed in the sidebar. Read like Group(), so call FUCK::RefreshTools() after it changes.
		virtual bool        ShowInSidebar() const { return true; }
	};

	/// @brief Implement this to add a floating, independent Window to the FUCK framework.
	class IWindow
	{
	public:
		virtual ~IWindow() = default;

		/// @brief Unique identifier for this specific window within your plugin.
		/// @note Used implicitly by the Host to track and save screen coordinates. (e.g. "MainOverlay")
		virtual const char* Id() const = 0;

		/// @brief The exact name of your SKSE plugin.
		/// @note Defaults to the name passed into FUCK::Connect().
		virtual const char* PluginName() const { return FUCK::g_pluginName; }

		/// @brief Display title shown on the Window chrome. Not translated, so return "$Key"_T for a localised one.
		/// @note May be called from any thread. Return a stable string, not one rebuilt in Draw().
		virtual const char* Title() const = 0;

		virtual void        Draw() = 0;
		virtual void        RenderOverlay() {}
		virtual bool        IsOpen() const       = 0;
		virtual void        SetOpen(bool a_open) = 0;
		virtual WindowFlags GetFlags() const { return WindowFlags::kNone; }
		virtual ImVec2      GetDefaultSize() const { return ImVec2(400.0f, 300.0f); }
		virtual ImVec2      GetDefaultPos() const { return ImVec2(0.0f, 0.0f); }

		virtual bool OnAsyncInput(const void*) { return false; }
	};
}

// ==================================================
// [ SECTION 2 ] C-ABI INTERFACE
// ==================================================

#pragma pack(push, 1)
struct FUCK_Interface
{
	unsigned int version;

	// Registration
	void (*RegisterTool)(FUCK::ITool*);
	void (*RegisterWindow)(FUCK::IWindow*);
	void (*UnregisterWindow)(FUCK::IWindow*);

	// Display
	float (*GetResolutionScale)();
	float (*GetGlobalScale)();
	float (*GetUserScale)();
	void (*GetDisplaySize)(float*, float*);
	void (*TranslateScaleformToScreen)(float, float, float*, float*);
	ImFont* (*GetFont)(FUCK::Font);
	void (*PushFont)(ImFont*, float);
	void (*PopFont)();
	void (*SuspendRendering)(bool);
	void (*SetMenuOpen)(bool);
	bool (*IsMenuOpen)();

	// IO
	float (*GetDeltaTime)();
	double (*GetTime)();
	void (*GetMouseDelta)(float*, float*);
	void (*GetMousePos)(float*, float*);
	float (*GetMouseWheel)();

	// Styling
	void (*PushStyleColor)(ImGuiCol, const ImVec4&);
	void (*PopStyleColor)(int);
	void (*PushStyleVar)(ImGuiStyleVar, float);
	void (*PushStyleVarVec)(ImGuiStyleVar, const ImVec2&);
	void (*PopStyleVar)(int);
	float (*GetStyleVar)(ImGuiStyleVar);
	void (*GetStyleVarVec)(ImGuiStyleVar, float*, float*);
	void (*GetStyleColorVec4)(ImGuiCol, float*, float*, float*, float*);
	void (*SetWindowFontScale)(float);

	// Layout
	void (*SetCursorPosX)(float);
	void (*SetCursorPosY)(float);
	void (*GetCursorPos)(float*, float*);
	void (*SetCursorPos)(float, float);
	void (*GetCursorScreenPos)(float*, float*);
	void (*SetCursorScreenPos)(float, float);
	void (*AlignTextToFramePadding)();
	void (*GetContentRegionAvail)(float*, float*);
	float (*CalcItemWidth)();
	void (*CalcTextSize)(const char*, const char*, bool, float, float*, float*);
	void (*GetItemRectMin)(float*, float*);
	void (*GetItemRectMax)(float*, float*);
	void (*SetNextItemWidth)(float);
	void (*SetNextItemOpen)(bool, int);
	void (*Dummy)(float, float);
	void (*Spacing)();
	void (*Separator)();
	void (*SeparatorThick)();
	void (*SeparatorText)(const char*);
	float (*GetColumnWidth)(int);

	// Metrics
	float (*GetTextLineHeight)();
	float (*GetTextLineHeightWithSpacing)();
	float (*GetFrameHeight)();
	float (*GetFrameHeightWithSpacing)();

	// Utils
	void (*LoadTranslation)(const char*);
	const char* (*GetTranslation)(const char*);
	void (*SanitizePath)(char*, const char*, size_t);
	void (*GetPluginConfigPath)(const char*, char*, size_t);
	void (*LoadPluginINI)(const char* pluginName, void* userdata, void (*callback)(FUCK_SIMPLEINI_TYPE&, void*));
	void (*SavePluginINI)(const char* pluginName, void* userdata, void (*callback)(FUCK_SIMPLEINI_TYPE&, void*));
	void (*LoadPluginINIDefaults)(const char*, void*, void (*)(FUCK_SIMPLEINI_TYPE&, void*));
	void (*LoadPluginKeybinds)(const char* pluginName, void* userdata, void (*callback)(FUCK_SIMPLEINI_TYPE&, void*));
	void (*SavePluginKeybinds)(const char* pluginName, void* userdata, void (*callback)(FUCK_SIMPLEINI_TYPE&, void*));
	void (*LoadPluginKeybindsDefaults)(const char* pluginName, void* userdata, void (*callback)(FUCK_SIMPLEINI_TYPE&, void*));
	void (*PushItemFlag)(FUCK::ItemFlags, bool);
	void (*PopItemFlag)();
	void (*HelpMarker)(const char*);
	void (*PushID_Str)(const char*);
	void (*PushID_Int)(int);
	void (*PushID_Ptr)(const void*);
	void (*PopID)();

	// Menu Events
	void (*AddMenuListener)(void* userdata, void (*callback)(const char* menuName, bool opening, void* userdata));
	void (*RemoveMenuListener)(void* userdata);

	// Assets
	void* (*LoadImage)(const char*, bool);
	void (*ReleaseImage)(void*);
	void (*GetImageInfo)(void*, float*, float*);
	void* (*GetIconForKey)(std::uint32_t);
	void (*GetIconSizeForKey)(std::uint32_t, float*, float*);
	void (*Spinner)(const char*, float, float, const ImVec4&);
	void (*DrawOverlay)(FUCK::Overlay, float, ImU32, float, float, float, float);

	// Game Control
	void (*SetGameTimeFrozen)(bool);
	void (*SetAutoVanityBlocked)(bool);
	void (*SetHardPause)(bool);
	void (*SetSoftPause)(bool);
	void (*ForceCursor)(bool);

	// Input
	bool (*IsInputPressed)(const void*, std::uint32_t);
	bool (*IsInputDown)(std::uint32_t);
	float (*GetAnalogInput)(std::uint32_t);
	bool (*IsModifierPressed)(FUCK::Modifier);
	int (*GetInputDevice)();
	const char* (*GetKeyName)(std::uint32_t);
	bool (*IsGamepadKey)(std::uint32_t);

	bool (*IsBinding)();
	void (*AbortBinding)();
	void (*StartBinding)(std::uint32_t, std::int32_t, std::int32_t, bool);
	FUCK::BindResult (*UpdateBinding)(const void*, std::uint32_t*, std::int32_t*, std::int32_t*);
	FUCK::BindResult (*GetInputBind)(const void*, std::uint32_t*, std::int32_t*, std::int32_t*);

	bool (*DrawManagedHotkey)(const char*, FUCK::ManagedHotkey*, int, float, float);
	bool (*UpdateManagedHotkey)(const void*, FUCK::ManagedHotkey*);
	bool (*ProcessManagedHotkey)(const void*, FUCK::ManagedHotkey*);
	bool (*IsManagedHotkeyDown)(FUCK::ManagedHotkey*);

	// Interaction
	bool (*IsPopupOpen)(const char*, int);
	bool (*IsItemHovered)(int);
	bool (*IsItemClicked)(int);
	bool (*IsItemActive)();
	bool (*IsItemFocused)();
	bool (*IsItemDeactivated)();
	bool (*IsItemDeactivatedAfterEdit)();
	bool (*IsAnyItemActive)();
	bool (*IsAnyItemHovered)();
	bool (*IsWindowFocused)(int);
	bool (*IsWindowHovered)(int);
	bool (*IsMouseDown)(int);
	bool (*IsMouseClicked)(int, bool);
	bool (*IsMouseReleased)(int);
	bool (*IsKeyDown)(ImGuiKey);
	bool (*IsKeyPressed)(ImGuiKey, bool);
	void (*SetKeyboardFocusHere)(int);
	void (*SetItemDefaultFocus)();

	bool (*BeginDragDropSource)(int);
	bool (*SetDragDropPayload)(const char*, const void*, size_t, int);
	void (*EndDragDropSource)();
	bool (*BeginDragDropTarget)();
	const ImGuiPayload* (*AcceptDragDropPayload)(const char*, int);
	void (*EndDragDropTarget)();

	// Drawing Primitives
	void (*DrawRect)(const ImVec2&, const ImVec2&, const ImVec4&, float, float);
	void (*DrawRectFilled)(const ImVec2&, const ImVec2&, const ImVec4&, float);
	void (*DrawLine)(const ImVec2&, const ImVec2&, const ImVec4&, float);
	void (*DrawImage)(void*, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec4&);
	void (*DrawImageQuad)(void*, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec4&);
	void (*AddImage)(void*, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec4&);
	void (*DrawBackgroundImage)(void*, float);
	void (*DrawBackgroundLine)(float, float, float, float, unsigned int, float);
	void (*DrawBackgroundRect)(const ImVec2&, const ImVec2&, ImU32, float);

	// Screen primitives
	void (*DrawScreenRect)(const ImVec2&, const ImVec2&, ImU32, float, float);
	void (*DrawScreenRectFilled)(const ImVec2&, const ImVec2&, ImU32, float);
	void (*DrawScreenLine)(float, float, float, float, ImU32, float);

	// Windows
	void (*SetNextWindowPos)(float, float, int, float, float);
	void (*SetNextWindowSize)(float, float, int);
	void (*GetWindowPos)(float*, float*);
	void (*GetWindowSize)(float*, float*);
	void (*SetWindowPos)(float, float, int);
	void (*SetWindowSize)(float, float, int);
	bool (*BeginWindow)(const char*, bool*, int);
	void (*EndWindow)();
	void (*ExtendWindowPastBorder)();
	void (*BeginChild)(const char*, float, float, bool, int);
	void (*EndChild)();
	bool (*TreeNode)(const char*);
	void (*TreePop)();
	bool (*BeginPopupContextItem)(const char*, int);
	void (*EndPopup)();

	// Widgets
	bool (*Button)(const char*);
	bool (*InvisibleButton)(const char*, const ImVec2&, int);
	bool (*Checkbox)(const char*, bool*, bool, bool);
	bool (*Hotkey)(const char*, std::uint32_t, std::int32_t, std::int32_t, bool, bool, bool);
	bool (*ToggleButton)(const char*, bool*, bool, bool);
	bool (*InputText)(const char*, char*, size_t, int);
	bool (*ColorEdit3)(const char*, float[3], int);
	bool (*ColorEdit4)(const char*, float[4], int);
	bool (*SliderFloat)(const char*, float*, float, float, const char*);
	bool (*SliderInt)(const char*, int*, int, int, const char*);
	bool (*DragInt)(const char*, int*, float, int, int, const char*);
	bool (*DragFloat)(const char*, float*, float, float, float, const char*);
	bool (*DragFloat2)(const char*, float[2], float, float, float, const char*);
	bool (*DragFloat3)(const char*, float[3], float, float, float, const char*);
	bool (*DragFloat4)(const char*, float[4], float, float, float, const char*);
	bool (*Combo)(const char*, int*, const char* const*, int);
	bool (*ComboWithFilter)(const char*, int*, const char* const*, int, int);
	bool (*ComboForm)(const char*, std::uint32_t*, std::uint8_t);
	bool (*ComboFormStr)(const char*, char*, size_t, std::uint8_t);
	bool (*Selectable)(const char*, bool, int, const ImVec2&);
	ImGuiTableSortSpecs* (*GetTableSortSpecs)();
	void (*Header)(const char*);
	void (*LeftLabel)(const char*);

	void (*TextColored)(const ImVec4&, const char*);
	void (*TextColoredWrapped)(const ImVec4&, const char*);
	void (*TextDisabled)(const char*);
	void (*CenteredText)(const char*, bool);
	void (*CenteredTextWithArrows)(const char*, const char*, bool*, bool*, bool*);

	bool (*ButtonIconWithLabel)(const char*, void*, float, float, bool, bool);
	bool (*ImageButton)(const char*, void*, float, float, const ImVec4*);
	void (*Stepper)(const char*, const char*, bool*, bool*);

	bool (*BeginTabBar)(const char*, int);
	void (*EndTabBar)();
	bool (*BeginTabItem)(const char*, int);
	void (*EndTabItem)();

	bool (*BeginTable)(const char*, int, int, const ImVec2&, float);
	void (*EndTable)();
	void (*TableSetupColumn)(const char*, int, float, std::uint32_t);
	void (*TableNextRow)(int, float);
	bool (*TableNextColumn)();
	void (*TableHeadersRow)();
	void (*TableSetBgColor)(int, ImU32, int);

	void (*Columns)(int, const char*, bool);
	void (*NextColumn)();

	void (*SameLine)(float, float);
	bool (*CollapsingHeader)(const char*, int);
	void (*BeginGroup)();
	void (*EndGroup)();
	void (*BeginDisabled)(bool);
	void (*EndDisabled)();

	bool (*IsWidgetFocused)(const char*);
	void (*SetTooltip)(const char*);
	void (*Indent)(float);
	void (*Unindent)(float);
	void (*Text)(const char*);
	void (*TextWrapped)(const char*);
	void (*TextUnformatted)(const char*, const char*);

	// Version 2
	void (*SeparatorVertical)();
	void (*PushItemWidth)(float);
	void (*PopItemWidth)();
	bool (*BeginTooltip)();
	void (*EndTooltip)();
	void (*SetScrollHereY)(float);
	bool (*InputTextMultiline)(const char*, char*, size_t, const ImVec2&, int);

	// Version 3
	void (*SetHotkeyEnabled)(bool);
	void (*SetWindowFocus)();
	void (*CloseCurrentPopup)();
	void (*OpenPopup)(const char*, int);
	bool (*BeginPopup)(const char*, int);
	bool (*BeginPopupModal)(const char*, bool*, int);
	bool (*IsWindowAppearing)();
	void (*PushTextWrapPos)(float);
	void (*PopTextWrapPos)();
	void (*SetNavCursorVisible)(bool);

	void (*DrawCircle)(const ImVec2&, float, const ImVec4&, int, float);
	void (*DrawCircleFilled)(const ImVec2&, float, const ImVec4&, int);
	void (*DrawScreenCircle)(const ImVec2&, float, ImU32, int, float);
	void (*DrawScreenCircleFilled)(const ImVec2&, float, ImU32, int);

	void (*DrawQuad)(const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec4&, float);
	void (*DrawQuadFilled)(const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, const ImVec4&);
	void (*DrawScreenQuad)(const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, ImU32, float);
	void (*DrawScreenQuadFilled)(const ImVec2&, const ImVec2&, const ImVec2&, const ImVec2&, ImU32);

	void (*DrawTriangle)(const ImVec2&, const ImVec2&, const ImVec2&, const ImVec4&, float);
	void (*DrawTriangleFilled)(const ImVec2&, const ImVec2&, const ImVec2&, const ImVec4&);
	void (*DrawScreenTriangle)(const ImVec2&, const ImVec2&, const ImVec2&, ImU32, float);
	void (*DrawScreenTriangleFilled)(const ImVec2&, const ImVec2&, const ImVec2&, ImU32);

	bool (*TreeNodeEx)(const char*, int);

	// Version 4
	bool (*WorldToScreenLoc)(const float[3], float*, float*);

	void (*AddWindowListener)(void* userdata, void (*callback)(const char* pluginName, const char* windowId, bool opening, void* userdata));
	void (*RemoveWindowListener)(void* userdata);
	bool (*IsPluginWindowOpen)(const char* pluginName, const char* windowId);

	void (*TableSetupScrollFreeze)(int, int);
	void (*TableSetColumnIndex)(int);
	int (*TableGetColumnIndex)();
	int (*TableGetRowIndex)();
	int (*TableGetColumnCount)();

	float (*GetScrollX)();
	float (*GetScrollY)();
	float (*GetScrollMaxX)();
	float (*GetScrollMaxY)();
	void (*SetScrollX)(float);
	void (*SetScrollY)(float);

	bool (*SliderAngle)(const char*, float*, float, float, const char*);
	bool (*VSliderFloat)(const char*, const ImVec2&, float*, float, float, const char*);
	bool (*VSliderButton)(const char*, const ImVec2&, float*, float, float, const char*, const char*, const char*, bool*, bool*);

	// Version 5
	void (*PushGamepadTweakFastDisabled)();
	void (*PopGamepadTweakFastDisabled)();
	void (*PushGamepadTweakSlowDisabled)();
	void (*PopGamepadTweakSlowDisabled)();

	// Version 6
	bool (*ColorButton)(const char*, const ImVec4&, int, const ImVec2&, bool);
	bool (*ColorPicker3)(const char*, float[3], int);
	bool (*ColorPicker4)(const char*, float[4], int, const float*);
	bool (*IsItemActivated)();
	bool (*IsHotkeyEnabled)();
	float (*GetWindowScale)();
	float (*GetWindowRotation)();
	void (*BeginStickyList)(const char*, float, float, bool);
	void (*EndStickyList)();
	bool (*StickyHeader)(const char*, bool*, int, int);
	void (*RefreshTools)();
	bool (*IsGameHUDHidden)();
	void (*EnumerateHUDWidgets)(void* userdata, void (*callback)(const char* pluginName, const char* windowId, const char* title, void* userdata));
	void (*SetWindowAlphaOverride)(const char* pluginName, const char* windowId, float alpha);

	// Version 7
	bool (*ComboWithKeyBadges)(const char*, int*, const char* const*, int, int, const std::uint32_t*, int*);
	bool (*BeginCameraSession)(const FUCK::CameraSessionOptions*);
	void (*EndCameraSession)(FUCK::ICameraClient*);
	bool (*IsCameraSessionActive)(FUCK::ICameraClient*);
	bool (*IsCameraSessionPaused)(FUCK::ICameraClient*);
	void (*RevertCamera)(FUCK::ICameraClient*);
	void (*UpdateCamera)(FUCK::ICameraClient*, float);
	void (*MoveCamera)(const FUCK::CameraMove*, FUCK::InputDevice, float);
	void (*ResetCamera)(FUCK::ICameraClient*);
	bool (*IsCameraModified)(FUCK::ICameraClient*);
	void (*SetCameraBodyFraming)(FUCK::ICameraClient*, bool, const FUCK::CameraScreenRect*, float);
	bool (*IsCameraBodyFramed)(FUCK::ICameraClient*);
	bool (*GetCameraOffsets)(FUCK::ICameraClient*, FUCK::CameraOffsets*);
	bool (*SetCameraOffsets)(FUCK::ICameraClient*, const FUCK::CameraOffsets*);
	bool (*GetCameraPose)(FUCK::ICameraClient*, FUCK::CameraPose*);
	bool (*SetCameraPose)(FUCK::ICameraClient*, const FUCK::CameraPose*);
	bool (*GetCameraFov)(float*, float*);
	bool (*SaveCameraPreset)(FUCK::ICameraClient*, const char*, bool);
	bool (*LoadCameraPreset)(FUCK::ICameraClient*, const char*);
	bool (*DeleteCameraPreset)(FUCK::ICameraClient*, const char*);
	int (*GetCameraPresetCount)(FUCK::ICameraClient*);
	const char* (*GetCameraPresetName)(FUCK::ICameraClient*, int);
	bool (*DrawCameraPresets)(FUCK::ICameraClient*, float);
	bool (*DrawCameraSettings)(int);
	void (*DrawCameraHelpMarker)(int, const char*, const char*);
	void (*SetActorFrozen)(const char*, RE::Actor*, bool);
	bool (*IsActorFrozen)(RE::Actor*);
	void* (*RequestImage)(const FUCK::ImageRequest*);
	int (*GetImageState)(void*);
	int (*GetInputCaptureState)();
};
#pragma pack(pop)

// ==================================================
// [ SECTION 3 ] C++ PUBLIC API WRAPPER
// ==================================================

namespace FUCK
{
	// --------------------------------------------------
	// Init & Registration
	// --------------------------------------------------

	inline FUCK_Interface*& GetInterface()
	{
		static FUCK_Interface* s = nullptr;
		return s;
	}

	/// @brief Connects to the FUCK Host Framework.
	/// @param pluginName The exact name of your SKSE plugin. Used automatically for translations and settings directories.
	/// @note The pointer is kept, not copied, so pass a literal.
	inline bool Connect(const char* pluginName)
	{
		if (!pluginName || pluginName[0] == '\0') {
			SKSE::log::error("FUCK API Connection failed: You must provide a valid pluginName.");
			return false;
		}

		auto handle = GetModuleHandleW(L"FUCK.dll");
		if (!handle)
			return false;
		auto fetcher = (void* (*)())GetProcAddress(handle, "RequestFUCK");
		if (!fetcher)
			return false;
		auto* iface = static_cast<FUCK_Interface*>(fetcher());

		// Hard enforce that the host is at least the version this header was compiled for
		if (!iface || iface->version < FUCK_API_VERSION) {
			SKSE::log::error("FUCK API Version Mismatch: Expected {}, found {}", FUCK_API_VERSION, iface ? iface->version : 0);
			return false;
		}

		GetInterface() = iface;

		// --- Cache the plugin name globally for this DLL ---
		g_pluginName = pluginName;
		GetInterface()->LoadTranslation(pluginName);

		SKSE::log::info("Connected to FUCK API version {}", iface->version);
		return true;
	}

	/// @brief The host keeps the pointer for the session, so the tool must outlive it. Tools can't be unregistered.
	/// @note A second tool with the same PluginName() and Name() is ignored.
	inline void RegisterTool(ITool* tool)
	{
		if (auto i = GetInterface())
			i->RegisterTool(tool);
	}
	/// @brief The host keeps the pointer until UnregisterWindow(), so the window must outlive it.
	/// @note A second window with the same PluginName() and Id() is ignored.
	inline void RegisterWindow(IWindow* window)
	{
		if (auto i = GetInterface())
			i->RegisterWindow(window);
	}
	inline void UnregisterWindow(IWindow* window)
	{
		if (auto i = GetInterface())
			i->UnregisterWindow(window);
	}

	// --------------------------------------------------
	// Display, Styles, Metrics
	// --------------------------------------------------

	/// @brief Gets the physical screen resolution scale multiplier (Base 1080p).
	inline float GetResolutionScale() { return GetInterface() ? GetInterface()->GetResolutionScale() : 1.0f; }

	/// @brief Gets the overall screen resolution combined with the user's selected UI Size Slider multiplier.
	inline float GetGlobalScale() { return GetInterface() ? GetInterface()->GetGlobalScale() : 1.0f; }

	/// @brief Gets the user's custom UI Size Slider multiplier.
	inline float GetUserScale() { return GetInterface() ? GetInterface()->GetUserScale() : 1.0f; }

	/// @brief Scales by the Monitor Resolution.
	inline float  Scale(float value) { return value * GetResolutionScale(); }
	inline ImVec2 Scale(float x, float y) { return ImVec2(x * GetResolutionScale(), y * GetResolutionScale()); }
	inline ImVec2 Scale(const ImVec2& value) { return ImVec2(value.x * GetResolutionScale(), value.y * GetResolutionScale()); }

	/// @brief Scales by both Resolution and the User's UI Size slider.
	inline float  UIScale(float value) { return value * GetGlobalScale(); }
	inline ImVec2 UIScale(float x, float y) { return ImVec2(x * GetGlobalScale(), y * GetGlobalScale()); }
	inline ImVec2 UIScale(const ImVec2& value) { return ImVec2(value.x * GetGlobalScale(), value.y * GetGlobalScale()); }

	inline ImVec2 GetDisplaySize()
	{
		if (auto i = GetInterface()) {
			ImVec2 s;
			i->GetDisplaySize(&s.x, &s.y);
			return s;
		}
		return ImVec2(0, 0);
	}

	/// @brief Converts standard 1280x720 Flash Stage coordinates into physical screen pixels (accounting for Aspect Ratio borders).
	inline ImVec2 TranslateScaleformToScreen(float stageX, float stageY)
	{
		ImVec2 screenPos;
		if (auto i = GetInterface())
			i->TranslateScaleformToScreen(stageX, stageY, &screenPos.x, &screenPos.y);
		return screenPos;
	}

	/// @brief Converts standard 1280x720 Flash Stage coordinates into physical screen pixels (accounting for Aspect Ratio borders).
	inline ImVec2 TranslateScaleformToScreen(const ImVec2& stagePos)
	{
		return TranslateScaleformToScreen(stagePos.x, stagePos.y);
	}

	inline void SuspendRendering(bool suspend)
	{
		if (auto i = GetInterface())
			i->SuspendRendering(suspend);
	}
	inline void SetMenuOpen(bool open)
	{
		if (auto i = GetInterface())
			i->SetMenuOpen(open);
	}
	inline bool IsMenuOpen()
	{
		if (auto i = GetInterface())
			return i->IsMenuOpen();
		return false;
	}

	inline ImFont* GetFont(Font font) { return GetInterface() ? GetInterface()->GetFont(font) : nullptr; }
	inline void    PushFont(ImFont* font, float size = 0.0f)
	{
		if (auto i = GetInterface())
			i->PushFont(font, size);
	}
	inline void PopFont()
	{
		if (auto i = GetInterface())
			i->PopFont();
	}
	inline void SetWindowFontScale(float scale)
	{
		if (auto i = GetInterface())
			i->SetWindowFontScale(scale);
	}

	inline void PushStyleColor(ImGuiCol idx, const ImVec4& col)
	{
		if (auto i = GetInterface())
			i->PushStyleColor(idx, col);
	}
	inline void PopStyleColor(int count = 1)
	{
		if (auto i = GetInterface())
			i->PopStyleColor(count);
	}
	inline void PushStyleVar(ImGuiStyleVar idx, float val)
	{
		if (auto i = GetInterface())
			i->PushStyleVar(idx, val);
	}
	inline void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
	{
		if (auto i = GetInterface())
			i->PushStyleVarVec(idx, val);
	}
	inline void PopStyleVar(int count = 1)
	{
		if (auto i = GetInterface())
			i->PopStyleVar(count);
	}

	inline float  GetStyleVar(ImGuiStyleVar idx) { return GetInterface() ? GetInterface()->GetStyleVar(idx) : 0.0f; }
	inline ImVec2 GetStyleVarVec(ImGuiStyleVar idx)
	{
		ImVec2 v(0, 0);
		if (auto i = GetInterface())
			i->GetStyleVarVec(idx, &v.x, &v.y);
		return v;
	}
	inline ImVec4 GetStyleColorVec4(ImGuiCol idx)
	{
		ImVec4 v(0, 0, 0, 0);
		if (auto i = GetInterface())
			i->GetStyleColorVec4(idx, &v.x, &v.y, &v.z, &v.w);
		return v;
	}

	inline float GetTextLineHeight() { return GetInterface() ? GetInterface()->GetTextLineHeight() : 0.0f; }
	inline float GetTextLineHeightWithSpacing() { return GetInterface() ? GetInterface()->GetTextLineHeightWithSpacing() : 0.0f; }
	inline float GetFrameHeight() { return GetInterface() ? GetInterface()->GetFrameHeight() : 0.0f; }
	inline float GetFrameHeightWithSpacing() { return GetInterface() ? GetInterface()->GetFrameHeightWithSpacing() : 0.0f; }

	// --------------------------------------------------
	// Layout & Cursor
	// --------------------------------------------------

	inline void SetCursorPosX(float x)
	{
		if (auto i = GetInterface())
			i->SetCursorPosX(x);
	}
	inline void SetCursorPosY(float y)
	{
		if (auto i = GetInterface())
			i->SetCursorPosY(y);
	}
	inline ImVec2 GetCursorPos()
	{
		ImVec2 p;
		if (auto i = GetInterface())
			i->GetCursorPos(&p.x, &p.y);
		return p;
	}
	inline void SetCursorPos(const ImVec2& pos)
	{
		if (auto i = GetInterface())
			i->SetCursorPos(pos.x, pos.y);
	}
	inline ImVec2 GetCursorScreenPos()
	{
		ImVec2 p;
		if (auto i = GetInterface())
			i->GetCursorScreenPos(&p.x, &p.y);
		return p;
	}
	inline void SetCursorScreenPos(const ImVec2& pos)
	{
		if (auto i = GetInterface())
			i->SetCursorScreenPos(pos.x, pos.y);
	}
	inline void AlignTextToFramePadding()
	{
		if (auto i = GetInterface())
			i->AlignTextToFramePadding();
	}
	inline ImVec2 GetContentRegionAvail()
	{
		ImVec2 s;
		if (auto i = GetInterface())
			i->GetContentRegionAvail(&s.x, &s.y);
		return s;
	}
	inline float  CalcItemWidth() { return GetInterface() ? GetInterface()->CalcItemWidth() : 0.0f; }
	inline ImVec2 CalcTextSize(const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false, float wrap_width = -1.0f)
	{
		ImVec2 s;
		if (auto i = GetInterface())
			i->CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width, &s.x, &s.y);
		return s;
	}

	inline ImVec2 GetItemRectMin()
	{
		ImVec2 p;
		if (auto i = GetInterface())
			i->GetItemRectMin(&p.x, &p.y);
		return p;
	}
	inline ImVec2 GetItemRectMax()
	{
		ImVec2 p;
		if (auto i = GetInterface())
			i->GetItemRectMax(&p.x, &p.y);
		return p;
	}
	inline ImVec2 GetItemRectSize()
	{
		ImVec2 min = GetItemRectMin();
		ImVec2 max = GetItemRectMax();
		return ImVec2(max.x - min.x, max.y - min.y);
	}
	inline void SetNextItemWidth(float width)
	{
		if (auto i = GetInterface())
			i->SetNextItemWidth(width);
	}
	inline void SetNextItemOpen(bool is_open, int cond = 0)
	{
		if (auto i = GetInterface())
			i->SetNextItemOpen(is_open, cond);
	}

	inline void Dummy(const ImVec2& size)
	{
		if (auto i = GetInterface())
			i->Dummy(size.x, size.y);
	}
	inline void Spacing(int count = 1)
	{
		if (auto i = GetInterface()) {
			for (int k = 0; k < count; ++k) i->Spacing();
		}
	}
	inline void Indent(float width = 0.0f)
	{
		if (auto i = GetInterface())
			i->Indent(width);
	}
	inline void Unindent(float width = 0.0f)
	{
		if (auto i = GetInterface())
			i->Unindent(width);
	}
	inline void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f)
	{
		if (auto i = GetInterface())
			i->SameLine(offset_from_start_x, spacing);
	}
	inline void Separator()
	{
		if (auto i = GetInterface())
			i->Separator();
	}
	inline void SeparatorThick()
	{
		if (auto i = GetInterface())
			i->SeparatorThick();
	}
	inline void SeparatorText(const char* label)
	{
		if (auto i = GetInterface())
			i->SeparatorText(label);
	}

	inline void BeginGroup()
	{
		if (auto i = GetInterface())
			i->BeginGroup();
	}
	inline void EndGroup()
	{
		if (auto i = GetInterface())
			i->EndGroup();
	}
	inline void BeginDisabled(bool disabled = true)
	{
		if (auto i = GetInterface())
			i->BeginDisabled(disabled);
	}
	inline void EndDisabled()
	{
		if (auto i = GetInterface())
			i->EndDisabled();
	}
	inline void PushID(const char* str_id)
	{
		if (auto i = GetInterface())
			i->PushID_Str(str_id);
	}
	inline void PushID(int int_id)
	{
		if (auto i = GetInterface())
			i->PushID_Int(int_id);
	}
	inline void PushID(const void* ptr_id)
	{
		if (auto i = GetInterface())
			i->PushID_Ptr(ptr_id);
	}
	inline void PopID()
	{
		if (auto i = GetInterface())
			i->PopID();
	}
	inline void PushItemFlag(ItemFlags flag, bool enabled)
	{
		if (auto i = GetInterface())
			i->PushItemFlag(flag, enabled);
	}
	inline void PopItemFlag()
	{
		if (auto i = GetInterface())
			i->PopItemFlag();
	}

	// --------------------------------------------------
	// Game State & Input Control
	// --------------------------------------------------

	inline float  GetDeltaTime() { return GetInterface() ? GetInterface()->GetDeltaTime() : 0.0f; }
	inline double GetTime() { return GetInterface() ? GetInterface()->GetTime() : 0.0; }
	inline ImVec2 GetMouseDelta()
	{
		ImVec2 p(0, 0);
		if (auto i = GetInterface())
			i->GetMouseDelta(&p.x, &p.y);
		return p;
	}
	inline ImVec2 GetMousePos()
	{
		ImVec2 p(0, 0);
		if (auto i = GetInterface())
			i->GetMousePos(&p.x, &p.y);
		return p;
	}
	inline float GetMouseWheel() { return GetInterface() ? GetInterface()->GetMouseWheel() : 0.0f; }
	inline void  SetGameTimeFrozen(bool frozen)
	{
		if (auto i = GetInterface())
			i->SetGameTimeFrozen(frozen);
	}
	inline void SetAutoVanityBlocked(bool blocked)
	{
		if (auto i = GetInterface())
			i->SetAutoVanityBlocked(blocked);
	}
	inline void SetHardPause(bool paused)
	{
		if (auto i = GetInterface())
			i->SetHardPause(paused);
	}
	inline void SetSoftPause(bool paused)
	{
		if (auto i = GetInterface())
			i->SetSoftPause(paused);
	}
	inline void ForceCursor(bool force)
	{
		if (auto i = GetInterface())
			i->ForceCursor(force);
	}

	inline bool        IsInputPressed(const void* inputEvent, std::uint32_t keyId) { return GetInterface() ? GetInterface()->IsInputPressed(inputEvent, keyId) : false; }
	inline bool        IsInputDown(std::uint32_t keyId) { return GetInterface() ? GetInterface()->IsInputDown(keyId) : false; }
	inline float       GetAnalogInput(std::uint32_t keyId) { return GetInterface() ? GetInterface()->GetAnalogInput(keyId) : 0.0f; }
	inline bool        IsModifierPressed(Modifier mod) { return GetInterface() ? GetInterface()->IsModifierPressed(mod) : false; }
	inline InputDevice GetInputDevice() { return GetInterface() ? static_cast<InputDevice>(GetInterface()->GetInputDevice()) : InputDevice::kMouseKeyboard; }
	inline const char* GetKeyName(std::uint32_t k) { return GetInterface() ? GetInterface()->GetKeyName(k) : ""; }
	inline bool        IsGamepadKey(std::uint32_t k) { return GetInterface() ? GetInterface()->IsGamepadKey(k) : false; }

	inline void AbortBinding()
	{
		if (auto i = GetInterface())
			i->AbortBinding();
	}
	inline bool IsBinding() { return GetInterface() ? GetInterface()->IsBinding() : false; }
	inline void StartBinding(std::uint32_t key, std::int32_t mod1, std::int32_t mod2, bool disallowModifiers = false)
	{
		if (auto i = GetInterface())
			i->StartBinding(key, mod1, mod2, disallowModifiers);
	}
	inline BindResult UpdateBinding(const void* inputEvent, std::uint32_t* outKey, std::int32_t* outMod1, std::int32_t* outMod2) { return GetInterface() ? GetInterface()->UpdateBinding(inputEvent, outKey, outMod1, outMod2) : BindResult::kNone; }
	inline BindResult GetInputBind(const void* inputEvent, std::uint32_t* outKey, std::int32_t* outMod1, std::int32_t* outMod2) { return GetInterface() ? GetInterface()->GetInputBind(inputEvent, outKey, outMod1, outMod2) : BindResult::kNone; }

	// --------------------------------------------------
	// Widgets & Interaction
	// --------------------------------------------------

	inline bool IsPopupOpen(const char* str_id = nullptr, PopupFlags flags = PopupFlags::kNone) { return GetInterface() ? GetInterface()->IsPopupOpen(str_id, static_cast<int>(flags)) : false; }
	inline bool IsItemHovered(int flags = 0) { return GetInterface() ? GetInterface()->IsItemHovered(flags) : false; }
	inline bool IsItemClicked(int mouse_button = 0) { return GetInterface() ? GetInterface()->IsItemClicked(mouse_button) : false; }
	inline bool IsItemActive() { return GetInterface() ? GetInterface()->IsItemActive() : false; }
	inline bool IsItemFocused() { return GetInterface() ? GetInterface()->IsItemFocused() : false; }
	inline bool IsItemDeactivated() { return GetInterface() ? GetInterface()->IsItemDeactivated() : false; }
	inline bool IsItemDeactivatedAfterEdit() { return GetInterface() ? GetInterface()->IsItemDeactivatedAfterEdit() : false; }
	inline bool IsAnyItemActive() { return GetInterface() ? GetInterface()->IsAnyItemActive() : false; }
	inline bool IsAnyItemHovered() { return GetInterface() ? GetInterface()->IsAnyItemHovered() : false; }
	inline bool IsWindowFocused(int flags = 0) { return GetInterface() ? GetInterface()->IsWindowFocused(flags) : false; }
	inline bool IsWindowHovered(int flags = 0) { return GetInterface() ? GetInterface()->IsWindowHovered(flags) : false; }
	inline bool IsMouseDown(int button) { return GetInterface() ? GetInterface()->IsMouseDown(button) : false; }
	inline bool IsMouseClicked(int button, bool repeat = false) { return GetInterface() ? GetInterface()->IsMouseClicked(button, repeat) : false; }
	inline bool IsMouseReleased(int button) { return GetInterface() ? GetInterface()->IsMouseReleased(button) : false; }
	inline bool IsKeyDown(ImGuiKey key) { return GetInterface() ? GetInterface()->IsKeyDown(key) : false; }
	inline bool IsKeyPressed(ImGuiKey key, bool repeat = true) { return GetInterface() ? GetInterface()->IsKeyPressed(key, repeat) : false; }

	inline void SetKeyboardFocusHere(int offset = 0)
	{
		if (auto i = GetInterface())
			i->SetKeyboardFocusHere(offset);
	}
	inline void SetItemDefaultFocus()
	{
		if (auto i = GetInterface())
			i->SetItemDefaultFocus();
	}
	inline bool IsWidgetFocused(const char* label) { return GetInterface() ? GetInterface()->IsWidgetFocused(label) : false; }

	inline bool BeginDragDropSource(DragDropFlags flags = DragDropFlags::kNone) { return GetInterface() ? GetInterface()->BeginDragDropSource(static_cast<int>(flags)) : false; }
	inline bool SetDragDropPayload(const char* type, const void* data, size_t sz, int cond = 0) { return GetInterface() ? GetInterface()->SetDragDropPayload(type, data, sz, cond) : false; }
	inline void EndDragDropSource()
	{
		if (auto i = GetInterface())
			i->EndDragDropSource();
	}
	inline bool                BeginDragDropTarget() { return GetInterface() ? GetInterface()->BeginDragDropTarget() : false; }
	inline const ImGuiPayload* AcceptDragDropPayload(const char* type, DragDropFlags flags = DragDropFlags::kNone) { return GetInterface() ? GetInterface()->AcceptDragDropPayload(type, static_cast<int>(flags)) : nullptr; }
	inline void                EndDragDropTarget()
	{
		if (auto i = GetInterface())
			i->EndDragDropTarget();
	}

	inline bool Button(const char* label) { return GetInterface() ? GetInterface()->Button(label) : false; }
	inline bool InvisibleButton(const char* str_id, const ImVec2& size, int flags = 0) { return GetInterface() ? GetInterface()->InvisibleButton(str_id, size, flags) : false; }
	inline bool Checkbox(const char* label, bool* v, bool alignFar = true, bool labelLeft = true) { return GetInterface() ? GetInterface()->Checkbox(label, v, alignFar, labelLeft) : false; }
	inline bool Hotkey(const char* label, std::uint32_t key, std::int32_t modifier, std::int32_t modifier2, bool alignFar = true, bool labelLeft = true, bool flashing = false) { return GetInterface() ? GetInterface()->Hotkey(label, key, modifier, modifier2, alignFar, labelLeft, flashing) : false; }
	inline bool ToggleButton(const char* label, bool* v, bool alignFar = true, bool labelLeft = true) { return GetInterface() ? GetInterface()->ToggleButton(label, v, alignFar, labelLeft) : false; }
	inline bool InputText(const char* label, char* buf, size_t buf_size, int flags = 0) { return GetInterface() ? GetInterface()->InputText(label, buf, buf_size, flags) : false; }
	inline bool ColorEdit3(const char* label, float col[3], int flags = 0) { return GetInterface() ? GetInterface()->ColorEdit3(label, col, flags) : false; }
	inline bool ColorEdit4(const char* label, float col[4], int flags = 0) { return GetInterface() ? GetInterface()->ColorEdit4(label, col, flags) : false; }
	inline bool SliderFloat(const char* label, float* v, float min, float max, const char* fmt = "%.3f") { return GetInterface() ? GetInterface()->SliderFloat(label, v, min, max, fmt) : false; }
	inline bool SliderInt(const char* label, int* v, int min, int max, const char* fmt = "%d") { return GetInterface() ? GetInterface()->SliderInt(label, v, min, max, fmt) : false; }
	inline bool DragInt(const char* label, int* v, float speed = 1.0f, int min = 0, int max = 0, const char* fmt = "%d") { return GetInterface() ? GetInterface()->DragInt(label, v, speed, min, max, fmt) : false; }
	inline bool DragFloat(const char* label, float* v, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.3f") { return GetInterface() ? GetInterface()->DragFloat(label, v, speed, min, max, fmt) : false; }
	inline bool DragFloat2(const char* label, float v[2], float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.3f") { return GetInterface() ? GetInterface()->DragFloat2(label, v, speed, min, max, fmt) : false; }
	inline bool DragFloat3(const char* label, float v[3], float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.3f") { return GetInterface() ? GetInterface()->DragFloat3(label, v, speed, min, max, fmt) : false; }
	inline bool DragFloat4(const char* label, float v[4], float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.3f") { return GetInterface() ? GetInterface()->DragFloat4(label, v, speed, min, max, fmt) : false; }
	inline bool Combo(const char* label, int* current_item, const char* const* items, int items_count) { return GetInterface() ? GetInterface()->Combo(label, current_item, items, items_count) : false; }
	inline bool ComboWithFilter(const char* label, int* current_item, const char* const* items, int items_count, int popup_max_height_in_items = -1) { return GetInterface() ? GetInterface()->ComboWithFilter(label, current_item, items, items_count, popup_max_height_in_items) : false; }
	inline bool ComboForm(const char* label, std::uint32_t* currentFormID, std::uint8_t formType) { return GetInterface() ? GetInterface()->ComboForm(label, currentFormID, formType) : false; }
	inline bool ComboForm(const char* label, std::string* currentEdid, std::uint8_t formType)
	{
		if (!currentEdid || !GetInterface())
			return false;
		char buf[256];
		strncpy_s(buf, sizeof(buf), currentEdid->c_str(), _TRUNCATE);
		if (GetInterface()->ComboFormStr(label, buf, sizeof(buf), formType)) {
			*currentEdid = buf;
			return true;
		}
		return false;
	}

	inline bool Selectable(const char* label, bool selected = false, int flags = 0, const ImVec2& size = ImVec2(0, 0)) { return GetInterface() ? GetInterface()->Selectable(label, selected, flags, size) : false; }

	inline ImGuiTableSortSpecs* GetTableSortSpecs() { return GetInterface() ? GetInterface()->GetTableSortSpecs() : nullptr; }

	inline void Header(const char* label)
	{
		if (auto i = GetInterface())
			i->Header(label);
	}
	inline void LeftLabel(const char* label)
	{
		if (auto i = GetInterface())
			i->LeftLabel(label);
	}
	inline void CenteredText(const char* label, bool vertical = false)
	{
		if (auto i = GetInterface())
			i->CenteredText(label, vertical);
	}
	inline void SetTooltip(const char* fmt)
	{
		if (auto i = GetInterface())
			i->SetTooltip(fmt);
	}
	inline void HelpMarker(const char* desc)
	{
		if (auto i = GetInterface())
			i->HelpMarker(desc);
	}

	inline void TextColored(const ImVec4& color, const char* fmt, ...)
	{
		auto i = GetInterface();
		if (!i)
			return;
		va_list args;
		va_start(args, fmt);
		char buf[1024];
		vsnprintf(buf, 1024, fmt, args);
		va_end(args);
		i->TextColored(color, buf);
	}
	inline void TextColoredWrapped(const ImVec4& col, const char* fmt, ...)
	{
		auto i = GetInterface();
		if (!i)
			return;
		va_list args;
		va_start(args, fmt);
		char buf[1024];
		vsnprintf(buf, 1024, fmt, args);
		va_end(args);
		i->TextColoredWrapped(col, buf);
	}
	inline void TextDisabled(const char* fmt, ...)
	{
		auto i = GetInterface();
		if (!i)
			return;
		va_list args;
		va_start(args, fmt);
		char buf[1024];
		vsnprintf(buf, 1024, fmt, args);
		va_end(args);
		i->TextDisabled(buf);
	}
	inline void Text(const char* fmt, ...)
	{
		auto i = GetInterface();
		if (!i)
			return;
		va_list args;
		va_start(args, fmt);
		char buf[1024];
		vsnprintf(buf, 1024, fmt, args);
		va_end(args);
		i->Text(buf);
	}
	inline void TextWrapped(const char* fmt, ...)
	{
		auto i = GetInterface();
		if (!i)
			return;
		va_list args;
		va_start(args, fmt);
		char buf[1024];
		vsnprintf(buf, 1024, fmt, args);
		va_end(args);
		i->TextWrapped(buf);
	}
	inline void TextUnformatted(const char* text, const char* text_end = nullptr)
	{
		if (auto i = GetInterface())
			i->TextUnformatted(text, text_end);
	}

	inline bool ButtonIconWithLabel(const char* label, ImTextureID textureID, const ImVec2& size, bool alignFar = true, bool labelLeft = true) { return GetInterface() ? GetInterface()->ButtonIconWithLabel(label, reinterpret_cast<void*>(textureID), size.x, size.y, alignFar, labelLeft) : false; }
	inline bool ImageButton(const char* str_id, ImTextureID user_texture_id, const ImVec2& image_size, const ImVec4* tint = nullptr) { return GetInterface() ? GetInterface()->ImageButton(str_id, reinterpret_cast<void*>(user_texture_id), image_size.x, image_size.y, tint) : false; }
	inline void Stepper(const char* label, const char* text, bool* outLeft, bool* outRight)
	{
		if (auto i = GetInterface())
			i->Stepper(label, text, outLeft, outRight);
	}

	// --------------------------------------------------
	// Windows & Containers
	// --------------------------------------------------

	inline bool BeginWindow(const char* name, bool* p_open = nullptr, int flags = 0) { return GetInterface() ? GetInterface()->BeginWindow(name, p_open, flags) : false; }
	inline void EndWindow()
	{
		if (auto i = GetInterface())
			i->EndWindow();
	}
	inline void ExtendWindowPastBorder()
	{
		if (auto i = GetInterface())
			i->ExtendWindowPastBorder();
	}

	/// @brief Checks if a specific window registered by a plugin is currently open.
	inline bool IsPluginWindowOpen(const char* pluginName, const char* windowId)
	{
		return GetInterface() ? GetInterface()->IsPluginWindowOpen(pluginName, windowId) : false;
	}

	inline ImVec2 GetWindowPos()
	{
		if (auto i = GetInterface()) {
			ImVec2 p;
			i->GetWindowPos(&p.x, &p.y);
			return p;
		}
		return ImVec2(0, 0);
	}
	inline ImVec2 GetWindowSize()
	{
		if (auto i = GetInterface()) {
			ImVec2 p;
			i->GetWindowSize(&p.x, &p.y);
			return p;
		}
		return ImVec2(0, 0);
	}
	inline void SetWindowPos(const ImVec2& pos, int cond = 0)
	{
		if (auto i = GetInterface())
			i->SetWindowPos(pos.x, pos.y, cond);
	}
	inline void SetWindowSize(const ImVec2& size, int cond = 0)
	{
		if (auto i = GetInterface())
			i->SetWindowSize(size.x, size.y, cond);
	}
	inline void SetNextWindowPos(const ImVec2& pos, int cond = 0, const ImVec2& pivot = ImVec2(0, 0))
	{
		if (auto i = GetInterface())
			i->SetNextWindowPos(pos.x, pos.y, cond, pivot.x, pivot.y);
	}
	inline void SetNextWindowSize(const ImVec2& size, int cond = 0)
	{
		if (auto i = GetInterface())
			i->SetNextWindowSize(size.x, size.y, cond);
	}

	inline void BeginChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = false, int flags = 0)
	{
		if (auto i = GetInterface())
			i->BeginChild(str_id, size.x, size.y, border, flags);
	}
	inline void EndChild()
	{
		if (auto i = GetInterface())
			i->EndChild();
	}

	/// @brief Begins an inset child panel using an inside-out group margin.
	/// This prevents native ImGui window padding from shrinking the clipping rect and breaking scrollbars.
	inline void BeginPanelFrame(const char* str_id, float padding = 8.0f)
	{
		float p = Scale(padding);
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(p, p));
		BeginChild(str_id, ImVec2(0, 0), false, 0);
		Dummy(ImVec2(0.0f, Scale(4.0f)));
		Indent(p);
		BeginGroup();
	}

	/// @brief Closes the panel opened by BeginPanelFrame.
	inline void EndPanelFrame(float padding = 8.0f)
	{
		EndGroup();
		Unindent(Scale(padding));
		Dummy(ImVec2(0.0f, Scale(4.0f)));
		EndChild();
		PopStyleVar(1);
	}

	inline bool TreeNode(const char* label, int flags = 0)
	{
		return GetInterface() ? GetInterface()->TreeNodeEx(label, flags) : false;
	}

	inline void TreePop()
	{
		if (auto i = GetInterface())
			i->TreePop();
	}
	inline bool CollapsingHeader(const char* label, int flags = 0) { return GetInterface() ? GetInterface()->CollapsingHeader(label, flags) : false; }

	inline bool BeginPopupContextItem(const char* str_id = nullptr, int mouse_button = 1)
	{
		return GetInterface() ? GetInterface()->BeginPopupContextItem(str_id, mouse_button) : false;
	}
	inline void EndPopup()
	{
		if (auto i = GetInterface())
			i->EndPopup();
	}

	inline bool BeginTabBar(const char* str_id, int flags = 0) { return GetInterface() ? GetInterface()->BeginTabBar(str_id, flags) : false; }
	inline void EndTabBar()
	{
		if (auto i = GetInterface())
			i->EndTabBar();
	}
	inline bool BeginTabItem(const char* label, int flags = 0) { return GetInterface() ? GetInterface()->BeginTabItem(label, flags) : false; }
	inline void EndTabItem()
	{
		if (auto i = GetInterface())
			i->EndTabItem();
	}

	inline bool BeginTable(const char* str_id, int column, TableFlags flags = TableFlags::kNone, const ImVec2& outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f) { return GetInterface() ? GetInterface()->BeginTable(str_id, column, static_cast<int>(flags), outer_size, inner_width) : false; }
	inline void EndTable()
	{
		if (auto i = GetInterface())
			i->EndTable();
	}

	inline void TableSetBgColor(TableBgTarget target, ImU32 color, int column_n = -1)
	{
		if (auto i = GetInterface())
			i->TableSetBgColor(static_cast<int>(target), color, column_n);
	}

	inline float GetColumnWidth(int column_index = -1)
	{
		return GetInterface() ? GetInterface()->GetColumnWidth(column_index) : 0.0f;
	}

	/// @brief Utility to capture a Table Column's width ratio upon releasing the mouse.
	/// Returns true when the weight has meaningfully changed so you can selectively trigger a config save.
	inline bool UpdateProportionalWeight(float& inOutWeight, float totalWidth)
	{
		if (IsMouseReleased(0) && totalWidth > 100.0f) {
			float childWidth = GetWindowSize().x;
			float newWeight  = childWidth / totalWidth;
			if (std::abs(inOutWeight - newWeight) > 0.01f) {
				inOutWeight = newWeight;
				return true;
			}
		}
		return false;
	}

	inline void TableSetupColumn(const char* label, TableColumnFlags flags = TableColumnFlags::kNone, float init_width_or_weight = 0.0f, std::uint32_t user_id = 0)
	{
		if (auto i = GetInterface())
			i->TableSetupColumn(label, static_cast<int>(flags), init_width_or_weight, user_id);
	}
	inline void TableNextRow(int row_flags = 0, float min_row_height = 0.0f)
	{
		if (auto i = GetInterface())
			i->TableNextRow(row_flags, min_row_height);
	}
	inline bool TableNextColumn() { return GetInterface() ? GetInterface()->TableNextColumn() : false; }
	inline void TableHeadersRow()
	{
		if (auto i = GetInterface())
			i->TableHeadersRow();
	}

	inline void Columns(int count = 1, const char* id = nullptr, bool border = true)
	{
		if (auto i = GetInterface())
			i->Columns(count, id, border);
	}
	inline void NextColumn()
	{
		if (auto i = GetInterface())
			i->NextColumn();
	}

	// --------------------------------------------------
	// Drawing & Assets
	// --------------------------------------------------

	inline ImTextureID GetIconForKey(std::uint32_t key, ImVec2* outSize = nullptr)
	{
		if (auto i = GetInterface()) {
			if (outSize)
				i->GetIconSizeForKey(key, &outSize->x, &outSize->y);
			return reinterpret_cast<ImTextureID>(i->GetIconForKey(key));
		}
		return static_cast<ImTextureID>(0);
	}
	inline void Spinner(const char* label, float radius, float thickness, const ImVec4& color)
	{
		if (auto i = GetInterface())
			i->Spinner(label, radius, thickness, color);
	}
	inline void DrawOverlay(Overlay type, float thickness = 1.0f, ImU32 color = 0, float paramA = 0.0f, float paramB = 0.0f, float paramC = 0.0f, float paramD = 0.0f)
	{
		if (auto i = GetInterface())
			i->DrawOverlay(type, thickness, color, paramA, paramB, paramC, paramD);
	}
	inline void DrawGrid(float thickness = 0.0f, ImU32 color = 0, float rows = 3.0f, float cols = 3.0f, float rotationDeg = 0.0f) { DrawOverlay(Overlay::kGrid, thickness, color, rows, cols, rotationDeg, 0.0f); }
	inline void DrawCrosshair(float thickness = 1.0f, ImU32 color = 0, float rows = 1.0f, float cols = 1.0f) { DrawOverlay(Overlay::kCrosshair, thickness, color, rows, cols, 0.0f, 0.0f); }
	inline void DrawGoldenSpiral(float thickness = 1.0f, ImU32 color = 0, int anchorIndex = 0, float turns = 6.0f, float rotationDeg = 0.0f, float scale = 1.0f, bool showSquares = false)
	{
		float pA = static_cast<float>(anchorIndex) + (showSquares ? 10.0f : 0.0f);
		DrawOverlay(Overlay::kGoldenSpiral, thickness, color, pA, turns, rotationDeg, scale);
	}
	inline void DrawGoldenGrid(float thickness = 1.0f, ImU32 color = 0, int subdivisions = 0) { DrawOverlay(Overlay::kGoldenRatio, thickness, color, static_cast<float>(subdivisions), 0.0f, 0.0f, 0.0f); }
	inline void DrawTriangle(float thickness = 1.0f, ImU32 color = 0, bool mirror = false) { DrawOverlay(Overlay::kTriangle, thickness, color, mirror ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f); }

	inline void DrawRect(const ImVec2& min, const ImVec2& max, const ImVec4& col, float rounding = 0.0f, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawRect(min, max, col, rounding, thickness);
	}
	inline void DrawRectFilled(const ImVec2& min, const ImVec2& max, const ImVec4& col, float rounding = 0.0f)
	{
		if (auto i = GetInterface())
			i->DrawRectFilled(min, max, col, rounding);
	}
	inline void DrawLine(const ImVec2& p1, const ImVec2& p2, const ImVec4& col, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawLine(p1, p2, col, thickness);
	}
	inline void DrawImage(ImTextureID textureId, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
	{
		if (auto i = GetInterface())
			i->DrawImage((void*)textureId, size, uv0, uv1, tint_col);
	}
	inline void DrawImageQuad(ImTextureID textureId, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3, const ImVec2& uv4, const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
	{
		if (auto i = GetInterface())
			i->DrawImageQuad((void*)textureId, p1, p2, p3, p4, uv1, uv2, uv3, uv4, tint_col);
	}
	inline void AddImage(ImTextureID textureId, const ImVec2& min, const ImVec2& max, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& col = ImVec4(1, 1, 1, 1))
	{
		if (auto i = GetInterface())
			i->AddImage((void*)textureId, min, max, uv0, uv1, col);
	}
	inline void DrawBackgroundImage(ImTextureID tex, float alpha)
	{
		if (auto i = GetInterface())
			i->DrawBackgroundImage((void*)tex, alpha);
	}
	inline void DrawBackgroundLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness)
	{
		if (auto i = GetInterface())
			i->DrawBackgroundLine(p1.x, p1.y, p2.x, p2.y, col, thickness);
	}
	inline void DrawBackgroundRect(const ImVec2& min, const ImVec2& max, ImU32 col, float thickness)
	{
		if (auto i = GetInterface())
			i->DrawBackgroundRect(min, max, col, thickness);
	}
	inline void DrawScreenRect(const ImVec2& min, const ImVec2& max, ImU32 col, float rounding = 0.0f, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawScreenRect(min, max, col, rounding, thickness);
	}
	inline void DrawScreenRectFilled(const ImVec2& min, const ImVec2& max, ImU32 col, float rounding = 0.0f)
	{
		if (auto i = GetInterface())
			i->DrawScreenRectFilled(min, max, col, rounding);
	}
	inline void DrawScreenLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawScreenLine(p1.x, p1.y, p2.x, p2.y, col, thickness);
	}

	// --------------------------------------------------
	// Strings & Utilities
	// --------------------------------------------------

	/// @brief Translates a "$Key". The result stays valid for the session, or is the key itself if it has no translation.
	inline const char* Translate(const char* key) { return GetInterface() ? GetInterface()->GetTranslation(key) : key; }
	inline void        LoadTranslation(const char* pluginName)
	{
		if (auto i = GetInterface())
			i->LoadTranslation(pluginName);
	}
	inline void SanitizePath(char* dest, const char* source, size_t size)
	{
		if (auto i = GetInterface())
			i->SanitizePath(dest, source, size);
	}
	inline void GetPluginConfigPath(const char* pluginName, char* dest, size_t size)
	{
		if (auto i = GetInterface())
			i->GetPluginConfigPath(pluginName, dest, size);
	}

	/// @brief Absolute path to a plugin's config folder, defaulting to this plugin. Needs no INI support.
	inline std::string GetConfigDirectory(const char* pluginName = g_pluginName)
	{
		char buf[512] = { 0 };
		GetPluginConfigPath(pluginName, buf, sizeof(buf));
		return std::string(buf);
	}

	inline void AddMenuListener(void* userdata, void (*callback)(const char* menuName, bool opening, void* userdata))
	{
		if (auto i = GetInterface())
			i->AddMenuListener(userdata, callback);
	}
	inline void RemoveMenuListener(void* userdata)
	{
		if (auto i = GetInterface())
			i->RemoveMenuListener(userdata);
	}

	// ==================================================
	// [ SECTION 4 ] SMART WRAPPERS & UTILITIES
	// ==================================================

	/// @brief A tool that's just a name, a group and a draw function, for pages that need nothing more.
	/// @note The name and group are copied, so they can be built at runtime. Create it after Connect() so "$Key"_T
	/// names are translated. It can't be copied or moved, so it stays where the host's pointer expects it.
	class SimpleTool : public ITool
	{
	public:
		SimpleTool(std::string a_name, std::string a_group, std::function<void()> a_draw) :
			_name(std::move(a_name)), _group(std::move(a_group)), _draw(std::move(a_draw))
		{}

		SimpleTool(const SimpleTool&)            = delete;
		SimpleTool& operator=(const SimpleTool&) = delete;

		const char* Name() const override { return _name.c_str(); }
		const char* Group() const override { return _group.empty() ? nullptr : _group.c_str(); }
		void        Draw() override
		{
			if (_draw)
				_draw();
		}

	private:
		std::string           _name;
		std::string           _group;
		std::function<void()> _draw;
	};

	/// @brief RAII wrapper for custom images.
	/// @note The handle is the host's, not a texture ImGui can draw. Only pass it to FUCK's image functions.
	class Image
	{
	public:
		Image() = default;
		Image(const char* a_path, bool a_resizeToScreen = false)
		{
			if (auto i = GetInterface()) {
				_handle = i->LoadImage(a_path, a_resizeToScreen);
				if (_handle)
					i->GetImageInfo(_handle, &_width, &_height);
			}
		}
		~Image() { Reset(); }
		Image(Image&& other) noexcept :
			_handle(other._handle), _width(other._width), _height(other._height)
		{
			other._handle = nullptr;
			other._width  = 0.0f;
			other._height = 0.0f;
		}
		Image& operator=(Image&& other) noexcept
		{
			if (this != &other) {
				Reset();
				_handle       = other._handle;
				_width        = other._width;
				_height       = other._height;
				other._handle = nullptr;
				other._width  = 0.0f;
				other._height = 0.0f;
			}
			return *this;
		}
		Image(const Image&)            = delete;
		Image& operator=(const Image&) = delete;

		/// @brief An image loaded through RequestImage(), e.g. in the background. Its size is known once it's ready.
		static Image Request(const ImageRequest& a_request)
		{
			Image image;
			if (auto i = GetInterface()) {
				image._handle = i->RequestImage(&a_request);
				if (image._handle)
					i->GetImageInfo(image._handle, &image._width, &image._height);
			}
			return image;
		}

		/// @brief Whether there's an image, loaded or still loading. See GetState() for which.
		[[nodiscard]] bool        IsLoaded() const { return _handle != nullptr; }
		[[nodiscard]] ImageState  GetState() const
		{
			auto i = GetInterface();
			return i && _handle ? static_cast<ImageState>(i->GetImageState(_handle)) : ImageState::kNone;
		}
		[[nodiscard]] bool        IsReady() const { return GetState() == ImageState::kReady; }
		[[nodiscard]] ImTextureID GetID() const { return (ImTextureID)_handle; }
		operator ImTextureID() const { return (ImTextureID)_handle; }
		operator void*() const { return _handle; }

		[[nodiscard]] float  GetWidth() const { return GetSize().x; }
		[[nodiscard]] float  GetHeight() const { return GetSize().y; }
		[[nodiscard]] ImVec2 GetSize() const
		{
			// A background load's size is read once it's ready
			if (_handle && _width == 0.0f) {
				if (auto i = GetInterface())
					i->GetImageInfo(_handle, &_width, &_height);
			}
			return ImVec2(_width, _height);
		}

		void Reset()
		{
			if (_handle) {
				if (auto i = GetInterface())
					i->ReleaseImage(_handle);
				_handle = nullptr;
				_width  = 0.0f;
				_height = 0.0f;
			}
		}

	private:
		void*         _handle = nullptr;
		mutable float _width  = 0.0f;
		mutable float _height = 0.0f;
	};

	/// @brief Draws an image rotated about its centre, in screen space.
	/// @param angleDegrees Rotation angle in degrees (clockwise).
	inline void DrawImageRotated(ImTextureID textureId, const ImVec2& pos, const ImVec2& size, float angleDegrees, const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
	{
		ImVec2 center = { pos.x + size.x * 0.5f, pos.y + size.y * 0.5f };
		float  rad    = angleDegrees * (3.14159265f / 180.0f);
		float  cosA   = std::cos(rad);
		float  sinA   = std::sin(rad);

		float halfW = size.x * 0.5f;
		float halfH = size.y * 0.5f;

		// Rotate each corner's offset from the centre
		ImVec2 p1 = { center.x - halfW * cosA + halfH * sinA, center.y - halfW * sinA - halfH * cosA };  // Top Left
		ImVec2 p2 = { center.x + halfW * cosA + halfH * sinA, center.y + halfW * sinA - halfH * cosA };  // Top Right
		ImVec2 p3 = { center.x + halfW * cosA - halfH * sinA, center.y + halfW * sinA + halfH * cosA };  // Bottom Right
		ImVec2 p4 = { center.x - halfW * cosA - halfH * sinA, center.y - halfW * sinA + halfH * cosA };  // Bottom Left

		DrawImageQuad(textureId, p1, p2, p3, p4, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, tint_col);
	}

	/// @brief String copy utility.
	template <size_t N>
	inline void StringCopy(char (&dest)[N], const char* source)
	{
		if (!source) {
			dest[0] = '\0';
			return;
		}
		strncpy_s(dest, N, source, _TRUNCATE);
	}

	template <size_t N>
	inline void StringCopy(char (&dest)[N], const std::string& source)
	{
		strncpy_s(dest, N, source.c_str(), _TRUNCATE);
	}

#ifdef FUCK_API_ENABLE_SIMPLEINI

	namespace INI
	{
		template <class T>
		class Fields;
	}

	/// @brief Convenience wrapper for Plugin INI Loading/Saving.
	class PluginSettings
	{
	public:
		// --- Automatically defaults to the globally registered plugin name ---
		PluginSettings(const char* a_pluginName = FUCK::g_pluginName) :
			_pluginName(a_pluginName) {}

		using INIFunc = std::function<void(CSimpleIniA&)>;

		void Load(INIFunc a_func) const
		{
			if (!a_func)
				return;
			if (auto* i = GetInterface())
				i->LoadPluginINI(_pluginName, &a_func, [](CSimpleIniA& ini, void* ud) { (*static_cast<INIFunc*>(ud))(ini); });
		}

		void Save(INIFunc a_func) const
		{
			if (!a_func)
				return;
			if (auto* i = GetInterface())
				i->SavePluginINI(_pluginName, &a_func, [](CSimpleIniA& ini, void* ud) { (*static_cast<INIFunc*>(ud))(ini); });
		}

		/// @brief Loads or saves a settings struct through its list of fields, see INI::Fields. Defaults are the struct's own unless passed.
		template <class T>
		void Load(const INI::Fields<T>& a_fields, T& a_values, const T& a_defaults = T{}) const
		{
			Load([&](CSimpleIniA& ini) { a_fields.Load(ini, a_values, a_defaults); });
		}

		template <class T>
		void Save(const INI::Fields<T>& a_fields, const T& a_values, const T& a_defaults = T{}) const
		{
			Save([&](CSimpleIniA& ini) { a_fields.Save(ini, a_values, a_defaults); });
		}

		/// @brief Loads the default shipped INI, ignoring user settings.
		void LoadDefaults(INIFunc a_func) const
		{
			if (!a_func)
				return;
			if (auto* i = GetInterface())
				i->LoadPluginINIDefaults(_pluginName, &a_func, [](CSimpleIniA& ini, void* ud) { (*static_cast<INIFunc*>(ud))(ini); });
		}

		void LoadKeybinds(INIFunc a_func) const
		{
			if (!a_func)
				return;
			if (auto* i = GetInterface())
				i->LoadPluginKeybinds(_pluginName, &a_func, [](CSimpleIniA& ini, void* ud) { (*static_cast<INIFunc*>(ud))(ini); });
		}

		void SaveKeybinds(INIFunc a_func) const
		{
			if (!a_func)
				return;
			if (auto* i = GetInterface())
				i->SavePluginKeybinds(_pluginName, &a_func, [](CSimpleIniA& ini, void* ud) { (*static_cast<INIFunc*>(ud))(ini); });
		}

		void LoadKeybindsDefaults(INIFunc a_func) const
		{
			if (!a_func)
				return;
			if (auto* i = GetInterface())
				i->LoadPluginKeybindsDefaults(_pluginName, &a_func, [](CSimpleIniA& ini, void* ud) { (*static_cast<INIFunc*>(ud))(ini); });
		}

		/// @brief Returns the absolute path string pointing to this plugin's dedicated config folder.
		std::string GetConfigDirectory() const { return FUCK::GetConfigDirectory(_pluginName); }

	private:
		const char* _pluginName;
	};

	/// @brief Delta save/load INI values.
	namespace INI
	{
		inline bool LoadBool(const CSimpleIniA& ini, const char* sec, const char* key, bool defVal)
		{
			return ini.GetBoolValue(sec, key, defVal);
		}

		inline float LoadFloat(const CSimpleIniA& ini, const char* sec, const char* key, float defVal)
		{
			return static_cast<float>(ini.GetDoubleValue(sec, key, static_cast<double>(defVal)));
		}

		// Handles mixed ints
		template <typename T>
		inline T LoadInt(const CSimpleIniA& ini, const char* sec, const char* key, T defVal)
		{
			static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "FUCK::INI::LoadInt requires integral or enum types.");
			return static_cast<T>(ini.GetLongValue(sec, key, static_cast<long>(defVal)));
		}

		inline void SaveBool(CSimpleIniA& ini, const char* sec, const char* key, bool val, bool defVal)
		{
			if (val == defVal)
				ini.Delete(sec, key, true);
			else
				ini.SetBoolValue(sec, key, val);
		}

		// Handles mixed ints
		template <typename T, typename U>
		inline void SaveInt(CSimpleIniA& ini, const char* sec, const char* key, T val, U defVal)
		{
			static_assert(std::is_integral_v<T> && std::is_integral_v<U>, "FUCK::INI::SaveInt requires integral types.");
			if (val == static_cast<T>(defVal)) {
				ini.Delete(sec, key, true);
			} else {
				ini.SetLongValue(sec, key, static_cast<long>(val));
			}
		}

		inline void SaveDouble(CSimpleIniA& ini, const char* sec, const char* key, double val, double defVal, const char* fmt = "%.2f")
		{
			if (std::abs(val - defVal) < 0.00001) {
				ini.Delete(sec, key, true);
			} else {
				char buf[64];
				snprintf(buf, sizeof(buf), fmt, val);
				ini.SetValue(sec, key, buf);
			}
		}

		inline void SaveString(CSimpleIniA& ini, const char* sec, const char* key, const char* val, const char* defVal)
		{
			if (strcmp(val, defVal) == 0)
				ini.Delete(sec, key, true);
			else
				ini.SetValue(sec, key, val);
		}

		template <size_t N>
		inline void LoadString(const CSimpleIniA& ini, const char* sec, const char* key, char (&dest)[N], const char* defVal)
		{
			const char* val = ini.GetValue(sec, key, defVal);
			strncpy_s(dest, N, val, _TRUNCATE);
		}

		/// @brief A settings struct's INI keys, each named once with the member it fills. Defaults are the struct's own initialisers
		/// unless others are passed, and a value left at its default is taken out of the user's INI rather than written.
		/// Members can be bool, float, double, std::string, or any integer or enum type.
		/// @code
		/// struct Values { bool enabled = true; float speed = 1.0f; };
		///
		/// const FUCK::INI::Fields<Values> fields{ "General", {
		///     { "bEnable", &Values::enabled },
		///     { "fSpeed",  &Values::speed   },
		/// } };
		///
		/// settings.Load(fields, values);
		/// settings.Save(fields, values);
		/// @endcode
		template <class T>
		class Fields
		{
		public:
			struct Field
			{
				template <class M>
				Field(const char* a_key, M T::*a_member) :
					key(a_key),
					load([a_member](const CSimpleIniA& ini, const char* sec, const char* key, T& values, const T& defaults) {
						Read(ini, sec, key, values.*a_member, defaults.*a_member);
					}),
					save([a_member](CSimpleIniA& ini, const char* sec, const char* key, const T& values, const T& defaults) {
						Write(ini, sec, key, values.*a_member, defaults.*a_member);
					})
				{}

				const char*                                                                     key;
				std::function<void(const CSimpleIniA&, const char*, const char*, T&, const T&)> load;
				std::function<void(CSimpleIniA&, const char*, const char*, const T&, const T&)> save;
			};

			Fields(const char* a_section, std::initializer_list<Field> a_fields) :
				_section(a_section), _fields(a_fields) {}

			void Load(const CSimpleIniA& ini, T& a_values, const T& a_defaults = T{}) const
			{
				for (const auto& field : _fields) {
					field.load(ini, _section, field.key, a_values, a_defaults);
				}
			}

			void Save(CSimpleIniA& ini, const T& a_values, const T& a_defaults = T{}) const
			{
				for (const auto& field : _fields) {
					field.save(ini, _section, field.key, a_values, a_defaults);
				}
			}

		private:
			template <class M>
			static void Read(const CSimpleIniA& ini, const char* sec, const char* key, M& val, const M& defVal)
			{
				if constexpr (std::is_same_v<M, bool>) {
					val = LoadBool(ini, sec, key, defVal);
				} else if constexpr (std::is_floating_point_v<M>) {
					val = static_cast<M>(ini.GetDoubleValue(sec, key, static_cast<double>(defVal)));
				} else if constexpr (std::is_same_v<M, std::string>) {
					val = ini.GetValue(sec, key, defVal.c_str());
				} else {
					static_assert(std::is_integral_v<M> || std::is_enum_v<M>, "FUCK::INI::Fields: unsupported member type.");
					val = LoadInt(ini, sec, key, defVal);
				}
			}

			template <class M>
			static void Write(CSimpleIniA& ini, const char* sec, const char* key, const M& val, const M& defVal)
			{
				if constexpr (std::is_same_v<M, bool>) {
					SaveBool(ini, sec, key, val, defVal);
				} else if constexpr (std::is_floating_point_v<M>) {
					SaveDouble(ini, sec, key, static_cast<double>(val), static_cast<double>(defVal), "%g");
				} else if constexpr (std::is_same_v<M, std::string>) {
					SaveString(ini, sec, key, val.c_str(), defVal.c_str());
				} else if constexpr (std::is_enum_v<M>) {
					using U = std::underlying_type_t<M>;
					SaveInt(ini, sec, key, static_cast<U>(val), static_cast<U>(defVal));
				} else {
					SaveInt(ini, sec, key, val, defVal);
				}
			}

			const char*        _section;
			std::vector<Field> _fields;
		};
	}
#endif  // FUCK_API_ENABLE_SIMPLEINI

	/// @brief RAII Wrapper for listening to Skyrim UI Menu events.
	class MenuEventListener
	{
	public:
		using Callback = std::function<void(const char* menuName, bool opening)>;

		MenuEventListener() = default;
		explicit MenuEventListener(Callback a_cb) :
			_callback(std::move(a_cb))
		{
			AddMenuListener(this, &MenuEventListener::Dispatch);
		}
		~MenuEventListener()
		{
			if (_callback)
				RemoveMenuListener(this);
		}

		MenuEventListener(const MenuEventListener&)            = delete;
		MenuEventListener& operator=(const MenuEventListener&) = delete;
		MenuEventListener& operator=(MenuEventListener&& other) noexcept
		{
			if (this != &other) {
				if (_callback)
					RemoveMenuListener(this);
				_callback = std::move(other._callback);
				if (_callback) {
					RemoveMenuListener(&other);
					AddMenuListener(this, &MenuEventListener::Dispatch);
					other._callback = nullptr;
				}
			}
			return *this;
		}

	private:
		static void Dispatch(const char* menuName, bool opening, void* userdata)
		{
			static_cast<MenuEventListener*>(userdata)->_callback(menuName, opening);
		}
		Callback _callback;
	};

	/// @brief RAII Wrapper for listening to FUCK Window Open/Close events from any plugin.
	class WindowEventListener
	{
	public:
		using Callback = std::function<void(const char* pluginName, const char* windowId, bool opening)>;

		WindowEventListener() = default;
		explicit WindowEventListener(Callback a_cb) :
			_callback(std::move(a_cb))
		{
			if (auto i = GetInterface())
				i->AddWindowListener(this, &WindowEventListener::Dispatch);
		}
		~WindowEventListener()
		{
			if (_callback)
				if (auto i = GetInterface())
					i->RemoveWindowListener(this);
		}

		WindowEventListener(const WindowEventListener&)            = delete;
		WindowEventListener& operator=(const WindowEventListener&) = delete;
		WindowEventListener& operator=(WindowEventListener&& other) noexcept
		{
			if (this != &other) {
				if (_callback)
					if (auto i = GetInterface())
						i->RemoveWindowListener(this);

				_callback = std::move(other._callback);

				if (_callback) {
					if (auto i = GetInterface()) {
						i->RemoveWindowListener(&other);
						i->AddWindowListener(this, &WindowEventListener::Dispatch);
					}
					other._callback = nullptr;
				}
			}
			return *this;
		}

	private:
		static void Dispatch(const char* pluginName, const char* windowId, bool opening, void* userdata)
		{
			static_cast<WindowEventListener*>(userdata)->_callback(pluginName, windowId, opening);
		}
		Callback _callback;
	};

	/// @brief Used for assigning Hotkeys within the FUCK interface.
	inline bool DrawManagedHotkey(const char* label, ManagedHotkey& h, HotkeyFlags flags = HotkeyFlags::kNone, float iconScale = 1.0f, float labelScale = 1.0f)
	{
		if (auto i = GetInterface())
			return i->DrawManagedHotkey(label, &h, static_cast<int>(flags), iconScale, labelScale);
		return false;
	}

	inline bool UpdateManagedHotkey(const void* e, ManagedHotkey& h)
	{
		return GetInterface() ? GetInterface()->UpdateManagedHotkey(e, &h) : false;
	}

	inline bool ProcessManagedHotkey(const void* e, ManagedHotkey& h)
	{
		return GetInterface() ? GetInterface()->ProcessManagedHotkey(e, &h) : false;
	}

	inline bool IsManagedHotkeyDown(ManagedHotkey& h)
	{
		return GetInterface() ? GetInterface()->IsManagedHotkeyDown(&h) : false;
	}

	inline void AbortManagedHotkey(ManagedHotkey& h)
	{
		if (h.isBinding) {
			AbortBinding();
			h.isBinding = false;
		}
	}

	// --------------------------------------------------
	// Overloads & Templates
	// --------------------------------------------------

	/// @brief Pushes a font scaled by a fractional multiplier. Must be paired with FUCK::PopFont().
	inline void PushFontScaled(ImFont* font, float scale)
	{
		if (!font)
			font = GetFont(Font::kRegular);

		// Fallback to 30.0f (framework's base size) if font is somehow null
		float baseSize = font ? font->LegacySize : 30.0f;

		PushFont(font, baseSize * GetGlobalScale() * scale);
	}

	/// @brief Colour for an editor state, shared by the bounds and resize handles.
	inline ImU32 GetEditorBoundsColor(EditorBoundsState state)
	{
		switch (state) {
		case EditorBoundsState::kLocked:
			return IM_COL32(150, 150, 150, 150);  // Grey
		case EditorBoundsState::kSelected:
			return IM_COL32(51, 204, 51, 255);  // Green
		case EditorBoundsState::kHovered:
			return IM_COL32(0, 255, 0, 255);  // Bright Green
		case EditorBoundsState::kNormal:
		default:
			return IM_COL32(204, 51, 255, 255);  // Purple
		}
	}

	/// @brief Rotates a point about a centre, clockwise in screen space.
	inline ImVec2 RotatePoint(const ImVec2& a_point, const ImVec2& a_center, float a_degrees)
	{
		float rad = a_degrees * (3.14159265f / 180.0f);
		float c   = std::cos(rad);
		float s   = std::sin(rad);
		float dx  = a_point.x - a_center.x;
		float dy  = a_point.y - a_center.y;
		return { a_center.x + dx * c - dy * s, a_center.y + dx * s + dy * c };
	}

	/// @brief Visual for UI widget editing. Handles Screen-Space and Window-Space.
	/// @note With rotation, the outline turns about its centre clockwise, in degrees.
	inline void DrawEditorBounds(const ImVec2& min, const ImVec2& max, EditorBoundsState state = EditorBoundsState::kNormal, float thickness = 2.0f, bool screenSpace = false, const ImVec2* customAnchor = nullptr, float rotation = 0.0f)
	{
		ImU32 boundsColor = GetEditorBoundsColor(state);

		if (rotation != 0.0f) {
			ImVec2 center    = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
			ImVec2 corners[] = {
				RotatePoint(min, center, rotation),
				RotatePoint({ max.x, min.y }, center, rotation),
				RotatePoint(max, center, rotation),
				RotatePoint({ min.x, max.y }, center, rotation)
			};
			ImVec4 colV4 = ImGui::ColorConvertU32ToFloat4(boundsColor);
			for (int i = 0; i < 4; ++i) {
				if (screenSpace)
					DrawScreenLine(corners[i], corners[(i + 1) % 4], boundsColor, thickness);
				else
					DrawLine(corners[i], corners[(i + 1) % 4], colV4, thickness);
			}
		} else if (screenSpace) {
			DrawScreenRect(min, max, boundsColor, 0.0f, thickness);
		} else {
			ImVec4 colV4 = ImGui::ColorConvertU32ToFloat4(boundsColor);
			DrawRect(min, max, colV4, 0.0f, thickness);
		}

		ImVec2 anchor      = customAnchor ? *customAnchor : ImVec2(min.x + (max.x - min.x) * 0.5f, min.y + (max.y - min.y) * 0.5f);
		float  crossSize   = Scale(10.0f);
		ImU32  anchorColor = IM_COL32(255, 0, 0, 204);

		if (screenSpace) {
			DrawScreenLine({ anchor.x - crossSize, anchor.y }, { anchor.x + crossSize, anchor.y }, anchorColor, 2.0f);
			DrawScreenLine({ anchor.x, anchor.y - crossSize }, { anchor.x, anchor.y + crossSize }, anchorColor, 2.0f);
		} else {
			ImVec4 anchorV4 = ImGui::ColorConvertU32ToFloat4(anchorColor);
			DrawLine({ anchor.x - crossSize, anchor.y }, { anchor.x + crossSize, anchor.y }, anchorV4, 2.0f);
			DrawLine({ anchor.x, anchor.y - crossSize }, { anchor.x, anchor.y + crossSize }, anchorV4, 2.0f);
		}
	}

	/// @brief Drag state for EditorResizeHandles. Keep one per widget, between frames.
	struct EditorResizeState
	{
		int    corner  = -1;         // 0-3 clockwise from top-left, -1 = idle
		bool   hovered = false;      // skip starting a move while true
		ImVec2 scale{ 1.0f, 1.0f };  // size against the rect at drag start, equal when aspect is locked
		ImVec2 pivot;                // fixed opposite corner, in screen space
		ImVec2 startMin;
		ImVec2 startMax;
		ImVec2 startMouse;

		bool IsActive() const { return corner >= 0; }
	};

	/// @brief Result of an editor handle for this frame.
	enum class EditorHandleResult
	{
		kNone,
		kStarted,
		kDragging,
		kFinished
	};

	/// @brief Corner drag-to-resize handles for an editor outline (screen space). Outputs the resized rect.
	/// Note: a_windowHovered says whether the handles may take the mouse, e.g. IsWindowHovered() for handles drawn in a window.
	/// With a_rotation, the box turns about its centre and the output rect is unrotated, so use scale and pivot instead.
	/// Each corner is a button, so grabbing one doesn't also drag the window. Call after your content, as the cursor is left after the handles.
	/// @note For more than one in the same window, wrap each in PushID()/PopID() so their handles don't clash.
	inline EditorHandleResult EditorResizeHandles(const ImVec2& a_min, const ImVec2& a_max, EditorResizeState& a_state, ImVec2& a_outMin, ImVec2& a_outMax,
		bool a_windowHovered, bool a_lockAspect = true, const ImVec2& a_minSize = { 50.0f, 50.0f }, const ImVec2& a_maxSize = { 100000.0f, 100000.0f }, float a_rotation = 0.0f)
	{
		auto cornerOf = [](const ImVec2& a_lo, const ImVec2& a_hi, int a_index) {
			return ImVec2(a_index == 1 || a_index == 2 ? a_hi.x : a_lo.x, a_index >= 2 ? a_hi.y : a_lo.y);
		};

		const float  half   = Scale(6.0f);
		const ImVec2 mouse  = GetMousePos();
		const ImVec2 center = { (a_min.x + a_max.x) * 0.5f, (a_min.y + a_max.y) * 0.5f };

		a_outMin = a_min;
		a_outMax = a_max;

		// An invisible button on each corner, so pressing one doesn't also drag the window it's drawn in.
		// The cursor isn't moved back afterwards, as ImGui errors when a moved cursor has no item after it
		static const char* const buttonIds[4] = { "##FUCK_EditorResize0", "##FUCK_EditorResize1", "##FUCK_EditorResize2", "##FUCK_EditorResize3" };
		for (int i = 0; i < 4; ++i) {
			ImVec2 c = RotatePoint(cornerOf(a_min, a_max, i), center, a_rotation);
			SetCursorScreenPos({ c.x - half, c.y - half });
			InvisibleButton(buttonIds[i], { half * 2.0f, half * 2.0f });
		}

		int hoveredCorner = -1;
		if (!a_state.IsActive() && a_windowHovered) {
			for (int i = 0; i < 4; ++i) {
				ImVec2 c = RotatePoint(cornerOf(a_min, a_max, i), center, a_rotation);
				if (std::abs(mouse.x - c.x) <= half && std::abs(mouse.y - c.y) <= half) {
					hoveredCorner = i;
					break;
				}
			}
		}
		a_state.hovered = hoveredCorner >= 0;

		auto result = EditorHandleResult::kNone;

		if (a_state.IsActive() && !IsMouseDown(0)) {
			a_state.corner = -1;
			result         = EditorHandleResult::kFinished;
		} else if (hoveredCorner >= 0 && IsMouseClicked(0)) {
			a_state.corner     = hoveredCorner;
			a_state.pivot      = RotatePoint(cornerOf(a_min, a_max, (hoveredCorner + 2) % 4), center, a_rotation);
			a_state.startMin   = a_min;
			a_state.startMax   = a_max;
			a_state.startMouse = mouse;
			a_state.scale      = { 1.0f, 1.0f };
			result             = EditorHandleResult::kStarted;
		}

		if (a_state.IsActive()) {
			ImVec2 size(a_state.startMax.x - a_state.startMin.x, a_state.startMax.y - a_state.startMin.y);
			ImVec2 grabbed = cornerOf(a_state.startMin, a_state.startMax, a_state.corner);
			ImVec2 pivot   = cornerOf(a_state.startMin, a_state.startMax, (a_state.corner + 2) % 4);

			// Turn the drag back into the box's own unrotated axes
			ImVec2 drag = RotatePoint({ mouse.x - a_state.startMouse.x, mouse.y - a_state.startMouse.y }, { 0.0f, 0.0f }, -a_rotation);

			// Moving away from the pivot grows the rect
			float dirX = grabbed.x > pivot.x ? 1.0f : -1.0f;
			float dirY = grabbed.y > pivot.y ? 1.0f : -1.0f;
			float dx   = drag.x * dirX;
			float dy   = drag.y * dirY;

			float newW, newH;
			if (a_lockAspect && size.x > 0.0f && size.y > 0.0f) {
				// Project onto the diagonal so both axes scale together
				float diag     = std::sqrt(size.x * size.x + size.y * size.y);
				float scale    = (diag + (dx * size.x + dy * size.y) / diag) / diag;
				float minScale = std::max(a_minSize.x / size.x, a_minSize.y / size.y);
				float maxScale = std::max(std::min(a_maxSize.x / size.x, a_maxSize.y / size.y), minScale);
				scale          = std::clamp(scale, minScale, maxScale);
				newW           = size.x * scale;
				newH           = size.y * scale;
			} else {
				newW = std::clamp(size.x + dx, a_minSize.x, a_maxSize.x);
				newH = std::clamp(size.y + dy, a_minSize.y, a_maxSize.y);
			}
			a_state.scale = { size.x > 0.0f ? newW / size.x : 1.0f, size.y > 0.0f ? newH / size.y : 1.0f };

			a_outMin.x = dirX > 0.0f ? pivot.x : pivot.x - newW;
			a_outMin.y = dirY > 0.0f ? pivot.y : pivot.y - newH;
			a_outMax.x = a_outMin.x + newW;
			a_outMax.y = a_outMin.y + newH;

			if (result == EditorHandleResult::kNone)
				result = EditorHandleResult::kDragging;
		}

		for (int i = 0; i < 4; ++i) {
			ImVec2 c     = RotatePoint(cornerOf(a_min, a_max, i), center, a_rotation);
			auto   state = a_state.corner == i ? EditorBoundsState::kSelected :
			               hoveredCorner == i  ? EditorBoundsState::kHovered :
			                                     EditorBoundsState::kNormal;
			ImU32  col   = GetEditorBoundsColor(state);
			DrawScreenRectFilled({ c.x - half, c.y - half }, { c.x + half, c.y + half }, col);
		}

		return result;
	}

	/// @brief Drag state for EditorRotateHandle. Keep one per widget, between frames.
	struct EditorRotateState
	{
		bool  active     = false;
		bool  hovered    = false;  // skip starting a move while true
		float angle      = 0.0f;   // degrees turned clockwise since the drag started
		float startAngle = 0.0f;
	};

	/// @brief Rotation handle above an editor outline (screen space), turning about a_pivot. Hold Shift to snap.
	/// Note: a_windowHovered says whether the handle may take the mouse, e.g. IsWindowHovered() for a handle drawn in a window.
	/// With a_rotation, the handle turns with the box. Call last in the window, as the cursor is left after the handle.
	/// @note For more than one in the same window, wrap each in PushID()/PopID() so their handles don't clash..
	inline EditorHandleResult EditorRotateHandle(const ImVec2& a_min, const ImVec2& a_max, const ImVec2& a_pivot, EditorRotateState& a_state, bool a_windowHovered,
		float a_snapDegrees = 15.0f, float a_rotation = 0.0f)
	{
		constexpr float RAD2DEG = 180.0f / 3.14159265f;

		const float  radius = Scale(6.0f);
		const ImVec2 center = { (a_min.x + a_max.x) * 0.5f, (a_min.y + a_max.y) * 0.5f };
		const ImVec2 stem   = RotatePoint({ center.x, a_min.y }, center, a_rotation);
		const ImVec2 handle = RotatePoint({ center.x, a_min.y - Scale(24.0f) }, center, a_rotation);
		const ImVec2 mouse  = GetMousePos();

		// An invisible button on the handle, so pressing it doesn't also drag the window it's drawn in.
		// The cursor isn't moved back afterwards, as ImGui errors when a moved cursor has no item after it
		SetCursorScreenPos({ handle.x - radius, handle.y - radius });
		InvisibleButton("##FUCK_EditorRotate", { radius * 2.0f, radius * 2.0f });

		float dx        = mouse.x - handle.x;
		float dy        = mouse.y - handle.y;
		a_state.hovered = !a_state.active && a_windowHovered && (dx * dx + dy * dy) <= radius * radius;

		// Screen Y points down, so atan2 already increases clockwise
		float mouseAngle = std::atan2(mouse.y - a_pivot.y, mouse.x - a_pivot.x) * RAD2DEG;

		auto result = EditorHandleResult::kNone;
		if (a_state.active && !IsMouseDown(0)) {
			a_state.active = false;
			result         = EditorHandleResult::kFinished;
		} else if (a_state.hovered && IsMouseClicked(0)) {
			a_state.active     = true;
			a_state.startAngle = mouseAngle;
			a_state.angle      = 0.0f;
			result             = EditorHandleResult::kStarted;
		}

		if (a_state.active) {
			float angle = std::remainder(mouseAngle - a_state.startAngle, 360.0f);
			if (IsKeyDown(ImGuiMod_Shift) && a_snapDegrees > 0.0f)
				angle = std::round(angle / a_snapDegrees) * a_snapDegrees;
			a_state.angle = angle;

			if (result == EditorHandleResult::kNone)
				result = EditorHandleResult::kDragging;
		}

		auto  state = a_state.active  ? EditorBoundsState::kSelected :
		              a_state.hovered ? EditorBoundsState::kHovered :
		                                EditorBoundsState::kNormal;
		ImU32 col   = GetEditorBoundsColor(state);
		DrawScreenLine(stem, handle, col, 2.0f);
		DrawScreenRectFilled({ handle.x - radius, handle.y - radius }, { handle.x + radius, handle.y + radius }, col, radius);

		return result;
	}

	/// @brief Clamps a window or widget position to the screen bounds if it strays too far off-screen.
	inline bool ClampPosToScreen(ImVec2& pos, float outOfBoundsTolerance = 50.0f)
	{
		ImVec2 displaySize = GetDisplaySize();
		if (displaySize.x <= 0.0f || displaySize.y <= 0.0f) {
			return false;
		}

		bool  changed   = false;
		float tolerance = Scale(outOfBoundsTolerance);

		// X Axis
		if (pos.x > displaySize.x - tolerance) {
			pos.x   = std::max(0.0f, displaySize.x - tolerance);
			changed = true;
		} else if (pos.x < 0.0f) {
			pos.x   = 0.0f;
			changed = true;
		}

		// Y Axis
		if (pos.y > displaySize.y - tolerance) {
			pos.y   = std::max(0.0f, displaySize.y - tolerance);
			changed = true;
		} else if (pos.y < 0.0f) {
			pos.y   = 0.0f;
			changed = true;
		}

		return changed;
	}

	enum class PosInitResult
	{
		kNotReady,
		kUnchanged,
		kChanged
	};

	/// @brief Handles first-frame initialization, default loading, and screen clamping for custom-positioned windows.
	inline PosInitResult InitializeCustomPosition(ImVec2& pos, const ImVec2& defaultPos, bool& outHasClamped, float tolerance = 50.0f)
	{
		ImVec2 displaySize = GetDisplaySize();
		if (displaySize.x <= 100.0f || displaySize.y <= 100.0f) {
			return PosInitResult::kNotReady;
		}

		bool changed = false;

		if (pos.x < 0.0f || pos.y < 0.0f) {
			pos     = defaultPos;
			changed = true;
		} else if (!outHasClamped) {
			if (ClampPosToScreen(pos, tolerance)) {
				changed = true;
			}
			outHasClamped = true;
		}

		return changed ? PosInitResult::kChanged : PosInitResult::kUnchanged;
	}

	/// @brief WASD key widget nudging.
	inline bool WASDNudge(float& outDeltaX, float& outDeltaY, bool isActiveOrHovered, float step = 1.0f, float sprintMult = 10.0f)
	{
		if (!isActiveOrHovered || IsMouseDown(0))
			return false;

		float moveStep = step / GetResolutionScale();
		if (IsKeyDown(ImGuiMod_Shift))
			moveStep = (step * sprintMult) / GetResolutionScale();

		outDeltaX = 0.0f;
		outDeltaY = 0.0f;
		if (IsKeyPressed(ImGuiKey_W, true))
			outDeltaY -= moveStep;
		if (IsKeyPressed(ImGuiKey_S, true))
			outDeltaY += moveStep;
		if (IsKeyPressed(ImGuiKey_A, true))
			outDeltaX -= moveStep;
		if (IsKeyPressed(ImGuiKey_D, true))
			outDeltaX += moveStep;

		return (outDeltaX != 0.0f || outDeltaY != 0.0f);
	}

	inline bool Combo(const char* label, int* current_item, const std::vector<std::string>& items)
	{
		std::vector<const char*> ptrs(items.size());
		for (size_t i = 0; i < items.size(); ++i) ptrs[i] = items[i].c_str();
		return Combo(label, current_item, ptrs.data(), static_cast<int>(ptrs.size()));
	}

	inline bool ComboWithFilter(const char* label, int* current_item, std::span<const std::string> items, int popup_max_height_in_items = -1)
	{
		std::vector<const char*> ptrs(items.size());
		for (size_t i = 0; i < items.size(); ++i) ptrs[i] = items[i].c_str();
		return GetInterface() ? GetInterface()->ComboWithFilter(label, current_item, ptrs.data(), static_cast<int>(ptrs.size()), popup_max_height_in_items) : false;
	}

	inline bool InputText(const char* label, std::string* str, int flags = 0)
	{
		if (!str)
			return false;
		char buf[2048];
		strncpy_s(buf, sizeof(buf), str->c_str(), _TRUNCATE);
		const bool changed = InputText(label, buf, sizeof(buf), flags);
		if (changed)
			*str = buf;
		return changed;
	}

	template <typename T>
	bool EnumStepper(const char* label, T* current_val, const std::vector<std::string>& items, bool a_translate = true)
	{
		if (items.empty())
			return false;

		int idx = static_cast<int>(*current_val);
		if (idx < 0)
			idx = 0;
		if (idx >= static_cast<int>(items.size()))
			idx = static_cast<int>(items.size()) - 1;

		bool l = false, r = false;

		const char* displayText = a_translate ? Translate(items[idx].c_str()) : items[idx].c_str();

		Stepper(label, displayText, &l, &r);

		if (l) {
			*current_val = static_cast<T>((idx - 1 + static_cast<int>(items.size())) % static_cast<int>(items.size()));
			return true;
		}
		if (r) {
			*current_val = static_cast<T>((idx + 1) % static_cast<int>(items.size()));
			return true;
		}
		return false;
	}

	// --------------------------------------------------
	// Version 2
	// --------------------------------------------------

	inline void SeparatorVertical()
	{
		if (auto i = GetInterface())
			i->SeparatorVertical();
	}

	inline void PushItemWidth(float item_width)
	{
		if (auto i = GetInterface())
			i->PushItemWidth(item_width);
	}

	inline void PopItemWidth()
	{
		if (auto i = GetInterface())
			i->PopItemWidth();
	}

	inline bool BeginTooltip()
	{
		return GetInterface() ? GetInterface()->BeginTooltip() : false;
	}

	inline void EndTooltip()
	{
		if (auto i = GetInterface())
			i->EndTooltip();
	}

	inline void SetScrollHereY(float center_y_ratio = 0.5f)
	{
		if (auto i = GetInterface())
			i->SetScrollHereY(center_y_ratio);
	}

	inline bool InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0), int flags = 0)
	{
		return GetInterface() ? GetInterface()->InputTextMultiline(label, buf, buf_size, size, flags) : false;
	}

	inline bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), int flags = 0)
	{
		if (!str || !GetInterface())
			return false;

		char buf[4096];
		strncpy_s(buf, sizeof(buf), str->c_str(), _TRUNCATE);
		const bool changed = InputTextMultiline(label, buf, sizeof(buf), size, flags);
		if (changed)
			*str = buf;
		return changed;
	}

	// --------------------------------------------------
	// Version 3
	// --------------------------------------------------

	/// @brief Enables or disables the hotkey that toggles the FUCK menu. Plugins' own hotkeys are unaffected.
	/// @note Shared by all plugins, so re-enable it when you're done.
	inline void SetHotkeyEnabled(bool enabled)
	{
		if (auto i = GetInterface())
			i->SetHotkeyEnabled(enabled);
	}

	inline void SetWindowFocus()
	{
		if (auto i = GetInterface())
			i->SetWindowFocus();
	}

	inline void CloseCurrentPopup()
	{
		if (auto i = GetInterface())
			i->CloseCurrentPopup();
	}

	inline void OpenPopup(const char* str_id, PopupFlags flags = PopupFlags::kNone)
	{
		if (auto i = GetInterface())
			i->OpenPopup(str_id, static_cast<int>(flags));
	}

	/// @param flags Currently ignored. Reserved for future updates.
	inline bool BeginPopup(const char* str_id, WindowFlags flags = WindowFlags::kNone)
	{
		return GetInterface() ? GetInterface()->BeginPopup(str_id, static_cast<int>(flags)) : false;
	}

	/// @param flags Currently ignored. Reserved for future updates.
	inline bool BeginPopupModal(const char* name, bool* p_open = nullptr, WindowFlags flags = WindowFlags::kNone)
	{
		return GetInterface() ? GetInterface()->BeginPopupModal(name, p_open, static_cast<int>(flags)) : false;
	}

	inline bool IsWindowAppearing()
	{
		return GetInterface() ? GetInterface()->IsWindowAppearing() : false;
	}

	inline void PushTextWrapPos(float wrap_local_pos_x = 0.0f)
	{
		if (auto i = GetInterface())
			i->PushTextWrapPos(wrap_local_pos_x);
	}

	inline void PopTextWrapPos()
	{
		if (auto i = GetInterface())
			i->PopTextWrapPos();
	}

	inline void SetNavCursorVisible(bool visible)
	{
		if (auto i = GetInterface())
			i->SetNavCursorVisible(visible);
	}

	inline void DrawCircle(const ImVec2& center, float radius, const ImVec4& color, int num_segments = 0, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawCircle(center, radius, color, num_segments, thickness);
	}

	inline void DrawCircleFilled(const ImVec2& center, float radius, const ImVec4& color, int num_segments = 0)
	{
		if (auto i = GetInterface())
			i->DrawCircleFilled(center, radius, color, num_segments);
	}

	inline void DrawScreenCircle(const ImVec2& center, float radius, ImU32 color, int num_segments = 0, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawScreenCircle(center, radius, color, num_segments, thickness);
	}

	inline void DrawScreenCircleFilled(const ImVec2& center, float radius, ImU32 color, int num_segments = 0)
	{
		if (auto i = GetInterface())
			i->DrawScreenCircleFilled(center, radius, color, num_segments);
	}

	inline void DrawQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec4& color, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawQuad(p1, p2, p3, p4, color, thickness);
	}

	inline void DrawQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec4& color)
	{
		if (auto i = GetInterface())
			i->DrawQuadFilled(p1, p2, p3, p4, color);
	}

	inline void DrawScreenQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 color, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawScreenQuad(p1, p2, p3, p4, color, thickness);
	}

	inline void DrawScreenQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 color)
	{
		if (auto i = GetInterface())
			i->DrawScreenQuadFilled(p1, p2, p3, p4, color);
	}

	inline void DrawTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec4& color, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawTriangle(p1, p2, p3, color, thickness);
	}

	inline void DrawTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec4& color)
	{
		if (auto i = GetInterface())
			i->DrawTriangleFilled(p1, p2, p3, color);
	}

	inline void DrawScreenTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 color, float thickness = 1.0f)
	{
		if (auto i = GetInterface())
			i->DrawScreenTriangle(p1, p2, p3, color, thickness);
	}

	inline void DrawScreenTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 color)
	{
		if (auto i = GetInterface())
			i->DrawScreenTriangleFilled(p1, p2, p3, color);
	}

	// --------------------------------------------------
	// Version 4
	// --------------------------------------------------

	inline bool WorldToScreenLoc(const float worldPos[3], ImVec2& screenLocOut)
	{
		return GetInterface() ? GetInterface()->WorldToScreenLoc(worldPos, &screenLocOut.x, &screenLocOut.y) : false;
	}

	/// @brief Converts a 3D world coordinate to 2D screen coordinates.
	/// Returns false if the coordinate is behind the camera.
	template <typename NiPoint3Like>
		requires requires(const NiPoint3Like& a_p) { a_p.x; a_p.y; a_p.z; }
	inline bool WorldToScreenLoc(const NiPoint3Like& worldLoc, ImVec2& screenLocOut)
	{
		float pos[3] = { worldLoc.x, worldLoc.y, worldLoc.z };
		return WorldToScreenLoc(pos, screenLocOut);
	}

	inline void TableSetupScrollFreeze(int cols, int rows)
	{
		if (auto i = GetInterface())
			i->TableSetupScrollFreeze(cols, rows);
	}
	inline void TableSetColumnIndex(int column_n)
	{
		if (auto i = GetInterface())
			i->TableSetColumnIndex(column_n);
	}
	inline int TableGetColumnIndex() { return GetInterface() ? GetInterface()->TableGetColumnIndex() : 0; }
	inline int TableGetRowIndex() { return GetInterface() ? GetInterface()->TableGetRowIndex() : 0; }
	inline int TableGetColumnCount() { return GetInterface() ? GetInterface()->TableGetColumnCount() : 0; }

	inline float GetScrollX() { return GetInterface() ? GetInterface()->GetScrollX() : 0.0f; }
	inline float GetScrollY() { return GetInterface() ? GetInterface()->GetScrollY() : 0.0f; }
	inline float GetScrollMaxX() { return GetInterface() ? GetInterface()->GetScrollMaxX() : 0.0f; }
	inline float GetScrollMaxY() { return GetInterface() ? GetInterface()->GetScrollMaxY() : 0.0f; }
	inline void  SetScrollX(float scroll_x)
	{
		if (auto i = GetInterface())
			i->SetScrollX(scroll_x);
	}
	inline void SetScrollY(float scroll_y)
	{
		if (auto i = GetInterface())
			i->SetScrollY(scroll_y);
	}

	inline bool SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "%.0f deg")
	{
		return GetInterface() ? GetInterface()->SliderAngle(label, v_rad, v_degrees_min, v_degrees_max, format) : false;
	}
	/// @brief Vertical slider, sized to just the slider.
	inline bool VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format = "%.3f")
	{
		return GetInterface() ? GetInterface()->VSliderFloat(label, size, v, v_min, v_max, format) : false;
	}
	/// @brief Vertical slider with optional buttons above and below. A button is drawn when its label is set.
	/// @note A button without a label keeps its space, so a row of these with the same slider size lines up,
	/// including one with no buttons. For a lone slider without that space, use VSliderFloat.
	inline bool VSliderButton(const char* str_id, const ImVec2& slider_size, float* v, float v_min, float v_max, const char* format = "%.3f", const char* top_label = nullptr, const char* bottom_label = nullptr, bool* out_top_pressed = nullptr, bool* out_bottom_pressed = nullptr)
	{
		return GetInterface() ? GetInterface()->VSliderButton(str_id, slider_size, v, v_min, v_max, format, top_label, bottom_label, out_top_pressed, out_bottom_pressed) : false;
	}

	// --------------------------------------------------
	// Version 5
	// --------------------------------------------------

	/// @brief Disables the gamepad "tweak fast" modifier (holding RB) on sliders and drags.
	/// While RB is held, ImGui normally multiplies every d-pad step by 10. Push this around
	/// widgets where your plugin uses RB for something else (e.g. a hold-to-interact modifier)
	/// so each d-pad press moves exactly one step.
	inline void PushGamepadTweakFastDisabled()
	{
		if (auto i = GetInterface())
			i->PushGamepadTweakFastDisabled();
	}

	/// @brief Pops a PushGamepadTweakFastDisabled(). The modifier is active again once every push has been popped.
	inline void PopGamepadTweakFastDisabled()
	{
		if (auto i = GetInterface())
			i->PopGamepadTweakFastDisabled();
	}

	/// @brief Disables the gamepad "tweak slow" modifier (holding LB) on sliders and drags.
	/// While LB is held, ImGui normally makes d-pad steps finer (1/10 on drags and on sliders
	/// with a decimal format). Push this around widgets where your plugin uses LB for something else.
	inline void PushGamepadTweakSlowDisabled()
	{
		if (auto i = GetInterface())
			i->PushGamepadTweakSlowDisabled();
	}

	/// @brief Pops a PushGamepadTweakSlowDisabled(). The modifier is active again once every push has been popped.
	inline void PopGamepadTweakSlowDisabled()
	{
		if (auto i = GetInterface())
			i->PopGamepadTweakSlowDisabled();
	}

	namespace detail
	{
		// The style before the outermost PushScale()
		struct ScaleBase
		{
			ImVec2 framePadding;
			ImVec2 itemSpacing;
			ImVec2 itemInnerSpacing;
			float  frameRounding;
			float  grabRounding;
		};

		inline std::vector<ScaleBase>& ScaleStack()
		{
			static thread_local std::vector<ScaleBase> s_stack;
			return s_stack;
		}
	}

	/// @brief Scales the font, padding, spacing and rounding until PopScale(). Nested calls replace the scale, they don't compound.
	/// @param font Defaults to the regular font.
	inline void PushScale(float scaleMultiplier, ImFont* font = nullptr)
	{
		auto& stack = detail::ScaleStack();

		const detail::ScaleBase base = stack.empty() ?
		                                   detail::ScaleBase{
											   GetStyleVarVec(ImGuiStyleVar_FramePadding),
											   GetStyleVarVec(ImGuiStyleVar_ItemSpacing),
											   GetStyleVarVec(ImGuiStyleVar_ItemInnerSpacing),
											   GetStyleVar(ImGuiStyleVar_FrameRounding),
											   GetStyleVar(ImGuiStyleVar_GrabRounding)
										   } :
		                                   stack.front();
		stack.push_back(base);

		PushFontScaled(font, scaleMultiplier);

		PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(base.framePadding.x * scaleMultiplier, base.framePadding.y * scaleMultiplier));
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(base.itemSpacing.x * scaleMultiplier, base.itemSpacing.y * scaleMultiplier));
		PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(base.itemInnerSpacing.x * scaleMultiplier, base.itemInnerSpacing.y * scaleMultiplier));
		PushStyleVar(ImGuiStyleVar_FrameRounding, base.frameRounding * scaleMultiplier);
		PushStyleVar(ImGuiStyleVar_GrabRounding, base.grabRounding * scaleMultiplier);
	}

	/// @brief Pops the styling pushed by PushScale.
	inline void PopScale()
	{
		PopStyleVar(5);
		PopFont();

		auto& stack = detail::ScaleStack();
		if (!stack.empty())
			stack.pop_back();
	}

	/// @brief A slider that displays values scaled by a multiplier (e.g., displaying 1.5 as "150").
	/// The underlying float value remains unchanged in memory.
	inline bool ScaledSliderFloat(const char* label, float* v, float min, float max, float visualScaleMultiplier, const char* fmt = "%.0f")
	{
		float display = *v * visualScaleMultiplier;
		if (SliderFloat(label, &display, min * visualScaleMultiplier, max * visualScaleMultiplier, fmt)) {
			*v = display / visualScaleMultiplier;
			return true;
		}
		return false;
	}

	/// @brief A vertical slider that displays values scaled by a multiplier.
	inline bool ScaledVSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, float visualScaleMultiplier, const char* format = "%.0f")
	{
		float display = *v * visualScaleMultiplier;
		if (VSliderFloat(label, size, &display, v_min * visualScaleMultiplier, v_max * visualScaleMultiplier, format)) {
			*v = display / visualScaleMultiplier;
			return true;
		}
		return false;
	}

	/// @brief VSliderButton that displays values scaled by a multiplier.
	inline bool ScaledVSliderButton(const char* str_id, const ImVec2& slider_size, float* v, float v_min, float v_max, float visualScaleMultiplier, const char* format = "%.0f", const char* top_label = nullptr, const char* bottom_label = nullptr, bool* out_top_pressed = nullptr, bool* out_bottom_pressed = nullptr)
	{
		float display = *v * visualScaleMultiplier;
		if (VSliderButton(str_id, slider_size, &display, v_min * visualScaleMultiplier, v_max * visualScaleMultiplier, format, top_label, bottom_label, out_top_pressed, out_bottom_pressed)) {
			*v = display / visualScaleMultiplier;
			return true;
		}
		return false;
	}

	// --------------------------------------------------
	// Version 6
	// --------------------------------------------------

	// Typed flag overloads of the existing ColorEdit functions
	inline bool ColorEdit3(const char* label, float col[3], ColorEditFlags flags) { return ColorEdit3(label, col, static_cast<int>(flags)); }
	inline bool ColorEdit4(const char* label, float col[4], ColorEditFlags flags) { return ColorEdit4(label, col, static_cast<int>(flags)); }

	/// @brief A clickable colour swatch. Zero size defaults to frame height.
	/// @param selected Draws the active border, e.g. for the current palette entry.
	inline bool ColorButton(const char* desc_id, const ImVec4& col, ColorEditFlags flags = ColorEditFlags::kNone, const ImVec2& size = ImVec2(0, 0), bool selected = false)
	{
		return GetInterface() ? GetInterface()->ColorButton(desc_id, col, static_cast<int>(flags), size, selected) : false;
	}

	/// @brief An inline colour picker (no popup).
	inline bool ColorPicker3(const char* label, float col[3], ColorEditFlags flags = ColorEditFlags::kNone)
	{
		return GetInterface() ? GetInterface()->ColorPicker3(label, col, static_cast<int>(flags)) : false;
	}

	/// @brief An inline colour picker (no popup).
	/// @note Alpha bar is always shown, unless kNoAlpha is passed.
	inline bool ColorPicker4(const char* label, float col[4], ColorEditFlags flags = ColorEditFlags::kNone, const float* ref_col = nullptr)
	{
		return GetInterface() ? GetInterface()->ColorPicker4(label, col, static_cast<int>(flags), ref_col) : false;
	}

	/// @brief Returns true on the frame the last item became active. Pair with IsItemDeactivatedAfterEdit() to capture values before an edit (e.g. for undo).
	inline bool IsItemActivated() { return GetInterface() ? GetInterface()->IsItemActivated() : false; }

	/// @brief Whether the hotkey that toggles the FUCK menu is enabled. See SetHotkeyEnabled.
	inline bool IsHotkeyEnabled() { return GetInterface() ? GetInterface()->IsHotkeyEnabled() : true; }

	/// @brief Scale of the current kScalable window, set by its corner handles. 1 otherwise.
	inline float GetWindowScale() { return GetInterface() ? GetInterface()->GetWindowScale() : 1.0f; }

	/// @brief Clockwise degrees of the current kRotatable window, set by its rotation handle. 0 otherwise.
	/// @note Only drawn content can turn, e.g. with DrawImageRotated. FUCK doesn't clip it to the window.
	inline float GetWindowRotation() { return GetInterface() ? GetInterface()->GetWindowRotation() : 0.0f; }

	/// @brief TextDisabled that wraps at the edge of the available space.
	inline void TextDisabledWrapped(const char* fmt, ...)
	{
		auto i = GetInterface();
		if (!i)
			return;
		va_list args;
		va_start(args, fmt);
		char buf[1024];
		vsnprintf(buf, 1024, fmt, args);
		va_end(args);
		i->TextColoredWrapped(GetStyleColorVec4(ImGuiCol_TextDisabled), buf);
	}

	/// @brief Joins an icon glyph and text into one label, for any widget that takes a label.
	/// e.g. FUCK::Button(FUCK::IconText(ICON_FA_GEAR, "$MyPlugin_Settings"_T))
	/// @note Use the result straight away. To keep it, copy it into a std::string.
	inline const char* IconText(const char* icon, const char* text)
	{
		static thread_local char buffers[8][256];
		static thread_local int  next = 0;

		char* buf = buffers[next];
		next      = (next + 1) % 8;
		snprintf(buf, sizeof(buffers[0]), "%s %s", icon ? icon : "", text ? text : "");
		return buf;
	}

	/// @brief Begins a scrolling list whose section headers stick to the top as you scroll.
	/// @note Always pair with EndStickyList().
	inline void BeginStickyList(const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = true)
	{
		if (auto i = GetInterface())
			i->BeginStickyList(str_id, size.x, size.y, border);
	}

	/// @brief A section header in a sticky list. Draw the section's rows when it returns true.
	/// @param open Optional. Your own bool for the open state, e.g. for an Expand All button.
	/// @param count Optional. Shown on the right of the header.
	inline bool StickyHeader(const char* label, bool* open = nullptr, int count = -1, int flags = 0)
	{
		return GetInterface() ? GetInterface()->StickyHeader(label, open, count, flags) : false;
	}

	/// @brief Ends a sticky list.
	inline void EndStickyList()
	{
		if (auto i = GetInterface())
			i->EndStickyList();
	}

	/// @brief Tells the sidebar your tools have changed what they return from Name(), Group() or ShowInSidebar().
	/// They're read again on the next frame. Call it once after a batch of changes, not per tool. Safe from any thread.
	/// @note The objects and strings the old values pointed to must stay alive until then.
	inline void RefreshTools()
	{
		if (auto i = GetInterface())
			i->RefreshTools();
	}

	/// @brief Whether the game has hidden its own HUD: the intro cart ride, cutscenes, dialogue and the like.
	/// @note What kHideWithHUD windows follow. Updated once a frame. FUCK hiding the HUD under its own menu doesn't count.
	inline bool IsGameHUDHidden() { return GetInterface() ? GetInterface()->IsGameHUDHidden() : false; }

	/// @brief Calls a_func(pluginName, windowId, title) for each window flagged kHideWithHUD, open or not.
	/// @note The strings are only valid during the call.
	template <class F>
	inline void EnumerateHUDWidgets(F a_func)
	{
		if (auto i = GetInterface()) {
			i->EnumerateHUDWidgets(&a_func, [](const char* a_plugin, const char* a_id, const char* a_title, void* a_ud) {
				(*static_cast<F*>(a_ud))(a_plugin, a_id, a_title);
			});
		}
	}

	/// @brief Fades a kHideWithHUD window by a_alpha (0-1). At 0 it's hidden and takes no input. Safe from any thread.
	/// @note Lasts until cleared. Ignored while the FUCK menu is open if the window is kEditable.
	/// Doesn't affect IsOpen(), DrawScreen* or RenderOverlay().
	inline void SetWindowAlphaOverride(const char* pluginName, const char* windowId, float alpha)
	{
		if (auto i = GetInterface())
			i->SetWindowAlphaOverride(pluginName, windowId, alpha);
	}

	/// @brief Removes a SetWindowAlphaOverride().
	inline void ClearWindowAlphaOverride(const char* pluginName, const char* windowId)
	{
		SetWindowAlphaOverride(pluginName, windowId, -1.0f);
	}

	// --------------------------------------------------
	// Version 7
	// --------------------------------------------------

	/// @brief A searchable combo for entries that each have a key bound to them, such as presets with shortcuts.
	/// Each row can show its key's icon at the right-hand end, and the caller is told which row is under the cursor, so a key pressed over a row can be bound to it.
	/// Unlike ComboWithFilter there are no favourites, so entries keep the order given unless the search box is in use.
	/// @param badgeKeys One key code per item (items_count long), keyboard, mouse or gamepad, as GetIconForKey() takes.
	/// 0 for an item with no key, or nullptr to draw no icons at all.
	/// @param hoveredItem Receives the index into items (not into the filtered list) of the row under the mouse or gamepad focus while the list is open, else -1.
	/// nullptr if not needed.
	/// @return True on the frame an item is picked, as with Combo().
	inline bool ComboWithKeyBadges(const char* label, int* current_item, const char* const* items, int items_count, const std::uint32_t* badgeKeys, int* hoveredItem, int popup_max_height_in_items = -1)
	{
		return GetInterface() ? GetInterface()->ComboWithKeyBadges(label, current_item, items, items_count, popup_max_height_in_items, badgeKeys, hoveredItem) : false;
	}

	/// @brief Holds an actor still: no animation, AI or pushing, with the eyes kept open and looking ahead. Works whether or not the game is paused.
	/// @note The actor stays frozen while any plugin holds it, so releasing only lets go of this plugin's hold. Every hold is let go before a save loads.
	inline void SetActorFrozen(RE::Actor* actor, bool frozen)
	{
		if (auto i = GetInterface())
			i->SetActorFrozen(FUCK::g_pluginName, actor, frozen);
	}

	/// @brief Whether any plugin is holding the actor frozen.
	inline bool IsActorFrozen(RE::Actor* actor)
	{
		auto i = GetInterface();
		return i ? i->IsActorFrozen(actor) : false;
	}

	/// @brief Loads an image at a scale, in the background or at once, optionally keeping a cached copy, e.g. a gallery's thumbnails.
	/// Hold it in FUCK::Image (see Image::Request), which releases it. Releasing one still waiting cancels its load.
	/// @return The handle, drawn by FUCK's image functions once ready and skipped until then. nullptr if the path is missing, or a load done at once failed.
	inline void* RequestImage(const ImageRequest& request)
	{
		auto i = GetInterface();
		return i ? i->RequestImage(&request) : nullptr;
	}

	/// @brief Where a requested image is up to. Images from LoadImage() are always kReady.
	inline ImageState GetImageState(void* image)
	{
		auto i = GetInterface();
		return i && image ? static_cast<ImageState>(i->GetImageState(image)) : ImageState::kNone;
	}

	/// @brief What FUCK is taking input for this frame, so a plugin's own hotkeys can stand aside, e.g. while typing in a FUCK text field.
	/// Test it with &, e.g. GetInputCaptureState() & InputCaptureState::kTextInput. Call it from the main thread, as from an input sink or frame hook.
	/// @return kNone before FUCK's renderer is ready.
	inline InputCaptureState GetInputCaptureState()
	{
		auto i = GetInterface();
		return i ? static_cast<InputCaptureState>(i->GetInputCaptureState()) : InputCaptureState::kNone;
	}
}

// ==================================================
// [ SECTION 5 ] GLOBAL LITERALS
// ==================================================

/// @brief String literal to automatically run FUCK::Translate
inline const char* operator""_T(const char* str, std::size_t)
{
	return FUCK::Translate(str);
}
