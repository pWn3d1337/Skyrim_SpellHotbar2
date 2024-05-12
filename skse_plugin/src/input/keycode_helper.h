#pragma once
#include "imgui.h"

namespace SpellHotbar::Input {

	constexpr std::array<ImGuiKey, 282> dx_to_imgui = {
		ImGuiKey_None, //0
		ImGuiKey_Escape, //1  Escape
		ImGuiKey_1, //2  1
		ImGuiKey_2, //3  2
		ImGuiKey_3, //4  3
		ImGuiKey_4, //5  4
		ImGuiKey_5, //6  5
		ImGuiKey_6, //7  6
		ImGuiKey_7, //8  7
		ImGuiKey_8, //9  8
		ImGuiKey_9, //10  9
		ImGuiKey_0, //11  0
		ImGuiKey_Minus, //12  Minus
		ImGuiKey_Equal, //13  Equals
		ImGuiKey_Backspace, //14  Backspace
		ImGuiKey_Tab, //15  Tab
		ImGuiKey_Q, //16  Q
		ImGuiKey_W, //17  W
		ImGuiKey_E, //18  E
		ImGuiKey_R, //19  R
		ImGuiKey_T, //20  T
		ImGuiKey_Y, //21  Y
		ImGuiKey_U, //22  U
		ImGuiKey_I, //23  I
		ImGuiKey_O, //24  O
		ImGuiKey_P, //25  P
		ImGuiKey_LeftBracket, //26  Left Bracket
		ImGuiKey_RightBracket, //27  Right Bracket
		ImGuiKey_Enter, //28  Enter
		ImGuiKey_LeftCtrl, //29  Left Control
		ImGuiKey_A, //30  A
		ImGuiKey_S, //31  S
		ImGuiKey_D, //32  D
		ImGuiKey_F, //33  F
		ImGuiKey_G, //34  G
		ImGuiKey_H, //35  H
		ImGuiKey_J, //36  J
		ImGuiKey_K, //37  K
		ImGuiKey_L, //38  L
		ImGuiKey_Semicolon, //39  Semicolon
		ImGuiKey_Apostrophe, //40  Apostrophe
		ImGuiKey_None, //41  ~ (Console)  <--- TODO not supported by imgui?
		ImGuiKey_LeftShift, //42  Left Shift
		ImGuiKey_Backslash, //43  Back Slash
		ImGuiKey_Z, //44  Z
		ImGuiKey_X, //45  X
		ImGuiKey_C, //46  C
		ImGuiKey_V, //47  V
		ImGuiKey_B, //48  B
		ImGuiKey_N, //49  N
		ImGuiKey_M, //50  M
		ImGuiKey_Comma, //51  Comma
		ImGuiKey_Period, //52  Period
		ImGuiKey_Slash, //53  Forward Slash  
		ImGuiKey_RightShift, //54  Right Shift
		ImGuiKey_KeypadMultiply, //55  NUM*
		ImGuiKey_LeftAlt, //56  Left Alt
		ImGuiKey_Space, //57  Spacebar
		ImGuiKey_CapsLock, //58  Caps Lock
		ImGuiKey_F1, //59  F1
		ImGuiKey_F2, //60  F2
		ImGuiKey_F3, //61  F3
		ImGuiKey_F4, //62  F4
		ImGuiKey_F5, //63  F5
		ImGuiKey_F6, //64  F6
		ImGuiKey_F7, //65  F7
		ImGuiKey_F8, //66  F8
		ImGuiKey_F9, //67  F9
		ImGuiKey_F10, //68  F10
		ImGuiKey_NumLock, //69  Num Lock
		ImGuiKey_ScrollLock, //70  Scroll Lock
		ImGuiKey_Keypad7, //71  NUM7
		ImGuiKey_Keypad8, //72  NUM8
		ImGuiKey_Keypad9, //73  NUM9
		ImGuiKey_KeypadSubtract, //74  NUM-
		ImGuiKey_Keypad4, //75  NUM4
		ImGuiKey_Keypad5, //76  NUM5
		ImGuiKey_Keypad6, //77  NUM6
		ImGuiKey_KeypadAdd, //78  NUM+
		ImGuiKey_Keypad1, //79  NUM1
		ImGuiKey_Keypad2, //80  NUM2
		ImGuiKey_Keypad3, //81  NUM3
		ImGuiKey_Keypad0, //82  NUM0
		ImGuiKey_KeypadDecimal, //83  NUM.
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_F11, //87  F11
		ImGuiKey_F12, //88  F12
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_KeypadEnter, //156  NUM Enter
		ImGuiKey_RightCtrl, //157  Right Control
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_KeypadDivide, //181  NUM/
		ImGuiKey_None, //
		ImGuiKey_PrintScreen, //183  SysRq / PtrScr
		ImGuiKey_RightAlt, //184  Right Alt
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_Pause, //197  Pause
		ImGuiKey_None, //
		ImGuiKey_Home, //199  Home
		ImGuiKey_UpArrow, //200  Up Arrow
		ImGuiKey_PageUp, //201  PgUp
		ImGuiKey_None, //
		ImGuiKey_LeftArrow, //203  Left Arrow
		ImGuiKey_None, //
		ImGuiKey_RightArrow, //205  Right Arrow
		ImGuiKey_None, //
		ImGuiKey_End, //207  End
		ImGuiKey_DownArrow, //208  Down Arrow
		ImGuiKey_PageDown, //209  PgDown
		ImGuiKey_Insert, //210  Insert
		ImGuiKey_Delete, //211  Delete
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_None, //
		ImGuiKey_MouseLeft, //256  Left Mouse Button
		ImGuiKey_MouseRight, //257  Right Mouse Button
		ImGuiKey_MouseMiddle, //258  Middle/Wheel Mouse Button
		ImGuiKey_None, //259  Mouse Button 3  //TODO? Not supported by imgui?
		ImGuiKey_None, //260  Mouse Button 4
		ImGuiKey_None, //261  Mouse Button 5
		ImGuiKey_None, //262  Mouse Button 6
		ImGuiKey_None, //263  Mouse Button 7
		ImGuiKey_None, //264  Mouse Wheel Up
		ImGuiKey_None, //265  Mouse Wheel Down
		ImGuiKey_GamepadDpadUp, //266  DPAD_UP
		ImGuiKey_GamepadDpadDown, //267  DPAD_DOWN
		ImGuiKey_GamepadDpadLeft, //268  DPAD_LEFT
		ImGuiKey_GamepadDpadRight, //269  DPAD_RIGHT
		ImGuiKey_GamepadStart, //270  START
		ImGuiKey_GamepadBack, //271  BACK
		ImGuiKey_GamepadL1, //272  LEFT_THUMB
		ImGuiKey_GamepadR1, //273  RIGHT_THUMB
		ImGuiKey_GamepadL1, //274  LEFT_SHOULDER
		ImGuiKey_GamepadR1, //275  RIGHT_SHOULDER
		ImGuiKey_GamepadFaceDown, //276  A
		ImGuiKey_GamepadFaceRight, //277  B
		ImGuiKey_GamepadFaceLeft, //278  X
		ImGuiKey_GamepadFaceUp, //279  Y
		ImGuiKey_GamepadL2, //280  LT
		ImGuiKey_GamepadR2 //281  RT
	};

}