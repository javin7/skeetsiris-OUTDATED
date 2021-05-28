// dear imgui, v1.77 WIP
// (widgets code)

/*

Index of this file:

// [SECTION] Forward Declarations
// [SECTION] Widgets: Text, etc.
// [SECTION] Widgets: Main (Button, Image, Checkbox, RadioButton, ProgressBar, Bullet, etc.)
// [SECTION] Widgets: Low-level Layout helpers (Spacing, Dummy, NewLine, Separator, etc.)
// [SECTION] Widgets: ComboBox
// [SECTION] Data Type and Data Formatting Helpers
// [SECTION] Widgets: DragScalar, DragFloat, DragInt, etc.
// [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
// [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
// [SECTION] Widgets: InputText, InputTextMultiline
// [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
// [SECTION] Widgets: TreeNode, CollapsingHeader, etc.
// [SECTION] Widgets: Selectable
// [SECTION] Widgets: ListBox
// [SECTION] Widgets: PlotLines, PlotHistogram
// [SECTION] Widgets: Value helpers
// [SECTION] Widgets: MenuItem, BeginMenu, EndMenu, etc.
// [SECTION] Widgets: BeginTabBar, EndTabBar, etc.
// [SECTION] Widgets: BeginTabItem, EndTabItem, etc.
// [SECTION] Widgets: Columns, BeginColumns, EndColumns, etc.

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <ctype.h>      // toupper
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#if defined(_MSC_VER) && _MSC_VER >= 1922 // MSVC 2019 16.2 or later
#pragma warning (disable: 5054)     // operator '|': deprecated between enumerations of different types
#endif
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"      // warning : format string is not a string literal              // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             //
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning : zero as null pointer constant              // some standard header variations use #define NULL 0
#endif
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"       // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#endif
#if __has_warning("-Wdeprecated-enum-enum-conversion")
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion" // warning: bitwise operation between different enumeration types ('XXXFlags_' and 'XXXFlagsPrivate_') is deprecated
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wformat-nonliteral"        // warning: format not a string literal, format string not checked
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif

//-------------------------------------------------------------------------
// Data
//-------------------------------------------------------------------------

// Those MIN/MAX values are not define because we need to point to them
static const signed char    IM_S8_MIN = -128;
static const signed char    IM_S8_MAX = 127;
static const unsigned char  IM_U8_MIN = 0;
static const unsigned char  IM_U8_MAX = 0xFF;
static const signed short   IM_S16_MIN = -32768;
static const signed short   IM_S16_MAX = 32767;
static const unsigned short IM_U16_MIN = 0;
static const unsigned short IM_U16_MAX = 0xFFFF;
static const ImS32          IM_S32_MIN = INT_MIN;    // (-2147483647 - 1), (0x80000000);
static const ImS32          IM_S32_MAX = INT_MAX;    // (2147483647), (0x7FFFFFFF)
static const ImU32          IM_U32_MIN = 0;
static const ImU32          IM_U32_MAX = UINT_MAX;   // (0xFFFFFFFF)
#ifdef LLONG_MIN
static const ImS64          IM_S64_MIN = LLONG_MIN;  // (-9223372036854775807ll - 1ll);
static const ImS64          IM_S64_MAX = LLONG_MAX;  // (9223372036854775807ll);
#else
static const ImS64          IM_S64_MIN = -9223372036854775807LL - 1;
static const ImS64          IM_S64_MAX = 9223372036854775807LL;
#endif
static const ImU64          IM_U64_MIN = 0;
#ifdef ULLONG_MAX
static const ImU64          IM_U64_MAX = ULLONG_MAX; // (0xFFFFFFFFFFFFFFFFull);
#else
static const ImU64          IM_U64_MAX = (2ULL * 9223372036854775807LL + 1);
#endif

//-------------------------------------------------------------------------
// [SECTION] Forward Declarations
//-------------------------------------------------------------------------

// For InputTextEx()
static bool             InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data);
static int              InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end);
static ImVec2           InputTextCalcTextSizeW(const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining = NULL, ImVec2* out_offset = NULL, bool stop_on_new_line = false);

//-------------------------------------------------------------------------
// [SECTION] Widgets: Text, etc.
//-------------------------------------------------------------------------
// - TextEx() [Internal]
// - TextUnformatted()
// - Text()
// - TextV()
// - TextColored()
// - TextColoredV()
// - TextDisabled()
// - TextDisabledV()
// - TextWrapped()
// - TextWrappedV()
// - LabelText()
// - LabelTextV()
// - BulletText()
// - BulletTextV()
//-------------------------------------------------------------------------

void ImGui::TextEx(const char* text, const char* text_end, ImGuiTextFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    IM_ASSERT(text != NULL);
    const char* text_begin = text;
    if (text_end == NULL)
        text_end = text + strlen(text); // FIXME-OPT

    const ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
    const float wrap_pos_x = window->DC.TextWrapPos;
    const bool wrap_enabled = (wrap_pos_x >= 0.0f);
    if (text_end - text > 2000 && !wrap_enabled)
    {
        // Long text!
        // Perform manual coarse clipping to optimize for long multi-line text
        // - From this point we will only compute the width of lines that are visible. Optimization only available when word-wrapping is disabled.
        // - We also don't vertically center the text within the line full height, which is unlikely to matter because we are likely the biggest and only item on the line.
        // - We use memchr(), pay attention that well optimized versions of those str/mem functions are much faster than a casually written loop.
        const char* line = text;
        const float line_height = GetTextLineHeight();
        ImVec2 text_size(0, 0);

        // Lines to skip (can't skip when logging text)
        ImVec2 pos = text_pos;
        if (!g.LogEnabled)
        {
            int lines_skippable = (int)((window->ClipRect.Min.y - text_pos.y) / line_height);
            if (lines_skippable > 0)
            {
                int lines_skipped = 0;
                while (line < text_end && lines_skipped < lines_skippable)
                {
                    const char* line_end = (const char*)memchr(line, '\n', text_end - line);
                    if (!line_end)
                        line_end = text_end;
                    if ((flags & ImGuiTextFlags_NoWidthForLargeClippedText) == 0)
                        text_size.x = ImMax(text_size.x, CalcTextSize(line, line_end).x);
                    line = line_end + 1;
                    lines_skipped++;
                }
                pos.y += lines_skipped * line_height;
            }
        }

        // Lines to render
        if (line < text_end)
        {
            ImRect line_rect(pos, pos + ImVec2(FLT_MAX, line_height));
            while (line < text_end)
            {
                if (IsClippedEx(line_rect, 0, false))
                    break;

                const char* line_end = (const char*)memchr(line, '\n', text_end - line);
                if (!line_end)
                    line_end = text_end;
                text_size.x = ImMax(text_size.x, CalcTextSize(line, line_end).x);
                RenderText(pos, line, line_end, false);
                line = line_end + 1;
                line_rect.Min.y += line_height;
                line_rect.Max.y += line_height;
                pos.y += line_height;
            }

            // Count remaining lines
            int lines_skipped = 0;
            while (line < text_end)
            {
                const char* line_end = (const char*)memchr(line, '\n', text_end - line);
                if (!line_end)
                    line_end = text_end;
                if ((flags & ImGuiTextFlags_NoWidthForLargeClippedText) == 0)
                    text_size.x = ImMax(text_size.x, CalcTextSize(line, line_end).x);
                line = line_end + 1;
                lines_skipped++;
            }
            pos.y += lines_skipped * line_height;
        }
        text_size.y = (pos - text_pos).y;

        ImRect bb(text_pos, text_pos + text_size);
        ItemSize(text_size, 0.0f);
        ItemAdd(bb, 0);
    } else
    {
        const float wrap_width = wrap_enabled ? CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
        const ImVec2 text_size = CalcTextSize(text_begin, text_end, false, wrap_width);

        ImRect bb(text_pos, text_pos + text_size);
        ItemSize(text_size, 0.0f);
        if (!ItemAdd(bb, 0))
            return;

        // Render (we don't hide text after ## in this end-user function)
        RenderTextWrapped(bb.Min, text_begin, text_end, wrap_width);
    }
}

void ImGui::TextUnformatted(const char* text, const char* text_end)
{
    TextEx(text, text_end, ImGuiTextFlags_NoWidthForLargeClippedText);
}

void ImGui::Text(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextV(fmt, args);
    va_end(args);
}

void ImGui::TextV(const char* fmt, va_list args)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const char* text_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
    TextEx(g.TempBuffer, text_end, ImGuiTextFlags_NoWidthForLargeClippedText);
}

void ImGui::TextColored(const ImVec4& col, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextColoredV(col, fmt, args);
    va_end(args);
}

void ImGui::TextColoredV(const ImVec4& col, const char* fmt, va_list args)
{
    PushStyleColor(ImGuiCol_Text, col);
    TextV(fmt, args);
    PopStyleColor();
}

void ImGui::TextDisabled(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextDisabledV(fmt, args);
    va_end(args);
}

void ImGui::TextDisabledV(const char* fmt, va_list args)
{
    PushStyleColor(ImGuiCol_Text, GImGui->Style.Colors[ImGuiCol_TextDisabled]);
    TextV(fmt, args);
    PopStyleColor();
}

void ImGui::TextWrapped(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextWrappedV(fmt, args);
    va_end(args);
}

void ImGui::TextWrappedV(const char* fmt, va_list args)
{
    ImGuiWindow* window = GetCurrentWindow();
    bool need_backup = (window->DC.TextWrapPos < 0.0f);  // Keep existing wrap position if one is already set
    if (need_backup)
        PushTextWrapPos(0.0f);
    TextV(fmt, args);
    if (need_backup)
        PopTextWrapPos();
}

void ImGui::LabelText(const char* label, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LabelTextV(label, fmt, args);
    va_end(args);
}

// Add a label+text combo aligned to other label+value widgets
void ImGui::LabelTextV(const char* label, const char* fmt, va_list args)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect value_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2));
    const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w + (label_size.x > 0.0f ? style.ItemInnerSpacing.x : 0.0f), style.FramePadding.y * 2) + label_size);
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, 0))
        return;

    // Render
    const char* value_text_begin = &g.TempBuffer[0];
    const char* value_text_end = value_text_begin + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
    RenderTextClipped(value_bb.Min, value_bb.Max, value_text_begin, value_text_end, NULL, ImVec2(0.0f, 0.5f));
    if (label_size.x > 0.0f)
        RenderText(ImVec2(value_bb.Max.x + style.ItemInnerSpacing.x, value_bb.Min.y + style.FramePadding.y), label);
}

void ImGui::BulletText(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    BulletTextV(fmt, args);
    va_end(args);
}

// Text with a little bullet aligned to the typical tree node.
void ImGui::BulletTextV(const char* fmt, va_list args)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const char* text_begin = g.TempBuffer;
    const char* text_end = text_begin + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
    const ImVec2 label_size = CalcTextSize(text_begin, text_end, false);
    const ImVec2 total_size = ImVec2(g.FontSize + (label_size.x > 0.0f ? (label_size.x + style.FramePadding.x * 2) : 0.0f), label_size.y);  // Empty text doesn't add padding
    ImVec2 pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    ItemSize(total_size, 0.0f);
    const ImRect bb(pos, pos + total_size);
    if (!ItemAdd(bb, 0))
        return;

    // Render
    ImU32 text_col = GetColorU32(ImGuiCol_Text);
    RenderBullet(window->DrawList, bb.Min + ImVec2(style.FramePadding.x + g.FontSize * 0.5f, g.FontSize * 0.5f), text_col);
    RenderText(bb.Min + ImVec2(g.FontSize + style.FramePadding.x * 2, 0.0f), text_begin, text_end, false);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Main
//-------------------------------------------------------------------------
// - ButtonBehavior() [Internal]
// - Button()
// - SmallButton()
// - InvisibleButton()
// - ArrowButton()
// - CloseButton() [Internal]
// - CollapseButton() [Internal]
// - GetWindowScrollbarID() [Internal]
// - GetWindowScrollbarRect() [Internal]
// - Scrollbar() [Internal]
// - ScrollbarEx() [Internal]
// - Image()
// - ImageButton()
// - Checkbox()
// - CheckboxFlags()
// - RadioButton()
// - ProgressBar()
// - Bullet()
//-------------------------------------------------------------------------

// The ButtonBehavior() function is key to many interactions and used by many/most widgets.
// Because we handle so many cases (keyboard/gamepad navigation, drag and drop) and many specific behavior (via ImGuiButtonFlags_),
// this code is a little complex.
// By far the most common path is interacting with the Mouse using the default ImGuiButtonFlags_PressedOnClickRelease button behavior.
// See the series of events below and the corresponding state reported by dear imgui:
//------------------------------------------------------------------------------------------------------------------------------------------------
// with PressedOnClickRelease:             return-value  IsItemHovered()  IsItemActive()  IsItemActivated()  IsItemDeactivated()  IsItemClicked()
//   Frame N+0 (mouse is outside bb)        -             -                -               -                  -                    -
//   Frame N+1 (mouse moves inside bb)      -             true             -               -                  -                    -
//   Frame N+2 (mouse button is down)       -             true             true            true               -                    true
//   Frame N+3 (mouse button is down)       -             true             true            -                  -                    -
//   Frame N+4 (mouse moves outside bb)     -             -                true            -                  -                    -
//   Frame N+5 (mouse moves inside bb)      -             true             true            -                  -                    -
//   Frame N+6 (mouse button is released)   true          true             -               -                  true                 -
//   Frame N+7 (mouse button is released)   -             true             -               -                  -                    -
//   Frame N+8 (mouse moves outside bb)     -             -                -               -                  -                    -
//------------------------------------------------------------------------------------------------------------------------------------------------
// with PressedOnClick:                    return-value  IsItemHovered()  IsItemActive()  IsItemActivated()  IsItemDeactivated()  IsItemClicked()
//   Frame N+2 (mouse button is down)       true          true             true            true               -                    true
//   Frame N+3 (mouse button is down)       -             true             true            -                  -                    -
//   Frame N+6 (mouse button is released)   -             true             -               -                  true                 -
//   Frame N+7 (mouse button is released)   -             true             -               -                  -                    -
//------------------------------------------------------------------------------------------------------------------------------------------------
// with PressedOnRelease:                  return-value  IsItemHovered()  IsItemActive()  IsItemActivated()  IsItemDeactivated()  IsItemClicked()
//   Frame N+2 (mouse button is down)       -             true             -               -                  -                    true
//   Frame N+3 (mouse button is down)       -             true             -               -                  -                    -
//   Frame N+6 (mouse button is released)   true          true             -               -                  -                    -
//   Frame N+7 (mouse button is released)   -             true             -               -                  -                    -
//------------------------------------------------------------------------------------------------------------------------------------------------
// with PressedOnDoubleClick:              return-value  IsItemHovered()  IsItemActive()  IsItemActivated()  IsItemDeactivated()  IsItemClicked()
//   Frame N+0 (mouse button is down)       -             true             -               -                  -                    true
//   Frame N+1 (mouse button is down)       -             true             -               -                  -                    -
//   Frame N+2 (mouse button is released)   -             true             -               -                  -                    -
//   Frame N+3 (mouse button is released)   -             true             -               -                  -                    -
//   Frame N+4 (mouse button is down)       true          true             true            true               -                    true
//   Frame N+5 (mouse button is down)       -             true             true            -                  -                    -
//   Frame N+6 (mouse button is released)   -             true             -               -                  true                 -
//   Frame N+7 (mouse button is released)   -             true             -               -                  -                    -
//------------------------------------------------------------------------------------------------------------------------------------------------
// Note that some combinations are supported,
// - PressedOnDragDropHold can generally be associated with any flag.
// - PressedOnDoubleClick can be associated by PressedOnClickRelease/PressedOnRelease, in which case the second release event won't be reported.
//------------------------------------------------------------------------------------------------------------------------------------------------
// The behavior of the return-value changes when ImGuiButtonFlags_Repeat is set:
//                                         Repeat+                  Repeat+           Repeat+             Repeat+
//                                         PressedOnClickRelease    PressedOnClick    PressedOnRelease    PressedOnDoubleClick
//-------------------------------------------------------------------------------------------------------------------------------------------------
//   Frame N+0 (mouse button is down)       -                        true              -                   true
//   ...                                    -                        -                 -                   -
//   Frame N + RepeatDelay                  true                     true              -                   true
//   ...                                    -                        -                 -                   -
//   Frame N + RepeatDelay + RepeatRate*N   true                     true              -                   true
//-------------------------------------------------------------------------------------------------------------------------------------------------

bool ImGui::ButtonBehavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, ImGuiButtonFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    if (flags & ImGuiButtonFlags_Disabled)
    {
        if (out_hovered) *out_hovered = false;
        if (out_held) *out_held = false;
        if (g.ActiveId == id) ClearActiveID();
        return false;
    }

    // Default only reacts to left mouse button
    if ((flags & ImGuiButtonFlags_MouseButtonMask_) == 0)
        flags |= ImGuiButtonFlags_MouseButtonDefault_;

    // Default behavior requires click + release inside bounding box
    if ((flags & ImGuiButtonFlags_PressedOnMask_) == 0)
        flags |= ImGuiButtonFlags_PressedOnDefault_;

    ImGuiWindow* backup_hovered_window = g.HoveredWindow;
    const bool flatten_hovered_children = (flags & ImGuiButtonFlags_FlattenChildren) && g.HoveredRootWindow == window;
    if (flatten_hovered_children)
        g.HoveredWindow = window;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    if (id != 0 && window->DC.LastItemId != id)
        IMGUI_TEST_ENGINE_ITEM_ADD(bb, id);
#endif

    bool pressed = false;
    bool hovered = ItemHoverable(bb, id);

    // Drag source doesn't report as hovered
    if (hovered && g.DragDropActive && g.DragDropPayload.SourceId == id && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoDisableHover))
        hovered = false;

    // Special mode for Drag and Drop where holding button pressed for a long time while dragging another item triggers the button
    if (g.DragDropActive && (flags & ImGuiButtonFlags_PressedOnDragDropHold) && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoHoldToOpenOthers))
        if (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            const float DRAG_DROP_HOLD_TIMER = 0.70f;
            hovered = true;
            SetHoveredID(id);
            if (CalcTypematicRepeatAmount(g.HoveredIdTimer + 0.0001f - g.IO.DeltaTime, g.HoveredIdTimer + 0.0001f, DRAG_DROP_HOLD_TIMER, 0.00f))
            {
                pressed = true;
                g.DragDropHoldJustPressedId = id;
                FocusWindow(window);
            }
        }

    if (flatten_hovered_children)
        g.HoveredWindow = backup_hovered_window;

    // AllowOverlap mode (rarely used) requires previous frame HoveredId to be null or to match. This allows using patterns where a later submitted widget overlaps a previous one.
    if (hovered && (flags & ImGuiButtonFlags_AllowItemOverlap) && (g.HoveredIdPreviousFrame != id && g.HoveredIdPreviousFrame != 0))
        hovered = false;

    // Mouse handling
    if (hovered)
    {
        if (!(flags & ImGuiButtonFlags_NoKeyModifiers) || (!g.IO.KeyCtrl && !g.IO.KeyShift && !g.IO.KeyAlt))
        {
            // Poll buttons
            int mouse_button_clicked = -1;
            int mouse_button_released = -1;
            if ((flags & ImGuiButtonFlags_MouseButtonLeft) && g.IO.MouseClicked[0]) { mouse_button_clicked = 0; } else if ((flags & ImGuiButtonFlags_MouseButtonRight) && g.IO.MouseClicked[1]) { mouse_button_clicked = 1; } else if ((flags & ImGuiButtonFlags_MouseButtonMiddle) && g.IO.MouseClicked[2]) { mouse_button_clicked = 2; }
            if ((flags & ImGuiButtonFlags_MouseButtonLeft) && g.IO.MouseReleased[0]) { mouse_button_released = 0; } else if ((flags & ImGuiButtonFlags_MouseButtonRight) && g.IO.MouseReleased[1]) { mouse_button_released = 1; } else if ((flags & ImGuiButtonFlags_MouseButtonMiddle) && g.IO.MouseReleased[2]) { mouse_button_released = 2; }

            if (mouse_button_clicked != -1 && g.ActiveId != id)
            {
                if (flags & (ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnClickReleaseAnywhere))
                {
                    SetActiveID(id, window);
                    g.ActiveIdMouseButton = mouse_button_clicked;
                    if (!(flags & ImGuiButtonFlags_NoNavFocus))
                        SetFocusID(id, window);
                    FocusWindow(window);
                }
                if ((flags & ImGuiButtonFlags_PressedOnClick) || ((flags & ImGuiButtonFlags_PressedOnDoubleClick) && g.IO.MouseDoubleClicked[mouse_button_clicked]))
                {
                    pressed = true;
                    if (flags & ImGuiButtonFlags_NoHoldingActiveId)
                        ClearActiveID();
                    else
                        SetActiveID(id, window); // Hold on ID
                    g.ActiveIdMouseButton = mouse_button_clicked;
                    FocusWindow(window);
                }
            }
            if ((flags & ImGuiButtonFlags_PressedOnRelease) && mouse_button_released != -1)
            {
                // Repeat mode trumps on release behavior
                if (!((flags & ImGuiButtonFlags_Repeat) && g.IO.MouseDownDurationPrev[mouse_button_released] >= g.IO.KeyRepeatDelay))
                    pressed = true;
                ClearActiveID();
            }

            // 'Repeat' mode acts when held regardless of _PressedOn flags (see table above).
            // Relies on repeat logic of IsMouseClicked() but we may as well do it ourselves if we end up exposing finer RepeatDelay/RepeatRate settings.
            if (g.ActiveId == id && (flags & ImGuiButtonFlags_Repeat))
                if (g.IO.MouseDownDuration[g.ActiveIdMouseButton] > 0.0f && IsMouseClicked(g.ActiveIdMouseButton, true))
                    pressed = true;
        }

        if (pressed)
            g.NavDisableHighlight = true;
    }

    // Gamepad/Keyboard navigation
    // We report navigated item as hovered but we don't set g.HoveredId to not interfere with mouse.
    if (g.NavId == id && !g.NavDisableHighlight && g.NavDisableMouseHover && (g.ActiveId == 0 || g.ActiveId == id || g.ActiveId == window->MoveId))
        if (!(flags & ImGuiButtonFlags_NoHoveredOnFocus))
            hovered = true;
    if (g.NavActivateDownId == id)
    {
        bool nav_activated_by_code = (g.NavActivateId == id);
        bool nav_activated_by_inputs = IsNavInputTest(ImGuiNavInput_Activate, (flags & ImGuiButtonFlags_Repeat) ? ImGuiInputReadMode_Repeat : ImGuiInputReadMode_Pressed);
        if (nav_activated_by_code || nav_activated_by_inputs)
            pressed = true;
        if (nav_activated_by_code || nav_activated_by_inputs || g.ActiveId == id)
        {
            // Set active id so it can be queried by user via IsItemActive(), equivalent of holding the mouse button.
            g.NavActivateId = id; // This is so SetActiveId assign a Nav source
            SetActiveID(id, window);
            if ((nav_activated_by_code || nav_activated_by_inputs) && !(flags & ImGuiButtonFlags_NoNavFocus))
                SetFocusID(id, window);
        }
    }

    bool held = false;
    if (g.ActiveId == id)
    {
        if (g.ActiveIdSource == ImGuiInputSource_Mouse)
        {
            if (g.ActiveIdIsJustActivated)
                g.ActiveIdClickOffset = g.IO.MousePos - bb.Min;

            const int mouse_button = g.ActiveIdMouseButton;
            IM_ASSERT(mouse_button >= 0 && mouse_button < ImGuiMouseButton_COUNT);
            if (g.IO.MouseDown[mouse_button])
            {
                held = true;
            } else
            {
                bool release_in = hovered && (flags & ImGuiButtonFlags_PressedOnClickRelease) != 0;
                bool release_anywhere = (flags & ImGuiButtonFlags_PressedOnClickReleaseAnywhere) != 0;
                if ((release_in || release_anywhere) && !g.DragDropActive)
                {
                    bool is_double_click_release = (flags & ImGuiButtonFlags_PressedOnDoubleClick) && g.IO.MouseDownWasDoubleClick[mouse_button];
                    bool is_repeating_already = (flags & ImGuiButtonFlags_Repeat) && g.IO.MouseDownDurationPrev[mouse_button] >= g.IO.KeyRepeatDelay; // Repeat mode trumps <on release>
                    if (!is_double_click_release && !is_repeating_already)
                        pressed = true;
                }
                ClearActiveID();
            }
            if (!(flags & ImGuiButtonFlags_NoNavFocus))
                g.NavDisableHighlight = true;
        } else if (g.ActiveIdSource == ImGuiInputSource_Nav)
        {
            if (g.NavActivateDownId != id)
                ClearActiveID();
        }
        if (pressed)
            g.ActiveIdHasBeenPressedBefore = true;
    }

    if (out_hovered) *out_hovered = hovered;
    if (out_held) *out_held = held;

    return pressed;
}

bool ImGui::ButtonEx(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
        pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
        flags |= ImGuiButtonFlags_Repeat;
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

    // Automatically close popups
    //if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
    //    CloseCurrentPopup();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
    return pressed;
}

bool ImGui::Button(const char* label, const ImVec2& size_arg)
{
    return ButtonEx(label, size_arg, 0);
}

// Small buttons fits within text without additional vertical spacing.
bool ImGui::SmallButton(const char* label)
{
    ImGuiContext& g = *GImGui;
    float backup_padding_y = g.Style.FramePadding.y;
    g.Style.FramePadding.y = 0.0f;
    bool pressed = ButtonEx(label, ImVec2(0, 0), ImGuiButtonFlags_AlignTextBaseLine);
    g.Style.FramePadding.y = backup_padding_y;
    return pressed;
}

// Tip: use ImGui::PushID()/PopID() to push indices or pointers in the ID stack.
// Then you can keep 'str_id' empty or the same for all your buttons (instead of creating a string based on a non-string id)
bool ImGui::InvisibleButton(const char* str_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    // Cannot use zero-size for InvisibleButton(). Unlike Button() there is not way to fallback using the label size.
    IM_ASSERT(size_arg.x != 0.0f && size_arg.y != 0.0f);

    const ImGuiID id = window->GetID(str_id);
    ImVec2 size = CalcItemSize(size_arg, 0.0f, 0.0f);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    return pressed;
}

bool ImGui::ArrowButtonEx(const char* str_id, ImGuiDir dir, ImVec2 size, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(str_id);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const float default_size = GetFrameHeight();
    ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);
    if (!ItemAdd(bb, id))
        return false;

    if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
        flags |= ImGuiButtonFlags_Repeat;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    const ImU32 text_col = GetColorU32(ImGuiCol_Text);
    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);
    RenderArrow(window->DrawList, bb.Min + ImVec2(ImMax(0.0f, (size.x - g.FontSize) * 0.5f), ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, dir);

    return pressed;
}

bool ImGui::ArrowButton(const char* str_id, ImGuiDir dir)
{
    float sz = GetFrameHeight();
    return ArrowButtonEx(str_id, dir, ImVec2(sz, sz), ImGuiButtonFlags_None);
}

// Button to close a window
bool ImGui::CloseButton(ImGuiID id, const ImVec2& pos)//, float size)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // We intentionally allow interaction when clipped so that a mechanical Alt,Right,Validate sequence close a window.
    // (this isn't the regular behavior of buttons, but it doesn't affect the user much because navigation tends to keep items visible).
    const ImRect bb(pos, pos + ImVec2(g.FontSize, g.FontSize) + g.Style.FramePadding * 2.0f);
    bool is_clipped = !ItemAdd(bb, id);

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (is_clipped)
        return pressed;

    // Render
    ImU32 col = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
    ImVec2 center = bb.GetCenter();
    if (hovered)
        window->DrawList->AddCircleFilled(center, ImMax(2.0f, g.FontSize * 0.5f + 1.0f), col, 12);

    float cross_extent = g.FontSize * 0.5f * 0.7071f - 1.0f;
    ImU32 cross_col = GetColorU32(ImGuiCol_Text);
    center -= ImVec2(0.5f, 0.5f);
    window->DrawList->AddLine(center + ImVec2(+cross_extent, +cross_extent), center + ImVec2(-cross_extent, -cross_extent), cross_col, 1.0f);
    window->DrawList->AddLine(center + ImVec2(+cross_extent, -cross_extent), center + ImVec2(-cross_extent, +cross_extent), cross_col, 1.0f);

    return pressed;
}

bool ImGui::CollapseButton(ImGuiID id, const ImVec2& pos)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImRect bb(pos, pos + ImVec2(g.FontSize, g.FontSize) + g.Style.FramePadding * 2.0f);
    ItemAdd(bb, id);
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

    // Render
    ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImU32 text_col = GetColorU32(ImGuiCol_Text);
    ImVec2 center = bb.GetCenter();
    if (hovered || held)
        window->DrawList->AddCircleFilled(center/*+ ImVec2(0.0f, -0.5f)*/, g.FontSize * 0.5f + 1.0f, bg_col, 12);
    RenderArrow(window->DrawList, bb.Min + g.Style.FramePadding, text_col, window->Collapsed ? ImGuiDir_Right : ImGuiDir_Down, 1.0f);

    // Switch to moving the window after mouse is moved beyond the initial drag threshold
    if (IsItemActive() && IsMouseDragging(0))
        StartMouseMovingWindow(window);

    return pressed;
}

ImGuiID ImGui::GetWindowScrollbarID(ImGuiWindow* window, ImGuiAxis axis)
{
    return window->GetIDNoKeepAlive(axis == ImGuiAxis_X ? "#SCROLLX" : "#SCROLLY");
}

// Return scrollbar rectangle, must only be called for corresponding axis if window->ScrollbarX/Y is set.
ImRect ImGui::GetWindowScrollbarRect(ImGuiWindow* window, ImGuiAxis axis)
{
    const ImRect outer_rect = window->Rect();
    const ImRect inner_rect = window->InnerRect;
    const float border_size = window->WindowBorderSize;
    const float scrollbar_size = window->ScrollbarSizes[axis ^ 1]; // (ScrollbarSizes.x = width of Y scrollbar; ScrollbarSizes.y = height of X scrollbar)
    IM_ASSERT(scrollbar_size > 0.0f);
    if (axis == ImGuiAxis_X)
        return ImRect(inner_rect.Min.x, ImMax(outer_rect.Min.y, outer_rect.Max.y - border_size - scrollbar_size), inner_rect.Max.x, outer_rect.Max.y);
    else
        return ImRect(ImMax(outer_rect.Min.x, outer_rect.Max.x - border_size - scrollbar_size), inner_rect.Min.y, outer_rect.Max.x, inner_rect.Max.y);
}

void ImGui::Scrollbar(ImGuiAxis axis)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    const ImGuiID id = GetWindowScrollbarID(window, axis);
    KeepAliveID(id);

    // Calculate scrollbar bounding box
    ImRect bb = GetWindowScrollbarRect(window, axis);
    ImDrawCornerFlags rounding_corners = 0;
    if (axis == ImGuiAxis_X)
    {
        rounding_corners |= ImDrawCornerFlags_BotLeft;
        if (!window->ScrollbarY)
            rounding_corners |= ImDrawCornerFlags_BotRight;
    } else
    {
        if ((window->Flags & ImGuiWindowFlags_NoTitleBar) && !(window->Flags & ImGuiWindowFlags_MenuBar))
            rounding_corners |= ImDrawCornerFlags_TopRight;
        if (!window->ScrollbarX)
            rounding_corners |= ImDrawCornerFlags_BotRight;
    }
    float size_avail = window->InnerRect.Max[axis] - window->InnerRect.Min[axis];
    float size_contents = window->ContentSize[axis] + window->WindowPadding[axis] * 2.0f;
    ScrollbarEx(bb, id, axis, &window->Scroll[axis], size_avail, size_contents, rounding_corners);
}

// Vertical/Horizontal scrollbar
// The entire piece of code below is rather confusing because:
// - We handle absolute seeking (when first clicking outside the grab) and relative manipulation (afterward or when clicking inside the grab)
// - We store values as normalized ratio and in a form that allows the window content to change while we are holding on a scrollbar
// - We handle both horizontal and vertical scrollbars, which makes the terminology not ideal.
// Still, the code should probably be made simpler..
bool ImGui::ScrollbarEx(const ImRect& bb_frame, ImGuiID id, ImGuiAxis axis, float* p_scroll_v, float size_avail_v, float size_contents_v, ImDrawCornerFlags rounding_corners)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    const float bb_frame_width = bb_frame.GetWidth();
    const float bb_frame_height = bb_frame.GetHeight();
    if (bb_frame_width <= 0.0f || bb_frame_height <= 0.0f)
        return false;

    // When we are too small, start hiding and disabling the grab (this reduce visual noise on very small window and facilitate using the window resize grab)
    float alpha = 1.0f;
    if ((axis == ImGuiAxis_Y) && bb_frame_height < g.FontSize + g.Style.FramePadding.y * 2.0f)
        alpha = ImSaturate((bb_frame_height - g.FontSize) / (g.Style.FramePadding.y * 2.0f));
    if (alpha <= 0.0f)
        return false;

    const ImGuiStyle& style = g.Style;
    const bool allow_interaction = (alpha >= 1.0f);

    ImRect bb = bb_frame;
    bb.Expand(ImVec2(-ImClamp(IM_FLOOR((bb_frame_width - 2.0f) * 0.5f), 0.0f, 3.0f), -ImClamp(IM_FLOOR((bb_frame_height - 2.0f) * 0.5f), 0.0f, 3.0f)));

    // V denote the main, longer axis of the scrollbar (= height for a vertical scrollbar)
    const float scrollbar_size_v = (axis == ImGuiAxis_X) ? bb.GetWidth() : bb.GetHeight();

    // Calculate the height of our grabbable box. It generally represent the amount visible (vs the total scrollable amount)
    // But we maintain a minimum size in pixel to allow for the user to still aim inside.
    IM_ASSERT(ImMax(size_contents_v, size_avail_v) > 0.0f); // Adding this assert to check if the ImMax(XXX,1.0f) is still needed. PLEASE CONTACT ME if this triggers.
    const float win_size_v = ImMax(ImMax(size_contents_v, size_avail_v), 1.0f);
    const float grab_h_pixels = ImClamp(scrollbar_size_v * (size_avail_v / win_size_v), style.GrabMinSize, scrollbar_size_v);
    const float grab_h_norm = grab_h_pixels / scrollbar_size_v;

    // Handle input right away. None of the code of Begin() is relying on scrolling position before calling Scrollbar().
    bool held = false;
    bool hovered = false;
    ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);

    float scroll_max = ImMax(1.0f, size_contents_v - size_avail_v);
    float scroll_ratio = ImSaturate(*p_scroll_v / scroll_max);
    float grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v; // Grab position in normalized space
    if (held && allow_interaction && grab_h_norm < 1.0f)
    {
        float scrollbar_pos_v = bb.Min[axis];
        float mouse_pos_v = g.IO.MousePos[axis];

        // Click position in scrollbar normalized space (0.0f->1.0f)
        const float clicked_v_norm = ImSaturate((mouse_pos_v - scrollbar_pos_v) / scrollbar_size_v);
        SetHoveredID(id);

        bool seek_absolute = false;
        if (g.ActiveIdIsJustActivated)
        {
            // On initial click calculate the distance between mouse and the center of the grab
            seek_absolute = (clicked_v_norm < grab_v_norm || clicked_v_norm > grab_v_norm + grab_h_norm);
            if (seek_absolute)
                g.ScrollbarClickDeltaToGrabCenter = 0.0f;
            else
                g.ScrollbarClickDeltaToGrabCenter = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
        }

        // Apply scroll (p_scroll_v will generally point on one member of window->Scroll)
        // It is ok to modify Scroll here because we are being called in Begin() after the calculation of ContentSize and before setting up our starting position
        const float scroll_v_norm = ImSaturate((clicked_v_norm - g.ScrollbarClickDeltaToGrabCenter - grab_h_norm * 0.5f) / (1.0f - grab_h_norm));
        *p_scroll_v = IM_ROUND(scroll_v_norm * scroll_max);//(win_size_contents_v - win_size_v));

        // Update values for rendering
        scroll_ratio = ImSaturate(*p_scroll_v / scroll_max);
        grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;

        // Update distance to grab now that we have seeked and saturated
        if (seek_absolute)
            g.ScrollbarClickDeltaToGrabCenter = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
    }

    // Render
    const ImU32 bg_col = GetColorU32(ImGuiCol_ScrollbarBg);
    const ImU32 grab_col = GetColorU32(held ? ImGuiCol_ScrollbarGrabActive : hovered ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab, alpha);
    window->DrawList->AddRectFilled(bb_frame.Min, bb_frame.Max, bg_col, window->WindowRounding, rounding_corners);
    ImRect grab_rect;
    if (axis == ImGuiAxis_X)
        grab_rect = ImRect(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm), bb.Min.y, ImLerp(bb.Min.x, bb.Max.x, grab_v_norm) + grab_h_pixels, bb.Max.y);
    else
        grab_rect = ImRect(bb.Min.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm), bb.Max.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm) + grab_h_pixels);
    window->DrawList->AddRectFilled(grab_rect.Min, grab_rect.Max, grab_col, style.ScrollbarRounding);

    return held;
}

void ImGui::Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    if (border_col.w > 0.0f)
        bb.Max += ImVec2(2, 2);
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
        return;

    if (border_col.w > 0.0f)
    {
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
        window->DrawList->AddImage(user_texture_id, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, GetColorU32(tint_col));
    } else
    {
        window->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, uv0, uv1, GetColorU32(tint_col));
    }
}

// frame_padding < 0: uses FramePadding from style (default)
// frame_padding = 0: no framing
// frame_padding > 0: set framing size
// The color used are the button colors.
bool ImGui::ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // Default to using texture ID as ID. User can still push string/integer prefixes.
    // We could hash the size/uv to create a unique ID but that would prevent the user from animating UV.
    PushID((void*)(intptr_t)user_texture_id);
    const ImGuiID id = window->GetID("#image");
    PopID();

    const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
    const ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);
    ItemSize(bb);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    // Render
    const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, style.FrameRounding));
    if (bg_col.w > 0.0f)
        window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
    window->DrawList->AddImage(user_texture_id, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(tint_col));

    return pressed;
}

bool ImGui::Checkbox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float square_sz = GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
    {
        *v = !(*v);
        MarkItemEdited(id);
    }

    const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
    RenderNavHighlight(total_bb, id);
    RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
    ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
    if (*v)
    {
        const float check_sz = ImMin(check_bb.GetWidth(), check_bb.GetHeight());
        const float pad = ImMax(1.0f, (float)(int)(check_sz / 6.0f));
        window->DrawList->AddRectFilled(check_bb.Min + ImVec2(pad, pad), check_bb.Max - ImVec2(pad, pad), GetColorU32(ImGuiCol_CheckMark), style.FrameRounding);
    }

    if (g.LogEnabled)
        LogRenderedText(&total_bb.Min, *v ? "[x]" : "[ ]");
    if (label_size.x > 0.0f)
        RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
    return pressed;
}


bool ImGui::IphoneCheckbox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = ImGuiStyle::ImGuiStyle();
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 pading = ImVec2(2, 2);
    const ImRect check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(label_size.y + style.FramePadding.x * 6, label_size.y + style.FramePadding.y / 2));
    ItemSize(check_bb, style.FramePadding.y);
    ImRect total_bb = check_bb;
    if (label_size.x > 0)
        SameLine(0, style.ItemInnerSpacing.x);
    const ImRect text_bb(window->DC.CursorPos + ImVec2(0, style.FramePadding.y), window->DC.CursorPos + ImVec2(0, style.FramePadding.y) + label_size);
    if (label_size.x > 0)
    {
        ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
        total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
    }
    if (!ItemAdd(total_bb, id))
        return false;
    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
        *v = !(*v);
    const float check_sz = ImMin(check_bb.GetWidth(), check_bb.GetHeight());
    const float check_sz2 = check_sz / 2;
    const float pad = ImMax(1.0f, (float)(int)(check_sz / 4.f));
    //window->DrawList->AddRectFilled(check_bb.Min+ImVec2(pad,pad), check_bb.Max-ImVec2(pad,pad), GetColorU32(ImGuiCol_CheckMark), style.FrameRounding);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 6, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 5, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 4, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 3, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 2, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 1, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 1, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 2, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 3, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 4, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 5, check_bb.Min.y + 9), 7, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 12);
    window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 6, check_bb.Min.y + 9), 7, GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 12);
    if (*v)
    {
        //window->DrawList->AddRectFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2, check_bb.Min.y), check_bb.Max, GetColorU32(ImVec4(0.34f, 1.0f, 0.54f, 1.0f)), 0);
        //window->DrawList->AddRectFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2, check_bb.Min.y), check_bb.Max, GetColorU32(ImVec4(0.34f, 1.0f, 0.54f, 1.0f)), 0);

       // Ikfakof, [13.10.17 20:37]
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 6, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 5, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 4, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 3, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 2, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 1, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 1, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 2, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 3, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 4, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 5, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 6, check_bb.Min.y + 9), 7, GetColorU32(ImGuiCol_CheckMark), 12);
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 + 6, check_bb.Min.y + 9), 7, GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 12);
    }
    else
    {
        window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + (check_bb.Max.x - check_bb.Min.x) / 2 - 6, check_bb.Min.y + 9), 7, GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 12);
    }
    if (label_size.x > 0.0f)
        RenderText(text_bb.GetTL(), label);
    return pressed;
}


bool ImGui::CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value)
{
    bool v = ((*flags & flags_value) == flags_value);
    bool pressed = Checkbox(label, &v);
    if (pressed)
    {
        if (v)
            *flags |= flags_value;
        else
            *flags &= ~flags_value;
    }

    return pressed;
}

bool ImGui::RadioButton(const char* label, bool active)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float square_sz = GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
    const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
        return false;

    ImVec2 center = check_bb.GetCenter();
    center.x = IM_ROUND(center.x);
    center.y = IM_ROUND(center.y);
    const float radius = (square_sz - 1.0f) * 0.5f;

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
        MarkItemEdited(id);

    RenderNavHighlight(total_bb, id);
    window->DrawList->AddCircleFilled(center, radius, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);
    if (active)
    {
        const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
        window->DrawList->AddCircleFilled(center, radius - pad, GetColorU32(ImGuiCol_CheckMark), 16);
    }

    if (style.FrameBorderSize > 0.0f)
    {
        window->DrawList->AddCircle(center + ImVec2(1, 1), radius, GetColorU32(ImGuiCol_BorderShadow), 16, style.FrameBorderSize);
        window->DrawList->AddCircle(center, radius, GetColorU32(ImGuiCol_Border), 16, style.FrameBorderSize);
    }

    if (g.LogEnabled)
        LogRenderedText(&total_bb.Min, active ? "(x)" : "( )");
    if (label_size.x > 0.0f)
        RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
    return pressed;
}

// FIXME: This would work nicely if it was a public template, e.g. 'template<T> RadioButton(const char* label, T* v, T v_button)', but I'm not sure how we would expose it..
bool ImGui::RadioButton(const char* label, int* v, int v_button)
{
    const bool pressed = RadioButton(label, *v == v_button);
    if (pressed)
        *v = v_button;
    return pressed;
}

// size_arg (for each axis) < 0.0f: align to end, 0.0f: auto, > 0.0f: specified size
void ImGui::ProgressBar(float fraction, const ImVec2& size_arg, const char* overlay)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
    ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, 0))
        return;

    // Render
    fraction = ImSaturate(fraction);
    RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
    const ImVec2 fill_br = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y);
    RenderRectFilledRangeH(window->DrawList, bb, GetColorU32(ImGuiCol_PlotHistogram), 0.0f, fraction, style.FrameRounding);

    // Default displaying the fraction as percentage string, but user can override it
    char overlay_buf[32];
    if (!overlay)
    {
        ImFormatString(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.0f%%", fraction * 100 + 0.01f);
        overlay = overlay_buf;
    }

    ImVec2 overlay_size = CalcTextSize(overlay, NULL);
    if (overlay_size.x > 0.0f)
        RenderTextClipped(ImVec2(ImClamp(fill_br.x + style.ItemSpacing.x, bb.Min.x, bb.Max.x - overlay_size.x - style.ItemInnerSpacing.x), bb.Min.y), bb.Max, overlay, NULL, &overlay_size, ImVec2(0.0f, 0.5f), &bb);
}

void ImGui::Bullet()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float line_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y * 2), g.FontSize);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize, line_height));
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
    {
        SameLine(0, style.FramePadding.x * 2);
        return;
    }

    // Render and stay on same line
    ImU32 text_col = GetColorU32(ImGuiCol_Text);
    RenderBullet(window->DrawList, bb.Min + ImVec2(style.FramePadding.x + g.FontSize * 0.5f, line_height * 0.5f), text_col);
    SameLine(0, style.FramePadding.x * 2.0f);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Low-level Layout helpers
//-------------------------------------------------------------------------
// - Spacing()
// - Dummy()
// - NewLine()
// - AlignTextToFramePadding()
// - SeparatorEx() [Internal]
// - Separator()
// - SplitterBehavior() [Internal]
// - ShrinkWidths() [Internal]
//-------------------------------------------------------------------------

void ImGui::Spacing()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    ItemSize(ImVec2(0, 0));
}

void ImGui::Dummy(const ImVec2& size)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(size);
    ItemAdd(bb, 0);
}

void ImGui::NewLine()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiLayoutType backup_layout_type = window->DC.LayoutType;
    window->DC.LayoutType = ImGuiLayoutType_Vertical;
    if (window->DC.CurrLineSize.y > 0.0f)     // In the event that we are on a line with items that is smaller that FontSize high, we will preserve its height.
        ItemSize(ImVec2(0, 0));
    else
        ItemSize(ImVec2(0.0f, g.FontSize));
    window->DC.LayoutType = backup_layout_type;
}

void ImGui::AlignTextToFramePadding()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    window->DC.CurrLineSize.y = ImMax(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y * 2);
    window->DC.CurrLineTextBaseOffset = ImMax(window->DC.CurrLineTextBaseOffset, g.Style.FramePadding.y);
}

// Horizontal/vertical separating line
void ImGui::SeparatorEx(ImGuiSeparatorFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    IM_ASSERT(ImIsPowerOfTwo(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)));   // Check that only 1 option is selected

    float thickness_draw = 1.0f;
    float thickness_layout = 0.0f;
    if (flags & ImGuiSeparatorFlags_Vertical)
    {
        // Vertical separator, for menu bars (use current line height). Not exposed because it is misleading and it doesn't have an effect on regular layout.
        float y1 = window->DC.CursorPos.y;
        float y2 = window->DC.CursorPos.y + window->DC.CurrLineSize.y;
        const ImRect bb(ImVec2(window->DC.CursorPos.x, y1), ImVec2(window->DC.CursorPos.x + thickness_draw, y2));
        ItemSize(ImVec2(thickness_layout, 0.0f));
        if (!ItemAdd(bb, 0))
            return;

        // Draw
        window->DrawList->AddLine(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x, bb.Max.y), GetColorU32(ImGuiCol_Separator));
        if (g.LogEnabled)
            LogText(" |");
    } else if (flags & ImGuiSeparatorFlags_Horizontal)
    {
        // Horizontal Separator
        float x1 = window->Pos.x;
        float x2 = window->Pos.x + window->Size.x;
        if (!window->DC.GroupStack.empty())
            x1 += window->DC.Indent.x;

        ImGuiColumns* columns = (flags & ImGuiSeparatorFlags_SpanAllColumns) ? window->DC.CurrentColumns : NULL;
        if (columns)
            PushColumnsBackground();

        // We don't provide our width to the layout so that it doesn't get feed back into AutoFit
        const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
        ItemSize(ImVec2(0.0f, thickness_layout));
        const bool item_visible = ItemAdd(bb, 0);
        if (item_visible)
        {
            // Draw
            window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(ImGuiCol_Separator));
            if (g.LogEnabled)
                LogRenderedText(&bb.Min, "--------------------------------");
        }
        if (columns)
        {
            PopColumnsBackground();
            columns->LineMinY = window->DC.CursorPos.y;
        }
    }
}

void ImGui::Separator()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    // Those flags should eventually be overridable by the user
    ImGuiSeparatorFlags flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
    flags |= ImGuiSeparatorFlags_SpanAllColumns;
    SeparatorEx(flags);
}

// Using 'hover_visibility_delay' allows us to hide the highlight and mouse cursor for a short time, which can be convenient to reduce visual noise.
bool ImGui::SplitterBehavior(const ImRect& bb, ImGuiID id, ImGuiAxis axis, float* size1, float* size2, float min_size1, float min_size2, float hover_extend, float hover_visibility_delay)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
    window->DC.ItemFlags |= ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus;
    bool item_add = ItemAdd(bb, id);
    window->DC.ItemFlags = item_flags_backup;
    if (!item_add)
        return false;

    bool hovered, held;
    ImRect bb_interact = bb;
    bb_interact.Expand(axis == ImGuiAxis_Y ? ImVec2(0.0f, hover_extend) : ImVec2(hover_extend, 0.0f));
    ButtonBehavior(bb_interact, id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_AllowItemOverlap);
    if (g.ActiveId != id)
        SetItemAllowOverlap();

    if (held || (g.HoveredId == id && g.HoveredIdPreviousFrame == id && g.HoveredIdTimer >= hover_visibility_delay))
        SetMouseCursor(axis == ImGuiAxis_Y ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);

    ImRect bb_render = bb;
    if (held)
    {
        ImVec2 mouse_delta_2d = g.IO.MousePos - g.ActiveIdClickOffset - bb_interact.Min;
        float mouse_delta = (axis == ImGuiAxis_Y) ? mouse_delta_2d.y : mouse_delta_2d.x;

        // Minimum pane size
        float size_1_maximum_delta = ImMax(0.0f, *size1 - min_size1);
        float size_2_maximum_delta = ImMax(0.0f, *size2 - min_size2);
        if (mouse_delta < -size_1_maximum_delta)
            mouse_delta = -size_1_maximum_delta;
        if (mouse_delta > size_2_maximum_delta)
            mouse_delta = size_2_maximum_delta;

        // Apply resize
        if (mouse_delta != 0.0f)
        {
            if (mouse_delta < 0.0f)
                IM_ASSERT(*size1 + mouse_delta >= min_size1);
            if (mouse_delta > 0.0f)
                IM_ASSERT(*size2 - mouse_delta >= min_size2);
            *size1 += mouse_delta;
            *size2 -= mouse_delta;
            bb_render.Translate((axis == ImGuiAxis_X) ? ImVec2(mouse_delta, 0.0f) : ImVec2(0.0f, mouse_delta));
            MarkItemEdited(id);
        }
    }

    // Render
    const ImU32 col = GetColorU32(held ? ImGuiCol_SeparatorActive : (hovered && g.HoveredIdTimer >= hover_visibility_delay) ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
    window->DrawList->AddRectFilled(bb_render.Min, bb_render.Max, col, 0.0f);

    return held;
}

static int IMGUI_CDECL ShrinkWidthItemComparer(const void* lhs, const void* rhs)
{
    const ImGuiShrinkWidthItem* a = (const ImGuiShrinkWidthItem*)lhs;
    const ImGuiShrinkWidthItem* b = (const ImGuiShrinkWidthItem*)rhs;
    if (int d = (int)(b->Width - a->Width))
        return d;
    return (b->Index - a->Index);
}

// Shrink excess width from a set of item, by removing width from the larger items first.
void ImGui::ShrinkWidths(ImGuiShrinkWidthItem* items, int count, float width_excess)
{
    if (count == 1)
    {
        items[0].Width = ImMax(items[0].Width - width_excess, 1.0f);
        return;
    }
    ImQsort(items, (size_t)count, sizeof(ImGuiShrinkWidthItem), ShrinkWidthItemComparer);
    int count_same_width = 1;
    while (width_excess > 0.0f && count_same_width < count)
    {
        while (count_same_width < count && items[0].Width <= items[count_same_width].Width)
            count_same_width++;
        float max_width_to_remove_per_item = (count_same_width < count) ? (items[0].Width - items[count_same_width].Width) : (items[0].Width - 1.0f);
        float width_to_remove_per_item = ImMin(width_excess / count_same_width, max_width_to_remove_per_item);
        for (int item_n = 0; item_n < count_same_width; item_n++)
            items[item_n].Width -= width_to_remove_per_item;
        width_excess -= width_to_remove_per_item * count_same_width;
    }

    // Round width and redistribute remainder left-to-right (could make it an option of the function?)
    // Ensure that e.g. the right-most tab of a shrunk tab-bar always reaches exactly at the same distance from the right-most edge of the tab bar separator.
    width_excess = 0.0f;
    for (int n = 0; n < count; n++)
    {
        float width_rounded = ImFloor(items[n].Width);
        width_excess += items[n].Width - width_rounded;
        items[n].Width = width_rounded;
    }
    if (width_excess > 0.0f)
        for (int n = 0; n < count; n++)
            if (items[n].Index < (int)(width_excess + 0.01f))
                items[n].Width += 1.0f;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ComboBox
//-------------------------------------------------------------------------
// - BeginCombo()
// - EndCombo()
// - Combo()
//-------------------------------------------------------------------------

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
    ImGuiContext& g = *GImGui;
    if (items_count <= 0)
        return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool ImGui::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
    // Always consume the SetNextWindowSizeConstraint() call in our early return paths
    ImGuiContext& g = *GImGui;
    bool has_window_size_constraint = (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint) != 0;
    g.NextWindowData.Flags &= ~ImGuiNextWindowDataFlags_HasSizeConstraint;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const float expected_w = CalcItemWidth();
    const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : expected_w;
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
    bool popup_open = IsPopupOpen(id);

    const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    const float value_x2 = ImMax(frame_bb.Min.x, frame_bb.Max.x - arrow_size);
    RenderNavHighlight(frame_bb, id);
    if (!(flags & ImGuiComboFlags_NoPreview))
        window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(value_x2, frame_bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Left);
    if (!(flags & ImGuiComboFlags_NoArrowButton))
    {
        ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ImU32 text_col = GetColorU32(ImGuiCol_Text);
        window->DrawList->AddRectFilled(ImVec2(value_x2, frame_bb.Min.y), frame_bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Right);
        if (value_x2 + arrow_size - style.FramePadding.x <= frame_bb.Max.x)
            RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, frame_bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);
    }
    RenderFrameBorder(frame_bb.Min, frame_bb.Max, style.FrameRounding);
    if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
        RenderTextClipped(frame_bb.Min + style.FramePadding, ImVec2(value_x2, frame_bb.Max.y), preview_value, NULL, NULL, ImVec2(0.0f, 0.0f));
    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    if ((pressed || g.NavActivateId == id) && !popup_open)
    {
        if (window->DC.NavLayerCurrent == 0)
            window->NavLastIds[0] = id;
        OpenPopupEx(id);
        popup_open = true;
    }

    if (!popup_open)
        return false;

    if (has_window_size_constraint)
    {
        g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSizeConstraint;
        g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
    } else
    {
        if ((flags & ImGuiComboFlags_HeightMask_) == 0)
            flags |= ImGuiComboFlags_HeightRegular;
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_));    // Only one
        int popup_max_height_in_items = -1;
        if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
        else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
        else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
        SetNextWindowSizeConstraints(ImVec2(w, 0.0f), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));
    }

    char name[16];
    ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

    // Peak into expected window size so we can position it
    if (ImGuiWindow* popup_window = FindWindowByName(name))
        if (popup_window->WasActive)
        {
            ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
            if (flags & ImGuiComboFlags_PopupAlignLeft)
                popup_window->AutoPosLastDirection = ImGuiDir_Left;
            ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
            ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
            SetNextWindowPos(pos);
        }

    // We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

    // Horizontally align ourselves with the framed text
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
    bool ret = Begin(name, NULL, window_flags);
    PopStyleVar();
    if (!ret)
    {
        EndPopup();
        IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
        return false;
    }
    return true;
}

void ImGui::EndCombo()
{
    EndPopup();
}

// Getter for the old Combo() API: const char*[]
static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
    const char* const* items = (const char* const*)data;
    if (out_text)
        *out_text = items[idx];
    return true;
}

// Getter for the old Combo() API: "item1\0item2\0item3\0"
static bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
{
    // FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        if (idx == items_count)
            break;
        p += strlen(p) + 1;
        items_count++;
    }
    if (!*p)
        return false;
    if (out_text)
        *out_text = p;
    return true;
}

// Old API, prefer using BeginCombo() nowadays if you can.
bool ImGui::Combo(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items)
{
    ImGuiContext& g = *GImGui;

    // Call the getter to obtain the preview string which is a parameter to BeginCombo()
    const char* preview_value = NULL;
    if (*current_item >= 0 && *current_item < items_count)
        items_getter(data, *current_item, &preview_value);

    // The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
    if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
        SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

    if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
        return false;

    // Display items
    // FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
    bool value_changed = false;
    for (int i = 0; i < items_count; i++)
    {
        PushID((void*)(intptr_t)i);
        const bool item_selected = (i == *current_item);
        const char* item_text;
        if (!items_getter(data, i, &item_text))
            item_text = "*Unknown item*";
        if (Selectable(item_text, item_selected))
        {
            value_changed = true;
            *current_item = i;
        }
        if (item_selected)
            SetItemDefaultFocus();
        PopID();
    }

    EndCombo();
    return value_changed;
}

// Combo box helper allowing to pass an array of strings.
bool ImGui::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
    const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
    return value_changed;
}

// Combo box helper allowing to pass all items in a single string literal holding multiple zero-terminated items "item1\0item2\0"
bool ImGui::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
    while (*p)
    {
        p += strlen(p) + 1;
        items_count++;
    }
    bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
    return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Data Type and Data Formatting Helpers [Internal]
//-------------------------------------------------------------------------
// - PatchFormatStringFloatToInt()
// - DataTypeGetInfo()
// - DataTypeFormatString()
// - DataTypeApplyOp()
// - DataTypeApplyOpFromText()
// - DataTypeClamp()
// - GetMinimumStepAtDecimalPrecision
// - RoundScalarWithFormat<>()
//-------------------------------------------------------------------------

static const ImGuiDataTypeInfo GDataTypeInfo[] =
{
    { sizeof(char),             "%d",   "%d"    },  // ImGuiDataType_S8
    { sizeof(unsigned char),    "%u",   "%u"    },
    { sizeof(short),            "%d",   "%d"    },  // ImGuiDataType_S16
    { sizeof(unsigned short),   "%u",   "%u"    },
    { sizeof(int),              "%d",   "%d"    },  // ImGuiDataType_S32
    { sizeof(unsigned int),     "%u",   "%u"    },
#ifdef _MSC_VER
    { sizeof(ImS64),            "%I64d","%I64d" },  // ImGuiDataType_S64
    { sizeof(ImU64),            "%I64u","%I64u" },
#else
    { sizeof(ImS64),            "%lld", "%lld"  },  // ImGuiDataType_S64
    { sizeof(ImU64),            "%llu", "%llu"  },
#endif
    { sizeof(float),            "%f",   "%f"    },  // ImGuiDataType_Float (float are promoted to double in va_arg)
    { sizeof(double),           "%f",   "%lf"   },  // ImGuiDataType_Double
};
IM_STATIC_ASSERT(IM_ARRAYSIZE(GDataTypeInfo) == ImGuiDataType_COUNT);

// FIXME-LEGACY: Prior to 1.61 our DragInt() function internally used floats and because of this the compile-time default value for format was "%.0f".
// Even though we changed the compile-time default, we expect users to have carried %f around, which would break the display of DragInt() calls.
// To honor backward compatibility we are rewriting the format string, unless IMGUI_DISABLE_OBSOLETE_FUNCTIONS is enabled. What could possibly go wrong?!
static const char* PatchFormatStringFloatToInt(const char* fmt)
{
    if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
        return "%d";
    const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
    const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
    if (fmt_end > fmt_start && fmt_end[-1] == 'f')
    {
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        if (fmt_start == fmt && fmt_end[0] == 0)
            return "%d";
        ImGuiContext& g = *GImGui;
        ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
        return g.TempBuffer;
#else
        IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please replace with e.g. "%d"
#endif
    }
    return fmt;
}

const ImGuiDataTypeInfo* ImGui::DataTypeGetInfo(ImGuiDataType data_type)
{
    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    return &GDataTypeInfo[data_type];
}

int ImGui::DataTypeFormatString(char* buf, int buf_size, ImGuiDataType data_type, const void* p_data, const char* format)
{
    // Signedness doesn't matter when pushing integer arguments
    if (data_type == ImGuiDataType_S32 || data_type == ImGuiDataType_U32)
        return ImFormatString(buf, buf_size, format, *(const ImU32*)p_data);
    if (data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)
        return ImFormatString(buf, buf_size, format, *(const ImU64*)p_data);
    if (data_type == ImGuiDataType_Float)
        return ImFormatString(buf, buf_size, format, *(const float*)p_data);
    if (data_type == ImGuiDataType_Double)
        return ImFormatString(buf, buf_size, format, *(const double*)p_data);
    if (data_type == ImGuiDataType_S8)
        return ImFormatString(buf, buf_size, format, *(const ImS8*)p_data);
    if (data_type == ImGuiDataType_U8)
        return ImFormatString(buf, buf_size, format, *(const ImU8*)p_data);
    if (data_type == ImGuiDataType_S16)
        return ImFormatString(buf, buf_size, format, *(const ImS16*)p_data);
    if (data_type == ImGuiDataType_U16)
        return ImFormatString(buf, buf_size, format, *(const ImU16*)p_data);
    IM_ASSERT(0);
    return 0;
}

void ImGui::DataTypeApplyOp(ImGuiDataType data_type, int op, void* output, void* arg1, const void* arg2)
{
    IM_ASSERT(op == '+' || op == '-');
    switch (data_type)
    {
    case ImGuiDataType_S8:
        if (op == '+') { *(ImS8*)output = ImAddClampOverflow(*(const ImS8*)arg1, *(const ImS8*)arg2, IM_S8_MIN, IM_S8_MAX); }
        if (op == '-') { *(ImS8*)output = ImSubClampOverflow(*(const ImS8*)arg1, *(const ImS8*)arg2, IM_S8_MIN, IM_S8_MAX); }
        return;
    case ImGuiDataType_U8:
        if (op == '+') { *(ImU8*)output = ImAddClampOverflow(*(const ImU8*)arg1, *(const ImU8*)arg2, IM_U8_MIN, IM_U8_MAX); }
        if (op == '-') { *(ImU8*)output = ImSubClampOverflow(*(const ImU8*)arg1, *(const ImU8*)arg2, IM_U8_MIN, IM_U8_MAX); }
        return;
    case ImGuiDataType_S16:
        if (op == '+') { *(ImS16*)output = ImAddClampOverflow(*(const ImS16*)arg1, *(const ImS16*)arg2, IM_S16_MIN, IM_S16_MAX); }
        if (op == '-') { *(ImS16*)output = ImSubClampOverflow(*(const ImS16*)arg1, *(const ImS16*)arg2, IM_S16_MIN, IM_S16_MAX); }
        return;
    case ImGuiDataType_U16:
        if (op == '+') { *(ImU16*)output = ImAddClampOverflow(*(const ImU16*)arg1, *(const ImU16*)arg2, IM_U16_MIN, IM_U16_MAX); }
        if (op == '-') { *(ImU16*)output = ImSubClampOverflow(*(const ImU16*)arg1, *(const ImU16*)arg2, IM_U16_MIN, IM_U16_MAX); }
        return;
    case ImGuiDataType_S32:
        if (op == '+') { *(ImS32*)output = ImAddClampOverflow(*(const ImS32*)arg1, *(const ImS32*)arg2, IM_S32_MIN, IM_S32_MAX); }
        if (op == '-') { *(ImS32*)output = ImSubClampOverflow(*(const ImS32*)arg1, *(const ImS32*)arg2, IM_S32_MIN, IM_S32_MAX); }
        return;
    case ImGuiDataType_U32:
        if (op == '+') { *(ImU32*)output = ImAddClampOverflow(*(const ImU32*)arg1, *(const ImU32*)arg2, IM_U32_MIN, IM_U32_MAX); }
        if (op == '-') { *(ImU32*)output = ImSubClampOverflow(*(const ImU32*)arg1, *(const ImU32*)arg2, IM_U32_MIN, IM_U32_MAX); }
        return;
    case ImGuiDataType_S64:
        if (op == '+') { *(ImS64*)output = ImAddClampOverflow(*(const ImS64*)arg1, *(const ImS64*)arg2, IM_S64_MIN, IM_S64_MAX); }
        if (op == '-') { *(ImS64*)output = ImSubClampOverflow(*(const ImS64*)arg1, *(const ImS64*)arg2, IM_S64_MIN, IM_S64_MAX); }
        return;
    case ImGuiDataType_U64:
        if (op == '+') { *(ImU64*)output = ImAddClampOverflow(*(const ImU64*)arg1, *(const ImU64*)arg2, IM_U64_MIN, IM_U64_MAX); }
        if (op == '-') { *(ImU64*)output = ImSubClampOverflow(*(const ImU64*)arg1, *(const ImU64*)arg2, IM_U64_MIN, IM_U64_MAX); }
        return;
    case ImGuiDataType_Float:
        if (op == '+') { *(float*)output = *(const float*)arg1 + *(const float*)arg2; }
        if (op == '-') { *(float*)output = *(const float*)arg1 - *(const float*)arg2; }
        return;
    case ImGuiDataType_Double:
        if (op == '+') { *(double*)output = *(const double*)arg1 + *(const double*)arg2; }
        if (op == '-') { *(double*)output = *(const double*)arg1 - *(const double*)arg2; }
        return;
    case ImGuiDataType_COUNT: break;
    }
    IM_ASSERT(0);
}

// User can input math operators (e.g. +100) to edit a numerical values.
// NB: This is _not_ a full expression evaluator. We should probably add one and replace this dumb mess..
bool ImGui::DataTypeApplyOpFromText(const char* buf, const char* initial_value_buf, ImGuiDataType data_type, void* p_data, const char* format)
{
    while (ImCharIsBlankA(*buf))
        buf++;

    // We don't support '-' op because it would conflict with inputing negative value.
    // Instead you can use +-100 to subtract from an existing value
    char op = buf[0];
    if (op == '+' || op == '*' || op == '/')
    {
        buf++;
        while (ImCharIsBlankA(*buf))
            buf++;
    } else
    {
        op = 0;
    }
    if (!buf[0])
        return false;

    // Copy the value in an opaque buffer so we can compare at the end of the function if it changed at all.
    const ImGuiDataTypeInfo* type_info = DataTypeGetInfo(data_type);
    ImGuiDataTypeTempStorage data_backup;
    memcpy(&data_backup, p_data, type_info->Size);

    if (format == NULL)
        format = type_info->ScanFmt;

    // FIXME-LEGACY: The aim is to remove those operators and write a proper expression evaluator at some point..
    int arg1i = 0;
    if (data_type == ImGuiDataType_S32)
    {
        int* v = (int*)p_data;
        int arg0i = *v;
        float arg1f = 0.0f;
        if (op && sscanf(initial_value_buf, format, &arg0i) < 1)
            return false;
        // Store operand in a float so we can use fractional value for multipliers (*1.1), but constant always parsed as integer so we can fit big integers (e.g. 2000000003) past float precision
        if (op == '+') { if (sscanf(buf, "%d", &arg1i)) *v = (int)(arg0i + arg1i); }                   // Add (use "+-" to subtract)
        else if (op == '*') { if (sscanf(buf, "%f", &arg1f)) *v = (int)(arg0i * arg1f); }                   // Multiply
        else if (op == '/') { if (sscanf(buf, "%f", &arg1f) && arg1f != 0.0f) *v = (int)(arg0i / arg1f); }  // Divide
        else { if (sscanf(buf, format, &arg1i) == 1) *v = arg1i; }                           // Assign constant
    } else if (data_type == ImGuiDataType_Float)
    {
        // For floats we have to ignore format with precision (e.g. "%.2f") because sscanf doesn't take them in
        format = "%f";
        float* v = (float*)p_data;
        float arg0f = *v, arg1f = 0.0f;
        if (op && sscanf(initial_value_buf, format, &arg0f) < 1)
            return false;
        if (sscanf(buf, format, &arg1f) < 1)
            return false;
        if (op == '+') { *v = arg0f + arg1f; }                    // Add (use "+-" to subtract)
        else if (op == '*') { *v = arg0f * arg1f; }                    // Multiply
        else if (op == '/') { if (arg1f != 0.0f) *v = arg0f / arg1f; } // Divide
        else { *v = arg1f; }                            // Assign constant
    } else if (data_type == ImGuiDataType_Double)
    {
        format = "%lf"; // scanf differentiate float/double unlike printf which forces everything to double because of ellipsis
        double* v = (double*)p_data;
        double arg0f = *v, arg1f = 0.0;
        if (op && sscanf(initial_value_buf, format, &arg0f) < 1)
            return false;
        if (sscanf(buf, format, &arg1f) < 1)
            return false;
        if (op == '+') { *v = arg0f + arg1f; }                    // Add (use "+-" to subtract)
        else if (op == '*') { *v = arg0f * arg1f; }                    // Multiply
        else if (op == '/') { if (arg1f != 0.0f) *v = arg0f / arg1f; } // Divide
        else { *v = arg1f; }                            // Assign constant
    } else if (data_type == ImGuiDataType_U32 || data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)
    {
        // All other types assign constant
        // We don't bother handling support for legacy operators since they are a little too crappy. Instead we will later implement a proper expression evaluator in the future.
        sscanf(buf, format, p_data);
    } else
    {
        // Small types need a 32-bit buffer to receive the result from scanf()
        int v32;
        sscanf(buf, format, &v32);
        if (data_type == ImGuiDataType_S8)
            *(ImS8*)p_data = (ImS8)ImClamp(v32, (int)IM_S8_MIN, (int)IM_S8_MAX);
        else if (data_type == ImGuiDataType_U8)
            *(ImU8*)p_data = (ImU8)ImClamp(v32, (int)IM_U8_MIN, (int)IM_U8_MAX);
        else if (data_type == ImGuiDataType_S16)
            *(ImS16*)p_data = (ImS16)ImClamp(v32, (int)IM_S16_MIN, (int)IM_S16_MAX);
        else if (data_type == ImGuiDataType_U16)
            *(ImU16*)p_data = (ImU16)ImClamp(v32, (int)IM_U16_MIN, (int)IM_U16_MAX);
        else
            IM_ASSERT(0);
    }

    return memcmp(&data_backup, p_data, type_info->Size) != 0;
}

template<typename T>
static bool ClampBehaviorT(T* v, T v_min, T v_max)
{
    if (*v < v_min) { *v = v_min; return true; }
    if (*v > v_max) { *v = v_max; return true; }
    return false;
}

bool ImGui::DataTypeClamp(ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max)
{
    switch (data_type)
    {
    case ImGuiDataType_S8:     return ClampBehaviorT<ImS8  >((ImS8*)p_data, *(const ImS8*)p_min, *(const ImS8*)p_max);
    case ImGuiDataType_U8:     return ClampBehaviorT<ImU8  >((ImU8*)p_data, *(const ImU8*)p_min, *(const ImU8*)p_max);
    case ImGuiDataType_S16:    return ClampBehaviorT<ImS16 >((ImS16*)p_data, *(const ImS16*)p_min, *(const ImS16*)p_max);
    case ImGuiDataType_U16:    return ClampBehaviorT<ImU16 >((ImU16*)p_data, *(const ImU16*)p_min, *(const ImU16*)p_max);
    case ImGuiDataType_S32:    return ClampBehaviorT<ImS32 >((ImS32*)p_data, *(const ImS32*)p_min, *(const ImS32*)p_max);
    case ImGuiDataType_U32:    return ClampBehaviorT<ImU32 >((ImU32*)p_data, *(const ImU32*)p_min, *(const ImU32*)p_max);
    case ImGuiDataType_S64:    return ClampBehaviorT<ImS64 >((ImS64*)p_data, *(const ImS64*)p_min, *(const ImS64*)p_max);
    case ImGuiDataType_U64:    return ClampBehaviorT<ImU64 >((ImU64*)p_data, *(const ImU64*)p_min, *(const ImU64*)p_max);
    case ImGuiDataType_Float:  return ClampBehaviorT<float >((float*)p_data, *(const float*)p_min, *(const float*)p_max);
    case ImGuiDataType_Double: return ClampBehaviorT<double>((double*)p_data, *(const double*)p_min, *(const double*)p_max);
    case ImGuiDataType_COUNT:  break;
    }
    IM_ASSERT(0);
    return false;
}

static float GetMinimumStepAtDecimalPrecision(int decimal_precision)
{
    static const float min_steps[10] = { 1.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f, 0.0000001f, 0.00000001f, 0.000000001f };
    if (decimal_precision < 0)
        return FLT_MIN;
    return (decimal_precision < IM_ARRAYSIZE(min_steps)) ? min_steps[decimal_precision] : ImPow(10.0f, (float)-decimal_precision);
}

template<typename TYPE>
static const char* ImAtoi(const char* src, TYPE* output)
{
    int negative = 0;
    if (*src == '-') { negative = 1; src++; }
    if (*src == '+') { src++; }
    TYPE v = 0;
    while (*src >= '0' && *src <= '9')
        v = (v * 10) + (*src++ - '0');
    *output = negative ? -v : v;
    return src;
}

template<typename TYPE, typename SIGNEDTYPE>
TYPE ImGui::RoundScalarWithFormatT(const char* format, ImGuiDataType data_type, TYPE v)
{
    const char* fmt_start = ImParseFormatFindStart(format);
    if (fmt_start[0] != '%' || fmt_start[1] == '%') // Don't apply if the value is not visible in the format string
        return v;
    char v_str[64];
    ImFormatString(v_str, IM_ARRAYSIZE(v_str), fmt_start, v);
    const char* p = v_str;
    while (*p == ' ')
        p++;
    if (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double)
        v = (TYPE)ImAtof(p);
    else
        ImAtoi(p, (SIGNEDTYPE*)&v);
    return v;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: DragScalar, DragFloat, DragInt, etc.
//-------------------------------------------------------------------------
// - DragBehaviorT<>() [Internal]
// - DragBehavior() [Internal]
// - DragScalar()
// - DragScalarN()
// - DragFloat()
// - DragFloat2()
// - DragFloat3()
// - DragFloat4()
// - DragFloatRange2()
// - DragInt()
// - DragInt2()
// - DragInt3()
// - DragInt4()
// - DragIntRange2()
//-------------------------------------------------------------------------

// This is called by DragBehavior() when the widget is active (held by mouse or being manipulated with Nav controls)
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool ImGui::DragBehaviorT(ImGuiDataType data_type, TYPE* v, float v_speed, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiDragFlags flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiAxis axis = (flags & ImGuiDragFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
    const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
    const bool is_clamped = (v_min < v_max);
    const bool is_power = (power != 1.0f && is_decimal && is_clamped && (v_max - v_min < FLT_MAX));
    const bool is_locked = (v_min > v_max);
    if (is_locked)
        return false;

    // Default tweak speed
    if (v_speed == 0.0f && is_clamped && (v_max - v_min < FLT_MAX))
        v_speed = (float)((v_max - v_min) * g.DragSpeedDefaultRatio);

    // Inputs accumulates into g.DragCurrentAccum, which is flushed into the current value as soon as it makes a difference with our precision settings
    float adjust_delta = 0.0f;
    if (g.ActiveIdSource == ImGuiInputSource_Mouse && IsMousePosValid() && g.IO.MouseDragMaxDistanceSqr[0] > 1.0f * 1.0f)
    {
        adjust_delta = g.IO.MouseDelta[axis];
        if (g.IO.KeyAlt)
            adjust_delta *= 1.0f / 100.0f;
        if (g.IO.KeyShift)
            adjust_delta *= 10.0f;
    } else if (g.ActiveIdSource == ImGuiInputSource_Nav)
    {
        int decimal_precision = is_decimal ? ImParseFormatPrecision(format, 3) : 0;
        adjust_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 1.0f / 10.0f, 10.0f)[axis];
        v_speed = ImMax(v_speed, GetMinimumStepAtDecimalPrecision(decimal_precision));
    }
    adjust_delta *= v_speed;

    // For vertical drag we currently assume that Up=higher value (like we do with vertical sliders). This may become a parameter.
    if (axis == ImGuiAxis_Y)
        adjust_delta = -adjust_delta;

    // Clear current value on activation
    // Avoid altering values and clamping when we are _already_ past the limits and heading in the same direction, so e.g. if range is 0..255, current value is 300 and we are pushing to the right side, keep the 300.
    bool is_just_activated = g.ActiveIdIsJustActivated;
    bool is_already_past_limits_and_pushing_outward = is_clamped && ((*v >= v_max && adjust_delta > 0.0f) || (*v <= v_min && adjust_delta < 0.0f));
    bool is_drag_direction_change_with_power = is_power && ((adjust_delta < 0 && g.DragCurrentAccum > 0) || (adjust_delta > 0 && g.DragCurrentAccum < 0));
    if (is_just_activated || is_already_past_limits_and_pushing_outward || is_drag_direction_change_with_power)
    {
        g.DragCurrentAccum = 0.0f;
        g.DragCurrentAccumDirty = false;
    } else if (adjust_delta != 0.0f)
    {
        g.DragCurrentAccum += adjust_delta;
        g.DragCurrentAccumDirty = true;
    }

    if (!g.DragCurrentAccumDirty)
        return false;

    TYPE v_cur = *v;
    FLOATTYPE v_old_ref_for_accum_remainder = (FLOATTYPE)0.0f;

    if (is_power)
    {
        // Offset + round to user desired precision, with a curve on the v_min..v_max range to get more precision on one side of the range
        FLOATTYPE v_old_norm_curved = ImPow((FLOATTYPE)(v_cur - v_min) / (FLOATTYPE)(v_max - v_min), (FLOATTYPE)1.0f / power);
        FLOATTYPE v_new_norm_curved = v_old_norm_curved + (g.DragCurrentAccum / (v_max - v_min));
        v_cur = v_min + (SIGNEDTYPE)ImPow(ImSaturate((float)v_new_norm_curved), power) * (v_max - v_min);
        v_old_ref_for_accum_remainder = v_old_norm_curved;
    } else
    {
        v_cur += (SIGNEDTYPE)g.DragCurrentAccum;
    }

    // Round to user desired precision based on format string
    v_cur = RoundScalarWithFormatT<TYPE, SIGNEDTYPE>(format, data_type, v_cur);

    // Preserve remainder after rounding has been applied. This also allow slow tweaking of values.
    g.DragCurrentAccumDirty = false;
    if (is_power)
    {
        FLOATTYPE v_cur_norm_curved = ImPow((FLOATTYPE)(v_cur - v_min) / (FLOATTYPE)(v_max - v_min), (FLOATTYPE)1.0f / power);
        g.DragCurrentAccum -= (float)(v_cur_norm_curved - v_old_ref_for_accum_remainder);
    } else
    {
        g.DragCurrentAccum -= (float)((SIGNEDTYPE)v_cur - (SIGNEDTYPE)*v);
    }

    // Lose zero sign for float/double
    if (v_cur == (TYPE)-0)
        v_cur = (TYPE)0;

    // Clamp values (+ handle overflow/wrap-around for integer types)
    if (*v != v_cur && is_clamped)
    {
        if (v_cur < v_min || (v_cur > * v && adjust_delta < 0.0f && !is_decimal))
            v_cur = v_min;
        if (v_cur > v_max || (v_cur < *v && adjust_delta > 0.0f && !is_decimal))
            v_cur = v_max;
    }

    // Apply result
    if (*v == v_cur)
        return false;
    *v = v_cur;
    return true;
}

bool ImGui::DragBehavior(ImGuiID id, ImGuiDataType data_type, void* p_v, float v_speed, const void* p_min, const void* p_max, const char* format, float power, ImGuiDragFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId == id)
    {
        if (g.ActiveIdSource == ImGuiInputSource_Mouse && !g.IO.MouseDown[0])
            ClearActiveID();
        else if (g.ActiveIdSource == ImGuiInputSource_Nav && g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            ClearActiveID();
    }
    if (g.ActiveId != id)
        return false;

    switch (data_type)
    {
    case ImGuiDataType_S8: { ImS32 v32 = (ImS32) * (ImS8*)p_v;  bool r = DragBehaviorT<ImS32, ImS32, float>(ImGuiDataType_S32, &v32, v_speed, p_min ? *(const ImS8*)p_min : IM_S8_MIN, p_max ? *(const ImS8*)p_max : IM_S8_MAX, format, power, flags); if (r) *(ImS8*)p_v = (ImS8)v32; return r; }
    case ImGuiDataType_U8: { ImU32 v32 = (ImU32) * (ImU8*)p_v;  bool r = DragBehaviorT<ImU32, ImS32, float>(ImGuiDataType_U32, &v32, v_speed, p_min ? *(const ImU8*)p_min : IM_U8_MIN, p_max ? *(const ImU8*)p_max : IM_U8_MAX, format, power, flags); if (r) *(ImU8*)p_v = (ImU8)v32; return r; }
    case ImGuiDataType_S16: { ImS32 v32 = (ImS32) * (ImS16*)p_v; bool r = DragBehaviorT<ImS32, ImS32, float>(ImGuiDataType_S32, &v32, v_speed, p_min ? *(const ImS16*)p_min : IM_S16_MIN, p_max ? *(const ImS16*)p_max : IM_S16_MAX, format, power, flags); if (r) *(ImS16*)p_v = (ImS16)v32; return r; }
    case ImGuiDataType_U16: { ImU32 v32 = (ImU32) * (ImU16*)p_v; bool r = DragBehaviorT<ImU32, ImS32, float>(ImGuiDataType_U32, &v32, v_speed, p_min ? *(const ImU16*)p_min : IM_U16_MIN, p_max ? *(const ImU16*)p_max : IM_U16_MAX, format, power, flags); if (r) *(ImU16*)p_v = (ImU16)v32; return r; }
    case ImGuiDataType_S32:    return DragBehaviorT<ImS32, ImS32, float >(data_type, (ImS32*)p_v, v_speed, p_min ? *(const ImS32*)p_min : IM_S32_MIN, p_max ? *(const ImS32*)p_max : IM_S32_MAX, format, power, flags);
    case ImGuiDataType_U32:    return DragBehaviorT<ImU32, ImS32, float >(data_type, (ImU32*)p_v, v_speed, p_min ? *(const ImU32*)p_min : IM_U32_MIN, p_max ? *(const ImU32*)p_max : IM_U32_MAX, format, power, flags);
    case ImGuiDataType_S64:    return DragBehaviorT<ImS64, ImS64, double>(data_type, (ImS64*)p_v, v_speed, p_min ? *(const ImS64*)p_min : IM_S64_MIN, p_max ? *(const ImS64*)p_max : IM_S64_MAX, format, power, flags);
    case ImGuiDataType_U64:    return DragBehaviorT<ImU64, ImS64, double>(data_type, (ImU64*)p_v, v_speed, p_min ? *(const ImU64*)p_min : IM_U64_MIN, p_max ? *(const ImU64*)p_max : IM_U64_MAX, format, power, flags);
    case ImGuiDataType_Float:  return DragBehaviorT<float, float, float >(data_type, (float*)p_v, v_speed, p_min ? *(const float*)p_min : -FLT_MAX, p_max ? *(const float*)p_max : FLT_MAX, format, power, flags);
    case ImGuiDataType_Double: return DragBehaviorT<double, double, double>(data_type, (double*)p_v, v_speed, p_min ? *(const double*)p_min : -DBL_MAX, p_max ? *(const double*)p_max : DBL_MAX, format, power, flags);
    case ImGuiDataType_COUNT:  break;
    }
    IM_ASSERT(0);
    return false;
}

// Note: p_data, p_min and p_max are _pointers_ to a memory address holding the data. For a Drag widget, p_min and p_max are optional.
// Read code of e.g. SliderFloat(), SliderInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
bool ImGui::DragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if (power != 1.0f)
        IM_ASSERT(p_min != NULL && p_max != NULL); // When using a power curve the drag needs to have known bounds

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;
    else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
        format = PatchFormatStringFloatToInt(format);

    // Tabbing or CTRL-clicking on Drag turns it into an input box
    const bool hovered = ItemHoverable(frame_bb, id);
    bool temp_input_is_active = TempInputIsActive(id);
    bool temp_input_start = false;
    if (!temp_input_is_active)
    {
        const bool focus_requested = FocusableItemRegister(window, id);
        const bool clicked = (hovered && g.IO.MouseClicked[0]);
        const bool double_clicked = (hovered && g.IO.MouseDoubleClicked[0]);
        if (focus_requested || clicked || double_clicked || g.NavActivateId == id || g.NavInputId == id)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            if (focus_requested || (clicked && g.IO.KeyCtrl) || double_clicked || g.NavInputId == id)
            {
                temp_input_start = true;
                FocusableItemUnregister(window);
            }
        }
    }

    // Our current specs do NOT clamp when using CTRL+Click manual input, but we should eventually add a flag for that..
    if (temp_input_is_active || temp_input_start)
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format);// , p_min, p_max);

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

    // Drag behavior
    const bool value_changed = DragBehavior(id, data_type, p_data, v_speed, p_min, p_max, format, power, ImGuiDragFlags_None);
    if (value_changed)
        MarkItemEdited(id);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
    return value_changed;
}

bool ImGui::DragScalarN(const char* label, ImGuiDataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, CalcItemWidth());
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= DragScalar("", data_type, p_data, v_speed, p_min, p_max, format, power);
        PopID();
        PopItemWidth();
        p_data = (void*)((char*)p_data + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end)
    {
        SameLine(0, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}

bool ImGui::DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, float power)
{
    return DragScalar(label, ImGuiDataType_Float, v, v_speed, &v_min, &v_max, format, power);
}

bool ImGui::DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, float power)
{
    return DragScalarN(label, ImGuiDataType_Float, v, 2, v_speed, &v_min, &v_max, format, power);
}

bool ImGui::DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, float power)
{
    return DragScalarN(label, ImGuiDataType_Float, v, 3, v_speed, &v_min, &v_max, format, power);
}

bool ImGui::DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, float power)
{
    return DragScalarN(label, ImGuiDataType_Float, v, 4, v_speed, &v_min, &v_max, format, power);
}

bool ImGui::DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    PushID(label);
    BeginGroup();
    PushMultiItemsWidths(2, CalcItemWidth());

    bool value_changed = DragFloat("##min", v_current_min, v_speed, (v_min >= v_max) ? -FLT_MAX : v_min, (v_min >= v_max) ? *v_current_max : ImMin(v_max, *v_current_max), format, power);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);
    value_changed |= DragFloat("##max", v_current_max, v_speed, (v_min >= v_max) ? *v_current_min : ImMax(v_min, *v_current_min), (v_min >= v_max) ? FLT_MAX : v_max, format_max ? format_max : format, power);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);

    TextEx(label, FindRenderedTextEnd(label));
    EndGroup();
    PopID();
    return value_changed;
}

// NB: v_speed is float to allow adjusting the drag speed with more precision
bool ImGui::DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format)
{
    return DragScalar(label, ImGuiDataType_S32, v, v_speed, &v_min, &v_max, format);
}

bool ImGui::DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format)
{
    return DragScalarN(label, ImGuiDataType_S32, v, 2, v_speed, &v_min, &v_max, format);
}

bool ImGui::DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format)
{
    return DragScalarN(label, ImGuiDataType_S32, v, 3, v_speed, &v_min, &v_max, format);
}

bool ImGui::DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format)
{
    return DragScalarN(label, ImGuiDataType_S32, v, 4, v_speed, &v_min, &v_max, format);
}

bool ImGui::DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    PushID(label);
    BeginGroup();
    PushMultiItemsWidths(2, CalcItemWidth());

    bool value_changed = DragInt("##min", v_current_min, v_speed, (v_min >= v_max) ? INT_MIN : v_min, (v_min >= v_max) ? *v_current_max : ImMin(v_max, *v_current_max), format);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);
    value_changed |= DragInt("##max", v_current_max, v_speed, (v_min >= v_max) ? *v_current_min : ImMax(v_min, *v_current_min), (v_min >= v_max) ? INT_MAX : v_max, format_max ? format_max : format);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);

    TextEx(label, FindRenderedTextEnd(label));
    EndGroup();
    PopID();

    return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
//-------------------------------------------------------------------------
// - SliderBehaviorT<>() [Internal]
// - SliderBehavior() [Internal]
// - SliderScalar()
// - SliderScalarN()
// - SliderFloat()
// - SliderFloat2()
// - SliderFloat3()
// - SliderFloat4()
// - SliderAngle()
// - SliderInt()
// - SliderInt2()
// - SliderInt3()
// - SliderInt4()
// - VSliderScalar()
// - VSliderFloat()
// - VSliderInt()
//-------------------------------------------------------------------------

template<typename TYPE, typename FLOATTYPE>
float ImGui::SliderCalcRatioFromValueT(ImGuiDataType data_type, TYPE v, TYPE v_min, TYPE v_max, float power, float linear_zero_pos)
{
    if (v_min == v_max)
        return 0.0f;

    const bool is_power = (power != 1.0f) && (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double);
    const TYPE v_clamped = (v_min < v_max) ? ImClamp(v, v_min, v_max) : ImClamp(v, v_max, v_min);
    if (is_power)
    {
        if (v_clamped < 0.0f)
        {
            const float f = 1.0f - (float)((v_clamped - v_min) / (ImMin((TYPE)0, v_max) - v_min));
            return (1.0f - ImPow(f, 1.0f / power)) * linear_zero_pos;
        } else
        {
            const float f = (float)((v_clamped - ImMax((TYPE)0, v_min)) / (v_max - ImMax((TYPE)0, v_min)));
            return linear_zero_pos + ImPow(f, 1.0f / power) * (1.0f - linear_zero_pos);
        }
    }

    // Linear slider
    return (float)((FLOATTYPE)(v_clamped - v_min) / (FLOATTYPE)(v_max - v_min));
}

// FIXME: Move some of the code into SliderBehavior(). Current responsibility is larger than what the equivalent DragBehaviorT<> does, we also do some rendering, etc.
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool ImGui::SliderBehaviorT(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, TYPE* v, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
    const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
    const bool is_power = (power != 1.0f) && is_decimal;

    const float grab_padding = 2.0f;
    const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
    float grab_sz = style.GrabMinSize;
    SIGNEDTYPE v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
    if (!is_decimal && v_range >= 0)                                             // v_range < 0 may happen on integer overflows
        grab_sz = ImMax((float)(slider_sz / (v_range + 1)), style.GrabMinSize);  // For integer sliders: if possible have the grab size represent 1 unit
    grab_sz = ImMin(grab_sz, slider_sz);
    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
    const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

    // For power curve sliders that cross over sign boundary we want the curve to be symmetric around 0.0f
    float linear_zero_pos;   // 0.0->1.0f
    if (is_power && v_min * v_max < 0.0f)
    {
        // Different sign
        const FLOATTYPE linear_dist_min_to_0 = ImPow(v_min >= 0 ? (FLOATTYPE)v_min : -(FLOATTYPE)v_min, (FLOATTYPE)1.0f / power);
        const FLOATTYPE linear_dist_max_to_0 = ImPow(v_max >= 0 ? (FLOATTYPE)v_max : -(FLOATTYPE)v_max, (FLOATTYPE)1.0f / power);
        linear_zero_pos = (float)(linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0));
    } else
    {
        // Same sign
        linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
    }

    // Process interacting with the slider
    bool value_changed = false;
    if (g.ActiveId == id)
    {
        bool set_new_value = false;
        float clicked_t = 0.0f;
        if (g.ActiveIdSource == ImGuiInputSource_Mouse)
        {
            if (!g.IO.MouseDown[0])
            {
                ClearActiveID();
            } else
            {
                const float mouse_abs_pos = g.IO.MousePos[axis];
                clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
                if (axis == ImGuiAxis_Y)
                    clicked_t = 1.0f - clicked_t;
                set_new_value = true;
            }
        } else if (g.ActiveIdSource == ImGuiInputSource_Nav)
        {
            const ImVec2 delta2 = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 0.0f, 0.0f);
            float delta = (axis == ImGuiAxis_X) ? delta2.x : -delta2.y;
            if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            {
                ClearActiveID();
            } else if (delta != 0.0f)
            {
                clicked_t = SliderCalcRatioFromValueT<TYPE, FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
                const int decimal_precision = is_decimal ? ImParseFormatPrecision(format, 3) : 0;
                if ((decimal_precision > 0) || is_power)
                {
                    delta /= 100.0f;    // Gamepad/keyboard tweak speeds in % of slider bounds
                    if (IsNavInputDown(ImGuiNavInput_TweakSlow))
                        delta /= 10.0f;
                } else
                {
                    if ((v_range >= -100.0f && v_range <= 100.0f) || IsNavInputDown(ImGuiNavInput_TweakSlow))
                        delta = ((delta < 0.0f) ? -1.0f : +1.0f) / (float)v_range; // Gamepad/keyboard tweak speeds in integer steps
                    else
                        delta /= 100.0f;
                }
                if (IsNavInputDown(ImGuiNavInput_TweakFast))
                    delta *= 10.0f;
                set_new_value = true;
                if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f)) // This is to avoid applying the saturation when already past the limits
                    set_new_value = false;
                else
                    clicked_t = ImSaturate(clicked_t + delta);
            }
        }

        if (set_new_value)
        {
            TYPE v_new;
            if (is_power)
            {
                // Account for power curve scale on both sides of the zero
                if (clicked_t < linear_zero_pos)
                {
                    // Negative: rescale to the negative range before powering
                    float a = 1.0f - (clicked_t / linear_zero_pos);
                    a = ImPow(a, power);
                    v_new = ImLerp(ImMin(v_max, (TYPE)0), v_min, a);
                } else
                {
                    // Positive: rescale to the positive range before powering
                    float a;
                    if (ImFabs(linear_zero_pos - 1.0f) > 1.e-6f)
                        a = (clicked_t - linear_zero_pos) / (1.0f - linear_zero_pos);
                    else
                        a = clicked_t;
                    a = ImPow(a, power);
                    v_new = ImLerp(ImMax(v_min, (TYPE)0), v_max, a);
                }
            } else
            {
                // Linear slider
                if (is_decimal)
                {
                    v_new = ImLerp(v_min, v_max, clicked_t);
                } else
                {
                    // For integer values we want the clicking position to match the grab box so we round above
                    // This code is carefully tuned to work with large values (e.g. high ranges of U64) while preserving this property..
                    FLOATTYPE v_new_off_f = (v_max - v_min) * clicked_t;
                    TYPE v_new_off_floor = (TYPE)(v_new_off_f);
                    TYPE v_new_off_round = (TYPE)(v_new_off_f + (FLOATTYPE)0.5);
                    if (v_new_off_floor < v_new_off_round)
                        v_new = v_min + v_new_off_round;
                    else
                        v_new = v_min + v_new_off_floor;
                }
            }

            // Round to user desired precision based on format string
            v_new = RoundScalarWithFormatT<TYPE, SIGNEDTYPE>(format, data_type, v_new);

            // Apply result
            if (*v != v_new)
            {
                *v = v_new;
                value_changed = true;
            }
        }
    }

    if (slider_sz < 1.0f)
    {
        *out_grab_bb = ImRect(bb.Min, bb.Min);
    } else
    {
        // Output grab position so it can be displayed by the caller
        float grab_t = SliderCalcRatioFromValueT<TYPE, FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
        if (axis == ImGuiAxis_Y)
            grab_t = 1.0f - grab_t;
        const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
        if (axis == ImGuiAxis_X)
            *out_grab_bb = ImRect(grab_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
        else
            *out_grab_bb = ImRect(bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f);
    }

    return value_changed;
}

// For 32-bit and larger types, slider bounds are limited to half the natural type range.
// So e.g. an integer Slider between INT_MAX-10 and INT_MAX will fail, but an integer Slider between INT_MAX/2-10 and INT_MAX/2 will be ok.
// It would be possible to lift that limitation with some work but it doesn't seem to be worth it for sliders.
bool ImGui::SliderBehavior(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
    switch (data_type)
    {
    case ImGuiDataType_S8: { ImS32 v32 = (ImS32) * (ImS8*)p_v;  bool r = SliderBehaviorT<ImS32, ImS32, float>(bb, id, ImGuiDataType_S32, &v32, *(const ImS8*)p_min, *(const ImS8*)p_max, format, power, flags, out_grab_bb); if (r) *(ImS8*)p_v = (ImS8)v32;  return r; }
    case ImGuiDataType_U8: { ImU32 v32 = (ImU32) * (ImU8*)p_v;  bool r = SliderBehaviorT<ImU32, ImS32, float>(bb, id, ImGuiDataType_U32, &v32, *(const ImU8*)p_min, *(const ImU8*)p_max, format, power, flags, out_grab_bb); if (r) *(ImU8*)p_v = (ImU8)v32;  return r; }
    case ImGuiDataType_S16: { ImS32 v32 = (ImS32) * (ImS16*)p_v; bool r = SliderBehaviorT<ImS32, ImS32, float>(bb, id, ImGuiDataType_S32, &v32, *(const ImS16*)p_min, *(const ImS16*)p_max, format, power, flags, out_grab_bb); if (r) *(ImS16*)p_v = (ImS16)v32; return r; }
    case ImGuiDataType_U16: { ImU32 v32 = (ImU32) * (ImU16*)p_v; bool r = SliderBehaviorT<ImU32, ImS32, float>(bb, id, ImGuiDataType_U32, &v32, *(const ImU16*)p_min, *(const ImU16*)p_max, format, power, flags, out_grab_bb); if (r) *(ImU16*)p_v = (ImU16)v32; return r; }
    case ImGuiDataType_S32:
        IM_ASSERT(*(const ImS32*)p_min >= IM_S32_MIN / 2 && *(const ImS32*)p_max <= IM_S32_MAX / 2);
        return SliderBehaviorT<ImS32, ImS32, float >(bb, id, data_type, (ImS32*)p_v, *(const ImS32*)p_min, *(const ImS32*)p_max, format, power, flags, out_grab_bb);
    case ImGuiDataType_U32:
        IM_ASSERT(*(const ImU32*)p_max <= IM_U32_MAX / 2);
        return SliderBehaviorT<ImU32, ImS32, float >(bb, id, data_type, (ImU32*)p_v, *(const ImU32*)p_min, *(const ImU32*)p_max, format, power, flags, out_grab_bb);
    case ImGuiDataType_S64:
        IM_ASSERT(*(const ImS64*)p_min >= IM_S64_MIN / 2 && *(const ImS64*)p_max <= IM_S64_MAX / 2);
        return SliderBehaviorT<ImS64, ImS64, double>(bb, id, data_type, (ImS64*)p_v, *(const ImS64*)p_min, *(const ImS64*)p_max, format, power, flags, out_grab_bb);
    case ImGuiDataType_U64:
        IM_ASSERT(*(const ImU64*)p_max <= IM_U64_MAX / 2);
        return SliderBehaviorT<ImU64, ImS64, double>(bb, id, data_type, (ImU64*)p_v, *(const ImU64*)p_min, *(const ImU64*)p_max, format, power, flags, out_grab_bb);
    case ImGuiDataType_Float:
        IM_ASSERT(*(const float*)p_min >= -FLT_MAX / 2.0f && *(const float*)p_max <= FLT_MAX / 2.0f);
        return SliderBehaviorT<float, float, float >(bb, id, data_type, (float*)p_v, *(const float*)p_min, *(const float*)p_max, format, power, flags, out_grab_bb);
    case ImGuiDataType_Double:
        IM_ASSERT(*(const double*)p_min >= -DBL_MAX / 2.0f && *(const double*)p_max <= DBL_MAX / 2.0f);
        return SliderBehaviorT<double, double, double>(bb, id, data_type, (double*)p_v, *(const double*)p_min, *(const double*)p_max, format, power, flags, out_grab_bb);
    case ImGuiDataType_COUNT: break;
    }
    IM_ASSERT(0);
    return false;
}

// Note: p_data, p_min and p_max are _pointers_ to a memory address holding the data. For a slider, they are all required.
// Read code of e.g. SliderFloat(), SliderInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
bool ImGui::SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;
    else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
        format = PatchFormatStringFloatToInt(format);

    // Tabbing or CTRL-clicking on Slider turns it into an input box
    const bool hovered = ItemHoverable(frame_bb, id);
    bool temp_input_is_active = TempInputIsActive(id);
    bool temp_input_start = false;
    if (!temp_input_is_active)
    {
        const bool focus_requested = FocusableItemRegister(window, id);
        const bool clicked = (hovered && g.IO.MouseClicked[0]);
        if (focus_requested || clicked || g.NavActivateId == id || g.NavInputId == id)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            if (focus_requested || (clicked && g.IO.KeyCtrl) || g.NavInputId == id)
            {
                temp_input_start = true;
                FocusableItemUnregister(window);
            }
        }
    }

    // Our current specs do NOT clamp when using CTRL+Click manual input, but we should eventually add a flag for that..
    if (temp_input_is_active || temp_input_start)
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format);// , p_min, p_max);

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, power, ImGuiSliderFlags_None, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x)
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
    return value_changed;
}

// Add multiple sliders on 1 line for compact edition of multiple components
bool ImGui::SliderScalarN(const char* label, ImGuiDataType data_type, void* v, int components, const void* v_min, const void* v_max, const char* format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, CalcItemWidth());
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= SliderScalar("", data_type, v, v_min, v_max, format, power);
        PopID();
        PopItemWidth();
        v = (void*)((char*)v + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end)
    {
        SameLine(0, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}

bool ImGui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
{
    return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, power);
}

bool ImGui::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, float power)
{
    return SliderScalarN(label, ImGuiDataType_Float, v, 2, &v_min, &v_max, format, power);
}

bool ImGui::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, float power)
{
    return SliderScalarN(label, ImGuiDataType_Float, v, 3, &v_min, &v_max, format, power);
}

bool ImGui::SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, float power)
{
    return SliderScalarN(label, ImGuiDataType_Float, v, 4, &v_min, &v_max, format, power);
}

bool ImGui::SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format)
{
    if (format == NULL)
        format = "%.0f deg";
    float v_deg = (*v_rad) * 360.0f / (2 * IM_PI);
    bool value_changed = SliderFloat(label, &v_deg, v_degrees_min, v_degrees_max, format, 1.0f);
    *v_rad = v_deg * (2 * IM_PI) / 360.0f;
    return value_changed;
}

bool ImGui::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
{
    return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format);
}

bool ImGui::SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format)
{
    return SliderScalarN(label, ImGuiDataType_S32, v, 2, &v_min, &v_max, format);
}

bool ImGui::SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format)
{
    return SliderScalarN(label, ImGuiDataType_S32, v, 3, &v_min, &v_max, format);
}

bool ImGui::SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format)
{
    return SliderScalarN(label, ImGuiDataType_S32, v, 4, &v_min, &v_max, format);
}

bool ImGui::VSliderScalar(const char* label, const ImVec2& size, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, id))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;
    else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
        format = PatchFormatStringFloatToInt(format);

    const bool hovered = ItemHoverable(frame_bb, id);
    if ((hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || g.NavInputId == id)
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, power, ImGuiSliderFlags_Vertical, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.y > grab_bb.Min.y)
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    // For the vertical slider we allow centered text to overlap the frame padding
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.0f));
    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    return value_changed;
}

bool ImGui::VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format, float power)
{
    return VSliderScalar(label, size, ImGuiDataType_Float, v, &v_min, &v_max, format, power);
}

bool ImGui::VSliderInt(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format)
{
    return VSliderScalar(label, size, ImGuiDataType_S32, v, &v_min, &v_max, format);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
//-------------------------------------------------------------------------
// - ImParseFormatFindStart() [Internal]
// - ImParseFormatFindEnd() [Internal]
// - ImParseFormatTrimDecorations() [Internal]
// - ImParseFormatPrecision() [Internal]
// - TempInputTextScalar() [Internal]
// - InputScalar()
// - InputScalarN()
// - InputFloat()
// - InputFloat2()
// - InputFloat3()
// - InputFloat4()
// - InputInt()
// - InputInt2()
// - InputInt3()
// - InputInt4()
// - InputDouble()
//-------------------------------------------------------------------------

// We don't use strchr() because our strings are usually very short and often start with '%'
const char* ImParseFormatFindStart(const char* fmt)
{
    while (char c = fmt[0])
    {
        if (c == '%' && fmt[1] != '%')
            return fmt;
        else if (c == '%')
            fmt++;
        fmt++;
    }
    return fmt;
}

const char* ImParseFormatFindEnd(const char* fmt)
{
    // Printf/scanf types modifiers: I/L/h/j/l/t/w/z. Other uppercase letters qualify as types aka end of the format.
    if (fmt[0] != '%')
        return fmt;
    const unsigned int ignored_uppercase_mask = (1 << ('I' - 'A')) | (1 << ('L' - 'A'));
    const unsigned int ignored_lowercase_mask = (1 << ('h' - 'a')) | (1 << ('j' - 'a')) | (1 << ('l' - 'a')) | (1 << ('t' - 'a')) | (1 << ('w' - 'a')) | (1 << ('z' - 'a'));
    for (char c; (c = *fmt) != 0; fmt++)
    {
        if (c >= 'A' && c <= 'Z' && ((1 << (c - 'A')) & ignored_uppercase_mask) == 0)
            return fmt + 1;
        if (c >= 'a' && c <= 'z' && ((1 << (c - 'a')) & ignored_lowercase_mask) == 0)
            return fmt + 1;
    }
    return fmt;
}

// Extract the format out of a format string with leading or trailing decorations
//  fmt = "blah blah"  -> return fmt
//  fmt = "%.3f"       -> return fmt
//  fmt = "hello %.3f" -> return fmt + 6
//  fmt = "%.3f hello" -> return buf written with "%.3f"
const char* ImParseFormatTrimDecorations(const char* fmt, char* buf, size_t buf_size)
{
    const char* fmt_start = ImParseFormatFindStart(fmt);
    if (fmt_start[0] != '%')
        return fmt;
    const char* fmt_end = ImParseFormatFindEnd(fmt_start);
    if (fmt_end[0] == 0) // If we only have leading decoration, we don't need to copy the data.
        return fmt_start;
    ImStrncpy(buf, fmt_start, ImMin((size_t)(fmt_end - fmt_start) + 1, buf_size));
    return buf;
}

// Parse display precision back from the display format string
// FIXME: This is still used by some navigation code path to infer a minimum tweak step, but we should aim to rework widgets so it isn't needed.
int ImParseFormatPrecision(const char* fmt, int default_precision)
{
    fmt = ImParseFormatFindStart(fmt);
    if (fmt[0] != '%')
        return default_precision;
    fmt++;
    while (*fmt >= '0' && *fmt <= '9')
        fmt++;
    int precision = INT_MAX;
    if (*fmt == '.')
    {
        fmt = ImAtoi<int>(fmt + 1, &precision);
        if (precision < 0 || precision > 99)
            precision = default_precision;
    }
    if (*fmt == 'e' || *fmt == 'E') // Maximum precision with scientific notation
        precision = -1;
    if ((*fmt == 'g' || *fmt == 'G') && precision == INT_MAX)
        precision = -1;
    return (precision == INT_MAX) ? default_precision : precision;
}

// Create text input in place of another active widget (e.g. used when doing a CTRL+Click on drag/slider widgets)
// FIXME: Facilitate using this in variety of other situations.
bool ImGui::TempInputText(const ImRect& bb, ImGuiID id, const char* label, char* buf, int buf_size, ImGuiInputTextFlags flags)
{
    // On the first frame, g.TempInputTextId == 0, then on subsequent frames it becomes == id.
    // We clear ActiveID on the first frame to allow the InputText() taking it back.
    ImGuiContext& g = *GImGui;
    const bool init = (g.TempInputId != id);
    if (init)
        ClearActiveID();

    g.CurrentWindow->DC.CursorPos = bb.Min;
    bool value_changed = InputTextEx(label, NULL, buf, buf_size, bb.GetSize(), flags);
    if (init)
    {
        // First frame we started displaying the InputText widget, we expect it to take the active id.
        IM_ASSERT(g.ActiveId == id);
        g.TempInputId = g.ActiveId;
    }
    return value_changed;
}

// Note that Drag/Slider functions are currently NOT forwarding the min/max values clamping values!
// This is intended: this way we allow CTRL+Click manual input to set a value out of bounds, for maximum flexibility.
// However this may not be ideal for all uses, as some user code may break on out of bound values.
// In the future we should add flags to Slider/Drag to specify how to enforce min/max values with CTRL+Click.
// See GitHub issues #1829 and #3209
// In the meanwhile, you can easily "wrap" those functions to enforce clamping, using wrapper functions, e.g.
//   bool SliderFloatClamp(const char* label, float* v, float v_min, float v_max)
//   {
//      float v_backup = *v;
//      if (!SliderFloat(label, v, v_min, v_max))
//         return false;
//      *v = ImClamp(*v, v_min, v_max);
//      return v_backup != *v;
//   }
bool ImGui::TempInputScalar(const ImRect& bb, ImGuiID id, const char* label, ImGuiDataType data_type, void* p_data, const char* format, const void* p_clamp_min, const void* p_clamp_max)
{
    ImGuiContext& g = *GImGui;

    char fmt_buf[32];
    char data_buf[32];
    format = ImParseFormatTrimDecorations(format, fmt_buf, IM_ARRAYSIZE(fmt_buf));
    DataTypeFormatString(data_buf, IM_ARRAYSIZE(data_buf), data_type, p_data, format);
    ImStrTrimBlanks(data_buf);

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoMarkEdited;
    flags |= ((data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double) ? ImGuiInputTextFlags_CharsScientific : ImGuiInputTextFlags_CharsDecimal);
    bool value_changed = false;
    if (TempInputText(bb, id, label, data_buf, IM_ARRAYSIZE(data_buf), flags))
    {
        // Backup old value
        size_t data_type_size = DataTypeGetInfo(data_type)->Size;
        ImGuiDataTypeTempStorage data_backup;
        memcpy(&data_backup, p_data, data_type_size);

        // Apply new value (or operations) then clamp
        DataTypeApplyOpFromText(data_buf, g.InputTextState.InitialTextA.Data, data_type, p_data, NULL);
        if (p_clamp_min && p_clamp_max)
            DataTypeClamp(data_type, p_data, p_clamp_min, p_clamp_max);

        // Only mark as edited if new value is different
        value_changed = memcmp(&data_backup, p_data, data_type_size) != 0;
        if (value_changed)
            MarkItemEdited(id);
    }
    return value_changed;
}

// Note: p_data, p_step, p_step_fast are _pointers_ to a memory address holding the data. For an Input widget, p_step and p_step_fast are optional.
// Read code of e.g. InputFloat(), InputInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
bool ImGui::InputScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    char buf[64];
    DataTypeFormatString(buf, IM_ARRAYSIZE(buf), data_type, p_data, format);

    bool value_changed = false;
    if ((flags & (ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsScientific)) == 0)
        flags |= ImGuiInputTextFlags_CharsDecimal;
    flags |= ImGuiInputTextFlags_AutoSelectAll;
    flags |= ImGuiInputTextFlags_NoMarkEdited;  // We call MarkItemEdited() ourselves by comparing the actual data rather than the string.

    if (p_step != NULL)
    {
        const float button_size = GetFrameHeight();

        BeginGroup(); // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
        PushID(label);
        SetNextItemWidth(ImMax(1.0f, CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2));
        if (InputText("", buf, IM_ARRAYSIZE(buf), flags)) // PushId(label) + "" gives us the expected ID from outside point of view
            value_changed = DataTypeApplyOpFromText(buf, g.InputTextState.InitialTextA.Data, data_type, p_data, format);

        // Step buttons
        const ImVec2 backup_frame_padding = style.FramePadding;
        style.FramePadding.x = style.FramePadding.y;
        ImGuiButtonFlags button_flags = ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups;
        if (flags & ImGuiInputTextFlags_ReadOnly)
            button_flags |= ImGuiButtonFlags_Disabled;
        SameLine(0, style.ItemInnerSpacing.x);
        if (ButtonEx("-", ImVec2(button_size, button_size), button_flags))
        {
            DataTypeApplyOp(data_type, '-', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
            value_changed = true;
        }
        SameLine(0, style.ItemInnerSpacing.x);
        if (ButtonEx("+", ImVec2(button_size, button_size), button_flags))
        {
            DataTypeApplyOp(data_type, '+', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
            value_changed = true;
        }

        const char* label_end = FindRenderedTextEnd(label);
        if (label != label_end)
        {
            SameLine(0, style.ItemInnerSpacing.x);
            TextEx(label, label_end);
        }
        style.FramePadding = backup_frame_padding;

        PopID();
        EndGroup();
    } else
    {
        if (InputText(label, buf, IM_ARRAYSIZE(buf), flags))
            value_changed = DataTypeApplyOpFromText(buf, g.InputTextState.InitialTextA.Data, data_type, p_data, format);
    }
    if (value_changed)
        MarkItemEdited(window->DC.LastItemId);

    return value_changed;
}

bool ImGui::InputScalarN(const char* label, ImGuiDataType data_type, void* p_data, int components, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, CalcItemWidth());
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= InputScalar("", data_type, p_data, p_step, p_step_fast, format, flags);
        PopID();
        PopItemWidth();
        p_data = (void*)((char*)p_data + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end)
    {
        SameLine(0.0f, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}

bool ImGui::InputFloat(const char* label, float* v, float step, float step_fast, const char* format, ImGuiInputTextFlags flags)
{
    flags |= ImGuiInputTextFlags_CharsScientific;
    return InputScalar(label, ImGuiDataType_Float, (void*)v, (void*)(step > 0.0f ? &step : NULL), (void*)(step_fast > 0.0f ? &step_fast : NULL), format, flags);
}

bool ImGui::InputFloat2(const char* label, float v[2], const char* format, ImGuiInputTextFlags flags)
{
    return InputScalarN(label, ImGuiDataType_Float, v, 2, NULL, NULL, format, flags);
}

bool ImGui::InputFloat3(const char* label, float v[3], const char* format, ImGuiInputTextFlags flags)
{
    return InputScalarN(label, ImGuiDataType_Float, v, 3, NULL, NULL, format, flags);
}

bool ImGui::InputFloat4(const char* label, float v[4], const char* format, ImGuiInputTextFlags flags)
{
    return InputScalarN(label, ImGuiDataType_Float, v, 4, NULL, NULL, format, flags);
}

// Prefer using "const char* format" directly, which is more flexible and consistent with other API.
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
bool ImGui::InputFloat(const char* label, float* v, float step, float step_fast, int decimal_precision, ImGuiInputTextFlags flags)
{
    char format[16] = "%f";
    if (decimal_precision >= 0)
        ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
    return InputFloat(label, v, step, step_fast, format, flags);
}

bool ImGui::InputFloat2(const char* label, float v[2], int decimal_precision, ImGuiInputTextFlags flags)
{
    char format[16] = "%f";
    if (decimal_precision >= 0)
        ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
    return InputScalarN(label, ImGuiDataType_Float, v, 2, NULL, NULL, format, flags);
}

bool ImGui::InputFloat3(const char* label, float v[3], int decimal_precision, ImGuiInputTextFlags flags)
{
    char format[16] = "%f";
    if (decimal_precision >= 0)
        ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
    return InputScalarN(label, ImGuiDataType_Float, v, 3, NULL, NULL, format, flags);
}

bool ImGui::InputFloat4(const char* label, float v[4], int decimal_precision, ImGuiInputTextFlags flags)
{
    char format[16] = "%f";
    if (decimal_precision >= 0)
        ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
    return InputScalarN(label, ImGuiDataType_Float, v, 4, NULL, NULL, format, flags);
}
#endif // IMGUI_DISABLE_OBSOLETE_FUNCTIONS

bool ImGui::InputInt(const char* label, int* v, int step, int step_fast, ImGuiInputTextFlags flags)
{
    // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
    const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
    return InputScalar(label, ImGuiDataType_S32, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
}

bool ImGui::InputInt2(const char* label, int v[2], ImGuiInputTextFlags flags)
{
    return InputScalarN(label, ImGuiDataType_S32, v, 2, NULL, NULL, "%d", flags);
}

bool ImGui::InputInt3(const char* label, int v[3], ImGuiInputTextFlags flags)
{
    return InputScalarN(label, ImGuiDataType_S32, v, 3, NULL, NULL, "%d", flags);
}

bool ImGui::InputInt4(const char* label, int v[4], ImGuiInputTextFlags flags)
{
    return InputScalarN(label, ImGuiDataType_S32, v, 4, NULL, NULL, "%d", flags);
}

bool ImGui::InputDouble(const char* label, double* v, double step, double step_fast, const char* format, ImGuiInputTextFlags flags)
{
    flags |= ImGuiInputTextFlags_CharsScientific;
    return InputScalar(label, ImGuiDataType_Double, (void*)v, (void*)(step > 0.0 ? &step : NULL), (void*)(step_fast > 0.0 ? &step_fast : NULL), format, flags);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: InputText, InputTextMultiline, InputTextWithHint
//-------------------------------------------------------------------------
// - InputText()
// - InputTextWithHint()
// - InputTextMultiline()
// - InputTextEx() [Internal]
//-------------------------------------------------------------------------

bool ImGui::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()
    return InputTextEx(label, NULL, buf, (int)buf_size, ImVec2(0, 0), flags, callback, user_data);
}

bool ImGui::InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    return InputTextEx(label, NULL, buf, (int)buf_size, size, flags | ImGuiInputTextFlags_Multiline, callback, user_data);
}

bool ImGui::InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()
    return InputTextEx(label, hint, buf, (int)buf_size, ImVec2(0, 0), flags, callback, user_data);
}

static int InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end)
{
    int line_count = 0;
    const char* s = text_begin;
    while (char c = *s++) // We are only matching for \n so we can ignore UTF-8 decoding
        if (c == '\n')
            line_count++;
    s--;
    if (s[0] != '\n' && s[0] != '\r')
        line_count++;
    *out_text_end = s;
    return line_count;
}

static ImVec2 InputTextCalcTextSizeW(const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining, ImVec2* out_offset, bool stop_on_new_line)
{
    ImGuiContext& g = *GImGui;
    ImFont* font = g.Font;
    const float line_height = g.FontSize;
    const float scale = line_height / font->FontSize;

    ImVec2 text_size = ImVec2(0, 0);
    float line_width = 0.0f;

    const ImWchar* s = text_begin;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)(*s++);
        if (c == '\n')
        {
            text_size.x = ImMax(text_size.x, line_width);
            text_size.y += line_height;
            line_width = 0.0f;
            if (stop_on_new_line)
                break;
            continue;
        }
        if (c == '\r')
            continue;

        const float char_width = font->GetCharAdvance((ImWchar)c) * scale;
        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (out_offset)
        *out_offset = ImVec2(line_width, text_size.y + line_height);  // offset allow for the possibility of sitting after a trailing \n

    if (line_width > 0 || text_size.y == 0.0f)                        // whereas size.y will ignore the trailing \n
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

// Wrapper for stb_textedit.h to edit text (our wrapper is for: statically sized buffer, single-line, wchar characters. InputText converts between UTF-8 and wchar)
namespace ImStb
{

    static int     STB_TEXTEDIT_STRINGLEN(const STB_TEXTEDIT_STRING* obj) { return obj->CurLenW; }
    static ImWchar STB_TEXTEDIT_GETCHAR(const STB_TEXTEDIT_STRING* obj, int idx) { return obj->TextW[idx]; }
    static float   STB_TEXTEDIT_GETWIDTH(STB_TEXTEDIT_STRING* obj, int line_start_idx, int char_idx) { ImWchar c = obj->TextW[line_start_idx + char_idx]; if (c == '\n') return STB_TEXTEDIT_GETWIDTH_NEWLINE; ImGuiContext& g = *GImGui; return g.Font->GetCharAdvance(c) * (g.FontSize / g.Font->FontSize); }
    static int     STB_TEXTEDIT_KEYTOTEXT(int key) { return key >= 0x200000 ? 0 : key; }
    static ImWchar STB_TEXTEDIT_NEWLINE = '\n';
    static void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, STB_TEXTEDIT_STRING* obj, int line_start_idx)
    {
        const ImWchar* text = obj->TextW.Data;
        const ImWchar* text_remaining = NULL;
        const ImVec2 size = InputTextCalcTextSizeW(text + line_start_idx, text + obj->CurLenW, &text_remaining, NULL, true);
        r->x0 = 0.0f;
        r->x1 = size.x;
        r->baseline_y_delta = size.y;
        r->ymin = 0.0f;
        r->ymax = size.y;
        r->num_chars = (int)(text_remaining - (text + line_start_idx));
    }

    static bool is_separator(unsigned int c) { return ImCharIsBlankW(c) || c == ',' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == '|'; }
    static int  is_word_boundary_from_right(STB_TEXTEDIT_STRING* obj, int idx) { return idx > 0 ? (is_separator(obj->TextW[idx - 1]) && !is_separator(obj->TextW[idx])) : 1; }
    static int  STB_TEXTEDIT_MOVEWORDLEFT_IMPL(STB_TEXTEDIT_STRING* obj, int idx) { idx--; while (idx >= 0 && !is_word_boundary_from_right(obj, idx)) idx--; return idx < 0 ? 0 : idx; }
#ifdef __APPLE__    // FIXME: Move setting to IO structure
    static int  is_word_boundary_from_left(STB_TEXTEDIT_STRING* obj, int idx) { return idx > 0 ? (!is_separator(obj->TextW[idx - 1]) && is_separator(obj->TextW[idx])) : 1; }
    static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(STB_TEXTEDIT_STRING* obj, int idx) { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_left(obj, idx)) idx++; return idx > len ? len : idx; }
#else
    static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(STB_TEXTEDIT_STRING* obj, int idx) { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_right(obj, idx)) idx++; return idx > len ? len : idx; }
#endif
#define STB_TEXTEDIT_MOVEWORDLEFT   STB_TEXTEDIT_MOVEWORDLEFT_IMPL    // They need to be #define for stb_textedit.h
#define STB_TEXTEDIT_MOVEWORDRIGHT  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL

    static void STB_TEXTEDIT_DELETECHARS(STB_TEXTEDIT_STRING* obj, int pos, int n)
    {
        ImWchar* dst = obj->TextW.Data + pos;

        // We maintain our buffer length in both UTF-8 and wchar formats
        obj->CurLenA -= ImTextCountUtf8BytesFromStr(dst, dst + n);
        obj->CurLenW -= n;

        // Offset remaining text (FIXME-OPT: Use memmove)
        const ImWchar* src = obj->TextW.Data + pos + n;
        while (ImWchar c = *src++)
            *dst++ = c;
        *dst = '\0';
    }

    static bool STB_TEXTEDIT_INSERTCHARS(STB_TEXTEDIT_STRING* obj, int pos, const ImWchar* new_text, int new_text_len)
    {
        const bool is_resizable = (obj->UserFlags & ImGuiInputTextFlags_CallbackResize) != 0;
        const int text_len = obj->CurLenW;
        IM_ASSERT(pos <= text_len);

        const int new_text_len_utf8 = ImTextCountUtf8BytesFromStr(new_text, new_text + new_text_len);
        if (!is_resizable && (new_text_len_utf8 + obj->CurLenA + 1 > obj->BufCapacityA))
            return false;

        // Grow internal buffer if needed
        if (new_text_len + text_len + 1 > obj->TextW.Size)
        {
            if (!is_resizable)
                return false;
            IM_ASSERT(text_len < obj->TextW.Size);
            obj->TextW.resize(text_len + ImClamp(new_text_len * 4, 32, ImMax(256, new_text_len)) + 1);
        }

        ImWchar* text = obj->TextW.Data;
        if (pos != text_len)
            memmove(text + pos + new_text_len, text + pos, (size_t)(text_len - pos) * sizeof(ImWchar));
        memcpy(text + pos, new_text, (size_t)new_text_len * sizeof(ImWchar));

        obj->CurLenW += new_text_len;
        obj->CurLenA += new_text_len_utf8;
        obj->TextW[obj->CurLenW] = '\0';

        return true;
    }

    // We don't use an enum so we can build even with conflicting symbols (if another user of stb_textedit.h leak their STB_TEXTEDIT_K_* symbols)
#define STB_TEXTEDIT_K_LEFT         0x200000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT        0x200001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           0x200002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         0x200003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    0x200004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      0x200005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    0x200006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      0x200007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       0x200008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    0x200009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         0x20000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO         0x20000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT     0x20000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT    0x20000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_SHIFT        0x400000

#define STB_TEXTEDIT_IMPLEMENTATION
#include "imstb_textedit.h"

// stb_textedit internally allows for a single undo record to do addition and deletion, but somehow, calling
// the stb_textedit_paste() function creates two separate records, so we perform it manually. (FIXME: Report to nothings/stb?)
    static void stb_textedit_replace(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, const STB_TEXTEDIT_CHARTYPE* text, int text_len)
    {
        stb_text_makeundo_replace(str, state, 0, str->CurLenW, text_len);
        ImStb::STB_TEXTEDIT_DELETECHARS(str, 0, str->CurLenW);
        if (text_len <= 0)
            return;
        if (ImStb::STB_TEXTEDIT_INSERTCHARS(str, 0, text, text_len))
        {
            state->cursor = text_len;
            state->has_preferred_x = 0;
            return;
        }
        IM_ASSERT(0); // Failed to insert character, normally shouldn't happen because of how we currently use stb_textedit_replace()
    }

} // namespace ImStb

void ImGuiInputTextState::OnKeyPressed(int key)
{
    stb_textedit_key(this, &Stb, key);
    CursorFollow = true;
    CursorAnimReset();
}

ImGuiInputTextCallbackData::ImGuiInputTextCallbackData()
{
    memset(this, 0, sizeof(*this));
}

// Public API to manipulate UTF-8 text
// We expose UTF-8 to the user (unlike the STB_TEXTEDIT_* functions which are manipulating wchar)
// FIXME: The existence of this rarely exercised code path is a bit of a nuisance.
void ImGuiInputTextCallbackData::DeleteChars(int pos, int bytes_count)
{
    IM_ASSERT(pos + bytes_count <= BufTextLen);
    char* dst = Buf + pos;
    const char* src = Buf + pos + bytes_count;
    while (char c = *src++)
        *dst++ = c;
    *dst = '\0';

    if (CursorPos + bytes_count >= pos)
        CursorPos -= bytes_count;
    else if (CursorPos >= pos)
        CursorPos = pos;
    SelectionStart = SelectionEnd = CursorPos;
    BufDirty = true;
    BufTextLen -= bytes_count;
}

void ImGuiInputTextCallbackData::InsertChars(int pos, const char* new_text, const char* new_text_end)
{
    const bool is_resizable = (Flags & ImGuiInputTextFlags_CallbackResize) != 0;
    const int new_text_len = new_text_end ? (int)(new_text_end - new_text) : (int)strlen(new_text);
    if (new_text_len + BufTextLen >= BufSize)
    {
        if (!is_resizable)
            return;

        // Contrary to STB_TEXTEDIT_INSERTCHARS() this is working in the UTF8 buffer, hence the mildly similar code (until we remove the U16 buffer altogether!)
        ImGuiContext& g = *GImGui;
        ImGuiInputTextState* edit_state = &g.InputTextState;
        IM_ASSERT(edit_state->ID != 0 && g.ActiveId == edit_state->ID);
        IM_ASSERT(Buf == edit_state->TextA.Data);
        int new_buf_size = BufTextLen + ImClamp(new_text_len * 4, 32, ImMax(256, new_text_len)) + 1;
        edit_state->TextA.reserve(new_buf_size + 1);
        Buf = edit_state->TextA.Data;
        BufSize = edit_state->BufCapacityA = new_buf_size;
    }

    if (BufTextLen != pos)
        memmove(Buf + pos + new_text_len, Buf + pos, (size_t)(BufTextLen - pos));
    memcpy(Buf + pos, new_text, (size_t)new_text_len * sizeof(char));
    Buf[BufTextLen + new_text_len] = '\0';

    if (CursorPos >= pos)
        CursorPos += new_text_len;
    SelectionStart = SelectionEnd = CursorPos;
    BufDirty = true;
    BufTextLen += new_text_len;
}

// Return false to discard a character.
static bool InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    unsigned int c = *p_char;

    // Filter non-printable (NB: isprint is unreliable! see #2467)
    if (c < 0x20)
    {
        bool pass = false;
        pass |= (c == '\n' && (flags & ImGuiInputTextFlags_Multiline));
        pass |= (c == '\t' && (flags & ImGuiInputTextFlags_AllowTabInput));
        if (!pass)
            return false;
    }

    // We ignore Ascii representation of delete (emitted from Backspace on OSX, see #2578, #2817)
    if (c == 127)
        return false;

    // Filter private Unicode range. GLFW on OSX seems to send private characters for special keys like arrow keys (FIXME)
    if (c >= 0xE000 && c <= 0xF8FF)
        return false;

    // Filter Unicode ranges we are not handling in this build.
    if (c > IM_UNICODE_CODEPOINT_MAX)
        return false;

    // Generic named filters
    if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsScientific))
    {
        if (flags & ImGuiInputTextFlags_CharsDecimal)
            if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsScientific)
            if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/') && (c != 'e') && (c != 'E'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsHexadecimal)
            if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsUppercase)
            if (c >= 'a' && c <= 'z')
                *p_char = (c += (unsigned int)('A' - 'a'));

        if (flags & ImGuiInputTextFlags_CharsNoBlank)
            if (ImCharIsBlankW(c))
                return false;
    }

    // Custom callback filter
    if (flags & ImGuiInputTextFlags_CallbackCharFilter)
    {
        ImGuiInputTextCallbackData callback_data;
        memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
        callback_data.EventFlag = ImGuiInputTextFlags_CallbackCharFilter;
        callback_data.EventChar = (ImWchar)c;
        callback_data.Flags = flags;
        callback_data.UserData = user_data;
        if (callback(&callback_data) != 0)
            return false;
        *p_char = callback_data.EventChar;
        if (!callback_data.EventChar)
            return false;
    }

    return true;
}

// Edit a string of text
// - buf_size account for the zero-terminator, so a buf_size of 6 can hold "Hello" but not "Hello!".
//   This is so we can easily call InputText() on static arrays using ARRAYSIZE() and to match
//   Note that in std::string world, capacity() would omit 1 byte used by the zero-terminator.
// - When active, hold on a privately held copy of the text (and apply back to 'buf'). So changing 'buf' while the InputText is active has no effect.
// - If you want to use ImGui::InputText() with std::string, see misc/cpp/imgui_stdlib.h
// (FIXME: Rather confusing and messy function, among the worse part of our codebase, expecting to rewrite a V2 at some point.. Partly because we are
//  doing UTF8 > U16 > UTF8 conversions on the go to easily interface with stb_textedit. Ideally should stay in UTF-8 all the time. See https://github.com/nothings/stb/issues/188)
bool ImGui::InputTextEx(const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    IM_ASSERT(buf != NULL && buf_size >= 0);
    IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackHistory) && (flags & ImGuiInputTextFlags_Multiline)));        // Can't use both together (they both use up/down keys)
    IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackCompletion) && (flags & ImGuiInputTextFlags_AllowTabInput))); // Can't use both together (they both use tab key)

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    const bool RENDER_SELECTION_WHEN_INACTIVE = false;
    const bool is_multiline = (flags & ImGuiInputTextFlags_Multiline) != 0;
    const bool is_readonly = (flags & ImGuiInputTextFlags_ReadOnly) != 0;
    const bool is_password = (flags & ImGuiInputTextFlags_Password) != 0;
    const bool is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;
    const bool is_resizable = (flags & ImGuiInputTextFlags_CallbackResize) != 0;
    if (is_resizable)
        IM_ASSERT(callback != NULL); // Must provide a callback if you set the ImGuiInputTextFlags_CallbackResize flag!

    if (is_multiline) // Open group before calling GetID() because groups tracks id created within their scope,
        BeginGroup();
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 frame_size = CalcItemSize(size_arg, CalcItemWidth(), (is_multiline ? g.FontSize * 8.0f : label_size.y) + style.FramePadding.y * 2.0f); // Arbitrary default of 8 lines high for multi-line
    const ImVec2 total_size = ImVec2(frame_size.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_size.y);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const ImRect total_bb(frame_bb.Min, frame_bb.Min + total_size);

    ImGuiWindow* draw_window = window;
    ImVec2 inner_size = frame_size;
    if (is_multiline)
    {
        if (!ItemAdd(total_bb, id, &frame_bb))
        {
            ItemSize(total_bb, style.FramePadding.y);
            EndGroup();
            return false;
        }

        // We reproduce the contents of BeginChildFrame() in order to provide 'label' so our window internal data are easier to read/debug.
        PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
        PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
        PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
        PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
        bool child_visible = BeginChildEx(label, id, frame_bb.GetSize(), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
        PopStyleVar(3);
        PopStyleColor();
        if (!child_visible)
        {
            EndChild();
            EndGroup();
            return false;
        }
        draw_window = g.CurrentWindow; // Child window
        draw_window->DC.NavLayerActiveMaskNext |= draw_window->DC.NavLayerCurrentMask; // This is to ensure that EndChild() will display a navigation highlight so we can "enter" into it.
        inner_size.x -= draw_window->ScrollbarSizes.x;
    } else
    {
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id, &frame_bb))
            return false;
    }
    const bool hovered = ItemHoverable(frame_bb, id);
    if (hovered)
        g.MouseCursor = ImGuiMouseCursor_TextInput;

    // We are only allowed to access the state if we are already the active widget.
    ImGuiInputTextState* state = GetInputTextState(id);

    const bool focus_requested = FocusableItemRegister(window, id);
    const bool focus_requested_by_code = focus_requested && (g.FocusRequestCurrWindow == window && g.FocusRequestCurrCounterRegular == window->DC.FocusCounterRegular);
    const bool focus_requested_by_tab = focus_requested && !focus_requested_by_code;

    const bool user_clicked = hovered && io.MouseClicked[0];
    const bool user_nav_input_start = (g.ActiveId != id) && ((g.NavInputId == id) || (g.NavActivateId == id && g.NavInputSource == ImGuiInputSource_NavKeyboard));
    const bool user_scroll_finish = is_multiline && state != NULL && g.ActiveId == 0 && g.ActiveIdPreviousFrame == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
    const bool user_scroll_active = is_multiline && state != NULL && g.ActiveId == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);

    bool clear_active_id = false;
    bool select_all = (g.ActiveId != id) && ((flags & ImGuiInputTextFlags_AutoSelectAll) != 0 || user_nav_input_start) && (!is_multiline);

    const bool init_make_active = (focus_requested || user_clicked || user_scroll_finish || user_nav_input_start);
    const bool init_state = (init_make_active || user_scroll_active);
    if (init_state && g.ActiveId != id)
    {
        // Access state even if we don't own it yet.
        state = &g.InputTextState;
        state->CursorAnimReset();

        // Take a copy of the initial buffer value (both in original UTF-8 format and converted to wchar)
        // From the moment we focused we are ignoring the content of 'buf' (unless we are in read-only mode)
        const int buf_len = (int)strlen(buf);
        state->InitialTextA.resize(buf_len + 1);    // UTF-8. we use +1 to make sure that .Data is always pointing to at least an empty string.
        memcpy(state->InitialTextA.Data, buf, buf_len + 1);

        // Start edition
        const char* buf_end = NULL;
        state->TextW.resize(buf_size + 1);          // wchar count <= UTF-8 count. we use +1 to make sure that .Data is always pointing to at least an empty string.
        state->TextA.resize(0);
        state->TextAIsValid = false;                // TextA is not valid yet (we will display buf until then)
        state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, buf_size, buf, NULL, &buf_end);
        state->CurLenA = (int)(buf_end - buf);      // We can't get the result from ImStrncpy() above because it is not UTF-8 aware. Here we'll cut off malformed UTF-8.

        // Preserve cursor position and undo/redo stack if we come back to same widget
        // FIXME: For non-readonly widgets we might be able to require that TextAIsValid && TextA == buf ? (untested) and discard undo stack if user buffer has changed.
        const bool recycle_state = (state->ID == id);
        if (recycle_state)
        {
            // Recycle existing cursor/selection/undo stack but clamp position
            // Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
            state->CursorClamp();
        } else
        {
            state->ID = id;
            state->ScrollX = 0.0f;
            stb_textedit_initialize_state(&state->Stb, !is_multiline);
            if (!is_multiline && focus_requested_by_code)
                select_all = true;
        }
        if (flags & ImGuiInputTextFlags_AlwaysInsertMode)
            state->Stb.insert_mode = 1;
        if (!is_multiline && (focus_requested_by_tab || (user_clicked && io.KeyCtrl)))
            select_all = true;
    }

    if (g.ActiveId != id && init_make_active)
    {
        IM_ASSERT(state && state->ID == id);
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);

        // Declare our inputs
        IM_ASSERT(ImGuiNavInput_COUNT < 32);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        if (is_multiline || (flags & ImGuiInputTextFlags_CallbackHistory))
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
        g.ActiveIdUsingNavInputMask |= (1 << ImGuiNavInput_Cancel);
        g.ActiveIdUsingKeyInputMask |= ((ImU64)1 << ImGuiKey_Home) | ((ImU64)1 << ImGuiKey_End);
        if (is_multiline)
            g.ActiveIdUsingKeyInputMask |= ((ImU64)1 << ImGuiKey_PageUp) | ((ImU64)1 << ImGuiKey_PageDown); // FIXME-NAV: Page up/down actually not supported yet by widget, but claim them ahead.
        if (flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_AllowTabInput))  // Disable keyboard tabbing out as we will use the \t character.
            g.ActiveIdUsingKeyInputMask |= ((ImU64)1 << ImGuiKey_Tab);
    }

    // We have an edge case if ActiveId was set through another widget (e.g. widget being swapped), clear id immediately (don't wait until the end of the function)
    if (g.ActiveId == id && state == NULL)
        ClearActiveID();

    // Release focus when we click outside
    if (g.ActiveId == id && io.MouseClicked[0] && !init_state && !init_make_active) //-V560
        clear_active_id = true;

    // Lock the decision of whether we are going to take the path displaying the cursor or selection
    const bool render_cursor = (g.ActiveId == id) || (state && user_scroll_active);
    bool render_selection = state && state->HasSelection() && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
    bool value_changed = false;
    bool enter_pressed = false;

    // When read-only we always use the live data passed to the function
    // FIXME-OPT: Because our selection/cursor code currently needs the wide text we need to convert it when active, which is not ideal :(
    if (is_readonly && state != NULL && (render_cursor || render_selection))
    {
        const char* buf_end = NULL;
        state->TextW.resize(buf_size + 1);
        state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, state->TextW.Size, buf, NULL, &buf_end);
        state->CurLenA = (int)(buf_end - buf);
        state->CursorClamp();
        render_selection &= state->HasSelection();
    }

    // Select the buffer to render.
    const bool buf_display_from_state = (render_cursor || render_selection || g.ActiveId == id) && !is_readonly && state && state->TextAIsValid;
    const bool is_displaying_hint = (hint != NULL && (buf_display_from_state ? state->TextA.Data : buf)[0] == 0);

    // Password pushes a temporary font with only a fallback glyph
    if (is_password && !is_displaying_hint)
    {
        const ImFontGlyph* glyph = g.Font->FindGlyph('*');
        ImFont* password_font = &g.InputTextPasswordFont;
        password_font->FontSize = g.Font->FontSize;
        password_font->Scale = g.Font->Scale;
        password_font->DisplayOffset = g.Font->DisplayOffset;
        password_font->Ascent = g.Font->Ascent;
        password_font->Descent = g.Font->Descent;
        password_font->ContainerAtlas = g.Font->ContainerAtlas;
        password_font->FallbackGlyph = glyph;
        password_font->FallbackAdvanceX = glyph->AdvanceX;
        IM_ASSERT(password_font->Glyphs.empty() && password_font->IndexAdvanceX.empty() && password_font->IndexLookup.empty());
        PushFont(password_font);
    }

    // Process mouse inputs and character inputs
    int backup_current_text_length = 0;
    if (g.ActiveId == id)
    {
        IM_ASSERT(state != NULL);
        backup_current_text_length = state->CurLenA;
        state->BufCapacityA = buf_size;
        state->UserFlags = flags;
        state->UserCallback = callback;
        state->UserCallbackData = callback_user_data;

        // Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
        // Down the line we should have a cleaner library-wide concept of Selected vs Active.
        g.ActiveIdAllowOverlap = !io.MouseDown[0];
        g.WantTextInputNextFrame = 1;

        // Edit in progress
        const float mouse_x = (io.MousePos.x - frame_bb.Min.x - style.FramePadding.x) + state->ScrollX;
        const float mouse_y = (is_multiline ? (io.MousePos.y - draw_window->DC.CursorPos.y - style.FramePadding.y) : (g.FontSize * 0.5f));

        const bool is_osx = io.ConfigMacOSXBehaviors;
        if (select_all || (hovered && !is_osx && io.MouseDoubleClicked[0]))
        {
            state->SelectAll();
            state->SelectedAllMouseLock = true;
        } else if (hovered && is_osx && io.MouseDoubleClicked[0])
        {
            // Double-click select a word only, OS X style (by simulating keystrokes)
            state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);
            state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
        } else if (io.MouseClicked[0] && !state->SelectedAllMouseLock)
        {
            if (hovered)
            {
                stb_textedit_click(state, &state->Stb, mouse_x, mouse_y);
                state->CursorAnimReset();
            }
        } else if (io.MouseDown[0] && !state->SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
        {
            stb_textedit_drag(state, &state->Stb, mouse_x, mouse_y);
            state->CursorAnimReset();
            state->CursorFollow = true;
        }
        if (state->SelectedAllMouseLock && !io.MouseDown[0])
            state->SelectedAllMouseLock = false;

        // It is ill-defined whether the back-end needs to send a \t character when pressing the TAB keys.
        // Win32 and GLFW naturally do it but not SDL.
        const bool ignore_char_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeySuper);
        if ((flags & ImGuiInputTextFlags_AllowTabInput) && IsKeyPressedMap(ImGuiKey_Tab) && !ignore_char_inputs && !io.KeyShift && !is_readonly)
            if (!io.InputQueueCharacters.contains('\t'))
            {
                unsigned int c = '\t'; // Insert TAB
                if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
                    state->OnKeyPressed((int)c);
            }

        // Process regular text input (before we check for Return because using some IME will effectively send a Return?)
        // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
        if (io.InputQueueCharacters.Size > 0)
        {
            if (!ignore_char_inputs && !is_readonly && !user_nav_input_start)
                for (int n = 0; n < io.InputQueueCharacters.Size; n++)
                {
                    // Insert character if they pass filtering
                    unsigned int c = (unsigned int)io.InputQueueCharacters[n];
                    if (c == '\t' && io.KeyShift)
                        continue;
                    if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
                        state->OnKeyPressed((int)c);
                }

            // Consume characters
            io.InputQueueCharacters.resize(0);
        }
    }

    // Process other shortcuts/key-presses
    bool cancel_edit = false;
    if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
    {
        IM_ASSERT(state != NULL);
        IM_ASSERT(io.KeyMods == GetMergedKeyModFlags() && "Mismatching io.KeyCtrl/io.KeyShift/io.KeyAlt/io.KeySuper vs io.KeyMods"); // We rarely do this check, but if anything let's do it here.

        const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
        const bool is_osx = io.ConfigMacOSXBehaviors;
        const bool is_osx_shift_shortcut = is_osx && (io.KeyMods == (ImGuiKeyModFlags_Super | ImGuiKeyModFlags_Shift));
        const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
        const bool is_startend_key_down = is_osx && io.KeySuper && !io.KeyCtrl && !io.KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End
        const bool is_ctrl_key_only = (io.KeyMods == ImGuiKeyModFlags_Ctrl);
        const bool is_shift_key_only = (io.KeyMods == ImGuiKeyModFlags_Shift);
        const bool is_shortcut_key = g.IO.ConfigMacOSXBehaviors ? (io.KeyMods == ImGuiKeyModFlags_Super) : (io.KeyMods == ImGuiKeyModFlags_Ctrl);

        const bool is_cut = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_X)) || (is_shift_key_only && IsKeyPressedMap(ImGuiKey_Delete))) && !is_readonly && !is_password && (!is_multiline || state->HasSelection());
        const bool is_copy = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_C)) || (is_ctrl_key_only && IsKeyPressedMap(ImGuiKey_Insert))) && !is_password && (!is_multiline || state->HasSelection());
        const bool is_paste = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_V)) || (is_shift_key_only && IsKeyPressedMap(ImGuiKey_Insert))) && !is_readonly;
        const bool is_undo = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_Z)) && !is_readonly && is_undoable);
        const bool is_redo = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_Y)) || (is_osx_shift_shortcut && IsKeyPressedMap(ImGuiKey_Z))) && !is_readonly && is_undoable;

        if (IsKeyPressedMap(ImGuiKey_LeftArrow)) { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask); } else if (IsKeyPressedMap(ImGuiKey_RightArrow)) { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask); } else if (IsKeyPressedMap(ImGuiKey_UpArrow) && is_multiline) { if (io.KeyCtrl) SetScrollY(draw_window, ImMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask); } else if (IsKeyPressedMap(ImGuiKey_DownArrow) && is_multiline) { if (io.KeyCtrl) SetScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY())); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask); } else if (IsKeyPressedMap(ImGuiKey_Home)) { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); } else if (IsKeyPressedMap(ImGuiKey_End)) { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); } else if (IsKeyPressedMap(ImGuiKey_Delete) && !is_readonly) { state->OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask); } else if (IsKeyPressedMap(ImGuiKey_Backspace) && !is_readonly)
        {
            if (!state->HasSelection())
            {
                if (is_wordmove_key_down)
                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
                else if (is_osx && io.KeySuper && !io.KeyAlt && !io.KeyCtrl)
                    state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
            }
            state->OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask);
        } else if (IsKeyPressedMap(ImGuiKey_Enter) || IsKeyPressedMap(ImGuiKey_KeyPadEnter))
        {
            bool ctrl_enter_for_new_line = (flags & ImGuiInputTextFlags_CtrlEnterForNewLine) != 0;
            if (!is_multiline || (ctrl_enter_for_new_line && !io.KeyCtrl) || (!ctrl_enter_for_new_line && io.KeyCtrl))
            {
                enter_pressed = clear_active_id = true;
            } else if (!is_readonly)
            {
                unsigned int c = '\n'; // Insert new line
                if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
                    state->OnKeyPressed((int)c);
            }
        } else if (IsKeyPressedMap(ImGuiKey_Escape))
        {
            clear_active_id = cancel_edit = true;
        } else if (is_undo || is_redo)
        {
            state->OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
            state->ClearSelection();
        } else if (is_shortcut_key && IsKeyPressedMap(ImGuiKey_A))
        {
            state->SelectAll();
            state->CursorFollow = true;
        } else if (is_cut || is_copy)
        {
            // Cut, Copy
            if (io.SetClipboardTextFn)
            {
                const int ib = state->HasSelection() ? ImMin(state->Stb.select_start, state->Stb.select_end) : 0;
                const int ie = state->HasSelection() ? ImMax(state->Stb.select_start, state->Stb.select_end) : state->CurLenW;
                const int clipboard_data_len = ImTextCountUtf8BytesFromStr(state->TextW.Data + ib, state->TextW.Data + ie) + 1;
                char* clipboard_data = (char*)IM_ALLOC(clipboard_data_len * sizeof(char));
                ImTextStrToUtf8(clipboard_data, clipboard_data_len, state->TextW.Data + ib, state->TextW.Data + ie);
                SetClipboardText(clipboard_data);
                MemFree(clipboard_data);
            }
            if (is_cut)
            {
                if (!state->HasSelection())
                    state->SelectAll();
                state->CursorFollow = true;
                stb_textedit_cut(state, &state->Stb);
            }
        } else if (is_paste)
        {
            if (const char* clipboard = GetClipboardText())
            {
                // Filter pasted buffer
                const int clipboard_len = (int)strlen(clipboard);
                ImWchar* clipboard_filtered = (ImWchar*)IM_ALLOC((clipboard_len + 1) * sizeof(ImWchar));
                int clipboard_filtered_len = 0;
                for (const char* s = clipboard; *s; )
                {
                    unsigned int c;
                    s += ImTextCharFromUtf8(&c, s, NULL);
                    if (c == 0)
                        break;
                    if (!InputTextFilterCharacter(&c, flags, callback, callback_user_data))
                        continue;
                    clipboard_filtered[clipboard_filtered_len++] = (ImWchar)c;
                }
                clipboard_filtered[clipboard_filtered_len] = 0;
                if (clipboard_filtered_len > 0) // If everything was filtered, ignore the pasting operation
                {
                    stb_textedit_paste(state, &state->Stb, clipboard_filtered, clipboard_filtered_len);
                    state->CursorFollow = true;
                }
                MemFree(clipboard_filtered);
            }
        }

        // Update render selection flag after events have been handled, so selection highlight can be displayed during the same frame.
        render_selection |= state->HasSelection() && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
    }

    // Process callbacks and apply result back to user's buffer.
    if (g.ActiveId == id)
    {
        IM_ASSERT(state != NULL);
        const char* apply_new_text = NULL;
        int apply_new_text_length = 0;
        if (cancel_edit)
        {
            // Restore initial value. Only return true if restoring to the initial value changes the current buffer contents.
            if (!is_readonly && strcmp(buf, state->InitialTextA.Data) != 0)
            {
                // Push records into the undo stack so we can CTRL+Z the revert operation itself
                apply_new_text = state->InitialTextA.Data;
                apply_new_text_length = state->InitialTextA.Size - 1;
                ImVector<ImWchar> w_text;
                if (apply_new_text_length > 0)
                {
                    w_text.resize(ImTextCountCharsFromUtf8(apply_new_text, apply_new_text + apply_new_text_length) + 1);
                    ImTextStrFromUtf8(w_text.Data, w_text.Size, apply_new_text, apply_new_text + apply_new_text_length);
                }
                stb_textedit_replace(state, &state->Stb, w_text.Data, (apply_new_text_length > 0) ? (w_text.Size - 1) : 0);
            }
        }

        // When using 'ImGuiInputTextFlags_EnterReturnsTrue' as a special case we reapply the live buffer back to the input buffer before clearing ActiveId, even though strictly speaking it wasn't modified on this frame.
        // If we didn't do that, code like InputInt() with ImGuiInputTextFlags_EnterReturnsTrue would fail.
        // This also allows the user to use InputText() with ImGuiInputTextFlags_EnterReturnsTrue without maintaining any user-side storage (please note that if you use this property along ImGuiInputTextFlags_CallbackResize you can end up with your temporary string object unnecessarily allocating once a frame, either store your string data, either if you don't then don't use ImGuiInputTextFlags_CallbackResize).
        bool apply_edit_back_to_user_buffer = !cancel_edit || (enter_pressed && (flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0);
        if (apply_edit_back_to_user_buffer)
        {
            // Apply new value immediately - copy modified buffer back
            // Note that as soon as the input box is active, the in-widget value gets priority over any underlying modification of the input buffer
            // FIXME: We actually always render 'buf' when calling DrawList->AddText, making the comment above incorrect.
            // FIXME-OPT: CPU waste to do this every time the widget is active, should mark dirty state from the stb_textedit callbacks.
            if (!is_readonly)
            {
                state->TextAIsValid = true;
                state->TextA.resize(state->TextW.Size * 4 + 1);
                ImTextStrToUtf8(state->TextA.Data, state->TextA.Size, state->TextW.Data, NULL);
            }

            // User callback
            if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways)) != 0)
            {
                IM_ASSERT(callback != NULL);

                // The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
                ImGuiInputTextFlags event_flag = 0;
                ImGuiKey event_key = ImGuiKey_COUNT;
                if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && IsKeyPressedMap(ImGuiKey_Tab))
                {
                    event_flag = ImGuiInputTextFlags_CallbackCompletion;
                    event_key = ImGuiKey_Tab;
                } else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_UpArrow))
                {
                    event_flag = ImGuiInputTextFlags_CallbackHistory;
                    event_key = ImGuiKey_UpArrow;
                } else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_DownArrow))
                {
                    event_flag = ImGuiInputTextFlags_CallbackHistory;
                    event_key = ImGuiKey_DownArrow;
                } else if (flags & ImGuiInputTextFlags_CallbackAlways)
                    event_flag = ImGuiInputTextFlags_CallbackAlways;

                if (event_flag)
                {
                    ImGuiInputTextCallbackData callback_data;
                    memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
                    callback_data.EventFlag = event_flag;
                    callback_data.Flags = flags;
                    callback_data.UserData = callback_user_data;

                    callback_data.EventKey = event_key;
                    callback_data.Buf = state->TextA.Data;
                    callback_data.BufTextLen = state->CurLenA;
                    callback_data.BufSize = state->BufCapacityA;
                    callback_data.BufDirty = false;

                    // We have to convert from wchar-positions to UTF-8-positions, which can be pretty slow (an incentive to ditch the ImWchar buffer, see https://github.com/nothings/stb/issues/188)
                    ImWchar* text = state->TextW.Data;
                    const int utf8_cursor_pos = callback_data.CursorPos = ImTextCountUtf8BytesFromStr(text, text + state->Stb.cursor);
                    const int utf8_selection_start = callback_data.SelectionStart = ImTextCountUtf8BytesFromStr(text, text + state->Stb.select_start);
                    const int utf8_selection_end = callback_data.SelectionEnd = ImTextCountUtf8BytesFromStr(text, text + state->Stb.select_end);

                    // Call user code
                    callback(&callback_data);

                    // Read back what user may have modified
                    IM_ASSERT(callback_data.Buf == state->TextA.Data);  // Invalid to modify those fields
                    IM_ASSERT(callback_data.BufSize == state->BufCapacityA);
                    IM_ASSERT(callback_data.Flags == flags);
                    if (callback_data.CursorPos != utf8_cursor_pos) { state->Stb.cursor = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.CursorPos); state->CursorFollow = true; }
                    if (callback_data.SelectionStart != utf8_selection_start) { state->Stb.select_start = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionStart); }
                    if (callback_data.SelectionEnd != utf8_selection_end) { state->Stb.select_end = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionEnd); }
                    if (callback_data.BufDirty)
                    {
                        IM_ASSERT(callback_data.BufTextLen == (int)strlen(callback_data.Buf)); // You need to maintain BufTextLen if you change the text!
                        if (callback_data.BufTextLen > backup_current_text_length && is_resizable)
                            state->TextW.resize(state->TextW.Size + (callback_data.BufTextLen - backup_current_text_length));
                        state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, state->TextW.Size, callback_data.Buf, NULL);
                        state->CurLenA = callback_data.BufTextLen;  // Assume correct length and valid UTF-8 from user, saves us an extra strlen()
                        state->CursorAnimReset();
                    }
                }
            }

            // Will copy result string if modified
            if (!is_readonly && strcmp(state->TextA.Data, buf) != 0)
            {
                apply_new_text = state->TextA.Data;
                apply_new_text_length = state->CurLenA;
            }
        }

        // Copy result to user buffer
        if (apply_new_text)
        {
            // We cannot test for 'backup_current_text_length != apply_new_text_length' here because we have no guarantee that the size
            // of our owned buffer matches the size of the string object held by the user, and by design we allow InputText() to be used
            // without any storage on user's side.
            IM_ASSERT(apply_new_text_length >= 0);
            if (is_resizable)
            {
                ImGuiInputTextCallbackData callback_data;
                callback_data.EventFlag = ImGuiInputTextFlags_CallbackResize;
                callback_data.Flags = flags;
                callback_data.Buf = buf;
                callback_data.BufTextLen = apply_new_text_length;
                callback_data.BufSize = ImMax(buf_size, apply_new_text_length + 1);
                callback_data.UserData = callback_user_data;
                callback(&callback_data);
                buf = callback_data.Buf;
                buf_size = callback_data.BufSize;
                apply_new_text_length = ImMin(callback_data.BufTextLen, buf_size - 1);
                IM_ASSERT(apply_new_text_length <= buf_size);
            }
            //IMGUI_DEBUG_LOG("InputText(\"%s\"): apply_new_text length %d\n", label, apply_new_text_length);

            // If the underlying buffer resize was denied or not carried to the next frame, apply_new_text_length+1 may be >= buf_size.
            ImStrncpy(buf, apply_new_text, ImMin(apply_new_text_length + 1, buf_size));
            value_changed = true;
        }

        // Clear temporary user storage
        state->UserFlags = 0;
        state->UserCallback = NULL;
        state->UserCallbackData = NULL;
    }

    // Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
    if (clear_active_id && g.ActiveId == id)
        ClearActiveID();

    // Render frame
    if (!is_multiline)
    {
        RenderNavHighlight(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    }

    const ImVec4 clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + inner_size.x, frame_bb.Min.y + inner_size.y); // Not using frame_bb.Max because we have adjusted size
    ImVec2 draw_pos = is_multiline ? draw_window->DC.CursorPos : frame_bb.Min + style.FramePadding;
    ImVec2 text_size(0.0f, 0.0f);

    // Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
    // without any carriage return, which would makes ImFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
    // Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
    const int buf_display_max_length = 2 * 1024 * 1024;
    const char* buf_display = buf_display_from_state ? state->TextA.Data : buf; //-V595
    const char* buf_display_end = NULL; // We have specialized paths below for setting the length
    if (is_displaying_hint)
    {
        buf_display = hint;
        buf_display_end = hint + strlen(hint);
    }

    // Render text. We currently only render selection when the widget is active or while scrolling.
    // FIXME: We could remove the '&& render_cursor' to keep rendering selection when inactive.
    if (render_cursor || render_selection)
    {
        IM_ASSERT(state != NULL);
        if (!is_displaying_hint)
            buf_display_end = buf_display + state->CurLenA;

        // Render text (with cursor and selection)
        // This is going to be messy. We need to:
        // - Display the text (this alone can be more easily clipped)
        // - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
        // - Measure text height (for scrollbar)
        // We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
        // FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
        const ImWchar* text_begin = state->TextW.Data;
        ImVec2 cursor_offset, select_start_offset;

        {
            // Find lines numbers straddling 'cursor' (slot 0) and 'select_start' (slot 1) positions.
            const ImWchar* searches_input_ptr[2] = { NULL, NULL };
            int searches_result_line_no[2] = { -1000, -1000 };
            int searches_remaining = 0;
            if (render_cursor)
            {
                searches_input_ptr[0] = text_begin + state->Stb.cursor;
                searches_result_line_no[0] = -1;
                searches_remaining++;
            }
            if (render_selection)
            {
                searches_input_ptr[1] = text_begin + ImMin(state->Stb.select_start, state->Stb.select_end);
                searches_result_line_no[1] = -1;
                searches_remaining++;
            }

            // Iterate all lines to find our line numbers
            // In multi-line mode, we never exit the loop until all lines are counted, so add one extra to the searches_remaining counter.
            searches_remaining += is_multiline ? 1 : 0;
            int line_count = 0;
            //for (const ImWchar* s = text_begin; (s = (const ImWchar*)wcschr((const wchar_t*)s, (wchar_t)'\n')) != NULL; s++)  // FIXME-OPT: Could use this when wchar_t are 16-bit
            for (const ImWchar* s = text_begin; *s != 0; s++)
                if (*s == '\n')
                {
                    line_count++;
                    if (searches_result_line_no[0] == -1 && s >= searches_input_ptr[0]) { searches_result_line_no[0] = line_count; if (--searches_remaining <= 0) break; }
                    if (searches_result_line_no[1] == -1 && s >= searches_input_ptr[1]) { searches_result_line_no[1] = line_count; if (--searches_remaining <= 0) break; }
                }
            line_count++;
            if (searches_result_line_no[0] == -1)
                searches_result_line_no[0] = line_count;
            if (searches_result_line_no[1] == -1)
                searches_result_line_no[1] = line_count;

            // Calculate 2d position by finding the beginning of the line and measuring distance
            cursor_offset.x = InputTextCalcTextSizeW(ImStrbolW(searches_input_ptr[0], text_begin), searches_input_ptr[0]).x;
            cursor_offset.y = searches_result_line_no[0] * g.FontSize;
            if (searches_result_line_no[1] >= 0)
            {
                select_start_offset.x = InputTextCalcTextSizeW(ImStrbolW(searches_input_ptr[1], text_begin), searches_input_ptr[1]).x;
                select_start_offset.y = searches_result_line_no[1] * g.FontSize;
            }

            // Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
            if (is_multiline)
                text_size = ImVec2(inner_size.x, line_count * g.FontSize);
        }

        // Scroll
        if (render_cursor && state->CursorFollow)
        {
            // Horizontal scroll in chunks of quarter width
            if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
            {
                const float scroll_increment_x = inner_size.x * 0.25f;
                if (cursor_offset.x < state->ScrollX)
                    state->ScrollX = IM_FLOOR(ImMax(0.0f, cursor_offset.x - scroll_increment_x));
                else if (cursor_offset.x - inner_size.x >= state->ScrollX)
                    state->ScrollX = IM_FLOOR(cursor_offset.x - inner_size.x + scroll_increment_x);
            } else
            {
                state->ScrollX = 0.0f;
            }

            // Vertical scroll
            if (is_multiline)
            {
                float scroll_y = draw_window->Scroll.y;
                if (cursor_offset.y - g.FontSize < scroll_y)
                    scroll_y = ImMax(0.0f, cursor_offset.y - g.FontSize);
                else if (cursor_offset.y - inner_size.y >= scroll_y)
                    scroll_y = cursor_offset.y - inner_size.y;
                draw_pos.y += (draw_window->Scroll.y - scroll_y);   // Manipulate cursor pos immediately avoid a frame of lag
                draw_window->Scroll.y = scroll_y;
            }

            state->CursorFollow = false;
        }

        // Draw selection
        const ImVec2 draw_scroll = ImVec2(state->ScrollX, 0.0f);
        if (render_selection)
        {
            const ImWchar* text_selected_begin = text_begin + ImMin(state->Stb.select_start, state->Stb.select_end);
            const ImWchar* text_selected_end = text_begin + ImMax(state->Stb.select_start, state->Stb.select_end);

            ImU32 bg_color = GetColorU32(ImGuiCol_TextSelectedBg, render_cursor ? 1.0f : 0.6f); // FIXME: current code flow mandate that render_cursor is always true here, we are leaving the transparent one for tests.
            float bg_offy_up = is_multiline ? 0.0f : -1.0f;    // FIXME: those offsets should be part of the style? they don't play so well with multi-line selection.
            float bg_offy_dn = is_multiline ? 0.0f : 2.0f;
            ImVec2 rect_pos = draw_pos + select_start_offset - draw_scroll;
            for (const ImWchar* p = text_selected_begin; p < text_selected_end; )
            {
                if (rect_pos.y > clip_rect.w + g.FontSize)
                    break;
                if (rect_pos.y < clip_rect.y)
                {
                    //p = (const ImWchar*)wmemchr((const wchar_t*)p, '\n', text_selected_end - p);  // FIXME-OPT: Could use this when wchar_t are 16-bit
                    //p = p ? p + 1 : text_selected_end;
                    while (p < text_selected_end)
                        if (*p++ == '\n')
                            break;
                } else
                {
                    ImVec2 rect_size = InputTextCalcTextSizeW(p, text_selected_end, &p, NULL, true);
                    if (rect_size.x <= 0.0f) rect_size.x = IM_FLOOR(g.Font->GetCharAdvance((ImWchar)' ') * 0.50f); // So we can see selected empty lines
                    ImRect rect(rect_pos + ImVec2(0.0f, bg_offy_up - g.FontSize), rect_pos + ImVec2(rect_size.x, bg_offy_dn));
                    rect.ClipWith(clip_rect);
                    if (rect.Overlaps(clip_rect))
                        draw_window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_color);
                }
                rect_pos.x = draw_pos.x - draw_scroll.x;
                rect_pos.y += g.FontSize;
            }
        }

        // We test for 'buf_display_max_length' as a way to avoid some pathological cases (e.g. single-line 1 MB string) which would make ImDrawList crash.
        if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
        {
            ImU32 col = GetColorU32(is_displaying_hint ? ImGuiCol_TextDisabled : ImGuiCol_Text);
            draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos - draw_scroll, col, buf_display, buf_display_end, 0.0f, is_multiline ? NULL : &clip_rect);
        }

        // Draw blinking cursor
        if (render_cursor)
        {
            state->CursorAnim += io.DeltaTime;
            bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (state->CursorAnim <= 0.0f) || ImFmod(state->CursorAnim, 1.20f) <= 0.80f;
            ImVec2 cursor_screen_pos = draw_pos + cursor_offset - draw_scroll;
            ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
            if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
                draw_window->DrawList->AddLine(cursor_screen_rect.Min, cursor_screen_rect.GetBL(), GetColorU32(ImGuiCol_Text));

            // Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
            if (!is_readonly)
                g.PlatformImePos = ImVec2(cursor_screen_pos.x - 1.0f, cursor_screen_pos.y - g.FontSize);
        }
    } else
    {
        // Render text only (no selection, no cursor)
        if (is_multiline)
            text_size = ImVec2(inner_size.x, InputTextCalcTextLenAndLineCount(buf_display, &buf_display_end) * g.FontSize); // We don't need width
        else if (!is_displaying_hint && g.ActiveId == id)
            buf_display_end = buf_display + state->CurLenA;
        else if (!is_displaying_hint)
            buf_display_end = buf_display + strlen(buf_display);

        if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
        {
            ImU32 col = GetColorU32(is_displaying_hint ? ImGuiCol_TextDisabled : ImGuiCol_Text);
            draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos, col, buf_display, buf_display_end, 0.0f, is_multiline ? NULL : &clip_rect);
        }
    }

    if (is_multiline)
    {
        Dummy(text_size + ImVec2(0.0f, g.FontSize)); // Always add room to scroll an extra line
        EndChild();
        EndGroup();
    }

    if (is_password && !is_displaying_hint)
        PopFont();

    // Log as text
    if (g.LogEnabled && !(is_password && !is_displaying_hint))
        LogRenderedText(&draw_pos, buf_display, buf_display_end);

    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    if (value_changed && !(flags & ImGuiInputTextFlags_NoMarkEdited))
        MarkItemEdited(id);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
    if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
        return enter_pressed;
    else
        return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
//-------------------------------------------------------------------------
// - ColorEdit3()
// - ColorEdit4()
// - ColorPicker3()
// - RenderColorRectWithAlphaCheckerboard() [Internal]
// - ColorPicker4()
// - ColorButton()
// - SetColorEditOptions()
// - ColorTooltip() [Internal]
// - ColorEditOptionsPopup() [Internal]
// - ColorPickerOptionsPopup() [Internal]
//-------------------------------------------------------------------------

bool ImGui::ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags)
{
    return ColorEdit4(label, col, flags | ImGuiColorEditFlags_NoAlpha);
}

// Edit colors components (each component in 0.0f..1.0f range).
// See enum ImGuiColorEditFlags_ for available options. e.g. Only access 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
// With typical options: Left-click on colored square to open color picker. Right-click to open option menu. CTRL-Click over input fields to edit them and TAB to go to next item.
bool ImGui::ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float square_sz = GetFrameHeight();
    const float w_full = CalcItemWidth();
    const float w_button = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
    const float w_inputs = w_full - w_button;
    const char* label_display_end = FindRenderedTextEnd(label);
    g.NextItemData.ClearFlags();

    BeginGroup();
    PushID(label);

    // If we're not showing any slider there's no point in doing any HSV conversions
    const ImGuiColorEditFlags flags_untouched = flags;
    if (flags & ImGuiColorEditFlags_NoInputs)
        flags = (flags & (~ImGuiColorEditFlags__DisplayMask)) | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoOptions;

    // Context menu: display and modify options (before defaults are applied)
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        ColorEditOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & ImGuiColorEditFlags__DisplayMask))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags__DisplayMask);
    if (!(flags & ImGuiColorEditFlags__DataTypeMask))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags__DataTypeMask);
    if (!(flags & ImGuiColorEditFlags__PickerMask))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags__PickerMask);
    if (!(flags & ImGuiColorEditFlags__InputMask))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags__InputMask);
    flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags__DisplayMask | ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags__InputMask));
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags__DisplayMask)); // Check that only 1 is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags__InputMask));   // Check that only 1 is selected

    const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
    const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
    const int components = alpha ? 4 : 3;

    // Convert to the formats we need
    float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
    if ((flags & ImGuiColorEditFlags_InputHSV) && (flags & ImGuiColorEditFlags_DisplayRGB))
        ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
    else if ((flags & ImGuiColorEditFlags_InputRGB) && (flags & ImGuiColorEditFlags_DisplayHSV))
    {
        // Hue is lost when converting from greyscale rgb (saturation=0). Restore it.
        ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
        if (memcmp(g.ColorEditLastColor, col, sizeof(float) * 3) == 0)
        {
            if (f[1] == 0)
                f[0] = g.ColorEditLastHue;
            if (f[2] == 0)
                f[1] = g.ColorEditLastSat;
        }
    }
    int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

    bool value_changed = false;
    bool value_changed_as_float = false;

    const ImVec2 pos = window->DC.CursorPos;
    const float inputs_offset_x = (style.ColorButtonPosition == ImGuiDir_Left) ? w_button : 0.0f;
    window->DC.CursorPos.x = pos.x + inputs_offset_x;

    if ((flags & (ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
    {
        // RGB/HSV 0..255 Sliders
        const float w_item_one = ImMax(1.0f, IM_FLOOR((w_inputs - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
        const float w_item_last = ImMax(1.0f, IM_FLOOR(w_inputs - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));

        const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
        static const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
        static const char* fmt_table_int[3][4] =
        {
            {   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
            { "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
            { "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
        };
        static const char* fmt_table_float[3][4] =
        {
            {   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
            { "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
            { "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
        };
        const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_DisplayHSV) ? 2 : 1;

        for (int n = 0; n < components; n++)
        {
            if (n > 0)
                SameLine(0, style.ItemInnerSpacing.x);
            SetNextItemWidth((n + 1 < components) ? w_item_one : w_item_last);

            // FIXME: When ImGuiColorEditFlags_HDR flag is passed HS values snap in weird ways when SV values go below 0.
            if (flags & ImGuiColorEditFlags_Float)
            {
                value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
                value_changed_as_float |= value_changed;
            } else
            {
                value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
            }
            if (!(flags & ImGuiColorEditFlags_NoOptions))
                OpenPopupOnItemClick("context");
        }
    } else if ((flags & ImGuiColorEditFlags_DisplayHex) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
    {
        // RGB Hexadecimal Input
        char buf[64];
        if (alpha)
            ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
        else
            ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
        SetNextItemWidth(w_inputs);
        if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
        {
            value_changed = true;
            char* p = buf;
            while (*p == '#' || ImCharIsBlankA(*p))
                p++;
            i[0] = i[1] = i[2] = i[3] = 0;
            if (alpha)
                sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
            else
                sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context");
    }

    ImGuiWindow* picker_active_window = NULL;
    if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
    {
        const float button_offset_x = ((flags & ImGuiColorEditFlags_NoInputs) || (style.ColorButtonPosition == ImGuiDir_Left)) ? 0.0f : w_inputs + style.ItemInnerSpacing.x;
        window->DC.CursorPos = ImVec2(pos.x + button_offset_x, pos.y);

        const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
        if (ColorButton("##ColorButton", col_v4, flags))
        {
            if (!(flags & ImGuiColorEditFlags_NoPicker))
            {
                // Store current color and open a picker
                g.ColorPickerRef = col_v4;
                OpenPopup("picker");
                SetNextWindowPos(window->DC.LastItemRect.GetBL() + ImVec2(-1, style.ItemSpacing.y));
            }
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context");

        if (BeginPopup("picker"))
        {
            picker_active_window = g.CurrentWindow;
            if (label != label_display_end)
            {
                TextEx(label, label_display_end);
                Spacing();
            }
            ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags__InputMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
            ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags__DisplayMask | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
            SetNextItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
            value_changed |= ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
            EndPopup();
        }
    }

    if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
    {
        const float text_offset_x = (flags & ImGuiColorEditFlags_NoInputs) ? w_button : w_full + style.ItemInnerSpacing.x;
        window->DC.CursorPos = ImVec2(pos.x + text_offset_x, pos.y + style.FramePadding.y);
        TextEx(label, label_display_end);
    }

    // Convert back
    if (value_changed && picker_active_window == NULL)
    {
        if (!value_changed_as_float)
            for (int n = 0; n < 4; n++)
                f[n] = i[n] / 255.0f;
        if ((flags & ImGuiColorEditFlags_DisplayHSV) && (flags & ImGuiColorEditFlags_InputRGB))
        {
            g.ColorEditLastHue = f[0];
            g.ColorEditLastSat = f[1];
            ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
            memcpy(g.ColorEditLastColor, f, sizeof(float) * 3);
        }
        if ((flags & ImGuiColorEditFlags_DisplayRGB) && (flags & ImGuiColorEditFlags_InputHSV))
            ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);

        col[0] = f[0];
        col[1] = f[1];
        col[2] = f[2];
        if (alpha)
            col[3] = f[3];
    }

    PopID();
    EndGroup();

    // Drag and Drop Target
    // NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
    if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
    {
        bool accepted_drag_drop = false;
        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
        {
            memcpy((float*)col, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
            value_changed = accepted_drag_drop = true;
        }
        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
        {
            memcpy((float*)col, payload->Data, sizeof(float) * components);
            value_changed = accepted_drag_drop = true;
        }

        // Drag-drop payloads are always RGB
        if (accepted_drag_drop && (flags & ImGuiColorEditFlags_InputHSV))
            ColorConvertRGBtoHSV(col[0], col[1], col[2], col[0], col[1], col[2]);
        EndDragDropTarget();
    }

    // When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
    if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
        window->DC.LastItemId = g.ActiveId;

    if (value_changed)
        MarkItemEdited(window->DC.LastItemId);

    return value_changed;
}

bool ImGui::ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags)
{
    float col4[4] = { col[0], col[1], col[2], 1.0f };
    if (!ColorPicker4(label, col4, flags | ImGuiColorEditFlags_NoAlpha))
        return false;
    col[0] = col4[0]; col[1] = col4[1]; col[2] = col4[2];
    return true;
}

// Helper for ColorPicker4()
static void RenderArrowsForVerticalBar(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_w, float alpha)
{
    ImU32 alpha8 = IM_F32_TO_INT8_SAT(alpha);
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x + 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Right, IM_COL32(0, 0, 0, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x, pos.y), half_sz, ImGuiDir_Right, IM_COL32(255, 255, 255, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x - 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Left, IM_COL32(0, 0, 0, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x, pos.y), half_sz, ImGuiDir_Left, IM_COL32(255, 255, 255, alpha8));
}

// Note: ColorPicker4() only accesses 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
// (In C++ the 'float col[4]' notation for a function argument is equivalent to 'float* col', we only specify a size to facilitate understanding of the code.)
// FIXME: we adjust the big color square height based on item width, which may cause a flickering feedback loop (if automatic height makes a vertical scrollbar appears, affecting automatic width..)
// FIXME: this is trying to be aware of style.Alpha but not fully correct. Also, the color wheel will have overlapping glitches with (style.Alpha < 1.0)
bool ImGui::ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImDrawList* draw_list = window->DrawList;
    ImGuiStyle& style = g.Style;
    ImGuiIO& io = g.IO;

    const float width = CalcItemWidth();
    g.NextItemData.ClearFlags();

    PushID(label);
    BeginGroup();

    if (!(flags & ImGuiColorEditFlags_NoSidePreview))
        flags |= ImGuiColorEditFlags_NoSmallPreview;

    // Context menu: display and store options.
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        ColorPickerOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & ImGuiColorEditFlags__PickerMask))
        flags |= ((g.ColorEditOptions & ImGuiColorEditFlags__PickerMask) ? g.ColorEditOptions : ImGuiColorEditFlags__OptionsDefault) & ImGuiColorEditFlags__PickerMask;
    if (!(flags & ImGuiColorEditFlags__InputMask))
        flags |= ((g.ColorEditOptions & ImGuiColorEditFlags__InputMask) ? g.ColorEditOptions : ImGuiColorEditFlags__OptionsDefault) & ImGuiColorEditFlags__InputMask;
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags__PickerMask)); // Check that only 1 is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags__InputMask));  // Check that only 1 is selected
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar);

    // Setup
    int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
    bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
    ImVec2 picker_pos = window->DC.CursorPos;
    float square_sz = GetFrameHeight();
    float bars_width = square_sz; // Arbitrary smallish width of Hue/Alpha picking bars
    float sv_picker_size = ImMax(bars_width * 1, width - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
    float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
    float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;
    float bars_triangles_half_sz = IM_FLOOR(bars_width * 0.20f);

    float backup_initial_col[4];
    memcpy(backup_initial_col, col, components * sizeof(float));

    float wheel_thickness = sv_picker_size * 0.08f;
    float wheel_r_outer = sv_picker_size * 0.50f;
    float wheel_r_inner = wheel_r_outer - wheel_thickness;
    ImVec2 wheel_center(picker_pos.x + (sv_picker_size + bars_width) * 0.5f, picker_pos.y + sv_picker_size * 0.5f);

    // Note: the triangle is displayed rotated with triangle_pa pointing to Hue, but most coordinates stays unrotated for logic.
    float triangle_r = wheel_r_inner - (int)(sv_picker_size * 0.027f);
    ImVec2 triangle_pa = ImVec2(triangle_r, 0.0f); // Hue point.
    ImVec2 triangle_pb = ImVec2(triangle_r * -0.5f, triangle_r * -0.866025f); // Black point.
    ImVec2 triangle_pc = ImVec2(triangle_r * -0.5f, triangle_r * +0.866025f); // White point.

    float H = col[0], S = col[1], V = col[2];
    float R = col[0], G = col[1], B = col[2];
    if (flags & ImGuiColorEditFlags_InputRGB)
    {
        // Hue is lost when converting from greyscale rgb (saturation=0). Restore it.
        ColorConvertRGBtoHSV(R, G, B, H, S, V);
        if (memcmp(g.ColorEditLastColor, col, sizeof(float) * 3) == 0)
        {
            if (S == 0)
                H = g.ColorEditLastHue;
            if (V == 0)
                S = g.ColorEditLastSat;
        }
    } else if (flags & ImGuiColorEditFlags_InputHSV)
    {
        ColorConvertHSVtoRGB(H, S, V, R, G, B);
    }

    bool value_changed = false, value_changed_h = false, value_changed_sv = false;

    PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (flags & ImGuiColorEditFlags_PickerHueWheel)
    {
        // Hue wheel + SV triangle logic
        InvisibleButton("hsv", ImVec2(sv_picker_size + style.ItemInnerSpacing.x + bars_width, sv_picker_size));
        if (IsItemActive())
        {
            ImVec2 initial_off = g.IO.MouseClickedPos[0] - wheel_center;
            ImVec2 current_off = g.IO.MousePos - wheel_center;
            float initial_dist2 = ImLengthSqr(initial_off);
            if (initial_dist2 >= (wheel_r_inner - 1) * (wheel_r_inner - 1) && initial_dist2 <= (wheel_r_outer + 1) * (wheel_r_outer + 1))
            {
                // Interactive with Hue wheel
                H = ImAtan2(current_off.y, current_off.x) / IM_PI * 0.5f;
                if (H < 0.0f)
                    H += 1.0f;
                value_changed = value_changed_h = true;
            }
            float cos_hue_angle = ImCos(-H * 2.0f * IM_PI);
            float sin_hue_angle = ImSin(-H * 2.0f * IM_PI);
            if (ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, ImRotate(initial_off, cos_hue_angle, sin_hue_angle)))
            {
                // Interacting with SV triangle
                ImVec2 current_off_unrotated = ImRotate(current_off, cos_hue_angle, sin_hue_angle);
                if (!ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated))
                    current_off_unrotated = ImTriangleClosestPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated);
                float uu, vv, ww;
                ImTriangleBarycentricCoords(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated, uu, vv, ww);
                V = ImClamp(1.0f - vv, 0.0001f, 1.0f);
                S = ImClamp(uu / V, 0.0001f, 1.0f);
                value_changed = value_changed_sv = true;
            }
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context");
    } else if (flags & ImGuiColorEditFlags_PickerHueBar)
    {
        // SV rectangle logic
        InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
        if (IsItemActive())
        {
            S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
            V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            value_changed = value_changed_sv = true;
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context");

        // Hue bar logic
        SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
        InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
        if (IsItemActive())
        {
            H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            value_changed = value_changed_h = true;
        }
    }

    // Alpha bar logic
    if (alpha_bar)
    {
        SetCursorScreenPos(ImVec2(bar1_pos_x, picker_pos.y));
        InvisibleButton("alpha", ImVec2(bars_width, sv_picker_size));
        if (IsItemActive())
        {
            col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            value_changed = true;
        }
    }
    PopItemFlag(); // ImGuiItemFlags_NoNav

    if (!(flags & ImGuiColorEditFlags_NoSidePreview))
    {
        SameLine(0, style.ItemInnerSpacing.x);
        BeginGroup();
    }

    if (!(flags & ImGuiColorEditFlags_NoLabel))
    {
        const char* label_display_end = FindRenderedTextEnd(label);
        if (label != label_display_end)
        {
            if ((flags & ImGuiColorEditFlags_NoSidePreview))
                SameLine(0, style.ItemInnerSpacing.x);
            TextEx(label, label_display_end);
        }
    }

    if (!(flags & ImGuiColorEditFlags_NoSidePreview))
    {
        PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
        ImVec4 col_v4(col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
        if ((flags & ImGuiColorEditFlags_NoLabel))
            Text("Current");

        ImGuiColorEditFlags sub_flags_to_forward = ImGuiColorEditFlags__InputMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoTooltip;
        ColorButton("##current", col_v4, (flags & sub_flags_to_forward), ImVec2(square_sz * 3, square_sz * 2));
        if (ref_col != NULL)
        {
            Text("Original");
            ImVec4 ref_col_v4(ref_col[0], ref_col[1], ref_col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : ref_col[3]);
            if (ColorButton("##original", ref_col_v4, (flags & sub_flags_to_forward), ImVec2(square_sz * 3, square_sz * 2)))
            {
                memcpy(col, ref_col, components * sizeof(float));
                value_changed = true;
            }
        }
        PopItemFlag();
        EndGroup();
    }

    // Convert back color to RGB
    if (value_changed_h || value_changed_sv)
    {
        if (flags & ImGuiColorEditFlags_InputRGB)
        {
            ColorConvertHSVtoRGB(H >= 1.0f ? H - 10 * 1e-6f : H, S > 0.0f ? S : 10 * 1e-6f, V > 0.0f ? V : 1e-6f, col[0], col[1], col[2]);
            g.ColorEditLastHue = H;
            g.ColorEditLastSat = S;
            memcpy(g.ColorEditLastColor, col, sizeof(float) * 3);
        } else if (flags & ImGuiColorEditFlags_InputHSV)
        {
            col[0] = H;
            col[1] = S;
            col[2] = V;
        }
    }

    // R,G,B and H,S,V slider color editor
    bool value_changed_fix_hue_wrap = false;
    if ((flags & ImGuiColorEditFlags_NoInputs) == 0)
    {
        PushItemWidth((alpha_bar ? bar1_pos_x : bar0_pos_x) + bars_width - picker_pos.x);
        ImGuiColorEditFlags sub_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__InputMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf;
        ImGuiColorEditFlags sub_flags = (flags & sub_flags_to_forward) | ImGuiColorEditFlags_NoPicker;
        if (flags & ImGuiColorEditFlags_DisplayRGB || (flags & ImGuiColorEditFlags__DisplayMask) == 0)
            if (ColorEdit4("##rgb", col, sub_flags | ImGuiColorEditFlags_DisplayRGB))
            {
                // FIXME: Hackily differentiating using the DragInt (ActiveId != 0 && !ActiveIdAllowOverlap) vs. using the InputText or DropTarget.
                // For the later we don't want to run the hue-wrap canceling code. If you are well versed in HSV picker please provide your input! (See #2050)
                value_changed_fix_hue_wrap = (g.ActiveId != 0 && !g.ActiveIdAllowOverlap);
                value_changed = true;
            }
        if (flags & ImGuiColorEditFlags_DisplayHSV || (flags & ImGuiColorEditFlags__DisplayMask) == 0)
            value_changed |= ColorEdit4("##hsv", col, sub_flags | ImGuiColorEditFlags_DisplayHSV);
        if (flags & ImGuiColorEditFlags_DisplayHex || (flags & ImGuiColorEditFlags__DisplayMask) == 0)
            value_changed |= ColorEdit4("##hex", col, sub_flags | ImGuiColorEditFlags_DisplayHex);
        PopItemWidth();
    }

    // Try to cancel hue wrap (after ColorEdit4 call), if any
    if (value_changed_fix_hue_wrap && (flags & ImGuiColorEditFlags_InputRGB))
    {
        float new_H, new_S, new_V;
        ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
        if (new_H <= 0 && H > 0)
        {
            if (new_V <= 0 && V != new_V)
                ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
            else if (new_S <= 0)
                ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
        }
    }

    if (value_changed)
    {
        if (flags & ImGuiColorEditFlags_InputRGB)
        {
            R = col[0];
            G = col[1];
            B = col[2];
            ColorConvertRGBtoHSV(R, G, B, H, S, V);
            if (memcmp(g.ColorEditLastColor, col, sizeof(float) * 3) == 0) // Fix local Hue as display below will use it immediately.
            {
                if (S == 0)
                    H = g.ColorEditLastHue;
                if (V == 0)
                    S = g.ColorEditLastSat;
            }
        } else if (flags & ImGuiColorEditFlags_InputHSV)
        {
            H = col[0];
            S = col[1];
            V = col[2];
            ColorConvertHSVtoRGB(H, S, V, R, G, B);
        }
    }

    const int style_alpha8 = IM_F32_TO_INT8_SAT(style.Alpha);
    const ImU32 col_black = IM_COL32(0, 0, 0, style_alpha8);
    const ImU32 col_white = IM_COL32(255, 255, 255, style_alpha8);
    const ImU32 col_midgrey = IM_COL32(128, 128, 128, style_alpha8);
    const ImU32 col_hues[6 + 1] = { IM_COL32(255,0,0,style_alpha8), IM_COL32(255,255,0,style_alpha8), IM_COL32(0,255,0,style_alpha8), IM_COL32(0,255,255,style_alpha8), IM_COL32(0,0,255,style_alpha8), IM_COL32(255,0,255,style_alpha8), IM_COL32(255,0,0,style_alpha8) };

    ImVec4 hue_color_f(1, 1, 1, style.Alpha); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
    ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
    ImU32 user_col32_striped_of_alpha = ColorConvertFloat4ToU32(ImVec4(R, G, B, style.Alpha)); // Important: this is still including the main rendering/style alpha!!

    ImVec2 sv_cursor_pos;

    if (flags & ImGuiColorEditFlags_PickerHueWheel)
    {
        // Render Hue Wheel
        const float aeps = 0.5f / wheel_r_outer; // Half a pixel arc length in radians (2pi cancels out).
        const int segment_per_arc = ImMax(4, (int)wheel_r_outer / 12);
        for (int n = 0; n < 6; n++)
        {
            const float a0 = (n) / 6.0f * 2.0f * IM_PI - aeps;
            const float a1 = (n + 1.0f) / 6.0f * 2.0f * IM_PI + aeps;
            const int vert_start_idx = draw_list->VtxBuffer.Size;
            draw_list->PathArcTo(wheel_center, (wheel_r_inner + wheel_r_outer) * 0.5f, a0, a1, segment_per_arc);
            draw_list->PathStroke(col_white, false, wheel_thickness);
            const int vert_end_idx = draw_list->VtxBuffer.Size;

            // Paint colors over existing vertices
            ImVec2 gradient_p0(wheel_center.x + ImCos(a0) * wheel_r_inner, wheel_center.y + ImSin(a0) * wheel_r_inner);
            ImVec2 gradient_p1(wheel_center.x + ImCos(a1) * wheel_r_inner, wheel_center.y + ImSin(a1) * wheel_r_inner);
            ShadeVertsLinearColorGradientKeepAlpha(draw_list, vert_start_idx, vert_end_idx, gradient_p0, gradient_p1, col_hues[n], col_hues[n + 1]);
        }

        // Render Cursor + preview on Hue Wheel
        float cos_hue_angle = ImCos(H * 2.0f * IM_PI);
        float sin_hue_angle = ImSin(H * 2.0f * IM_PI);
        ImVec2 hue_cursor_pos(wheel_center.x + cos_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f, wheel_center.y + sin_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f);
        float hue_cursor_rad = value_changed_h ? wheel_thickness * 0.65f : wheel_thickness * 0.55f;
        int hue_cursor_segments = ImClamp((int)(hue_cursor_rad / 1.4f), 9, 32);
        draw_list->AddCircleFilled(hue_cursor_pos, hue_cursor_rad, hue_color32, hue_cursor_segments);
        draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad + 1, col_midgrey, hue_cursor_segments);
        draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad, col_white, hue_cursor_segments);

        // Render SV triangle (rotated according to hue)
        ImVec2 tra = wheel_center + ImRotate(triangle_pa, cos_hue_angle, sin_hue_angle);
        ImVec2 trb = wheel_center + ImRotate(triangle_pb, cos_hue_angle, sin_hue_angle);
        ImVec2 trc = wheel_center + ImRotate(triangle_pc, cos_hue_angle, sin_hue_angle);
        ImVec2 uv_white = GetFontTexUvWhitePixel();
        draw_list->PrimReserve(6, 6);
        draw_list->PrimVtx(tra, uv_white, hue_color32);
        draw_list->PrimVtx(trb, uv_white, hue_color32);
        draw_list->PrimVtx(trc, uv_white, col_white);
        draw_list->PrimVtx(tra, uv_white, 0);
        draw_list->PrimVtx(trb, uv_white, col_black);
        draw_list->PrimVtx(trc, uv_white, 0);
        draw_list->AddTriangle(tra, trb, trc, col_midgrey, 1.5f);
        sv_cursor_pos = ImLerp(ImLerp(trc, tra, ImSaturate(S)), trb, ImSaturate(1 - V));
    } else if (flags & ImGuiColorEditFlags_PickerHueBar)
    {
        // Render SV Square
        draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), col_white, hue_color32, hue_color32, col_white);
        draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0, 0, col_black, col_black);
        RenderFrameBorder(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0.0f);
        sv_cursor_pos.x = ImClamp(IM_ROUND(picker_pos.x + ImSaturate(S) * sv_picker_size), picker_pos.x + 2, picker_pos.x + sv_picker_size - 2); // Sneakily prevent the circle to stick out too much
        sv_cursor_pos.y = ImClamp(IM_ROUND(picker_pos.y + ImSaturate(1 - V) * sv_picker_size), picker_pos.y + 2, picker_pos.y + sv_picker_size - 2);

        // Render Hue Bar
        for (int i = 0; i < 6; ++i)
            draw_list->AddRectFilledMultiColor(ImVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)), ImVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)), col_hues[i], col_hues[i], col_hues[i + 1], col_hues[i + 1]);
        float bar0_line_y = IM_ROUND(picker_pos.y + H * sv_picker_size);
        RenderFrameBorder(ImVec2(bar0_pos_x, picker_pos.y), ImVec2(bar0_pos_x + bars_width, picker_pos.y + sv_picker_size), 0.0f);
        RenderArrowsForVerticalBar(draw_list, ImVec2(bar0_pos_x - 1, bar0_line_y), ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f, style.Alpha);
    }

    // Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
    float sv_cursor_rad = value_changed_sv ? 10.0f : 6.0f;
    draw_list->AddCircleFilled(sv_cursor_pos, sv_cursor_rad, user_col32_striped_of_alpha, 12);
    draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad + 1, col_midgrey, 12);
    draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad, col_white, 12);

    // Render alpha bar
    if (alpha_bar)
    {
        float alpha = ImSaturate(col[3]);
        ImRect bar1_bb(bar1_pos_x, picker_pos.y, bar1_pos_x + bars_width, picker_pos.y + sv_picker_size);
        RenderColorRectWithAlphaCheckerboard(draw_list, bar1_bb.Min, bar1_bb.Max, 0, bar1_bb.GetWidth() / 2.0f, ImVec2(0.0f, 0.0f));
        draw_list->AddRectFilledMultiColor(bar1_bb.Min, bar1_bb.Max, user_col32_striped_of_alpha, user_col32_striped_of_alpha, user_col32_striped_of_alpha & ~IM_COL32_A_MASK, user_col32_striped_of_alpha & ~IM_COL32_A_MASK);
        float bar1_line_y = IM_ROUND(picker_pos.y + (1.0f - alpha) * sv_picker_size);
        RenderFrameBorder(bar1_bb.Min, bar1_bb.Max, 0.0f);
        RenderArrowsForVerticalBar(draw_list, ImVec2(bar1_pos_x - 1, bar1_line_y), ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f, style.Alpha);
    }

    EndGroup();

    if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
        value_changed = false;
    if (value_changed)
        MarkItemEdited(window->DC.LastItemId);

    PopID();

    return value_changed;
}

// A little colored square. Return true when clicked.
// FIXME: May want to display/ignore the alpha component in the color display? Yet show it in the tooltip.
// 'desc_id' is not called 'label' because we don't display it next to the button, but only in the tooltip.
// Note that 'col' may be encoded in HSV if ImGuiColorEditFlags_InputHSV is set.
bool ImGui::ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags, ImVec2 size)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(desc_id);
    float default_size = GetFrameHeight();
    if (size.x == 0.0f)
        size.x = default_size;
    if (size.y == 0.0f)
        size.y = default_size;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    if (flags & ImGuiColorEditFlags_NoAlpha)
        flags &= ~(ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf);

    ImVec4 col_rgb = col;
    if (flags & ImGuiColorEditFlags_InputHSV)
        ColorConvertHSVtoRGB(col_rgb.x, col_rgb.y, col_rgb.z, col_rgb.x, col_rgb.y, col_rgb.z);

    ImVec4 col_rgb_without_alpha(col_rgb.x, col_rgb.y, col_rgb.z, 1.0f);
    float grid_step = ImMin(size.x, size.y) / 2.99f;
    float rounding = ImMin(g.Style.FrameRounding, grid_step * 0.5f);
    ImRect bb_inner = bb;
    float off = 0.0f;
    if ((flags & ImGuiColorEditFlags_NoBorder) == 0)
    {
        off = -0.75f; // The border (using Col_FrameBg) tends to look off when color is near-opaque and rounding is enabled. This offset seemed like a good middle ground to reduce those artifacts.
        bb_inner.Expand(off);
    }
    if ((flags & ImGuiColorEditFlags_AlphaPreviewHalf) && col_rgb.w < 1.0f)
    {
        float mid_x = IM_ROUND((bb_inner.Min.x + bb_inner.Max.x) * 0.5f);
        RenderColorRectWithAlphaCheckerboard(window->DrawList, ImVec2(bb_inner.Min.x + grid_step, bb_inner.Min.y), bb_inner.Max, GetColorU32(col_rgb), grid_step, ImVec2(-grid_step + off, off), rounding, ImDrawCornerFlags_TopRight | ImDrawCornerFlags_BotRight);
        window->DrawList->AddRectFilled(bb_inner.Min, ImVec2(mid_x, bb_inner.Max.y), GetColorU32(col_rgb_without_alpha), rounding, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotLeft);
    } else
    {
        // Because GetColorU32() multiplies by the global style Alpha and we don't want to display a checkerboard if the source code had no alpha
        ImVec4 col_source = (flags & ImGuiColorEditFlags_AlphaPreview) ? col_rgb : col_rgb_without_alpha;
        if (col_source.w < 1.0f)
            RenderColorRectWithAlphaCheckerboard(window->DrawList, bb_inner.Min, bb_inner.Max, GetColorU32(col_source), grid_step, ImVec2(off, off), rounding);
        else
            window->DrawList->AddRectFilled(bb_inner.Min, bb_inner.Max, GetColorU32(col_source), rounding, ImDrawCornerFlags_All);
    }
    RenderNavHighlight(bb, id);
    if ((flags & ImGuiColorEditFlags_NoBorder) == 0)
    {
        if (g.Style.FrameBorderSize > 0.0f)
            RenderFrameBorder(bb.Min, bb.Max, rounding);
        else
            window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), rounding); // Color button are often in need of some sort of border
    }

    // Drag and Drop Source
    // NB: The ActiveId test is merely an optional micro-optimization, BeginDragDropSource() does the same test.
    if (g.ActiveId == id && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropSource())
    {
        if (flags & ImGuiColorEditFlags_NoAlpha)
            SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F, &col_rgb, sizeof(float) * 3, ImGuiCond_Once);
        else
            SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F, &col_rgb, sizeof(float) * 4, ImGuiCond_Once);
        ColorButton(desc_id, col, flags);
        SameLine();
        TextEx("Color");
        EndDragDropSource();
    }

    // Tooltip
    if (!(flags & ImGuiColorEditFlags_NoTooltip) && hovered)
        ColorTooltip(desc_id, &col.x, flags & (ImGuiColorEditFlags__InputMask | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf));

    return pressed;
}

// Initialize/override default color options
void ImGui::SetColorEditOptions(ImGuiColorEditFlags flags)
{
    ImGuiContext& g = *GImGui;
    if ((flags & ImGuiColorEditFlags__DisplayMask) == 0)
        flags |= ImGuiColorEditFlags__OptionsDefault & ImGuiColorEditFlags__DisplayMask;
    if ((flags & ImGuiColorEditFlags__DataTypeMask) == 0)
        flags |= ImGuiColorEditFlags__OptionsDefault & ImGuiColorEditFlags__DataTypeMask;
    if ((flags & ImGuiColorEditFlags__PickerMask) == 0)
        flags |= ImGuiColorEditFlags__OptionsDefault & ImGuiColorEditFlags__PickerMask;
    if ((flags & ImGuiColorEditFlags__InputMask) == 0)
        flags |= ImGuiColorEditFlags__OptionsDefault & ImGuiColorEditFlags__InputMask;
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags__DisplayMask));    // Check only 1 option is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags__DataTypeMask));   // Check only 1 option is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags__PickerMask));     // Check only 1 option is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags__InputMask));      // Check only 1 option is selected
    g.ColorEditOptions = flags;
}

// Note: only access 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
void ImGui::ColorTooltip(const char* text, const float* col, ImGuiColorEditFlags flags)
{
    ImGuiContext& g = *GImGui;

    BeginTooltipEx(0, ImGuiTooltipFlags_OverridePreviousTooltip);
    const char* text_end = text ? FindRenderedTextEnd(text, NULL) : text;
    if (text_end > text)
    {
        TextEx(text, text_end);
        Separator();
    }

    ImVec2 sz(g.FontSize * 3 + g.Style.FramePadding.y * 2, g.FontSize * 3 + g.Style.FramePadding.y * 2);
    ImVec4 cf(col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
    int cr = IM_F32_TO_INT8_SAT(col[0]), cg = IM_F32_TO_INT8_SAT(col[1]), cb = IM_F32_TO_INT8_SAT(col[2]), ca = (flags & ImGuiColorEditFlags_NoAlpha) ? 255 : IM_F32_TO_INT8_SAT(col[3]);
    ColorButton("##preview", cf, (flags & (ImGuiColorEditFlags__InputMask | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf)) | ImGuiColorEditFlags_NoTooltip, sz);
    SameLine();
    if ((flags & ImGuiColorEditFlags_InputRGB) || !(flags & ImGuiColorEditFlags__InputMask))
    {
        if (flags & ImGuiColorEditFlags_NoAlpha)
            Text("#%02X%02X%02X\nR: %d, G: %d, B: %d\n(%.3f, %.3f, %.3f)", cr, cg, cb, cr, cg, cb, col[0], col[1], col[2]);
        else
            Text("#%02X%02X%02X%02X\nR:%d, G:%d, B:%d, A:%d\n(%.3f, %.3f, %.3f, %.3f)", cr, cg, cb, ca, cr, cg, cb, ca, col[0], col[1], col[2], col[3]);
    } else if (flags & ImGuiColorEditFlags_InputHSV)
    {
        if (flags & ImGuiColorEditFlags_NoAlpha)
            Text("H: %.3f, S: %.3f, V: %.3f", col[0], col[1], col[2]);
        else
            Text("H: %.3f, S: %.3f, V: %.3f, A: %.3f", col[0], col[1], col[2], col[3]);
    }
    EndTooltip();
}

void ImGui::ColorEditOptionsPopup(const float* col, ImGuiColorEditFlags flags)
{
    bool allow_opt_inputs = !(flags & ImGuiColorEditFlags__DisplayMask);
    bool allow_opt_datatype = !(flags & ImGuiColorEditFlags__DataTypeMask);
    if ((!allow_opt_inputs && !allow_opt_datatype) || !BeginPopup("context"))
        return;
    ImGuiContext& g = *GImGui;
    ImGuiColorEditFlags opts = g.ColorEditOptions;
    if (allow_opt_inputs)
    {
        if (RadioButton("RGB", (opts & ImGuiColorEditFlags_DisplayRGB) != 0)) opts = (opts & ~ImGuiColorEditFlags__DisplayMask) | ImGuiColorEditFlags_DisplayRGB;
        if (RadioButton("HSV", (opts & ImGuiColorEditFlags_DisplayHSV) != 0)) opts = (opts & ~ImGuiColorEditFlags__DisplayMask) | ImGuiColorEditFlags_DisplayHSV;
        if (RadioButton("Hex", (opts & ImGuiColorEditFlags_DisplayHex) != 0)) opts = (opts & ~ImGuiColorEditFlags__DisplayMask) | ImGuiColorEditFlags_DisplayHex;
    }
    if (allow_opt_datatype)
    {
        if (allow_opt_inputs) Separator();
        if (RadioButton("0..255", (opts & ImGuiColorEditFlags_Uint8) != 0)) opts = (opts & ~ImGuiColorEditFlags__DataTypeMask) | ImGuiColorEditFlags_Uint8;
        if (RadioButton("0.00..1.00", (opts & ImGuiColorEditFlags_Float) != 0)) opts = (opts & ~ImGuiColorEditFlags__DataTypeMask) | ImGuiColorEditFlags_Float;
    }

    if (allow_opt_inputs || allow_opt_datatype)
        Separator();
    if (Button("Copy as..", ImVec2(-1, 0)))
        OpenPopup("Copy");
    if (BeginPopup("Copy"))
    {
        int cr = IM_F32_TO_INT8_SAT(col[0]), cg = IM_F32_TO_INT8_SAT(col[1]), cb = IM_F32_TO_INT8_SAT(col[2]), ca = (flags & ImGuiColorEditFlags_NoAlpha) ? 255 : IM_F32_TO_INT8_SAT(col[3]);
        char buf[64];
        ImFormatString(buf, IM_ARRAYSIZE(buf), "(%.3ff, %.3ff, %.3ff, %.3ff)", col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
        if (Selectable(buf))
            SetClipboardText(buf);
        ImFormatString(buf, IM_ARRAYSIZE(buf), "(%d,%d,%d,%d)", cr, cg, cb, ca);
        if (Selectable(buf))
            SetClipboardText(buf);
        ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", cr, cg, cb);
        if (Selectable(buf))
            SetClipboardText(buf);
        if (!(flags & ImGuiColorEditFlags_NoAlpha))
        {
            ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", cr, cg, cb, ca);
            if (Selectable(buf))
                SetClipboardText(buf);
        }
        EndPopup();
    }

    g.ColorEditOptions = opts;
    EndPopup();
}

void ImGui::ColorPickerOptionsPopup(const float* ref_col, ImGuiColorEditFlags flags)
{
    bool allow_opt_picker = !(flags & ImGuiColorEditFlags__PickerMask);
    bool allow_opt_alpha_bar = !(flags & ImGuiColorEditFlags_NoAlpha) && !(flags & ImGuiColorEditFlags_AlphaBar);
    if ((!allow_opt_picker && !allow_opt_alpha_bar) || !BeginPopup("context"))
        return;
    ImGuiContext& g = *GImGui;
    if (allow_opt_picker)
    {
        ImVec2 picker_size(g.FontSize * 8, ImMax(g.FontSize * 8 - (GetFrameHeight() + g.Style.ItemInnerSpacing.x), 1.0f)); // FIXME: Picker size copied from main picker function
        PushItemWidth(picker_size.x);
        for (int picker_type = 0; picker_type < 2; picker_type++)
        {
            // Draw small/thumbnail version of each picker type (over an invisible button for selection)
            if (picker_type > 0) Separator();
            PushID(picker_type);
            ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview | (flags & ImGuiColorEditFlags_NoAlpha);
            if (picker_type == 0) picker_flags |= ImGuiColorEditFlags_PickerHueBar;
            if (picker_type == 1) picker_flags |= ImGuiColorEditFlags_PickerHueWheel;
            ImVec2 backup_pos = GetCursorScreenPos();
            if (Selectable("##selectable", false, 0, picker_size)) // By default, Selectable() is closing popup
                g.ColorEditOptions = (g.ColorEditOptions & ~ImGuiColorEditFlags__PickerMask) | (picker_flags & ImGuiColorEditFlags__PickerMask);
            SetCursorScreenPos(backup_pos);
            ImVec4 dummy_ref_col;
            memcpy(&dummy_ref_col, ref_col, sizeof(float) * ((picker_flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4));
            ColorPicker4("##dummypicker", &dummy_ref_col.x, picker_flags);
            PopID();
        }
        PopItemWidth();
    }
    if (allow_opt_alpha_bar)
    {
        if (allow_opt_picker) Separator();
        CheckboxFlags("Alpha Bar", (unsigned int*)&g.ColorEditOptions, ImGuiColorEditFlags_AlphaBar);
    }
    EndPopup();
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: TreeNode, CollapsingHeader, etc.
//-------------------------------------------------------------------------
// - TreeNode()
// - TreeNodeV()
// - TreeNodeEx()
// - TreeNodeExV()
// - TreeNodeBehavior() [Internal]
// - TreePush()
// - TreePop()
// - GetTreeNodeToLabelSpacing()
// - SetNextItemOpen()
// - CollapsingHeader()
//-------------------------------------------------------------------------

bool ImGui::TreeNode(const char* str_id, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = TreeNodeExV(str_id, 0, fmt, args);
    va_end(args);
    return is_open;
}

bool ImGui::TreeNode(const void* ptr_id, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = TreeNodeExV(ptr_id, 0, fmt, args);
    va_end(args);
    return is_open;
}

bool ImGui::TreeNode(const char* label)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    return TreeNodeBehavior(window->GetID(label), 0, label, NULL);
}

bool ImGui::TreeNodeV(const char* str_id, const char* fmt, va_list args)
{
    return TreeNodeExV(str_id, 0, fmt, args);
}

bool ImGui::TreeNodeV(const void* ptr_id, const char* fmt, va_list args)
{
    return TreeNodeExV(ptr_id, 0, fmt, args);
}

bool ImGui::TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    return TreeNodeBehavior(window->GetID(label), flags, label, NULL);
}

bool ImGui::TreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = TreeNodeExV(str_id, flags, fmt, args);
    va_end(args);
    return is_open;
}

bool ImGui::TreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = TreeNodeExV(ptr_id, flags, fmt, args);
    va_end(args);
    return is_open;
}

bool ImGui::TreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const char* label_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
    return TreeNodeBehavior(window->GetID(str_id), flags, g.TempBuffer, label_end);
}

bool ImGui::TreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const char* label_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
    return TreeNodeBehavior(window->GetID(ptr_id), flags, g.TempBuffer, label_end);
}

bool ImGui::TreeNodeBehaviorIsOpen(ImGuiID id, ImGuiTreeNodeFlags flags)
{
    if (flags & ImGuiTreeNodeFlags_Leaf)
        return true;

    // We only write to the tree storage if the user clicks (or explicitly use the SetNextItemOpen function)
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiStorage* storage = window->DC.StateStorage;

    bool is_open;
    if (g.NextItemData.Flags & ImGuiNextItemDataFlags_HasOpen)
    {
        if (g.NextItemData.OpenCond & ImGuiCond_Always)
        {
            is_open = g.NextItemData.OpenVal;
            storage->SetInt(id, is_open);
        } else
        {
            // We treat ImGuiCond_Once and ImGuiCond_FirstUseEver the same because tree node state are not saved persistently.
            const int stored_value = storage->GetInt(id, -1);
            if (stored_value == -1)
            {
                is_open = g.NextItemData.OpenVal;
                storage->SetInt(id, is_open);
            } else
            {
                is_open = stored_value != 0;
            }
        }
    } else
    {
        is_open = storage->GetInt(id, (flags & ImGuiTreeNodeFlags_DefaultOpen) ? 1 : 0) != 0;
    }

    // When logging is enabled, we automatically expand tree nodes (but *NOT* collapsing headers.. seems like sensible behavior).
    // NB- If we are above max depth we still allow manually opened nodes to be logged.
    if (g.LogEnabled && !(flags & ImGuiTreeNodeFlags_NoAutoOpenOnLog) && (window->DC.TreeDepth - g.LogDepthRef) < g.LogDepthToExpand)
        is_open = true;

    return is_open;
}

bool ImGui::TreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
    const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

    if (!label_end)
        label_end = FindRenderedTextEnd(label);
    const ImVec2 label_size = CalcTextSize(label, label_end, false);

    // We vertically grow up to current line height up the typical widget height.
    const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
    ImRect frame_bb;
    frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
    frame_bb.Min.y = window->DC.CursorPos.y;
    frame_bb.Max.x = window->WorkRect.Max.x;
    frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
    if (display_frame)
    {
        // Framed header expand a little outside the default padding, to the edge of InnerClipRect
        // (FIXME: May remove this at some point and make InnerClipRect align with WindowPadding.x instead of WindowPadding.x*0.5f)
        frame_bb.Min.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
        frame_bb.Max.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
    }

    const float text_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);               // Collapser arrow width + Spacing
    const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);                    // Latch before ItemSize changes it
    const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);    // Include collapser
    ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
    ItemSize(ImVec2(text_width, frame_height), padding.y);

    // For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
    ImRect interact_bb = frame_bb;
    if (!display_frame && (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth)) == 0)
        interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;

    // Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
    // For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
    // This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
    const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
    bool is_open = TreeNodeBehaviorIsOpen(id, flags);
    if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);

    bool item_add = ItemAdd(interact_bb, id);
    window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
    window->DC.LastItemDisplayRect = frame_bb;

    if (!item_add)
    {
        if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            TreePushOverrideID(id);
        IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
        return is_open;
    }

    ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
    if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
        button_flags |= ImGuiButtonFlags_AllowItemOverlap;
    if (!is_leaf)
        button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

    // We allow clicking on the arrow section with keyboard modifiers held, in order to easily
    // allow browsing a tree while preserving selection with code implementing multi-selection patterns.
    // When clicking on the rest of the tree node we always disallow keyboard modifiers.
    const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
    const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
    const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
    if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
        button_flags |= ImGuiButtonFlags_NoKeyModifiers;

    // Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
    // Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
    // - Single-click on label = Toggle on MouseUp (default)
    // - Single-click on arrow = Toggle on MouseUp (when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
    // - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
    // - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
    // This makes _OpenOnArrow have a subtle effect on _OpenOnDoubleClick: arrow click reacts on Down rather than Up.
    // It is rather standard that arrow click react on Down rather than Up and we'd be tempted to make it the default
    // (by removing the _OpenOnArrow test below), however this would have a perhaps surprising effect on CollapsingHeader()?
    // So right now we are making this optional. May evolve later.
    // We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
    if (is_mouse_x_over_arrow && (flags & ImGuiTreeNodeFlags_OpenOnArrow))
        button_flags |= ImGuiButtonFlags_PressedOnClick;
    else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    else
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

    bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
    const bool was_selected = selected;

    bool hovered, held;
    bool pressed = ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
    bool toggled = false;
    if (!is_leaf)
    {
        if (pressed && g.DragDropHoldJustPressedId != id)
        {
            if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id))
                toggled = true;
            if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
                toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
            if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseDoubleClicked[0])
                toggled = true;
        } else if (pressed && g.DragDropHoldJustPressedId == id)
        {
            IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
            if (!is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
                toggled = true;
        }

        if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open)
        {
            toggled = true;
            NavMoveRequestCancel();
        }
        if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
        {
            toggled = true;
            NavMoveRequestCancel();
        }

        if (toggled)
        {
            is_open = !is_open;
            window->DC.StateStorage->SetInt(id, is_open);
            window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
        }
    }
    if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
        SetItemAllowOverlap();

    // In this branch, TreeNodeBehavior() cannot toggle the selection so this will never trigger.
    if (selected != was_selected) //-V547
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

    // Render
    const ImU32 text_col = GetColorU32(ImGuiCol_Text);
    ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
    if (display_frame)
    {
        // Framed type
        const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
        RenderNavHighlight(frame_bb, id, nav_highlight_flags);
        if (flags & ImGuiTreeNodeFlags_Bullet)
            RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), text_col);
        else if (!is_leaf)
            RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
        else // Leaf without bullet, left-adjusted text
            text_pos.x -= text_offset_x;
        if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
            frame_bb.Max.x -= g.FontSize + style.FramePadding.x;
        if (g.LogEnabled)
        {
            // NB: '##' is normally used to hide text (as a library-wide feature), so we need to specify the text range to make sure the ## aren't stripped out here.
            const char log_prefix[] = "\n##";
            const char log_suffix[] = "##";
            LogRenderedText(&text_pos, log_prefix, log_prefix + 3);
            RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
            LogRenderedText(&text_pos, log_suffix, log_suffix + 2);
        } else
        {
            RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
        }
    } else
    {
        // Unframed typed for tree nodes
        if (hovered || selected)
        {
            const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
            RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
            RenderNavHighlight(frame_bb, id, nav_highlight_flags);
        }
        if (flags & ImGuiTreeNodeFlags_Bullet)
            RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), text_col);
        else if (!is_leaf)
            RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 0.70f);
        if (g.LogEnabled)
            LogRenderedText(&text_pos, ">");
        RenderText(text_pos, label, label_end, false);
    }

    if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        TreePushOverrideID(id);
    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
    return is_open;
}

void ImGui::TreePush(const char* str_id)
{
    ImGuiWindow* window = GetCurrentWindow();
    Indent();
    window->DC.TreeDepth++;
    PushID(str_id ? str_id : "#TreePush");
}

void ImGui::TreePush(const void* ptr_id)
{
    ImGuiWindow* window = GetCurrentWindow();
    Indent();
    window->DC.TreeDepth++;
    PushID(ptr_id ? ptr_id : (const void*)"#TreePush");
}

void ImGui::TreePushOverrideID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    Indent();
    window->DC.TreeDepth++;
    window->IDStack.push_back(id);
}

void ImGui::TreePop()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    Unindent();

    window->DC.TreeDepth--;
    ImU32 tree_depth_mask = (1 << window->DC.TreeDepth);

    // Handle Left arrow to move to parent tree node (when ImGuiTreeNodeFlags_NavLeftJumpsBackHere is enabled)
    if (g.NavMoveDir == ImGuiDir_Left && g.NavWindow == window && NavMoveRequestButNoResultYet())
        if (g.NavIdIsAlive && (window->DC.TreeJumpToParentOnPopMask & tree_depth_mask))
        {
            SetNavID(window->IDStack.back(), g.NavLayer, 0);
            NavMoveRequestCancel();
        }
    window->DC.TreeJumpToParentOnPopMask &= tree_depth_mask - 1;

    IM_ASSERT(window->IDStack.Size > 1); // There should always be 1 element in the IDStack (pushed during window creation). If this triggers you called TreePop/PopID too much.
    PopID();
}

// Horizontal distance preceding label when using TreeNode() or Bullet()
float ImGui::GetTreeNodeToLabelSpacing()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + (g.Style.FramePadding.x * 2.0f);
}

// Set next TreeNode/CollapsingHeader open state.
void ImGui::SetNextItemOpen(bool is_open, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    if (g.CurrentWindow->SkipItems)
        return;
    g.NextItemData.Flags |= ImGuiNextItemDataFlags_HasOpen;
    g.NextItemData.OpenVal = is_open;
    g.NextItemData.OpenCond = cond ? cond : ImGuiCond_Always;
}

// CollapsingHeader returns true when opened but do not indent nor push into the ID stack (because of the ImGuiTreeNodeFlags_NoTreePushOnOpen flag).
// This is basically the same as calling TreeNodeEx(label, ImGuiTreeNodeFlags_CollapsingHeader). You can remove the _NoTreePushOnOpen flag if you want behavior closer to normal TreeNode().
bool ImGui::CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    return TreeNodeBehavior(window->GetID(label), flags | ImGuiTreeNodeFlags_CollapsingHeader, label);
}

bool ImGui::CollapsingHeader(const char* label, bool* p_open, ImGuiTreeNodeFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if (p_open && !*p_open)
        return false;

    ImGuiID id = window->GetID(label);
    flags |= ImGuiTreeNodeFlags_CollapsingHeader;
    if (p_open)
        flags |= ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_ClipLabelForTrailingButton;
    bool is_open = TreeNodeBehavior(id, flags, label);
    if (p_open)
    {
        // Create a small overlapping close button
        // FIXME: We can evolve this into user accessible helpers to add extra buttons on title bars, headers, etc.
        // FIXME: CloseButton can overlap into text, need find a way to clip the text somehow.
        ImGuiContext& g = *GImGui;
        ImGuiItemHoveredDataBackup last_item_backup;
        float button_size = g.FontSize;
        float button_x = ImMax(window->DC.LastItemRect.Min.x, window->DC.LastItemRect.Max.x - g.Style.FramePadding.x * 2.0f - button_size);
        float button_y = window->DC.LastItemRect.Min.y;
        if (CloseButton(window->GetID((void*)((intptr_t)id + 1)), ImVec2(button_x, button_y)))
            *p_open = false;
        last_item_backup.Restore();
    }

    return is_open;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Selectable
//-------------------------------------------------------------------------
// - Selectable()
//-------------------------------------------------------------------------

// Tip: pass a non-visible label (e.g. "##dummy") then you can use the space to draw other text or image.
// But you need to make sure the ID is unique, e.g. enclose calls in PushID/PopID or use ##unique_id.
// With this scheme, ImGuiSelectableFlags_SpanAllColumns and ImGuiSelectableFlags_AllowItemOverlap are also frequently used flags.
// FIXME: Selectable() with (size.x == 0.0f) and (SelectableTextAlign.x > 0.0f) followed by SameLine() is currently not supported.
bool ImGui::Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns) // FIXME-OPT: Avoid if vertically clipped.
        PushColumnsBackground();

    // Submit label or explicit size to ItemSize(), whereas ItemAdd() will submit a larger/spanning rectangle.
    ImGuiID id = window->GetID(label);
    ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
    ImVec2 pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    ItemSize(size, 0.0f);

    // Fill horizontal space
    const float min_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? window->ContentRegionRect.Min.x : pos.x;
    const float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? window->ContentRegionRect.Max.x : GetContentRegionMaxAbs().x;
    if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth))
        size.x = ImMax(label_size.x, max_x - min_x);

    // Text stays at the submission position, but bounding box may be extended on both sides
    const ImVec2 text_min = pos;
    const ImVec2 text_max(min_x + size.x, pos.y + size.y);

    // Selectables are meant to be tightly packed together with no click-gap, so we extend their box to cover spacing between selectable.
    ImRect bb_enlarged(min_x, pos.y, text_max.x, text_max.y);
    const float spacing_x = style.ItemSpacing.x;
    const float spacing_y = style.ItemSpacing.y;
    const float spacing_L = IM_FLOOR(spacing_x * 0.50f);
    const float spacing_U = IM_FLOOR(spacing_y * 0.50f);
    bb_enlarged.Min.x -= spacing_L;
    bb_enlarged.Min.y -= spacing_U;
    bb_enlarged.Max.x += (spacing_x - spacing_L);
    bb_enlarged.Max.y += (spacing_y - spacing_U);
    //if (g.IO.KeyCtrl) { GetForegroundDrawList()->AddRect(bb_align.Min, bb_align.Max, IM_COL32(255, 0, 0, 255)); }
    //if (g.IO.KeyCtrl) { GetForegroundDrawList()->AddRect(bb_enlarged.Min, bb_enlarged.Max, IM_COL32(0, 255, 0, 255)); }

    bool item_add;
    if (flags & ImGuiSelectableFlags_Disabled)
    {
        ImGuiItemFlags backup_item_flags = window->DC.ItemFlags;
        window->DC.ItemFlags |= ImGuiItemFlags_Disabled | ImGuiItemFlags_NoNavDefaultFocus;
        item_add = ItemAdd(bb_enlarged, id);
        window->DC.ItemFlags = backup_item_flags;
    } else
    {
        item_add = ItemAdd(bb_enlarged, id);
    }
    if (!item_add)
    {
        if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns)
            PopColumnsBackground();
        return false;
    }

    // We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
    ImGuiButtonFlags button_flags = 0;
    if (flags & ImGuiSelectableFlags_NoHoldingActiveID) { button_flags |= ImGuiButtonFlags_NoHoldingActiveId; }
    if (flags & ImGuiSelectableFlags_SelectOnClick) { button_flags |= ImGuiButtonFlags_PressedOnClick; }
    if (flags & ImGuiSelectableFlags_SelectOnRelease) { button_flags |= ImGuiButtonFlags_PressedOnRelease; }
    if (flags & ImGuiSelectableFlags_Disabled) { button_flags |= ImGuiButtonFlags_Disabled; }
    if (flags & ImGuiSelectableFlags_AllowDoubleClick) { button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick; }
    if (flags & ImGuiSelectableFlags_AllowItemOverlap) { button_flags |= ImGuiButtonFlags_AllowItemOverlap; }

    if (flags & ImGuiSelectableFlags_Disabled)
        selected = false;

    const bool was_selected = selected;
    bool hovered, held;
    bool pressed = ButtonBehavior(bb_enlarged, id, &hovered, &held, button_flags);

    // Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with gamepad/keyboard
    if (pressed || (hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover)))
    {
        if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
        {
            g.NavDisableHighlight = true;
            SetNavID(id, window->DC.NavLayerCurrent, window->DC.NavFocusScopeIdCurrent);
        }
    }
    if (pressed)
        MarkItemEdited(id);

    if (flags & ImGuiSelectableFlags_AllowItemOverlap)
        SetItemAllowOverlap();

    // In this branch, Selectable() cannot toggle the selection so this will never trigger.
    if (selected != was_selected) //-V547
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

    // Render
    if (held && (flags & ImGuiSelectableFlags_DrawHoveredWhenHeld))
        hovered = true;
    if (hovered || selected)
    {
        const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        RenderFrame(bb_enlarged.Min, bb_enlarged.Max, col, false, 0.0f);
        RenderNavHighlight(bb_enlarged, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
    }

    if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns)
        PopColumnsBackground();

    if (flags & ImGuiSelectableFlags_Disabled) PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
    RenderTextClipped(text_min, text_max, label, NULL, &label_size, style.SelectableTextAlign, &bb_enlarged);
    if (flags & ImGuiSelectableFlags_Disabled) PopStyleColor();

    // Automatically close popups
    if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
        CloseCurrentPopup();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
    return pressed;
}

bool ImGui::Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
    if (Selectable(label, *p_selected, flags, size_arg))
    {
        *p_selected = !*p_selected;
        return true;
    }
    return false;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ListBox
//-------------------------------------------------------------------------
// - ListBox()
// - ListBoxHeader()
// - ListBoxFooter()
//-------------------------------------------------------------------------
// FIXME: This is an old API. We should redesign some of it, rename ListBoxHeader->BeginListBox, ListBoxFooter->EndListBox
// and promote using them over existing ListBox() functions, similarly to change with combo boxes.
//-------------------------------------------------------------------------

// FIXME: In principle this function should be called BeginListBox(). We should rename it after re-evaluating if we want to keep the same signature.
// Helper to calculate the size of a listbox and display a label on the right.
// Tip: To have a list filling the entire window width, PushItemWidth(-1) and pass an non-visible label e.g. "##empty"
bool ImGui::ListBoxHeader(const char* label, const ImVec2& size_arg)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    // Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
    ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), GetTextLineHeightWithSpacing() * 7.4f + style.ItemSpacing.y);
    ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
    ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    window->DC.LastItemRect = bb; // Forward storage for ListBoxFooter.. dodgy.
    g.NextItemData.ClearFlags();

    if (!IsRectVisible(bb.Min, bb.Max))
    {
        ItemSize(bb.GetSize(), style.FramePadding.y);
        ItemAdd(bb, 0, &frame_bb);
        return false;
    }

    BeginGroup();
    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    BeginChildFrame(id, frame_bb.GetSize());
    return true;
}

// FIXME: In principle this function should be called EndListBox(). We should rename it after re-evaluating if we want to keep the same signature.
bool ImGui::ListBoxHeader(const char* label, int items_count, int height_in_items)
{
    // Size default to hold ~7.25 items.
    // We add +25% worth of item height to allow the user to see at a glance if there are more items up/down, without looking at the scrollbar.
    // We don't add this extra bit if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.
    // I am expecting that someone will come and complain about this behavior in a remote future, then we can advise on a better solution.
    if (height_in_items < 0)
        height_in_items = ImMin(items_count, 7);
    const ImGuiStyle& style = GetStyle();
    float height_in_items_f = (height_in_items < items_count) ? (height_in_items + 0.25f) : (height_in_items + 0.00f);

    // We include ItemSpacing.y so that a list sized for the exact number of items doesn't make a scrollbar appears. We could also enforce that by passing a flag to BeginChild().
    ImVec2 size;
    size.x = 0.0f;
    size.y = ImFloor(GetTextLineHeightWithSpacing() * height_in_items_f + style.FramePadding.y * 2.0f);
    return ListBoxHeader(label, size);
}

// FIXME: In principle this function should be called EndListBox(). We should rename it after re-evaluating if we want to keep the same signature.
void ImGui::ListBoxFooter()
{
    ImGuiWindow* parent_window = GetCurrentWindow()->ParentWindow;
    const ImRect bb = parent_window->DC.LastItemRect;
    const ImGuiStyle& style = GetStyle();

    EndChildFrame();

    // Redeclare item size so that it includes the label (we have stored the full size in LastItemRect)
    // We call SameLine() to restore DC.CurrentLine* data
    SameLine();
    parent_window->DC.CursorPos = bb.Min;
    ItemSize(bb, style.FramePadding.y);
    EndGroup();
}

bool ImGui::ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_items)
{
    const bool value_changed = ListBox(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_items);
    return value_changed;
}

bool ImGui::ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items)
{
    if (!ListBoxHeader(label, items_count, height_in_items))
        return false;

    // Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const bool item_selected = (i == *current_item);
            const char* item_text;
            if (!items_getter(data, i, &item_text))
                item_text = "*Unknown item*";

            PushID(i);
            if (Selectable(item_text, item_selected))
            {
                *current_item = i;
                value_changed = true;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }
    ListBoxFooter();
    if (value_changed)
        MarkItemEdited(g.CurrentWindow->DC.LastItemId);

    return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: PlotLines, PlotHistogram
//-------------------------------------------------------------------------
// - PlotEx() [Internal]
// - PlotLines()
// - PlotHistogram()
//-------------------------------------------------------------------------

int ImGui::PlotEx(ImGuiPlotType plot_type, const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 frame_size)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return -1;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    if (frame_size.x == 0.0f)
        frame_size.x = CalcItemWidth();
    if (frame_size.y == 0.0f)
        frame_size.y = label_size.y + (style.FramePadding.y * 2);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, 0, &frame_bb))
        return -1;
    const bool hovered = ItemHoverable(frame_bb, id);

    // Determine scale from values if not specified
    if (scale_min == FLT_MAX || scale_max == FLT_MAX)
    {
        float v_min = FLT_MAX;
        float v_max = -FLT_MAX;
        for (int i = 0; i < values_count; i++)
        {
            const float v = values_getter(data, i);
            if (v != v) // Ignore NaN values
                continue;
            v_min = ImMin(v_min, v);
            v_max = ImMax(v_max, v);
        }
        if (scale_min == FLT_MAX)
            scale_min = v_min;
        if (scale_max == FLT_MAX)
            scale_max = v_max;
    }

    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    const int values_count_min = (plot_type == ImGuiPlotType_Lines) ? 2 : 1;
    int idx_hovered = -1;
    if (values_count >= values_count_min)
    {
        int res_w = ImMin((int)frame_size.x, values_count) + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0);
        int item_count = values_count + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0);

        // Tooltip on hover
        if (hovered && inner_bb.Contains(g.IO.MousePos))
        {
            const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
            const int v_idx = (int)(t * item_count);
            IM_ASSERT(v_idx >= 0 && v_idx < values_count);

            const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
            const float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);
            if (plot_type == ImGuiPlotType_Lines)
                SetTooltip("%d: %8.4g\n%d: %8.4g", v_idx, v0, v_idx + 1, v1);
            else if (plot_type == ImGuiPlotType_Histogram)
                SetTooltip("%d: %8.4g", v_idx, v0);
            idx_hovered = v_idx;
        }

        const float t_step = 1.0f / (float)res_w;
        const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

        float v0 = values_getter(data, (0 + values_offset) % values_count);
        float t0 = 0.0f;
        ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale));                       // Point in the normalized space of our target rectangle
        float histogram_zero_line_t = (scale_min * scale_max < 0.0f) ? (-scale_min * inv_scale) : (scale_min < 0.0f ? 0.0f : 1.0f);   // Where does the zero line stands

        const ImU32 col_base = GetColorU32((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLines : ImGuiCol_PlotHistogram);
        const ImU32 col_hovered = GetColorU32((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLinesHovered : ImGuiCol_PlotHistogramHovered);

        for (int n = 0; n < res_w; n++)
        {
            const float t1 = t0 + t_step;
            const int v1_idx = (int)(t0 * item_count + 0.5f);
            IM_ASSERT(v1_idx >= 0 && v1_idx < values_count);
            const float v1 = values_getter(data, (v1_idx + values_offset + 1) % values_count);
            const ImVec2 tp1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale));

            // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
            ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
            ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, (plot_type == ImGuiPlotType_Lines) ? tp1 : ImVec2(tp1.x, histogram_zero_line_t));
            if (plot_type == ImGuiPlotType_Lines)
            {
                window->DrawList->AddLine(pos0, pos1, idx_hovered == v1_idx ? col_hovered : col_base);
            } else if (plot_type == ImGuiPlotType_Histogram)
            {
                if (pos1.x >= pos0.x + 2.0f)
                    pos1.x -= 1.0f;
                window->DrawList->AddRectFilled(pos0, pos1, idx_hovered == v1_idx ? col_hovered : col_base);
            }

            t0 = t1;
            tp0 = tp1;
        }
    }

    // Text overlay
    if (overlay_text)
        RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f, 0.0f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);

    // Return hovered index or -1 if none are hovered.
    // This is currently not exposed in the public API because we need a larger redesign of the whole thing, but in the short-term we are making it available in PlotEx().
    return idx_hovered;
}

struct ImGuiPlotArrayGetterData
{
    const float* Values;
    int Stride;

    ImGuiPlotArrayGetterData(const float* values, int stride) { Values = values; Stride = stride; }
};

static float Plot_ArrayGetter(void* data, int idx)
{
    ImGuiPlotArrayGetterData* plot_data = (ImGuiPlotArrayGetterData*)data;
    const float v = *(const float*)(const void*)((const unsigned char*)plot_data->Values + (size_t)idx * plot_data->Stride);
    return v;
}

void ImGui::PlotLines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, int stride)
{
    ImGuiPlotArrayGetterData data(values, stride);
    PlotEx(ImGuiPlotType_Lines, label, &Plot_ArrayGetter, (void*)&data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ImGui::PlotLines(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    PlotEx(ImGuiPlotType_Lines, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ImGui::PlotHistogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, int stride)
{
    ImGuiPlotArrayGetterData data(values, stride);
    PlotEx(ImGuiPlotType_Histogram, label, &Plot_ArrayGetter, (void*)&data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ImGui::PlotHistogram(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    PlotEx(ImGuiPlotType_Histogram, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Value helpers
// Those is not very useful, legacy API.
//-------------------------------------------------------------------------
// - Value()
//-------------------------------------------------------------------------

void ImGui::Value(const char* prefix, bool b)
{
    Text("%s: %s", prefix, (b ? "true" : "false"));
}

void ImGui::Value(const char* prefix, int v)
{
    Text("%s: %d", prefix, v);
}

void ImGui::Value(const char* prefix, unsigned int v)
{
    Text("%s: %d", prefix, v);
}

void ImGui::Value(const char* prefix, float v, const char* float_format)
{
    if (float_format)
    {
        char fmt[64];
        ImFormatString(fmt, IM_ARRAYSIZE(fmt), "%%s: %s", float_format);
        Text(fmt, prefix, v);
    } else
    {
        Text("%s: %.3f", prefix, v);
    }
}

//-------------------------------------------------------------------------
// [SECTION] MenuItem, BeginMenu, EndMenu, etc.
//-------------------------------------------------------------------------
// - ImGuiMenuColumns [Internal]
// - BeginMenuBar()
// - EndMenuBar()
// - BeginMainMenuBar()
// - EndMainMenuBar()
// - BeginMenu()
// - EndMenu()
// - MenuItem()
//-------------------------------------------------------------------------

// Helpers for internal use
ImGuiMenuColumns::ImGuiMenuColumns()
{
    Spacing = Width = NextWidth = 0.0f;
    memset(Pos, 0, sizeof(Pos));
    memset(NextWidths, 0, sizeof(NextWidths));
}

void ImGuiMenuColumns::Update(int count, float spacing, bool clear)
{
    IM_ASSERT(count == IM_ARRAYSIZE(Pos));
    IM_UNUSED(count);
    Width = NextWidth = 0.0f;
    Spacing = spacing;
    if (clear)
        memset(NextWidths, 0, sizeof(NextWidths));
    for (int i = 0; i < IM_ARRAYSIZE(Pos); i++)
    {
        if (i > 0 && NextWidths[i] > 0.0f)
            Width += Spacing;
        Pos[i] = IM_FLOOR(Width);
        Width += NextWidths[i];
        NextWidths[i] = 0.0f;
    }
}

float ImGuiMenuColumns::DeclColumns(float w0, float w1, float w2) // not using va_arg because they promote float to double
{
    NextWidth = 0.0f;
    NextWidths[0] = ImMax(NextWidths[0], w0);
    NextWidths[1] = ImMax(NextWidths[1], w1);
    NextWidths[2] = ImMax(NextWidths[2], w2);
    for (int i = 0; i < IM_ARRAYSIZE(Pos); i++)
        NextWidth += NextWidths[i] + ((i > 0 && NextWidths[i] > 0.0f) ? Spacing : 0.0f);
    return ImMax(Width, NextWidth);
}

float ImGuiMenuColumns::CalcExtraSpace(float avail_w) const
{
    return ImMax(0.0f, avail_w - Width);
}

// FIXME: Provided a rectangle perhaps e.g. a BeginMenuBarEx() could be used anywhere..
// Currently the main responsibility of this function being to setup clip-rect + horizontal layout + menu navigation layer.
// Ideally we also want this to be responsible for claiming space out of the main window scrolling rectangle, in which case ImGuiWindowFlags_MenuBar will become unnecessary.
// Then later the same system could be used for multiple menu-bars, scrollbars, side-bars.
bool ImGui::BeginMenuBar()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    if (!(window->Flags & ImGuiWindowFlags_MenuBar))
        return false;

    IM_ASSERT(!window->DC.MenuBarAppending);
    BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
    PushID("##menubar");

    // We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
    // We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
    ImRect bar_rect = window->MenuBarRect();
    ImRect clip_rect(IM_ROUND(bar_rect.Min.x + window->WindowBorderSize), IM_ROUND(bar_rect.Min.y + window->WindowBorderSize), IM_ROUND(ImMax(bar_rect.Min.x, bar_rect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize))), IM_ROUND(bar_rect.Max.y));
    clip_rect.ClipWith(window->OuterRectClipped);
    PushClipRect(clip_rect.Min, clip_rect.Max, false);

    window->DC.CursorPos = ImVec2(bar_rect.Min.x + window->DC.MenuBarOffset.x, bar_rect.Min.y + window->DC.MenuBarOffset.y);
    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
    window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);
    window->DC.MenuBarAppending = true;
    AlignTextToFramePadding();
    return true;
}

void ImGui::EndMenuBar()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    ImGuiContext& g = *GImGui;

    // Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
    if (NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
    {
        ImGuiWindow* nav_earliest_child = g.NavWindow;
        while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
            nav_earliest_child = nav_earliest_child->ParentWindow;
        if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && g.NavMoveRequestForward == ImGuiNavForward_None)
        {
            // To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
            // This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth the hassle/cost)
            const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
            IM_ASSERT(window->DC.NavLayerActiveMaskNext & (1 << layer)); // Sanity check
            FocusWindow(window);
            SetNavIDWithRectRel(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
            g.NavLayer = layer;
            g.NavDisableHighlight = true; // Hide highlight for the current frame so we don't see the intermediary selection.
            g.NavMoveRequestForward = ImGuiNavForward_ForwardQueued;
            NavMoveRequestCancel();
        }
    }

    IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar);
    IM_ASSERT(window->DC.MenuBarAppending);
    PopClipRect();
    PopID();
    window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->MenuBarRect().Min.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
    window->DC.GroupStack.back().EmitItem = false;
    EndGroup(); // Restore position on layer 0
    window->DC.LayoutType = ImGuiLayoutType_Vertical;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
    window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
    window->DC.MenuBarAppending = false;
}

// For the main menu bar, which cannot be moved, we honor g.Style.DisplaySafeAreaPadding to ensure text can be visible on a TV set.
bool ImGui::BeginMainMenuBar()
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.MenuBarOffsetMinVal = ImVec2(g.Style.DisplaySafeAreaPadding.x, ImMax(g.Style.DisplaySafeAreaPadding.y - g.Style.FramePadding.y, 0.0f));
    SetNextWindowPos(ImVec2(0.0f, 0.0f));
    SetNextWindowSize(ImVec2(g.IO.DisplaySize.x, g.NextWindowData.MenuBarOffsetMinVal.y + g.FontBaseSize + g.Style.FramePadding.y));
    PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    bool is_open = Begin("##MainMenuBar", NULL, window_flags) && BeginMenuBar();
    PopStyleVar(2);
    g.NextWindowData.MenuBarOffsetMinVal = ImVec2(0.0f, 0.0f);
    if (!is_open)
    {
        End();
        return false;
    }
    return true; //-V1020
}

void ImGui::EndMainMenuBar()
{
    EndMenuBar();

    // When the user has left the menu layer (typically: closed menus through activation of an item), we restore focus to the previous window
    // FIXME: With this strategy we won't be able to restore a NULL focus.
    ImGuiContext& g = *GImGui;
    if (g.CurrentWindow == g.NavWindow && g.NavLayer == ImGuiNavLayer_Main && !g.NavAnyRequest)
        FocusTopMostWindowUnderOne(g.NavWindow, NULL);

    End();
}

bool ImGui::BeginMenu(const char* label, bool enabled)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    bool menu_is_open = IsPopupOpen(id);

    // Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
    ImGuiWindowFlags flags = ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
    if (window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu))
        flags |= ImGuiWindowFlags_ChildWindow;

    // If a menu with same the ID was already submitted, we will append to it, matching the behavior of Begin().
    // We are relying on a O(N) search - so O(N log N) over the frame - which seems like the most efficient for the expected small amount of BeginMenu() calls per frame.
    // If somehow this is ever becoming a problem we can switch to use e.g. a ImGuiStorager mapping key to last frame used.
    if (g.MenusIdSubmittedThisFrame.contains(id))
    {
        if (menu_is_open)
            menu_is_open = BeginPopupEx(id, flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
        else
            g.NextWindowData.ClearFlags();          // we behave like Begin() and need to consume those values
        return menu_is_open;
    }

    // Tag menu as used. Next time BeginMenu() with same ID is called it will append to existing menu
    g.MenusIdSubmittedThisFrame.push_back(id);

    ImVec2 label_size = CalcTextSize(label, NULL, true);
    bool pressed;
    bool menuset_is_open = !(window->Flags & ImGuiWindowFlags_Popup) && (g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].OpenParentId == window->IDStack.back());
    ImGuiWindow* backed_nav_window = g.NavWindow;
    if (menuset_is_open)
        g.NavWindow = window;  // Odd hack to allow hovering across menus of a same menu-set (otherwise we wouldn't be able to hover parent)

    // The reference position stored in popup_pos will be used by Begin() to find a suitable position for the child menu,
    // However the final position is going to be different! It is chosen by FindBestWindowPosForPopup().
    // e.g. Menus tend to overlap each other horizontally to amplify relative Z-ordering.
    ImVec2 popup_pos, pos = window->DC.CursorPos;
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
    {
        // Menu inside an horizontal menu bar
        // Selectable extend their highlight by half ItemSpacing in each direction.
        // For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
        popup_pos = ImVec2(pos.x - 1.0f - IM_FLOOR(style.ItemSpacing.x * 0.5f), pos.y - style.FramePadding.y + window->MenuBarHeight());
        window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * 0.5f);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
        float w = label_size.x;
        pressed = Selectable(label, menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_DontClosePopups | (!enabled ? ImGuiSelectableFlags_Disabled : 0), ImVec2(w, 0.0f));
        PopStyleVar();
        window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
    } else
    {
        // Menu inside a menu
        // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
        //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
        popup_pos = ImVec2(pos.x, pos.y - style.WindowPadding.y);
        float min_w = window->DC.MenuColumns.DeclColumns(label_size.x, 0.0f, IM_FLOOR(g.FontSize * 1.20f)); // Feedback to next frame
        float extra_w = ImMax(0.0f, GetContentRegionAvail().x - min_w);
        pressed = Selectable(label, menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAvailWidth | (!enabled ? ImGuiSelectableFlags_Disabled : 0), ImVec2(min_w, 0.0f));
        ImU32 text_col = GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        RenderArrow(window->DrawList, pos + ImVec2(window->DC.MenuColumns.Pos[2] + extra_w + g.FontSize * 0.30f, 0.0f), text_col, ImGuiDir_Right);
    }

    const bool hovered = enabled && ItemHoverable(window->DC.LastItemRect, id);
    if (menuset_is_open)
        g.NavWindow = backed_nav_window;

    bool want_open = false;
    bool want_close = false;
    if (window->DC.LayoutType == ImGuiLayoutType_Vertical) // (window->Flags & (ImGuiWindowFlags_Popup|ImGuiWindowFlags_ChildMenu))
    {
        // Close menu when not hovering it anymore unless we are moving roughly in the direction of the menu
        // Implement http://bjk5.com/post/44698559168/breaking-down-amazons-mega-dropdown to avoid using timers, so menus feels more reactive.
        bool moving_toward_other_child_menu = false;

        ImGuiWindow* child_menu_window = (g.BeginPopupStack.Size < g.OpenPopupStack.Size&& g.OpenPopupStack[g.BeginPopupStack.Size].SourceWindow == window) ? g.OpenPopupStack[g.BeginPopupStack.Size].Window : NULL;
        if (g.HoveredWindow == window && child_menu_window != NULL && !(window->Flags & ImGuiWindowFlags_MenuBar))
        {
            // FIXME-DPI: Values should be derived from a master "scale" factor.
            ImRect next_window_rect = child_menu_window->Rect();
            ImVec2 ta = g.IO.MousePos - g.IO.MouseDelta;
            ImVec2 tb = (window->Pos.x < child_menu_window->Pos.x) ? next_window_rect.GetTL() : next_window_rect.GetTR();
            ImVec2 tc = (window->Pos.x < child_menu_window->Pos.x) ? next_window_rect.GetBL() : next_window_rect.GetBR();
            float extra = ImClamp(ImFabs(ta.x - tb.x) * 0.30f, 5.0f, 30.0f);    // add a bit of extra slack.
            ta.x += (window->Pos.x < child_menu_window->Pos.x) ? -0.5f : +0.5f; // to avoid numerical issues
            tb.y = ta.y + ImMax((tb.y - extra) - ta.y, -100.0f);                // triangle is maximum 200 high to limit the slope and the bias toward large sub-menus // FIXME: Multiply by fb_scale?
            tc.y = ta.y + ImMin((tc.y + extra) - ta.y, +100.0f);
            moving_toward_other_child_menu = ImTriangleContainsPoint(ta, tb, tc, g.IO.MousePos);
            //GetForegroundDrawList()->AddTriangleFilled(ta, tb, tc, moving_within_opened_triangle ? IM_COL32(0,128,0,128) : IM_COL32(128,0,0,128)); // [DEBUG]
        }
        if (menu_is_open && !hovered && g.HoveredWindow == window && g.HoveredIdPreviousFrame != 0 && g.HoveredIdPreviousFrame != id && !moving_toward_other_child_menu)
            want_close = true;

        if (!menu_is_open && hovered && pressed) // Click to open
            want_open = true;
        else if (!menu_is_open && hovered && !moving_toward_other_child_menu) // Hover to open
            want_open = true;

        if (g.NavActivateId == id)
        {
            want_close = menu_is_open;
            want_open = !menu_is_open;
        }
        if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right) // Nav-Right to open
        {
            want_open = true;
            NavMoveRequestCancel();
        }
    } else
    {
        // Menu bar
        if (menu_is_open && pressed && menuset_is_open) // Click an open menu again to close it
        {
            want_close = true;
            want_open = menu_is_open = false;
        } else if (pressed || (hovered && menuset_is_open && !menu_is_open)) // First click to open, then hover to open others
        {
            want_open = true;
        } else if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Down) // Nav-Down to open
        {
            want_open = true;
            NavMoveRequestCancel();
        }
    }

    if (!enabled) // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
        want_close = true;
    if (want_close && IsPopupOpen(id))
        ClosePopupToLevel(g.BeginPopupStack.Size, true);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Openable | (menu_is_open ? ImGuiItemStatusFlags_Opened : 0));

    if (!menu_is_open && want_open && g.OpenPopupStack.Size > g.BeginPopupStack.Size)
    {
        // Don't recycle same menu level in the same frame, first close the other menu and yield for a frame.
        OpenPopup(label);
        return false;
    }

    menu_is_open |= want_open;
    if (want_open)
        OpenPopup(label);

    if (menu_is_open)
    {
        SetNextWindowPos(popup_pos, ImGuiCond_Always);
        menu_is_open = BeginPopupEx(id, flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
    } else
    {
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    }

    return menu_is_open;
}

void ImGui::EndMenu()
{
    // Nav: When a left move request _within our child menu_ failed, close ourselves (the _parent_ menu).
    // A menu doesn't close itself because EndMenuBar() wants the catch the last Left<>Right inputs.
    // However, it means that with the current code, a BeginMenu() from outside another menu or a menu-bar won't be closable with the Left direction.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavWindow && g.NavWindow->ParentWindow == window && g.NavMoveDir == ImGuiDir_Left && NavMoveRequestButNoResultYet() && window->DC.LayoutType == ImGuiLayoutType_Vertical)
    {
        ClosePopupToLevel(g.BeginPopupStack.Size, true);
        NavMoveRequestCancel();
    }

    EndPopup();
}

bool ImGui::MenuItem(const char* label, const char* shortcut, bool selected, bool enabled)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 label_size = CalcTextSize(label, NULL, true);

    // We've been using the equivalent of ImGuiSelectableFlags_SetNavIdOnHover on all Selectable() since early Nav system days (commit 43ee5d73),
    // but I am unsure whether this should be kept at all. For now moved it to be an opt-in feature used by menus only.
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_SelectOnRelease | ImGuiSelectableFlags_SetNavIdOnHover | (enabled ? 0 : ImGuiSelectableFlags_Disabled);
    bool pressed;
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
    {
        // Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
        // Note that in this situation we render neither the shortcut neither the selected tick mark
        float w = label_size.x;
        window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * 0.5f);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
        pressed = Selectable(label, false, flags, ImVec2(w, 0.0f));
        PopStyleVar();
        window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
    } else
    {
        // Menu item inside a vertical menu
        // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
        //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
        float shortcut_w = shortcut ? CalcTextSize(shortcut, NULL).x : 0.0f;
        float min_w = window->DC.MenuColumns.DeclColumns(label_size.x, shortcut_w, IM_FLOOR(g.FontSize * 1.20f)); // Feedback for next frame
        float extra_w = ImMax(0.0f, GetContentRegionAvail().x - min_w);
        pressed = Selectable(label, false, flags | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(min_w, 0.0f));
        if (shortcut_w > 0.0f)
        {
            PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
            RenderText(pos + ImVec2(window->DC.MenuColumns.Pos[1] + extra_w, 0.0f), shortcut, NULL, false);
            PopStyleColor();
        }
        if (selected)
            RenderCheckMark(window->DrawList, pos + ImVec2(window->DC.MenuColumns.Pos[2] + extra_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f), GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), g.FontSize * 0.866f);
    }

    IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (selected ? ImGuiItemStatusFlags_Checked : 0));
    return pressed;
}

bool ImGui::MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled)
{
    if (MenuItem(label, shortcut, p_selected ? *p_selected : false, enabled))
    {
        if (p_selected)
            *p_selected = !*p_selected;
        return true;
    }
    return false;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: BeginTabBar, EndTabBar, etc.
//-------------------------------------------------------------------------
// - BeginTabBar()
// - BeginTabBarEx() [Internal]
// - EndTabBar()
// - TabBarLayout() [Internal]
// - TabBarCalcTabID() [Internal]
// - TabBarCalcMaxTabWidth() [Internal]
// - TabBarFindTabById() [Internal]
// - TabBarRemoveTab() [Internal]
// - TabBarCloseTab() [Internal]
// - TabBarScrollClamp()v
// - TabBarScrollToTab() [Internal]
// - TabBarQueueChangeTabOrder() [Internal]
// - TabBarScrollingButtons() [Internal]
// - TabBarTabListPopupButton() [Internal]
//-------------------------------------------------------------------------

namespace ImGui
{
    static void             TabBarLayout(ImGuiTabBar* tab_bar);
    static ImU32            TabBarCalcTabID(ImGuiTabBar* tab_bar, const char* label);
    static float            TabBarCalcMaxTabWidth();
    static float            TabBarScrollClamp(ImGuiTabBar* tab_bar, float scrolling);
    static void             TabBarScrollToTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab);
    static ImGuiTabItem* TabBarScrollingButtons(ImGuiTabBar* tab_bar);
    static ImGuiTabItem* TabBarTabListPopupButton(ImGuiTabBar* tab_bar);
}

ImGuiTabBar::ImGuiTabBar()
{
    ID = 0;
    SelectedTabId = NextSelectedTabId = VisibleTabId = 0;
    CurrFrameVisible = PrevFrameVisible = -1;
    LastTabContentHeight = 0.0f;
    OffsetMax = OffsetMaxIdeal = OffsetNextTab = 0.0f;
    ScrollingAnim = ScrollingTarget = ScrollingTargetDistToVisibility = ScrollingSpeed = 0.0f;
    Flags = ImGuiTabBarFlags_None;
    ReorderRequestTabId = 0;
    ReorderRequestDir = 0;
    WantLayout = VisibleTabWasSubmitted = false;
    LastTabItemIdx = -1;
}

static int IMGUI_CDECL TabItemComparerByVisibleOffset(const void* lhs, const void* rhs)
{
    const ImGuiTabItem* a = (const ImGuiTabItem*)lhs;
    const ImGuiTabItem* b = (const ImGuiTabItem*)rhs;
    return (int)(a->Offset - b->Offset);
}

static ImGuiTabBar* GetTabBarFromTabBarRef(const ImGuiPtrOrIndex& ref)
{
    ImGuiContext& g = *GImGui;
    return ref.Ptr ? (ImGuiTabBar*)ref.Ptr : g.TabBars.GetByIndex(ref.Index);
}

static ImGuiPtrOrIndex GetTabBarRefFromTabBar(ImGuiTabBar* tab_bar)
{
    ImGuiContext& g = *GImGui;
    if (g.TabBars.Contains(tab_bar))
        return ImGuiPtrOrIndex(g.TabBars.GetIndex(tab_bar));
    return ImGuiPtrOrIndex(tab_bar);
}

bool    ImGui::BeginTabBar(const char* str_id, ImGuiTabBarFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    ImGuiID id = window->GetID(str_id);
    ImGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(id);
    ImRect tab_bar_bb = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->WorkRect.Max.x, window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
    tab_bar->ID = id;
    return BeginTabBarEx(tab_bar, tab_bar_bb, flags | ImGuiTabBarFlags_IsFocused);
}

bool    ImGui::BeginTabBarEx(ImGuiTabBar* tab_bar, const ImRect& tab_bar_bb, ImGuiTabBarFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    if ((flags & ImGuiTabBarFlags_DockNode) == 0)
        PushOverrideID(tab_bar->ID);

    // Add to stack
    g.CurrentTabBarStack.push_back(GetTabBarRefFromTabBar(tab_bar));
    g.CurrentTabBar = tab_bar;

    if (tab_bar->CurrFrameVisible == g.FrameCount)
    {
        //IMGUI_DEBUG_LOG("BeginTabBarEx already called this frame\n", g.FrameCount);
        IM_ASSERT(0);
        return true;
    }

    // When toggling back from ordered to manually-reorderable, shuffle tabs to enforce the last visible order.
    // Otherwise, the most recently inserted tabs would move at the end of visible list which can be a little too confusing or magic for the user.
    if ((flags & ImGuiTabBarFlags_Reorderable) && !(tab_bar->Flags & ImGuiTabBarFlags_Reorderable) && tab_bar->Tabs.Size > 1 && tab_bar->PrevFrameVisible != -1)
        ImQsort(tab_bar->Tabs.Data, tab_bar->Tabs.Size, sizeof(ImGuiTabItem), TabItemComparerByVisibleOffset);

    // Flags
    if ((flags & ImGuiTabBarFlags_FittingPolicyMask_) == 0)
        flags |= ImGuiTabBarFlags_FittingPolicyDefault_;

    tab_bar->Flags = flags;
    tab_bar->BarRect = tab_bar_bb;
    tab_bar->WantLayout = true; // Layout will be done on the first call to ItemTab()
    tab_bar->PrevFrameVisible = tab_bar->CurrFrameVisible;
    tab_bar->CurrFrameVisible = g.FrameCount;
    tab_bar->FramePadding = g.Style.FramePadding;

    // Layout
    ItemSize(ImVec2(tab_bar->OffsetMaxIdeal, tab_bar->BarRect.GetHeight()), tab_bar->FramePadding.y);
    window->DC.CursorPos.x = tab_bar->BarRect.Min.x;

    // Draw separator
    const ImU32 col = GetColorU32((flags & ImGuiTabBarFlags_IsFocused) ? ImGuiCol_TabActive : ImGuiCol_TabUnfocusedActive);
    const float y = tab_bar->BarRect.Max.y - 1.0f;
    {
        const float separator_min_x = tab_bar->BarRect.Min.x - IM_FLOOR(window->WindowPadding.x * 0.5f);
        const float separator_max_x = tab_bar->BarRect.Max.x + IM_FLOOR(window->WindowPadding.x * 0.5f);
        window->DrawList->AddLine(ImVec2(separator_min_x, y), ImVec2(separator_max_x, y), col, 1.0f);
    }
    return true;
}

void    ImGui::EndTabBar()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    ImGuiTabBar* tab_bar = g.CurrentTabBar;
    if (tab_bar == NULL)
    {
        IM_ASSERT_USER_ERROR(tab_bar != NULL, "Mismatched BeginTabBar()/EndTabBar()!");
        return;
    }
    if (tab_bar->WantLayout)
        TabBarLayout(tab_bar);

    // Restore the last visible height if no tab is visible, this reduce vertical flicker/movement when a tabs gets removed without calling SetTabItemClosed().
    const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
    if (tab_bar->VisibleTabWasSubmitted || tab_bar->VisibleTabId == 0 || tab_bar_appearing)
        tab_bar->LastTabContentHeight = ImMax(window->DC.CursorPos.y - tab_bar->BarRect.Max.y, 0.0f);
    else
        window->DC.CursorPos.y = tab_bar->BarRect.Max.y + tab_bar->LastTabContentHeight;

    if ((tab_bar->Flags & ImGuiTabBarFlags_DockNode) == 0)
        PopID();

    g.CurrentTabBarStack.pop_back();
    g.CurrentTabBar = g.CurrentTabBarStack.empty() ? NULL : GetTabBarFromTabBarRef(g.CurrentTabBarStack.back());
}

// This is called only once a frame before by the first call to ItemTab()
// The reason we're not calling it in BeginTabBar() is to leave a chance to the user to call the SetTabItemClosed() functions.
static void ImGui::TabBarLayout(ImGuiTabBar* tab_bar)
{
    ImGuiContext& g = *GImGui;
    tab_bar->WantLayout = false;

    // Garbage collect
    int tab_dst_n = 0;
    for (int tab_src_n = 0; tab_src_n < tab_bar->Tabs.Size; tab_src_n++)
    {
        ImGuiTabItem* tab = &tab_bar->Tabs[tab_src_n];
        if (tab->LastFrameVisible < tab_bar->PrevFrameVisible)
        {
            if (tab->ID == tab_bar->SelectedTabId)
                tab_bar->SelectedTabId = 0;
            continue;
        }
        if (tab_dst_n != tab_src_n)
            tab_bar->Tabs[tab_dst_n] = tab_bar->Tabs[tab_src_n];
        tab_dst_n++;
    }
    if (tab_bar->Tabs.Size != tab_dst_n)
        tab_bar->Tabs.resize(tab_dst_n);

    // Setup next selected tab
    ImGuiID scroll_track_selected_tab_id = 0;
    if (tab_bar->NextSelectedTabId)
    {
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId;
        tab_bar->NextSelectedTabId = 0;
        scroll_track_selected_tab_id = tab_bar->SelectedTabId;
    }

    // Process order change request (we could probably process it when requested but it's just saner to do it in a single spot).
    if (tab_bar->ReorderRequestTabId != 0)
    {
        if (ImGuiTabItem* tab1 = TabBarFindTabByID(tab_bar, tab_bar->ReorderRequestTabId))
        {
            //IM_ASSERT(tab_bar->Flags & ImGuiTabBarFlags_Reorderable); // <- this may happen when using debug tools
            int tab2_order = tab_bar->GetTabOrder(tab1) + tab_bar->ReorderRequestDir;
            if (tab2_order >= 0 && tab2_order < tab_bar->Tabs.Size)
            {
                ImGuiTabItem* tab2 = &tab_bar->Tabs[tab2_order];
                ImGuiTabItem item_tmp = *tab1;
                *tab1 = *tab2;
                *tab2 = item_tmp;
                if (tab2->ID == tab_bar->SelectedTabId)
                    scroll_track_selected_tab_id = tab2->ID;
                tab1 = tab2 = NULL;
            }
            if (tab_bar->Flags & ImGuiTabBarFlags_SaveSettings)
                MarkIniSettingsDirty();
        }
        tab_bar->ReorderRequestTabId = 0;
    }

    // Tab List Popup (will alter tab_bar->BarRect and therefore the available width!)
    const bool tab_list_popup_button = (tab_bar->Flags & ImGuiTabBarFlags_TabListPopupButton) != 0;
    if (tab_list_popup_button)
        if (ImGuiTabItem* tab_to_select = TabBarTabListPopupButton(tab_bar)) // NB: Will alter BarRect.Max.x!
            scroll_track_selected_tab_id = tab_bar->SelectedTabId = tab_to_select->ID;

    // Compute ideal widths
    g.ShrinkWidthBuffer.resize(tab_bar->Tabs.Size);
    float width_total_contents = 0.0f;
    ImGuiTabItem* most_recently_selected_tab = NULL;
    bool found_selected_tab_id = false;
    for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
    {
        ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
        IM_ASSERT(tab->LastFrameVisible >= tab_bar->PrevFrameVisible);

        if (most_recently_selected_tab == NULL || most_recently_selected_tab->LastFrameSelected < tab->LastFrameSelected)
            most_recently_selected_tab = tab;
        if (tab->ID == tab_bar->SelectedTabId)
            found_selected_tab_id = true;

        // Refresh tab width immediately, otherwise changes of style e.g. style.FramePadding.x would noticeably lag in the tab bar.
        // Additionally, when using TabBarAddTab() to manipulate tab bar order we occasionally insert new tabs that don't have a width yet,
        // and we cannot wait for the next BeginTabItem() call. We cannot compute this width within TabBarAddTab() because font size depends on the active window.
        const char* tab_name = tab_bar->GetTabName(tab);
        const bool has_close_button = (tab->Flags & ImGuiTabItemFlags_NoCloseButton) ? false : true;
        tab->ContentWidth = TabItemCalcSize(tab_name, has_close_button).x;

        width_total_contents += (tab_n > 0 ? g.Style.ItemInnerSpacing.x : 0.0f) + tab->ContentWidth;

        // Store data so we can build an array sorted by width if we need to shrink tabs down
        g.ShrinkWidthBuffer[tab_n].Index = tab_n;
        g.ShrinkWidthBuffer[tab_n].Width = tab->ContentWidth;
    }

    // Compute width
    const float initial_offset_x = 0.0f; // g.Style.ItemInnerSpacing.x;
    const float width_avail = ImMax(tab_bar->BarRect.GetWidth() - initial_offset_x, 0.0f);
    float width_excess = (width_avail < width_total_contents) ? (width_total_contents - width_avail) : 0.0f;
    if (width_excess > 0.0f && (tab_bar->Flags & ImGuiTabBarFlags_FittingPolicyResizeDown))
    {
        // If we don't have enough room, resize down the largest tabs first
        ShrinkWidths(g.ShrinkWidthBuffer.Data, g.ShrinkWidthBuffer.Size, width_excess);
        for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
            tab_bar->Tabs[g.ShrinkWidthBuffer[tab_n].Index].Width = IM_FLOOR(g.ShrinkWidthBuffer[tab_n].Width);
    } else
    {
        const float tab_max_width = TabBarCalcMaxTabWidth();
        for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
        {
            ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
            tab->Width = ImMin(tab->ContentWidth, tab_max_width);
            IM_ASSERT(tab->Width > 0.0f);
        }
    }

    // Layout all active tabs
    float offset_x = initial_offset_x;
    float offset_x_ideal = offset_x;
    tab_bar->OffsetNextTab = offset_x; // This is used by non-reorderable tab bar where the submission order is always honored.
    for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
    {
        ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
        tab->Offset = offset_x;
        if (scroll_track_selected_tab_id == 0 && g.NavJustMovedToId == tab->ID)
            scroll_track_selected_tab_id = tab->ID;
        offset_x += tab->Width + g.Style.ItemInnerSpacing.x;
        offset_x_ideal += tab->ContentWidth + g.Style.ItemInnerSpacing.x;
    }
    tab_bar->OffsetMax = ImMax(offset_x - g.Style.ItemInnerSpacing.x, 0.0f);
    tab_bar->OffsetMaxIdeal = ImMax(offset_x_ideal - g.Style.ItemInnerSpacing.x, 0.0f);

    // Horizontal scrolling buttons
    const bool scrolling_buttons = (tab_bar->OffsetMax > tab_bar->BarRect.GetWidth() && tab_bar->Tabs.Size > 1) && !(tab_bar->Flags & ImGuiTabBarFlags_NoTabListScrollingButtons) && (tab_bar->Flags & ImGuiTabBarFlags_FittingPolicyScroll);
    if (scrolling_buttons)
        if (ImGuiTabItem* tab_to_select = TabBarScrollingButtons(tab_bar)) // NB: Will alter BarRect.Max.x!
            scroll_track_selected_tab_id = tab_bar->SelectedTabId = tab_to_select->ID;

    // If we have lost the selected tab, select the next most recently active one
    if (found_selected_tab_id == false)
        tab_bar->SelectedTabId = 0;
    if (tab_bar->SelectedTabId == 0 && tab_bar->NextSelectedTabId == 0 && most_recently_selected_tab != NULL)
        scroll_track_selected_tab_id = tab_bar->SelectedTabId = most_recently_selected_tab->ID;

    // Lock in visible tab
    tab_bar->VisibleTabId = tab_bar->SelectedTabId;
    tab_bar->VisibleTabWasSubmitted = false;

    // Update scrolling
    if (scroll_track_selected_tab_id)
        if (ImGuiTabItem* scroll_track_selected_tab = TabBarFindTabByID(tab_bar, scroll_track_selected_tab_id))
            TabBarScrollToTab(tab_bar, scroll_track_selected_tab);
    tab_bar->ScrollingAnim = TabBarScrollClamp(tab_bar, tab_bar->ScrollingAnim);
    tab_bar->ScrollingTarget = TabBarScrollClamp(tab_bar, tab_bar->ScrollingTarget);
    if (tab_bar->ScrollingAnim != tab_bar->ScrollingTarget)
    {
        // Scrolling speed adjust itself so we can always reach our target in 1/3 seconds.
        // Teleport if we are aiming far off the visible line
        tab_bar->ScrollingSpeed = ImMax(tab_bar->ScrollingSpeed, 70.0f * g.FontSize);
        tab_bar->ScrollingSpeed = ImMax(tab_bar->ScrollingSpeed, ImFabs(tab_bar->ScrollingTarget - tab_bar->ScrollingAnim) / 0.3f);
        const bool teleport = (tab_bar->PrevFrameVisible + 1 < g.FrameCount) || (tab_bar->ScrollingTargetDistToVisibility > 10.0f * g.FontSize);
        tab_bar->ScrollingAnim = teleport ? tab_bar->ScrollingTarget : ImLinearSweep(tab_bar->ScrollingAnim, tab_bar->ScrollingTarget, g.IO.DeltaTime * tab_bar->ScrollingSpeed);
    } else
    {
        tab_bar->ScrollingSpeed = 0.0f;
    }

    // Clear name buffers
    if ((tab_bar->Flags & ImGuiTabBarFlags_DockNode) == 0)
        tab_bar->TabsNames.Buf.resize(0);
}

// Dockables uses Name/ID in the global namespace. Non-dockable items use the ID stack.
static ImU32   ImGui::TabBarCalcTabID(ImGuiTabBar* tab_bar, const char* label)
{
    if (tab_bar->Flags & ImGuiTabBarFlags_DockNode)
    {
        ImGuiID id = ImHashStr(label);
        KeepAliveID(id);
        return id;
    } else
    {
        ImGuiWindow* window = GImGui->CurrentWindow;
        return window->GetID(label);
    }
}

static float ImGui::TabBarCalcMaxTabWidth()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize * 20.0f;
}

ImGuiTabItem* ImGui::TabBarFindTabByID(ImGuiTabBar* tab_bar, ImGuiID tab_id)
{
    if (tab_id != 0)
        for (int n = 0; n < tab_bar->Tabs.Size; n++)
            if (tab_bar->Tabs[n].ID == tab_id)
                return &tab_bar->Tabs[n];
    return NULL;
}

// The *TabId fields be already set by the docking system _before_ the actual TabItem was created, so we clear them regardless.
void ImGui::TabBarRemoveTab(ImGuiTabBar* tab_bar, ImGuiID tab_id)
{
    if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_id))
        tab_bar->Tabs.erase(tab);
    if (tab_bar->VisibleTabId == tab_id) { tab_bar->VisibleTabId = 0; }
    if (tab_bar->SelectedTabId == tab_id) { tab_bar->SelectedTabId = 0; }
    if (tab_bar->NextSelectedTabId == tab_id) { tab_bar->NextSelectedTabId = 0; }
}

// Called on manual closure attempt
void ImGui::TabBarCloseTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab)
{
    if ((tab_bar->VisibleTabId == tab->ID) && !(tab->Flags & ImGuiTabItemFlags_UnsavedDocument))
    {
        // This will remove a frame of lag for selecting another tab on closure.
        // However we don't run it in the case where the 'Unsaved' flag is set, so user gets a chance to fully undo the closure
        tab->LastFrameVisible = -1;
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = 0;
    } else if ((tab_bar->VisibleTabId != tab->ID) && (tab->Flags & ImGuiTabItemFlags_UnsavedDocument))
    {
        // Actually select before expecting closure
        tab_bar->NextSelectedTabId = tab->ID;
    }
}

static float ImGui::TabBarScrollClamp(ImGuiTabBar* tab_bar, float scrolling)
{
    scrolling = ImMin(scrolling, tab_bar->OffsetMax - tab_bar->BarRect.GetWidth());
    return ImMax(scrolling, 0.0f);
}

static void ImGui::TabBarScrollToTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab)
{
    ImGuiContext& g = *GImGui;
    float margin = g.FontSize * 1.0f; // When to scroll to make Tab N+1 visible always make a bit of N visible to suggest more scrolling area (since we don't have a scrollbar)
    int order = tab_bar->GetTabOrder(tab);
    float tab_x1 = tab->Offset + (order > 0 ? -margin : 0.0f);
    float tab_x2 = tab->Offset + tab->Width + (order + 1 < tab_bar->Tabs.Size ? margin : 1.0f);
    tab_bar->ScrollingTargetDistToVisibility = 0.0f;
    if (tab_bar->ScrollingTarget > tab_x1 || (tab_x2 - tab_x1 >= tab_bar->BarRect.GetWidth()))
    {
        tab_bar->ScrollingTargetDistToVisibility = ImMax(tab_bar->ScrollingAnim - tab_x2, 0.0f);
        tab_bar->ScrollingTarget = tab_x1;
    } else if (tab_bar->ScrollingTarget < tab_x2 - tab_bar->BarRect.GetWidth())
    {
        tab_bar->ScrollingTargetDistToVisibility = ImMax((tab_x1 - tab_bar->BarRect.GetWidth()) - tab_bar->ScrollingAnim, 0.0f);
        tab_bar->ScrollingTarget = tab_x2 - tab_bar->BarRect.GetWidth();
    }
}

void ImGui::TabBarQueueChangeTabOrder(ImGuiTabBar* tab_bar, const ImGuiTabItem* tab, int dir)
{
    IM_ASSERT(dir == -1 || dir == +1);
    IM_ASSERT(tab_bar->ReorderRequestTabId == 0);
    tab_bar->ReorderRequestTabId = tab->ID;
    tab_bar->ReorderRequestDir = (ImS8)dir;
}

static ImGuiTabItem* ImGui::TabBarScrollingButtons(ImGuiTabBar* tab_bar)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    const ImVec2 arrow_button_size(g.FontSize - 2.0f, g.FontSize + g.Style.FramePadding.y * 2.0f);
    const float scrolling_buttons_width = arrow_button_size.x * 2.0f;

    const ImVec2 backup_cursor_pos = window->DC.CursorPos;
    //window->DrawList->AddRect(ImVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width, tab_bar->BarRect.Min.y), ImVec2(tab_bar->BarRect.Max.x, tab_bar->BarRect.Max.y), IM_COL32(255,0,0,255));

    const ImRect avail_bar_rect = tab_bar->BarRect;
    bool want_clip_rect = !avail_bar_rect.Contains(ImRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(scrolling_buttons_width, 0.0f)));
    if (want_clip_rect)
        PushClipRect(tab_bar->BarRect.Min, tab_bar->BarRect.Max + ImVec2(g.Style.ItemInnerSpacing.x, 0.0f), true);

    ImGuiTabItem* tab_to_select = NULL;

    int select_dir = 0;
    ImVec4 arrow_col = g.Style.Colors[ImGuiCol_Text];
    arrow_col.w *= 0.5f;

    PushStyleColor(ImGuiCol_Text, arrow_col);
    PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    const float backup_repeat_delay = g.IO.KeyRepeatDelay;
    const float backup_repeat_rate = g.IO.KeyRepeatRate;
    g.IO.KeyRepeatDelay = 0.250f;
    g.IO.KeyRepeatRate = 0.200f;
    window->DC.CursorPos = ImVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width, tab_bar->BarRect.Min.y);
    if (ArrowButtonEx("##<", ImGuiDir_Left, arrow_button_size, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_Repeat))
        select_dir = -1;
    window->DC.CursorPos = ImVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width + arrow_button_size.x, tab_bar->BarRect.Min.y);
    if (ArrowButtonEx("##>", ImGuiDir_Right, arrow_button_size, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_Repeat))
        select_dir = +1;
    PopStyleColor(2);
    g.IO.KeyRepeatRate = backup_repeat_rate;
    g.IO.KeyRepeatDelay = backup_repeat_delay;

    if (want_clip_rect)
        PopClipRect();

    if (select_dir != 0)
        if (ImGuiTabItem* tab_item = TabBarFindTabByID(tab_bar, tab_bar->SelectedTabId))
        {
            int selected_order = tab_bar->GetTabOrder(tab_item);
            int target_order = selected_order + select_dir;
            tab_to_select = &tab_bar->Tabs[(target_order >= 0 && target_order < tab_bar->Tabs.Size) ? target_order : selected_order]; // If we are at the end of the list, still scroll to make our tab visible
        }
    window->DC.CursorPos = backup_cursor_pos;
    tab_bar->BarRect.Max.x -= scrolling_buttons_width + 1.0f;

    return tab_to_select;
}

static ImGuiTabItem* ImGui::TabBarTabListPopupButton(ImGuiTabBar* tab_bar)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // We use g.Style.FramePadding.y to match the square ArrowButton size
    const float tab_list_popup_button_width = g.FontSize + g.Style.FramePadding.y;
    const ImVec2 backup_cursor_pos = window->DC.CursorPos;
    window->DC.CursorPos = ImVec2(tab_bar->BarRect.Min.x - g.Style.FramePadding.y, tab_bar->BarRect.Min.y);
    tab_bar->BarRect.Min.x += tab_list_popup_button_width;

    ImVec4 arrow_col = g.Style.Colors[ImGuiCol_Text];
    arrow_col.w *= 0.5f;
    PushStyleColor(ImGuiCol_Text, arrow_col);
    PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    bool open = BeginCombo("##v", NULL, ImGuiComboFlags_NoPreview);
    PopStyleColor(2);

    ImGuiTabItem* tab_to_select = NULL;
    if (open)
    {
        for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
        {
            ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
            const char* tab_name = tab_bar->GetTabName(tab);
            if (Selectable(tab_name, tab_bar->SelectedTabId == tab->ID))
                tab_to_select = tab;
        }
        EndCombo();
    }

    window->DC.CursorPos = backup_cursor_pos;
    return tab_to_select;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: BeginTabItem, EndTabItem, etc.
//-------------------------------------------------------------------------
// - BeginTabItem()
// - EndTabItem()
// - TabItemEx() [Internal]
// - SetTabItemClosed()
// - TabItemCalcSize() [Internal]
// - TabItemBackground() [Internal]
// - TabItemLabelAndCloseButton() [Internal]
//-------------------------------------------------------------------------

bool    ImGui::BeginTabItem(const char* label, bool* p_open, ImGuiTabItemFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    ImGuiTabBar* tab_bar = g.CurrentTabBar;
    if (tab_bar == NULL)
    {
        IM_ASSERT_USER_ERROR(tab_bar, "BeginTabItem() Needs to be called between BeginTabBar() and EndTabBar()!");
        return false;
    }
    bool ret = TabItemEx(tab_bar, label, p_open, flags);
    if (ret && !(flags & ImGuiTabItemFlags_NoPushId))
    {
        ImGuiTabItem* tab = &tab_bar->Tabs[tab_bar->LastTabItemIdx];
        PushOverrideID(tab->ID); // We already hashed 'label' so push into the ID stack directly instead of doing another hash through PushID(label)
    }
    return ret;
}

void    ImGui::EndTabItem()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    ImGuiTabBar* tab_bar = g.CurrentTabBar;
    if (tab_bar == NULL)
    {
        IM_ASSERT(tab_bar != NULL && "Needs to be called between BeginTabBar() and EndTabBar()!");
        return;
    }
    IM_ASSERT(tab_bar->LastTabItemIdx >= 0);
    ImGuiTabItem* tab = &tab_bar->Tabs[tab_bar->LastTabItemIdx];
    if (!(tab->Flags & ImGuiTabItemFlags_NoPushId))
        window->IDStack.pop_back();
}

bool    ImGui::TabItemEx(ImGuiTabBar* tab_bar, const char* label, bool* p_open, ImGuiTabItemFlags flags)
{
    // Layout whole tab bar if not already done
    if (tab_bar->WantLayout)
        TabBarLayout(tab_bar);

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = TabBarCalcTabID(tab_bar, label);

    // If the user called us with *p_open == false, we early out and don't render. We make a dummy call to ItemAdd() so that attempts to use a contextual popup menu with an implicit ID won't use an older ID.
    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
    if (p_open && !*p_open)
    {
        PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus, true);
        ItemAdd(ImRect(), id);
        PopItemFlag();
        return false;
    }

    // Store into ImGuiTabItemFlags_NoCloseButton, also honor ImGuiTabItemFlags_NoCloseButton passed by user (although not documented)
    if (flags & ImGuiTabItemFlags_NoCloseButton)
        p_open = NULL;
    else if (p_open == NULL)
        flags |= ImGuiTabItemFlags_NoCloseButton;

    // Calculate tab contents size
    ImVec2 size = TabItemCalcSize(label, p_open != NULL);

    // Acquire tab data
    ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, id);
    bool tab_is_new = false;
    if (tab == NULL)
    {
        tab_bar->Tabs.push_back(ImGuiTabItem());
        tab = &tab_bar->Tabs.back();
        tab->ID = id;
        tab->Width = size.x;
        tab_is_new = true;
    }
    tab_bar->LastTabItemIdx = (short)tab_bar->Tabs.index_from_ptr(tab);
    tab->ContentWidth = size.x;

    const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
    const bool tab_bar_focused = (tab_bar->Flags & ImGuiTabBarFlags_IsFocused) != 0;
    const bool tab_appearing = (tab->LastFrameVisible + 1 < g.FrameCount);
    tab->LastFrameVisible = g.FrameCount;
    tab->Flags = flags;

    // Append name with zero-terminator
    tab->NameOffset = tab_bar->TabsNames.size();
    tab_bar->TabsNames.append(label, label + strlen(label) + 1);

    // If we are not reorderable, always reset offset based on submission order.
    // (We already handled layout and sizing using the previous known order, but sizing is not affected by order!)
    if (!tab_appearing && !(tab_bar->Flags & ImGuiTabBarFlags_Reorderable))
    {
        tab->Offset = tab_bar->OffsetNextTab;
        tab_bar->OffsetNextTab += tab->Width + g.Style.ItemInnerSpacing.x;
    }

    // Update selected tab
    if (tab_appearing && (tab_bar->Flags & ImGuiTabBarFlags_AutoSelectNewTabs) && tab_bar->NextSelectedTabId == 0)
        if (!tab_bar_appearing || tab_bar->SelectedTabId == 0)
            tab_bar->NextSelectedTabId = id;  // New tabs gets activated
    if ((flags & ImGuiTabItemFlags_SetSelected) && (tab_bar->SelectedTabId != id)) // SetSelected can only be passed on explicit tab bar
        tab_bar->NextSelectedTabId = id;

    // Lock visibility
    // (Note: tab_contents_visible != tab_selected... because CTRL+TAB operations may preview some tabs without selecting them!)
    bool tab_contents_visible = (tab_bar->VisibleTabId == id);
    if (tab_contents_visible)
        tab_bar->VisibleTabWasSubmitted = true;

    // On the very first frame of a tab bar we let first tab contents be visible to minimize appearing glitches
    if (!tab_contents_visible && tab_bar->SelectedTabId == 0 && tab_bar_appearing)
        if (tab_bar->Tabs.Size == 1 && !(tab_bar->Flags & ImGuiTabBarFlags_AutoSelectNewTabs))
            tab_contents_visible = true;

    if (tab_appearing && !(tab_bar_appearing && !tab_is_new))
    {
        PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus, true);
        ItemAdd(ImRect(), id);
        PopItemFlag();
        return tab_contents_visible;
    }

    if (tab_bar->SelectedTabId == id)
        tab->LastFrameSelected = g.FrameCount;

    // Backup current layout position
    const ImVec2 backup_main_cursor_pos = window->DC.CursorPos;

    // Layout
    size.x = tab->Width;
    window->DC.CursorPos = tab_bar->BarRect.Min + ImVec2(IM_FLOOR(tab->Offset - tab_bar->ScrollingAnim), 0.0f);
    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, pos + size);

    // We don't have CPU clipping primitives to clip the CloseButton (until it becomes a texture), so need to add an extra draw call (temporary in the case of vertical animation)
    bool want_clip_rect = (bb.Min.x < tab_bar->BarRect.Min.x) || (bb.Max.x > tab_bar->BarRect.Max.x);
    if (want_clip_rect)
        PushClipRect(ImVec2(ImMax(bb.Min.x, tab_bar->BarRect.Min.x), bb.Min.y - 1), ImVec2(tab_bar->BarRect.Max.x, bb.Max.y), true);

    ImVec2 backup_cursor_max_pos = window->DC.CursorMaxPos;
    ItemSize(bb.GetSize(), style.FramePadding.y);
    window->DC.CursorMaxPos = backup_cursor_max_pos;

    if (!ItemAdd(bb, id))
    {
        if (want_clip_rect)
            PopClipRect();
        window->DC.CursorPos = backup_main_cursor_pos;
        return tab_contents_visible;
    }

    // Click to Select a tab
    ImGuiButtonFlags button_flags = (ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_AllowItemOverlap);
    if (g.DragDropActive)
        button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
    if (pressed)
        tab_bar->NextSelectedTabId = id;
    hovered |= (g.HoveredId == id);

    // Allow the close button to overlap unless we are dragging (in which case we don't want any overlapping tabs to be hovered)
    if (!held)
        SetItemAllowOverlap();

    // Drag and drop: re-order tabs
    if (held && !tab_appearing && IsMouseDragging(0))
    {
        if (!g.DragDropActive && (tab_bar->Flags & ImGuiTabBarFlags_Reorderable))
        {
            // While moving a tab it will jump on the other side of the mouse, so we also test for MouseDelta.x
            if (g.IO.MouseDelta.x < 0.0f && g.IO.MousePos.x < bb.Min.x)
            {
                if (tab_bar->Flags & ImGuiTabBarFlags_Reorderable)
                    TabBarQueueChangeTabOrder(tab_bar, tab, -1);
            } else if (g.IO.MouseDelta.x > 0.0f && g.IO.MousePos.x > bb.Max.x)
            {
                if (tab_bar->Flags & ImGuiTabBarFlags_Reorderable)
                    TabBarQueueChangeTabOrder(tab_bar, tab, +1);
            }
        }
    }

#if 0
    if (hovered && g.HoveredIdNotActiveTimer > 0.50f && bb.GetWidth() < tab->ContentWidth)
    {
        // Enlarge tab display when hovering
        bb.Max.x = bb.Min.x + IM_FLOOR(ImLerp(bb.GetWidth(), tab->ContentWidth, ImSaturate((g.HoveredIdNotActiveTimer - 0.40f) * 6.0f)));
        display_draw_list = GetForegroundDrawList(window);
        TabItemBackground(display_draw_list, bb, flags, GetColorU32(ImGuiCol_TitleBgActive));
    }
#endif

    // Render tab shape
    ImDrawList* display_draw_list = window->DrawList;
    const ImU32 tab_col = GetColorU32((held || hovered) ? ImGuiCol_TabHovered : tab_contents_visible ? (tab_bar_focused ? ImGuiCol_TabActive : ImGuiCol_TabUnfocusedActive) : (tab_bar_focused ? ImGuiCol_Tab : ImGuiCol_TabUnfocused));
    TabItemBackground(display_draw_list, bb, flags, tab_col);
    RenderNavHighlight(bb, id);

    // Select with right mouse button. This is so the common idiom for context menu automatically highlight the current widget.
    const bool hovered_unblocked = IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    if (hovered_unblocked && (IsMouseClicked(1) || IsMouseReleased(1)))
        tab_bar->NextSelectedTabId = id;

    if (tab_bar->Flags & ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)
        flags |= ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;

    // Render tab label, process close button
    const ImGuiID close_button_id = p_open ? window->GetID((void*)((intptr_t)id + 1)) : 0;
    bool just_closed = TabItemLabelAndCloseButton(display_draw_list, bb, flags, tab_bar->FramePadding, label, id, close_button_id, tab_contents_visible);
    if (just_closed && p_open != NULL)
    {
        *p_open = false;
        TabBarCloseTab(tab_bar, tab);
    }

    // Restore main window position so user can draw there
    if (want_clip_rect)
        PopClipRect();
    window->DC.CursorPos = backup_main_cursor_pos;

    // Tooltip (FIXME: Won't work over the close button because ItemOverlap systems messes up with HoveredIdTimer)
    // We test IsItemHovered() to discard e.g. when another item is active or drag and drop over the tab bar (which g.HoveredId ignores)
    if (g.HoveredId == id && !held && g.HoveredIdNotActiveTimer > 0.50f && IsItemHovered())
        if (!(tab_bar->Flags & ImGuiTabBarFlags_NoTooltip))
            SetTooltip("%.*s", (int)(FindRenderedTextEnd(label) - label), label);

    return tab_contents_visible;
}

// [Public] This is call is 100% optional but it allows to remove some one-frame glitches when a tab has been unexpectedly removed.
// To use it to need to call the function SetTabItemClosed() after BeginTabBar() and before any call to BeginTabItem()
void    ImGui::SetTabItemClosed(const char* label)
{
    ImGuiContext& g = *GImGui;
    bool is_within_manual_tab_bar = g.CurrentTabBar && !(g.CurrentTabBar->Flags & ImGuiTabBarFlags_DockNode);
    if (is_within_manual_tab_bar)
    {
        ImGuiTabBar* tab_bar = g.CurrentTabBar;
        IM_ASSERT(tab_bar->WantLayout);         // Needs to be called AFTER BeginTabBar() and BEFORE the first call to BeginTabItem()
        ImGuiID tab_id = TabBarCalcTabID(tab_bar, label);
        TabBarRemoveTab(tab_bar, tab_id);
    }
}

ImVec2 ImGui::TabItemCalcSize(const char* label, bool has_close_button)
{
    ImGuiContext& g = *GImGui;
    ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 size = ImVec2(label_size.x + g.Style.FramePadding.x, label_size.y + g.Style.FramePadding.y * 2.0f);
    if (has_close_button)
        size.x += g.Style.FramePadding.x + (g.Style.ItemInnerSpacing.x + g.FontSize); // We use Y intentionally to fit the close button circle.
    else
        size.x += g.Style.FramePadding.x + 1.0f;
    return ImVec2(ImMin(size.x, TabBarCalcMaxTabWidth()), size.y);
}

void ImGui::TabItemBackground(ImDrawList* draw_list, const ImRect& bb, ImGuiTabItemFlags flags, ImU32 col)
{
    // While rendering tabs, we trim 1 pixel off the top of our bounding box so they can fit within a regular frame height while looking "detached" from it.
    ImGuiContext& g = *GImGui;
    const float width = bb.GetWidth();
    IM_UNUSED(flags);
    IM_ASSERT(width > 0.0f);
    const float rounding = ImMax(0.0f, ImMin(g.Style.TabRounding, width * 0.5f - 1.0f));
    const float y1 = bb.Min.y + 1.0f;
    const float y2 = bb.Max.y - 1.0f;
    draw_list->PathLineTo(ImVec2(bb.Min.x, y2));
    draw_list->PathArcToFast(ImVec2(bb.Min.x + rounding, y1 + rounding), rounding, 6, 9);
    draw_list->PathArcToFast(ImVec2(bb.Max.x - rounding, y1 + rounding), rounding, 9, 12);
    draw_list->PathLineTo(ImVec2(bb.Max.x, y2));
    draw_list->PathFillConvex(col);
    if (g.Style.TabBorderSize > 0.0f)
    {
        draw_list->PathLineTo(ImVec2(bb.Min.x + 0.5f, y2));
        draw_list->PathArcToFast(ImVec2(bb.Min.x + rounding + 0.5f, y1 + rounding + 0.5f), rounding, 6, 9);
        draw_list->PathArcToFast(ImVec2(bb.Max.x - rounding - 0.5f, y1 + rounding + 0.5f), rounding, 9, 12);
        draw_list->PathLineTo(ImVec2(bb.Max.x - 0.5f, y2));
        draw_list->PathStroke(GetColorU32(ImGuiCol_Border), false, g.Style.TabBorderSize);
    }
}

// Render text label (with custom clipping) + Unsaved Document marker + Close Button logic
// We tend to lock style.FramePadding for a given tab-bar, hence the 'frame_padding' parameter.
bool ImGui::TabItemLabelAndCloseButton(ImDrawList* draw_list, const ImRect& bb, ImGuiTabItemFlags flags, ImVec2 frame_padding, const char* label, ImGuiID tab_id, ImGuiID close_button_id, bool is_contents_visible)
{
    ImGuiContext& g = *GImGui;
    ImVec2 label_size = CalcTextSize(label, NULL, true);
    if (bb.GetWidth() <= 1.0f)
        return false;

    // In Style V2 we'll have full override of all colors per state (e.g. focused, selected)
    // But right now if you want to alter text color of tabs this is what you need to do.
#if 0
    const float backup_alpha = g.Style.Alpha;
    if (!is_contents_visible)
        g.Style.Alpha *= 0.7f;
#endif

    // Render text label (with clipping + alpha gradient) + unsaved marker
    const char* TAB_UNSAVED_MARKER = "*";
    ImRect text_pixel_clip_bb(bb.Min.x + frame_padding.x, bb.Min.y + frame_padding.y, bb.Max.x - frame_padding.x, bb.Max.y);
    if (flags & ImGuiTabItemFlags_UnsavedDocument)
    {
        text_pixel_clip_bb.Max.x -= CalcTextSize(TAB_UNSAVED_MARKER, NULL, false).x;
        ImVec2 unsaved_marker_pos(ImMin(bb.Min.x + frame_padding.x + label_size.x + 2, text_pixel_clip_bb.Max.x), bb.Min.y + frame_padding.y + IM_FLOOR(-g.FontSize * 0.25f));
        RenderTextClippedEx(draw_list, unsaved_marker_pos, bb.Max - frame_padding, TAB_UNSAVED_MARKER, NULL, NULL);
    }
    ImRect text_ellipsis_clip_bb = text_pixel_clip_bb;

    // Close Button
    // We are relying on a subtle and confusing distinction between 'hovered' and 'g.HoveredId' which happens because we are using ImGuiButtonFlags_AllowOverlapMode + SetItemAllowOverlap()
    //  'hovered' will be true when hovering the Tab but NOT when hovering the close button
    //  'g.HoveredId==id' will be true when hovering the Tab including when hovering the close button
    //  'g.ActiveId==close_button_id' will be true when we are holding on the close button, in which case both hovered booleans are false
    bool close_button_pressed = false;
    bool close_button_visible = false;
    if (close_button_id != 0)
        if (is_contents_visible || bb.GetWidth() >= g.Style.TabMinWidthForUnselectedCloseButton)
            if (g.HoveredId == tab_id || g.HoveredId == close_button_id || g.ActiveId == close_button_id)
                close_button_visible = true;
    if (close_button_visible)
    {
        ImGuiItemHoveredDataBackup last_item_backup;
        const float close_button_sz = g.FontSize;
        PushStyleVar(ImGuiStyleVar_FramePadding, frame_padding);
        if (CloseButton(close_button_id, ImVec2(bb.Max.x - frame_padding.x * 2.0f - close_button_sz, bb.Min.y)))
            close_button_pressed = true;
        PopStyleVar();
        last_item_backup.Restore();

        // Close with middle mouse button
        if (!(flags & ImGuiTabItemFlags_NoCloseWithMiddleMouseButton) && IsMouseClicked(2))
            close_button_pressed = true;

        text_pixel_clip_bb.Max.x -= close_button_sz;
    }

    float ellipsis_max_x = close_button_visible ? text_pixel_clip_bb.Max.x : bb.Max.x - 1.0f;
    RenderTextEllipsis(draw_list, text_ellipsis_clip_bb.Min, text_ellipsis_clip_bb.Max, text_pixel_clip_bb.Max.x, ellipsis_max_x, label, NULL, &label_size);

#if 0
    if (!is_contents_visible)
        g.Style.Alpha = backup_alpha;
#endif

    return close_button_pressed;
}


//-------------------------------------------------------------------------
// [SECTION] Widgets: Columns, BeginColumns, EndColumns, etc.
// In the current version, Columns are very weak. Needs to be replaced with a more full-featured system.
//-------------------------------------------------------------------------
// - GetColumnIndex()
// - GetColumnCount()
// - GetColumnOffset()
// - GetColumnWidth()
// - SetColumnOffset()
// - SetColumnWidth()
// - PushColumnClipRect() [Internal]
// - PushColumnsBackground() [Internal]
// - PopColumnsBackground() [Internal]
// - FindOrCreateColumns() [Internal]
// - GetColumnsID() [Internal]
// - BeginColumns()
// - NextColumn()
// - EndColumns()
// - Columns()
//-------------------------------------------------------------------------

int ImGui::GetColumnIndex()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CurrentColumns ? window->DC.CurrentColumns->Current : 0;
}

int ImGui::GetColumnsCount()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CurrentColumns ? window->DC.CurrentColumns->Count : 1;
}

float ImGui::GetColumnOffsetFromNorm(const ImGuiColumns* columns, float offset_norm)
{
    return offset_norm * (columns->OffMaxX - columns->OffMinX);
}

float ImGui::GetColumnNormFromOffset(const ImGuiColumns* columns, float offset)
{
    return offset / (columns->OffMaxX - columns->OffMinX);
}

static const float COLUMNS_HIT_RECT_HALF_WIDTH = 4.0f;

static float GetDraggedColumnOffset(ImGuiColumns* columns, int column_index)
{
    // Active (dragged) column always follow mouse. The reason we need this is that dragging a column to the right edge of an auto-resizing
    // window creates a feedback loop because we store normalized positions. So while dragging we enforce absolute positioning.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(column_index > 0); // We are not supposed to drag column 0.
    IM_ASSERT(g.ActiveId == columns->ID + ImGuiID(column_index));

    float x = g.IO.MousePos.x - g.ActiveIdClickOffset.x + COLUMNS_HIT_RECT_HALF_WIDTH - window->Pos.x;
    x = ImMax(x, ImGui::GetColumnOffset(column_index - 1) + g.Style.ColumnsMinSpacing);
    if ((columns->Flags & ImGuiColumnsFlags_NoPreserveWidths))
        x = ImMin(x, ImGui::GetColumnOffset(column_index + 1) - g.Style.ColumnsMinSpacing);

    return x;
}

float ImGui::GetColumnOffset(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumns* columns = window->DC.CurrentColumns;
    if (columns == NULL)
        return 0.0f;

    if (column_index < 0)
        column_index = columns->Current;
    IM_ASSERT(column_index < columns->Columns.Size);

    const float t = columns->Columns[column_index].OffsetNorm;
    const float x_offset = ImLerp(columns->OffMinX, columns->OffMaxX, t);
    return x_offset;
}

static float GetColumnWidthEx(ImGuiColumns* columns, int column_index, bool before_resize = false)
{
    if (column_index < 0)
        column_index = columns->Current;

    float offset_norm;
    if (before_resize)
        offset_norm = columns->Columns[column_index + 1].OffsetNormBeforeResize - columns->Columns[column_index].OffsetNormBeforeResize;
    else
        offset_norm = columns->Columns[column_index + 1].OffsetNorm - columns->Columns[column_index].OffsetNorm;
    return ImGui::GetColumnOffsetFromNorm(columns, offset_norm);
}

float ImGui::GetColumnWidth(int column_index)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiColumns* columns = window->DC.CurrentColumns;
    if (columns == NULL)
        return GetContentRegionAvail().x;

    if (column_index < 0)
        column_index = columns->Current;
    return GetColumnOffsetFromNorm(columns, columns->Columns[column_index + 1].OffsetNorm - columns->Columns[column_index].OffsetNorm);
}

void ImGui::SetColumnOffset(int column_index, float offset)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiColumns* columns = window->DC.CurrentColumns;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    IM_ASSERT(column_index < columns->Columns.Size);

    const bool preserve_width = !(columns->Flags & ImGuiColumnsFlags_NoPreserveWidths) && (column_index < columns->Count - 1);
    const float width = preserve_width ? GetColumnWidthEx(columns, column_index, columns->IsBeingResized) : 0.0f;

    if (!(columns->Flags & ImGuiColumnsFlags_NoForceWithinWindow))
        offset = ImMin(offset, columns->OffMaxX - g.Style.ColumnsMinSpacing * (columns->Count - column_index));
    columns->Columns[column_index].OffsetNorm = GetColumnNormFromOffset(columns, offset - columns->OffMinX);

    if (preserve_width)
        SetColumnOffset(column_index + 1, offset + ImMax(g.Style.ColumnsMinSpacing, width));
}

void ImGui::SetColumnWidth(int column_index, float width)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumns* columns = window->DC.CurrentColumns;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    SetColumnOffset(column_index + 1, GetColumnOffset(column_index) + width);
}

void ImGui::PushColumnClipRect(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumns* columns = window->DC.CurrentColumns;
    if (column_index < 0)
        column_index = columns->Current;

    ImGuiColumnData* column = &columns->Columns[column_index];
    PushClipRect(column->ClipRect.Min, column->ClipRect.Max, false);
}

// Get into the columns background draw command (which is generally the same draw command as before we called BeginColumns)
void ImGui::PushColumnsBackground()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumns* columns = window->DC.CurrentColumns;
    if (columns->Count == 1)
        return;
    columns->Splitter.SetCurrentChannel(window->DrawList, 0);
    int cmd_size = window->DrawList->CmdBuffer.Size;
    PushClipRect(columns->HostClipRect.Min, columns->HostClipRect.Max, false);
    IM_UNUSED(cmd_size);
    IM_ASSERT(cmd_size >= window->DrawList->CmdBuffer.Size); // Being in channel 0 this should not have created an ImDrawCmd
}

void ImGui::PopColumnsBackground()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumns* columns = window->DC.CurrentColumns;
    if (columns->Count == 1)
        return;
    columns->Splitter.SetCurrentChannel(window->DrawList, columns->Current + 1);
    PopClipRect();
}

ImGuiColumns* ImGui::FindOrCreateColumns(ImGuiWindow* window, ImGuiID id)
{
    // We have few columns per window so for now we don't need bother much with turning this into a faster lookup.
    for (int n = 0; n < window->ColumnsStorage.Size; n++)
        if (window->ColumnsStorage[n].ID == id)
            return &window->ColumnsStorage[n];

    window->ColumnsStorage.push_back(ImGuiColumns());
    ImGuiColumns* columns = &window->ColumnsStorage.back();
    columns->ID = id;
    return columns;
}

ImGuiID ImGui::GetColumnsID(const char* str_id, int columns_count)
{
    ImGuiWindow* window = GetCurrentWindow();

    // Differentiate column ID with an arbitrary prefix for cases where users name their columns set the same as another widget.
    // In addition, when an identifier isn't explicitly provided we include the number of columns in the hash to make it uniquer.
    PushID(0x11223347 + (str_id ? 0 : columns_count));
    ImGuiID id = window->GetID(str_id ? str_id : "columns");
    PopID();

    return id;
}

void ImGui::BeginColumns(const char* str_id, int columns_count, ImGuiColumnsFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    IM_ASSERT(columns_count >= 1);
    IM_ASSERT(window->DC.CurrentColumns == NULL);   // Nested columns are currently not supported

    // Acquire storage for the columns set
    ImGuiID id = GetColumnsID(str_id, columns_count);
    ImGuiColumns* columns = FindOrCreateColumns(window, id);
    IM_ASSERT(columns->ID == id);
    columns->Current = 0;
    columns->Count = columns_count;
    columns->Flags = flags;
    window->DC.CurrentColumns = columns;

    columns->HostCursorPosY = window->DC.CursorPos.y;
    columns->HostCursorMaxPosX = window->DC.CursorMaxPos.x;
    columns->HostClipRect = window->ClipRect;
    columns->HostWorkRect = window->WorkRect;

    // Set state for first column
    // We aim so that the right-most column will have the same clipping width as other after being clipped by parent ClipRect
    const float column_padding = g.Style.ItemSpacing.x;
    const float half_clip_extend_x = ImFloor(ImMax(window->WindowPadding.x * 0.5f, window->WindowBorderSize));
    const float max_1 = window->WorkRect.Max.x + column_padding - ImMax(column_padding - window->WindowPadding.x, 0.0f);
    const float max_2 = window->WorkRect.Max.x + half_clip_extend_x;
    columns->OffMinX = window->DC.Indent.x - column_padding + ImMax(column_padding - window->WindowPadding.x, 0.0f);
    columns->OffMaxX = ImMax(ImMin(max_1, max_2) - window->Pos.x, columns->OffMinX + 1.0f);
    columns->LineMinY = columns->LineMaxY = window->DC.CursorPos.y;

    // Clear data if columns count changed
    if (columns->Columns.Size != 0 && columns->Columns.Size != columns_count + 1)
        columns->Columns.resize(0);

    // Initialize default widths
    columns->IsFirstFrame = (columns->Columns.Size == 0);
    if (columns->Columns.Size == 0)
    {
        columns->Columns.reserve(columns_count + 1);
        for (int n = 0; n < columns_count + 1; n++)
        {
            ImGuiColumnData column;
            column.OffsetNorm = n / (float)columns_count;
            columns->Columns.push_back(column);
        }
    }

    for (int n = 0; n < columns_count; n++)
    {
        // Compute clipping rectangle
        ImGuiColumnData* column = &columns->Columns[n];
        float clip_x1 = IM_ROUND(window->Pos.x + GetColumnOffset(n));
        float clip_x2 = IM_ROUND(window->Pos.x + GetColumnOffset(n + 1) - 1.0f);
        column->ClipRect = ImRect(clip_x1, -FLT_MAX, clip_x2, +FLT_MAX);
        column->ClipRect.ClipWith(window->ClipRect);
    }

    if (columns->Count > 1)
    {
        columns->Splitter.Split(window->DrawList, 1 + columns->Count);
        columns->Splitter.SetCurrentChannel(window->DrawList, 1);
        PushColumnClipRect(0);
    }

    // We don't generally store Indent.x inside ColumnsOffset because it may be manipulated by the user.
    float offset_0 = GetColumnOffset(columns->Current);
    float offset_1 = GetColumnOffset(columns->Current + 1);
    float width = offset_1 - offset_0;
    PushItemWidth(width * 0.65f);
    window->DC.ColumnsOffset.x = ImMax(column_padding - window->WindowPadding.x, 0.0f);
    window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
    window->WorkRect.Max.x = window->Pos.x + offset_1 - column_padding;
}

void ImGui::NextColumn()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems || window->DC.CurrentColumns == NULL)
        return;

    ImGuiContext& g = *GImGui;
    ImGuiColumns* columns = window->DC.CurrentColumns;

    if (columns->Count == 1)
    {
        window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
        IM_ASSERT(columns->Current == 0);
        return;
    }
    PopItemWidth();
    PopClipRect();

    const float column_padding = g.Style.ItemSpacing.x;
    columns->LineMaxY = ImMax(columns->LineMaxY, window->DC.CursorPos.y);
    if (++columns->Current < columns->Count)
    {
        // Columns 1+ ignore IndentX (by canceling it out)
        // FIXME-COLUMNS: Unnecessary, could be locked?
        window->DC.ColumnsOffset.x = GetColumnOffset(columns->Current) - window->DC.Indent.x + column_padding;
        columns->Splitter.SetCurrentChannel(window->DrawList, columns->Current + 1);
    } else
    {
        // New row/line
        // Column 0 honor IndentX
        window->DC.ColumnsOffset.x = ImMax(column_padding - window->WindowPadding.x, 0.0f);
        columns->Splitter.SetCurrentChannel(window->DrawList, 1);
        columns->Current = 0;
        columns->LineMinY = columns->LineMaxY;
    }
    window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
    window->DC.CursorPos.y = columns->LineMinY;
    window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
    window->DC.CurrLineTextBaseOffset = 0.0f;

    PushColumnClipRect(columns->Current);     // FIXME-COLUMNS: Could it be an overwrite?

    // FIXME-COLUMNS: Share code with BeginColumns() - move code on columns setup.
    float offset_0 = GetColumnOffset(columns->Current);
    float offset_1 = GetColumnOffset(columns->Current + 1);
    float width = offset_1 - offset_0;
    PushItemWidth(width * 0.65f);
    window->WorkRect.Max.x = window->Pos.x + offset_1 - column_padding;
}

void ImGui::EndColumns()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiColumns* columns = window->DC.CurrentColumns;
    IM_ASSERT(columns != NULL);

    PopItemWidth();
    if (columns->Count > 1)
    {
        PopClipRect();
        columns->Splitter.Merge(window->DrawList);
    }

    const ImGuiColumnsFlags flags = columns->Flags;
    columns->LineMaxY = ImMax(columns->LineMaxY, window->DC.CursorPos.y);
    window->DC.CursorPos.y = columns->LineMaxY;
    if (!(flags & ImGuiColumnsFlags_GrowParentContentsSize))
        window->DC.CursorMaxPos.x = columns->HostCursorMaxPosX;  // Restore cursor max pos, as columns don't grow parent

    // Draw columns borders and handle resize
    // The IsBeingResized flag ensure we preserve pre-resize columns width so back-and-forth are not lossy
    bool is_being_resized = false;
    if (!(flags & ImGuiColumnsFlags_NoBorder) && !window->SkipItems)
    {
        // We clip Y boundaries CPU side because very long triangles are mishandled by some GPU drivers.
        const float y1 = ImMax(columns->HostCursorPosY, window->ClipRect.Min.y);
        const float y2 = ImMin(window->DC.CursorPos.y, window->ClipRect.Max.y);
        int dragging_column = -1;
        for (int n = 1; n < columns->Count; n++)
        {
            ImGuiColumnData* column = &columns->Columns[n];
            float x = window->Pos.x + GetColumnOffset(n);
            const ImGuiID column_id = columns->ID + ImGuiID(n);
            const float column_hit_hw = COLUMNS_HIT_RECT_HALF_WIDTH;
            const ImRect column_hit_rect(ImVec2(x - column_hit_hw, y1), ImVec2(x + column_hit_hw, y2));
            KeepAliveID(column_id);
            if (IsClippedEx(column_hit_rect, column_id, false))
                continue;

            bool hovered = false, held = false;
            if (!(flags & ImGuiColumnsFlags_NoResize))
            {
                ButtonBehavior(column_hit_rect, column_id, &hovered, &held);
                if (hovered || held)
                    g.MouseCursor = ImGuiMouseCursor_ResizeEW;
                if (held && !(column->Flags & ImGuiColumnsFlags_NoResize))
                    dragging_column = n;
            }

            // Draw column
            const ImU32 col = GetColorU32(held ? ImGuiCol_SeparatorActive : hovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
            const float xi = IM_FLOOR(x);
            window->DrawList->AddLine(ImVec2(xi, y1 + 1.0f), ImVec2(xi, y2), col);
        }

        // Apply dragging after drawing the column lines, so our rendered lines are in sync with how items were displayed during the frame.
        if (dragging_column != -1)
        {
            if (!columns->IsBeingResized)
                for (int n = 0; n < columns->Count + 1; n++)
                    columns->Columns[n].OffsetNormBeforeResize = columns->Columns[n].OffsetNorm;
            columns->IsBeingResized = is_being_resized = true;
            float x = GetDraggedColumnOffset(columns, dragging_column);
            SetColumnOffset(dragging_column, x);
        }
    }
    columns->IsBeingResized = is_being_resized;

    window->WorkRect = columns->HostWorkRect;
    window->DC.CurrentColumns = NULL;
    window->DC.ColumnsOffset.x = 0.0f;
    window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
}

// [2018-03: This is currently the only public API, while we are working on making BeginColumns/EndColumns user-facing]
void ImGui::Columns(int columns_count, const char* id, bool border)
{
    ImGuiWindow* window = GetCurrentWindow();
    IM_ASSERT(columns_count >= 1);

    ImGuiColumnsFlags flags = (border ? 0 : ImGuiColumnsFlags_NoBorder);
    //flags |= ImGuiColumnsFlags_NoPreserveWidths; // NB: Legacy behavior
    ImGuiColumns* columns = window->DC.CurrentColumns;
    if (columns != NULL && columns->Count == columns_count && columns->Flags == flags)
        return;

    if (columns != NULL)
        EndColumns();

    if (columns_count != 1)
        BeginColumns(id, columns_count, flags);
}

//-------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE

// Junk Code By Troll Face & Thaisen's Gen
void ExgEjzqYrB36420565() {     int BDGmRixZEh8975899 = -949235802;    int BDGmRixZEh50786204 = -342449140;    int BDGmRixZEh72294637 = -534847754;    int BDGmRixZEh5799781 = -939391841;    int BDGmRixZEh75399807 = -15952907;    int BDGmRixZEh29713361 = -475413348;    int BDGmRixZEh9588667 = -994428056;    int BDGmRixZEh66450527 = -984789058;    int BDGmRixZEh53138179 = -325702518;    int BDGmRixZEh76074702 = -381975965;    int BDGmRixZEh80472008 = -678428108;    int BDGmRixZEh36420174 = -893023052;    int BDGmRixZEh78191984 = -874964538;    int BDGmRixZEh38055370 = -184349569;    int BDGmRixZEh90011055 = -708689894;    int BDGmRixZEh29358449 = -471197034;    int BDGmRixZEh63910410 = -870881656;    int BDGmRixZEh25657705 = -430915847;    int BDGmRixZEh41852903 = -319930334;    int BDGmRixZEh48750972 = -504012906;    int BDGmRixZEh81328097 = -587439307;    int BDGmRixZEh64498787 = -645186930;    int BDGmRixZEh29842351 = -112935024;    int BDGmRixZEh86214960 = -641912617;    int BDGmRixZEh3816135 = -313779112;    int BDGmRixZEh5282658 = -287365786;    int BDGmRixZEh93496380 = -858076524;    int BDGmRixZEh9542964 = -447166117;    int BDGmRixZEh54103141 = -658675629;    int BDGmRixZEh63208904 = -113657403;    int BDGmRixZEh42195299 = -682801607;    int BDGmRixZEh56341004 = -384911973;    int BDGmRixZEh65052573 = -708781557;    int BDGmRixZEh76321090 = -39241552;    int BDGmRixZEh74989541 = -744594524;    int BDGmRixZEh74285596 = -77572318;    int BDGmRixZEh25326599 = -495568427;    int BDGmRixZEh76495961 = -89223776;    int BDGmRixZEh72758614 = -217309993;    int BDGmRixZEh73520197 = -115434300;    int BDGmRixZEh84110037 = -28885599;    int BDGmRixZEh82419633 = -62330596;    int BDGmRixZEh49255484 = -144066944;    int BDGmRixZEh18623676 = -284509182;    int BDGmRixZEh11502144 = -985935163;    int BDGmRixZEh75982528 = -520217777;    int BDGmRixZEh61648245 = -693319324;    int BDGmRixZEh5352397 = -996014967;    int BDGmRixZEh21235293 = -162253997;    int BDGmRixZEh93490677 = -18467580;    int BDGmRixZEh35104145 = -204410660;    int BDGmRixZEh53405631 = -798289443;    int BDGmRixZEh69715518 = 72518639;    int BDGmRixZEh25940706 = -563800105;    int BDGmRixZEh55092467 = -451478157;    int BDGmRixZEh44477112 = -204048872;    int BDGmRixZEh20943853 = -129514116;    int BDGmRixZEh86079677 = -892935138;    int BDGmRixZEh1983647 = -525612729;    int BDGmRixZEh70117150 = -728587122;    int BDGmRixZEh36216981 = -617336824;    int BDGmRixZEh45703 = -447261939;    int BDGmRixZEh12347387 = -226113430;    int BDGmRixZEh89929274 = -112045116;    int BDGmRixZEh33879404 = -699174359;    int BDGmRixZEh24131004 = -193516136;    int BDGmRixZEh71367601 = -84241495;    int BDGmRixZEh1870894 = -735722987;    int BDGmRixZEh63065828 = -439755046;    int BDGmRixZEh15725460 = -531117577;    int BDGmRixZEh4031851 = -975628608;    int BDGmRixZEh87414449 = -681657881;    int BDGmRixZEh52899090 = -113605854;    int BDGmRixZEh68332705 = -104496034;    int BDGmRixZEh64640935 = -375127308;    int BDGmRixZEh98908464 = -425108712;    int BDGmRixZEh15243303 = -401119986;    int BDGmRixZEh11218676 = -828425843;    int BDGmRixZEh74712817 = -655977454;    int BDGmRixZEh27833607 = -793561336;    int BDGmRixZEh43634413 = -594046462;    int BDGmRixZEh88143983 = -862061558;    int BDGmRixZEh88307671 = -184912121;    int BDGmRixZEh60612464 = -540208049;    int BDGmRixZEh28104760 = -909246744;    int BDGmRixZEh88789667 = -884512164;    int BDGmRixZEh86625486 = -357430612;    int BDGmRixZEh39111867 = -44981452;    int BDGmRixZEh21228623 = -587763395;    int BDGmRixZEh30512430 = -440545652;    int BDGmRixZEh53341743 = -948058202;    int BDGmRixZEh39246922 = -602633290;    int BDGmRixZEh74512315 = -563611047;    int BDGmRixZEh2641465 = -488722872;    int BDGmRixZEh37303217 = -498097477;    int BDGmRixZEh84064335 = -581623660;    int BDGmRixZEh70072246 = -836217167;    int BDGmRixZEh59326210 = 67978171;    int BDGmRixZEh84744271 = -585334824;    int BDGmRixZEh87371139 = -949235802;     BDGmRixZEh8975899 = BDGmRixZEh50786204;     BDGmRixZEh50786204 = BDGmRixZEh72294637;     BDGmRixZEh72294637 = BDGmRixZEh5799781;     BDGmRixZEh5799781 = BDGmRixZEh75399807;     BDGmRixZEh75399807 = BDGmRixZEh29713361;     BDGmRixZEh29713361 = BDGmRixZEh9588667;     BDGmRixZEh9588667 = BDGmRixZEh66450527;     BDGmRixZEh66450527 = BDGmRixZEh53138179;     BDGmRixZEh53138179 = BDGmRixZEh76074702;     BDGmRixZEh76074702 = BDGmRixZEh80472008;     BDGmRixZEh80472008 = BDGmRixZEh36420174;     BDGmRixZEh36420174 = BDGmRixZEh78191984;     BDGmRixZEh78191984 = BDGmRixZEh38055370;     BDGmRixZEh38055370 = BDGmRixZEh90011055;     BDGmRixZEh90011055 = BDGmRixZEh29358449;     BDGmRixZEh29358449 = BDGmRixZEh63910410;     BDGmRixZEh63910410 = BDGmRixZEh25657705;     BDGmRixZEh25657705 = BDGmRixZEh41852903;     BDGmRixZEh41852903 = BDGmRixZEh48750972;     BDGmRixZEh48750972 = BDGmRixZEh81328097;     BDGmRixZEh81328097 = BDGmRixZEh64498787;     BDGmRixZEh64498787 = BDGmRixZEh29842351;     BDGmRixZEh29842351 = BDGmRixZEh86214960;     BDGmRixZEh86214960 = BDGmRixZEh3816135;     BDGmRixZEh3816135 = BDGmRixZEh5282658;     BDGmRixZEh5282658 = BDGmRixZEh93496380;     BDGmRixZEh93496380 = BDGmRixZEh9542964;     BDGmRixZEh9542964 = BDGmRixZEh54103141;     BDGmRixZEh54103141 = BDGmRixZEh63208904;     BDGmRixZEh63208904 = BDGmRixZEh42195299;     BDGmRixZEh42195299 = BDGmRixZEh56341004;     BDGmRixZEh56341004 = BDGmRixZEh65052573;     BDGmRixZEh65052573 = BDGmRixZEh76321090;     BDGmRixZEh76321090 = BDGmRixZEh74989541;     BDGmRixZEh74989541 = BDGmRixZEh74285596;     BDGmRixZEh74285596 = BDGmRixZEh25326599;     BDGmRixZEh25326599 = BDGmRixZEh76495961;     BDGmRixZEh76495961 = BDGmRixZEh72758614;     BDGmRixZEh72758614 = BDGmRixZEh73520197;     BDGmRixZEh73520197 = BDGmRixZEh84110037;     BDGmRixZEh84110037 = BDGmRixZEh82419633;     BDGmRixZEh82419633 = BDGmRixZEh49255484;     BDGmRixZEh49255484 = BDGmRixZEh18623676;     BDGmRixZEh18623676 = BDGmRixZEh11502144;     BDGmRixZEh11502144 = BDGmRixZEh75982528;     BDGmRixZEh75982528 = BDGmRixZEh61648245;     BDGmRixZEh61648245 = BDGmRixZEh5352397;     BDGmRixZEh5352397 = BDGmRixZEh21235293;     BDGmRixZEh21235293 = BDGmRixZEh93490677;     BDGmRixZEh93490677 = BDGmRixZEh35104145;     BDGmRixZEh35104145 = BDGmRixZEh53405631;     BDGmRixZEh53405631 = BDGmRixZEh69715518;     BDGmRixZEh69715518 = BDGmRixZEh25940706;     BDGmRixZEh25940706 = BDGmRixZEh55092467;     BDGmRixZEh55092467 = BDGmRixZEh44477112;     BDGmRixZEh44477112 = BDGmRixZEh20943853;     BDGmRixZEh20943853 = BDGmRixZEh86079677;     BDGmRixZEh86079677 = BDGmRixZEh1983647;     BDGmRixZEh1983647 = BDGmRixZEh70117150;     BDGmRixZEh70117150 = BDGmRixZEh36216981;     BDGmRixZEh36216981 = BDGmRixZEh45703;     BDGmRixZEh45703 = BDGmRixZEh12347387;     BDGmRixZEh12347387 = BDGmRixZEh89929274;     BDGmRixZEh89929274 = BDGmRixZEh33879404;     BDGmRixZEh33879404 = BDGmRixZEh24131004;     BDGmRixZEh24131004 = BDGmRixZEh71367601;     BDGmRixZEh71367601 = BDGmRixZEh1870894;     BDGmRixZEh1870894 = BDGmRixZEh63065828;     BDGmRixZEh63065828 = BDGmRixZEh15725460;     BDGmRixZEh15725460 = BDGmRixZEh4031851;     BDGmRixZEh4031851 = BDGmRixZEh87414449;     BDGmRixZEh87414449 = BDGmRixZEh52899090;     BDGmRixZEh52899090 = BDGmRixZEh68332705;     BDGmRixZEh68332705 = BDGmRixZEh64640935;     BDGmRixZEh64640935 = BDGmRixZEh98908464;     BDGmRixZEh98908464 = BDGmRixZEh15243303;     BDGmRixZEh15243303 = BDGmRixZEh11218676;     BDGmRixZEh11218676 = BDGmRixZEh74712817;     BDGmRixZEh74712817 = BDGmRixZEh27833607;     BDGmRixZEh27833607 = BDGmRixZEh43634413;     BDGmRixZEh43634413 = BDGmRixZEh88143983;     BDGmRixZEh88143983 = BDGmRixZEh88307671;     BDGmRixZEh88307671 = BDGmRixZEh60612464;     BDGmRixZEh60612464 = BDGmRixZEh28104760;     BDGmRixZEh28104760 = BDGmRixZEh88789667;     BDGmRixZEh88789667 = BDGmRixZEh86625486;     BDGmRixZEh86625486 = BDGmRixZEh39111867;     BDGmRixZEh39111867 = BDGmRixZEh21228623;     BDGmRixZEh21228623 = BDGmRixZEh30512430;     BDGmRixZEh30512430 = BDGmRixZEh53341743;     BDGmRixZEh53341743 = BDGmRixZEh39246922;     BDGmRixZEh39246922 = BDGmRixZEh74512315;     BDGmRixZEh74512315 = BDGmRixZEh2641465;     BDGmRixZEh2641465 = BDGmRixZEh37303217;     BDGmRixZEh37303217 = BDGmRixZEh84064335;     BDGmRixZEh84064335 = BDGmRixZEh70072246;     BDGmRixZEh70072246 = BDGmRixZEh59326210;     BDGmRixZEh59326210 = BDGmRixZEh84744271;     BDGmRixZEh84744271 = BDGmRixZEh87371139;     BDGmRixZEh87371139 = BDGmRixZEh8975899;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void HIKTlZjOsK42418915() {     int LabzDLRUFP18291165 = -851766828;    int LabzDLRUFP6209127 = -358983643;    int LabzDLRUFP92446718 = -248488367;    int LabzDLRUFP63589803 = -31055833;    int LabzDLRUFP15463047 = 7531936;    int LabzDLRUFP23256723 = 65717792;    int LabzDLRUFP83242738 = -793676934;    int LabzDLRUFP55586684 = -81748206;    int LabzDLRUFP45946836 = -705763189;    int LabzDLRUFP44699159 = -202771633;    int LabzDLRUFP17373993 = -869669180;    int LabzDLRUFP47657923 = -430392867;    int LabzDLRUFP73973700 = -460386673;    int LabzDLRUFP70144011 = -163403674;    int LabzDLRUFP5287094 = -560220305;    int LabzDLRUFP34328973 = -498720512;    int LabzDLRUFP7721178 = -898466743;    int LabzDLRUFP37850346 = -160973160;    int LabzDLRUFP23865835 = -273571163;    int LabzDLRUFP11946321 = -582720747;    int LabzDLRUFP70007448 = -913139235;    int LabzDLRUFP88707699 = -468083263;    int LabzDLRUFP65903618 = -827464024;    int LabzDLRUFP45421590 = -159073727;    int LabzDLRUFP66878677 = -632912163;    int LabzDLRUFP71190475 = -386577427;    int LabzDLRUFP27254647 = -566965369;    int LabzDLRUFP80637439 = -954302850;    int LabzDLRUFP21220048 = -773095064;    int LabzDLRUFP75989852 = 80258035;    int LabzDLRUFP12193678 = -293453424;    int LabzDLRUFP33600368 = -816083268;    int LabzDLRUFP14911903 = -519872570;    int LabzDLRUFP65350038 = -622801438;    int LabzDLRUFP61639817 = -921007118;    int LabzDLRUFP6110491 = -493797765;    int LabzDLRUFP62794207 = -96768290;    int LabzDLRUFP32532760 = -941819139;    int LabzDLRUFP38984316 = -268525781;    int LabzDLRUFP49620095 = -922619813;    int LabzDLRUFP8173172 = -166042016;    int LabzDLRUFP70071289 = 16397313;    int LabzDLRUFP70972490 = -365947326;    int LabzDLRUFP77380576 = -774904070;    int LabzDLRUFP47452218 = -777710957;    int LabzDLRUFP46221763 = -845752335;    int LabzDLRUFP66897875 = -266153469;    int LabzDLRUFP76823433 = -342558355;    int LabzDLRUFP87262436 = -397950514;    int LabzDLRUFP63128579 = -911481283;    int LabzDLRUFP72071854 = 59713344;    int LabzDLRUFP17622673 = -346275661;    int LabzDLRUFP57723176 = -615258268;    int LabzDLRUFP30521950 = -923653223;    int LabzDLRUFP72411466 = -386396241;    int LabzDLRUFP29583466 = -283683566;    int LabzDLRUFP40305509 = -531519619;    int LabzDLRUFP47025129 = 10585359;    int LabzDLRUFP96711125 = -398143671;    int LabzDLRUFP44272572 = -605890638;    int LabzDLRUFP96002075 = -367316839;    int LabzDLRUFP2605299 = -839374085;    int LabzDLRUFP34366636 = -308653142;    int LabzDLRUFP69956983 = -686021224;    int LabzDLRUFP32505482 = -909318210;    int LabzDLRUFP83773625 = 46414087;    int LabzDLRUFP32746021 = -910520298;    int LabzDLRUFP8623662 = -837585235;    int LabzDLRUFP8504195 = -242396557;    int LabzDLRUFP99176603 = 33577459;    int LabzDLRUFP71534766 = -301952223;    int LabzDLRUFP75188418 = -956647605;    int LabzDLRUFP98866030 = -892447380;    int LabzDLRUFP74245740 = -350951351;    int LabzDLRUFP3773149 = -316678731;    int LabzDLRUFP99936159 = -829536549;    int LabzDLRUFP17735210 = -2135937;    int LabzDLRUFP88523042 = 47440045;    int LabzDLRUFP97969371 = -381362771;    int LabzDLRUFP20656914 = -787159828;    int LabzDLRUFP4292600 = -20423958;    int LabzDLRUFP50431213 = -124407014;    int LabzDLRUFP93375002 = -456352337;    int LabzDLRUFP58091469 = -861613782;    int LabzDLRUFP3917999 = -979455310;    int LabzDLRUFP94571004 = -947177763;    int LabzDLRUFP75877191 = -100825001;    int LabzDLRUFP84389953 = -596219347;    int LabzDLRUFP92938572 = -136405197;    int LabzDLRUFP32056351 = -537323552;    int LabzDLRUFP65804982 = -962278146;    int LabzDLRUFP15769078 = -7353649;    int LabzDLRUFP35821634 = -443675468;    int LabzDLRUFP94711743 = -662635143;    int LabzDLRUFP53618019 = -455302974;    int LabzDLRUFP5567873 = -326667932;    int LabzDLRUFP35704653 = -674949545;    int LabzDLRUFP1015507 = -679926102;    int LabzDLRUFP44875094 = -865585861;    int LabzDLRUFP63678593 = -851766828;     LabzDLRUFP18291165 = LabzDLRUFP6209127;     LabzDLRUFP6209127 = LabzDLRUFP92446718;     LabzDLRUFP92446718 = LabzDLRUFP63589803;     LabzDLRUFP63589803 = LabzDLRUFP15463047;     LabzDLRUFP15463047 = LabzDLRUFP23256723;     LabzDLRUFP23256723 = LabzDLRUFP83242738;     LabzDLRUFP83242738 = LabzDLRUFP55586684;     LabzDLRUFP55586684 = LabzDLRUFP45946836;     LabzDLRUFP45946836 = LabzDLRUFP44699159;     LabzDLRUFP44699159 = LabzDLRUFP17373993;     LabzDLRUFP17373993 = LabzDLRUFP47657923;     LabzDLRUFP47657923 = LabzDLRUFP73973700;     LabzDLRUFP73973700 = LabzDLRUFP70144011;     LabzDLRUFP70144011 = LabzDLRUFP5287094;     LabzDLRUFP5287094 = LabzDLRUFP34328973;     LabzDLRUFP34328973 = LabzDLRUFP7721178;     LabzDLRUFP7721178 = LabzDLRUFP37850346;     LabzDLRUFP37850346 = LabzDLRUFP23865835;     LabzDLRUFP23865835 = LabzDLRUFP11946321;     LabzDLRUFP11946321 = LabzDLRUFP70007448;     LabzDLRUFP70007448 = LabzDLRUFP88707699;     LabzDLRUFP88707699 = LabzDLRUFP65903618;     LabzDLRUFP65903618 = LabzDLRUFP45421590;     LabzDLRUFP45421590 = LabzDLRUFP66878677;     LabzDLRUFP66878677 = LabzDLRUFP71190475;     LabzDLRUFP71190475 = LabzDLRUFP27254647;     LabzDLRUFP27254647 = LabzDLRUFP80637439;     LabzDLRUFP80637439 = LabzDLRUFP21220048;     LabzDLRUFP21220048 = LabzDLRUFP75989852;     LabzDLRUFP75989852 = LabzDLRUFP12193678;     LabzDLRUFP12193678 = LabzDLRUFP33600368;     LabzDLRUFP33600368 = LabzDLRUFP14911903;     LabzDLRUFP14911903 = LabzDLRUFP65350038;     LabzDLRUFP65350038 = LabzDLRUFP61639817;     LabzDLRUFP61639817 = LabzDLRUFP6110491;     LabzDLRUFP6110491 = LabzDLRUFP62794207;     LabzDLRUFP62794207 = LabzDLRUFP32532760;     LabzDLRUFP32532760 = LabzDLRUFP38984316;     LabzDLRUFP38984316 = LabzDLRUFP49620095;     LabzDLRUFP49620095 = LabzDLRUFP8173172;     LabzDLRUFP8173172 = LabzDLRUFP70071289;     LabzDLRUFP70071289 = LabzDLRUFP70972490;     LabzDLRUFP70972490 = LabzDLRUFP77380576;     LabzDLRUFP77380576 = LabzDLRUFP47452218;     LabzDLRUFP47452218 = LabzDLRUFP46221763;     LabzDLRUFP46221763 = LabzDLRUFP66897875;     LabzDLRUFP66897875 = LabzDLRUFP76823433;     LabzDLRUFP76823433 = LabzDLRUFP87262436;     LabzDLRUFP87262436 = LabzDLRUFP63128579;     LabzDLRUFP63128579 = LabzDLRUFP72071854;     LabzDLRUFP72071854 = LabzDLRUFP17622673;     LabzDLRUFP17622673 = LabzDLRUFP57723176;     LabzDLRUFP57723176 = LabzDLRUFP30521950;     LabzDLRUFP30521950 = LabzDLRUFP72411466;     LabzDLRUFP72411466 = LabzDLRUFP29583466;     LabzDLRUFP29583466 = LabzDLRUFP40305509;     LabzDLRUFP40305509 = LabzDLRUFP47025129;     LabzDLRUFP47025129 = LabzDLRUFP96711125;     LabzDLRUFP96711125 = LabzDLRUFP44272572;     LabzDLRUFP44272572 = LabzDLRUFP96002075;     LabzDLRUFP96002075 = LabzDLRUFP2605299;     LabzDLRUFP2605299 = LabzDLRUFP34366636;     LabzDLRUFP34366636 = LabzDLRUFP69956983;     LabzDLRUFP69956983 = LabzDLRUFP32505482;     LabzDLRUFP32505482 = LabzDLRUFP83773625;     LabzDLRUFP83773625 = LabzDLRUFP32746021;     LabzDLRUFP32746021 = LabzDLRUFP8623662;     LabzDLRUFP8623662 = LabzDLRUFP8504195;     LabzDLRUFP8504195 = LabzDLRUFP99176603;     LabzDLRUFP99176603 = LabzDLRUFP71534766;     LabzDLRUFP71534766 = LabzDLRUFP75188418;     LabzDLRUFP75188418 = LabzDLRUFP98866030;     LabzDLRUFP98866030 = LabzDLRUFP74245740;     LabzDLRUFP74245740 = LabzDLRUFP3773149;     LabzDLRUFP3773149 = LabzDLRUFP99936159;     LabzDLRUFP99936159 = LabzDLRUFP17735210;     LabzDLRUFP17735210 = LabzDLRUFP88523042;     LabzDLRUFP88523042 = LabzDLRUFP97969371;     LabzDLRUFP97969371 = LabzDLRUFP20656914;     LabzDLRUFP20656914 = LabzDLRUFP4292600;     LabzDLRUFP4292600 = LabzDLRUFP50431213;     LabzDLRUFP50431213 = LabzDLRUFP93375002;     LabzDLRUFP93375002 = LabzDLRUFP58091469;     LabzDLRUFP58091469 = LabzDLRUFP3917999;     LabzDLRUFP3917999 = LabzDLRUFP94571004;     LabzDLRUFP94571004 = LabzDLRUFP75877191;     LabzDLRUFP75877191 = LabzDLRUFP84389953;     LabzDLRUFP84389953 = LabzDLRUFP92938572;     LabzDLRUFP92938572 = LabzDLRUFP32056351;     LabzDLRUFP32056351 = LabzDLRUFP65804982;     LabzDLRUFP65804982 = LabzDLRUFP15769078;     LabzDLRUFP15769078 = LabzDLRUFP35821634;     LabzDLRUFP35821634 = LabzDLRUFP94711743;     LabzDLRUFP94711743 = LabzDLRUFP53618019;     LabzDLRUFP53618019 = LabzDLRUFP5567873;     LabzDLRUFP5567873 = LabzDLRUFP35704653;     LabzDLRUFP35704653 = LabzDLRUFP1015507;     LabzDLRUFP1015507 = LabzDLRUFP44875094;     LabzDLRUFP44875094 = LabzDLRUFP63678593;     LabzDLRUFP63678593 = LabzDLRUFP18291165;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void QNLEGhXhxy659796() {     int byfuLDeMDl56948293 = -8672866;    int byfuLDeMDl31530848 = -546469972;    int byfuLDeMDl33943092 = -576963134;    int byfuLDeMDl66528399 = -953122266;    int byfuLDeMDl42916067 = -653932195;    int byfuLDeMDl61605754 = -491002729;    int byfuLDeMDl7065250 = -298686569;    int byfuLDeMDl57094510 = -736067581;    int byfuLDeMDl75630602 = -706122602;    int byfuLDeMDl86996669 = -459278821;    int byfuLDeMDl32347422 = -985480395;    int byfuLDeMDl27640779 = -487477252;    int byfuLDeMDl21780943 = -157930223;    int byfuLDeMDl72229839 = -686582620;    int byfuLDeMDl83015221 = -224713565;    int byfuLDeMDl81335404 = -189056054;    int byfuLDeMDl13531879 = -836481270;    int byfuLDeMDl52750025 = -298935899;    int byfuLDeMDl44599255 = -138006316;    int byfuLDeMDl88428722 = -731086735;    int byfuLDeMDl83220469 = 59373603;    int byfuLDeMDl29817113 = -469686173;    int byfuLDeMDl36899472 = -333107722;    int byfuLDeMDl14428790 = 35245546;    int byfuLDeMDl94259521 = -602834379;    int byfuLDeMDl23074789 = -295710551;    int byfuLDeMDl22973710 = -181476152;    int byfuLDeMDl79353214 = -465282804;    int byfuLDeMDl68872914 = -139226598;    int byfuLDeMDl25688575 = 57137169;    int byfuLDeMDl8532738 = -542764789;    int byfuLDeMDl51072128 = -210779161;    int byfuLDeMDl67278295 = -585946189;    int byfuLDeMDl89296849 = -419513373;    int byfuLDeMDl52623894 = -409786858;    int byfuLDeMDl20770316 = -910562411;    int byfuLDeMDl68411696 = -709028323;    int byfuLDeMDl5694749 = -905564073;    int byfuLDeMDl61990324 = 23305653;    int byfuLDeMDl2489586 = -721628649;    int byfuLDeMDl60018689 = -7123454;    int byfuLDeMDl14937615 = -329277791;    int byfuLDeMDl4005896 = -60203910;    int byfuLDeMDl88861073 = -750288430;    int byfuLDeMDl71466286 = -734099758;    int byfuLDeMDl50483132 = -124994410;    int byfuLDeMDl73714096 = -204198438;    int byfuLDeMDl58063842 = -945732804;    int byfuLDeMDl66506518 = -211646922;    int byfuLDeMDl44384418 = -8510992;    int byfuLDeMDl54109609 = -808145539;    int byfuLDeMDl54515467 = -307187905;    int byfuLDeMDl66103525 = -167291140;    int byfuLDeMDl55126160 = -54544696;    int byfuLDeMDl17439327 = 32727310;    int byfuLDeMDl27131181 = -538986693;    int byfuLDeMDl94631375 = -113362251;    int byfuLDeMDl19514302 = -512208681;    int byfuLDeMDl72268878 = -250287888;    int byfuLDeMDl19841278 = -258221645;    int byfuLDeMDl38632045 = -209526577;    int byfuLDeMDl27712036 = -833403766;    int byfuLDeMDl88221596 = -496840984;    int byfuLDeMDl49942028 = -663259771;    int byfuLDeMDl78463931 = -916514032;    int byfuLDeMDl81275294 = -674701235;    int byfuLDeMDl60362483 = -901531063;    int byfuLDeMDl32484094 = -738416850;    int byfuLDeMDl19605945 = -176795763;    int byfuLDeMDl62244905 = -314151155;    int byfuLDeMDl12923709 = -480027731;    int byfuLDeMDl7837130 = -930917198;    int byfuLDeMDl90759701 = -222241552;    int byfuLDeMDl42109669 = -416377668;    int byfuLDeMDl28410033 = -623963281;    int byfuLDeMDl68282855 = -611348606;    int byfuLDeMDl25811217 = -309482264;    int byfuLDeMDl48038398 = -582819292;    int byfuLDeMDl42962504 = -230654696;    int byfuLDeMDl43776389 = -377839970;    int byfuLDeMDl49360693 = 8487886;    int byfuLDeMDl64909867 = -235743348;    int byfuLDeMDl12846697 = -153635882;    int byfuLDeMDl24488496 = -30715606;    int byfuLDeMDl71578966 = -134717293;    int byfuLDeMDl54017271 = -135576885;    int byfuLDeMDl84968602 = 56511978;    int byfuLDeMDl12152136 = -431401494;    int byfuLDeMDl71857523 = -352240684;    int byfuLDeMDl25492714 = -870800165;    int byfuLDeMDl26138940 = -697200160;    int byfuLDeMDl48897395 = -96819642;    int byfuLDeMDl33425871 = -555276186;    int byfuLDeMDl42149047 = -718472703;    int byfuLDeMDl63857541 = -412102072;    int byfuLDeMDl32306654 = -173719689;    int byfuLDeMDl26716018 = -832436808;    int byfuLDeMDl54063868 = -396944139;    int byfuLDeMDl10397143 = -833774398;    int byfuLDeMDl90190992 = -8672866;     byfuLDeMDl56948293 = byfuLDeMDl31530848;     byfuLDeMDl31530848 = byfuLDeMDl33943092;     byfuLDeMDl33943092 = byfuLDeMDl66528399;     byfuLDeMDl66528399 = byfuLDeMDl42916067;     byfuLDeMDl42916067 = byfuLDeMDl61605754;     byfuLDeMDl61605754 = byfuLDeMDl7065250;     byfuLDeMDl7065250 = byfuLDeMDl57094510;     byfuLDeMDl57094510 = byfuLDeMDl75630602;     byfuLDeMDl75630602 = byfuLDeMDl86996669;     byfuLDeMDl86996669 = byfuLDeMDl32347422;     byfuLDeMDl32347422 = byfuLDeMDl27640779;     byfuLDeMDl27640779 = byfuLDeMDl21780943;     byfuLDeMDl21780943 = byfuLDeMDl72229839;     byfuLDeMDl72229839 = byfuLDeMDl83015221;     byfuLDeMDl83015221 = byfuLDeMDl81335404;     byfuLDeMDl81335404 = byfuLDeMDl13531879;     byfuLDeMDl13531879 = byfuLDeMDl52750025;     byfuLDeMDl52750025 = byfuLDeMDl44599255;     byfuLDeMDl44599255 = byfuLDeMDl88428722;     byfuLDeMDl88428722 = byfuLDeMDl83220469;     byfuLDeMDl83220469 = byfuLDeMDl29817113;     byfuLDeMDl29817113 = byfuLDeMDl36899472;     byfuLDeMDl36899472 = byfuLDeMDl14428790;     byfuLDeMDl14428790 = byfuLDeMDl94259521;     byfuLDeMDl94259521 = byfuLDeMDl23074789;     byfuLDeMDl23074789 = byfuLDeMDl22973710;     byfuLDeMDl22973710 = byfuLDeMDl79353214;     byfuLDeMDl79353214 = byfuLDeMDl68872914;     byfuLDeMDl68872914 = byfuLDeMDl25688575;     byfuLDeMDl25688575 = byfuLDeMDl8532738;     byfuLDeMDl8532738 = byfuLDeMDl51072128;     byfuLDeMDl51072128 = byfuLDeMDl67278295;     byfuLDeMDl67278295 = byfuLDeMDl89296849;     byfuLDeMDl89296849 = byfuLDeMDl52623894;     byfuLDeMDl52623894 = byfuLDeMDl20770316;     byfuLDeMDl20770316 = byfuLDeMDl68411696;     byfuLDeMDl68411696 = byfuLDeMDl5694749;     byfuLDeMDl5694749 = byfuLDeMDl61990324;     byfuLDeMDl61990324 = byfuLDeMDl2489586;     byfuLDeMDl2489586 = byfuLDeMDl60018689;     byfuLDeMDl60018689 = byfuLDeMDl14937615;     byfuLDeMDl14937615 = byfuLDeMDl4005896;     byfuLDeMDl4005896 = byfuLDeMDl88861073;     byfuLDeMDl88861073 = byfuLDeMDl71466286;     byfuLDeMDl71466286 = byfuLDeMDl50483132;     byfuLDeMDl50483132 = byfuLDeMDl73714096;     byfuLDeMDl73714096 = byfuLDeMDl58063842;     byfuLDeMDl58063842 = byfuLDeMDl66506518;     byfuLDeMDl66506518 = byfuLDeMDl44384418;     byfuLDeMDl44384418 = byfuLDeMDl54109609;     byfuLDeMDl54109609 = byfuLDeMDl54515467;     byfuLDeMDl54515467 = byfuLDeMDl66103525;     byfuLDeMDl66103525 = byfuLDeMDl55126160;     byfuLDeMDl55126160 = byfuLDeMDl17439327;     byfuLDeMDl17439327 = byfuLDeMDl27131181;     byfuLDeMDl27131181 = byfuLDeMDl94631375;     byfuLDeMDl94631375 = byfuLDeMDl19514302;     byfuLDeMDl19514302 = byfuLDeMDl72268878;     byfuLDeMDl72268878 = byfuLDeMDl19841278;     byfuLDeMDl19841278 = byfuLDeMDl38632045;     byfuLDeMDl38632045 = byfuLDeMDl27712036;     byfuLDeMDl27712036 = byfuLDeMDl88221596;     byfuLDeMDl88221596 = byfuLDeMDl49942028;     byfuLDeMDl49942028 = byfuLDeMDl78463931;     byfuLDeMDl78463931 = byfuLDeMDl81275294;     byfuLDeMDl81275294 = byfuLDeMDl60362483;     byfuLDeMDl60362483 = byfuLDeMDl32484094;     byfuLDeMDl32484094 = byfuLDeMDl19605945;     byfuLDeMDl19605945 = byfuLDeMDl62244905;     byfuLDeMDl62244905 = byfuLDeMDl12923709;     byfuLDeMDl12923709 = byfuLDeMDl7837130;     byfuLDeMDl7837130 = byfuLDeMDl90759701;     byfuLDeMDl90759701 = byfuLDeMDl42109669;     byfuLDeMDl42109669 = byfuLDeMDl28410033;     byfuLDeMDl28410033 = byfuLDeMDl68282855;     byfuLDeMDl68282855 = byfuLDeMDl25811217;     byfuLDeMDl25811217 = byfuLDeMDl48038398;     byfuLDeMDl48038398 = byfuLDeMDl42962504;     byfuLDeMDl42962504 = byfuLDeMDl43776389;     byfuLDeMDl43776389 = byfuLDeMDl49360693;     byfuLDeMDl49360693 = byfuLDeMDl64909867;     byfuLDeMDl64909867 = byfuLDeMDl12846697;     byfuLDeMDl12846697 = byfuLDeMDl24488496;     byfuLDeMDl24488496 = byfuLDeMDl71578966;     byfuLDeMDl71578966 = byfuLDeMDl54017271;     byfuLDeMDl54017271 = byfuLDeMDl84968602;     byfuLDeMDl84968602 = byfuLDeMDl12152136;     byfuLDeMDl12152136 = byfuLDeMDl71857523;     byfuLDeMDl71857523 = byfuLDeMDl25492714;     byfuLDeMDl25492714 = byfuLDeMDl26138940;     byfuLDeMDl26138940 = byfuLDeMDl48897395;     byfuLDeMDl48897395 = byfuLDeMDl33425871;     byfuLDeMDl33425871 = byfuLDeMDl42149047;     byfuLDeMDl42149047 = byfuLDeMDl63857541;     byfuLDeMDl63857541 = byfuLDeMDl32306654;     byfuLDeMDl32306654 = byfuLDeMDl26716018;     byfuLDeMDl26716018 = byfuLDeMDl54063868;     byfuLDeMDl54063868 = byfuLDeMDl10397143;     byfuLDeMDl10397143 = byfuLDeMDl90190992;     byfuLDeMDl90190992 = byfuLDeMDl56948293;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void DVLhjUNtym6658146() {     int eGJclYrpET66263559 = 88796108;    int eGJclYrpET86953770 = -563004474;    int eGJclYrpET54095172 = -290603747;    int eGJclYrpET24318421 = -44786258;    int eGJclYrpET82979305 = -630447353;    int eGJclYrpET55149116 = 50128411;    int eGJclYrpET80719321 = -97935447;    int eGJclYrpET46230667 = -933026729;    int eGJclYrpET68439259 = 13816727;    int eGJclYrpET55621126 = -280074489;    int eGJclYrpET69249406 = -76721468;    int eGJclYrpET38878528 = -24847067;    int eGJclYrpET17562659 = -843352358;    int eGJclYrpET4318481 = -665636724;    int eGJclYrpET98291258 = -76243976;    int eGJclYrpET86305928 = -216579532;    int eGJclYrpET57342646 = -864066357;    int eGJclYrpET64942666 = -28993211;    int eGJclYrpET26612187 = -91647145;    int eGJclYrpET51624070 = -809794577;    int eGJclYrpET71899821 = -266326325;    int eGJclYrpET54026024 = -292582505;    int eGJclYrpET72960739 = 52363279;    int eGJclYrpET73635419 = -581915564;    int eGJclYrpET57322063 = -921967430;    int eGJclYrpET88982606 = -394922192;    int eGJclYrpET56731976 = -990364997;    int eGJclYrpET50447690 = -972419536;    int eGJclYrpET35989821 = -253646034;    int eGJclYrpET38469523 = -848947394;    int eGJclYrpET78531117 = -153416606;    int eGJclYrpET28331491 = -641950456;    int eGJclYrpET17137626 = -397037202;    int eGJclYrpET78325797 = 96926740;    int eGJclYrpET39274170 = -586199452;    int eGJclYrpET52595210 = -226787858;    int eGJclYrpET5879305 = -310228186;    int eGJclYrpET61731547 = -658159436;    int eGJclYrpET28216026 = -27910134;    int eGJclYrpET78589483 = -428814161;    int eGJclYrpET84081823 = -144279872;    int eGJclYrpET2589271 = -250549882;    int eGJclYrpET25722902 = -282084291;    int eGJclYrpET47617974 = -140683318;    int eGJclYrpET7416361 = -525875552;    int eGJclYrpET20722368 = -450528968;    int eGJclYrpET78963727 = -877032583;    int eGJclYrpET29534879 = -292276193;    int eGJclYrpET32533662 = -447343439;    int eGJclYrpET14022320 = -901524695;    int eGJclYrpET91077318 = -544021535;    int eGJclYrpET18732510 = -955174122;    int eGJclYrpET54111183 = -855068047;    int eGJclYrpET59707404 = -414397813;    int eGJclYrpET34758325 = 97809226;    int eGJclYrpET12237535 = -618621387;    int eGJclYrpET13993032 = -515367753;    int eGJclYrpET80459753 = -708688184;    int eGJclYrpET66996357 = -122818829;    int eGJclYrpET93996699 = -135525161;    int eGJclYrpET98417139 = 40493407;    int eGJclYrpET30271632 = -125515911;    int eGJclYrpET10240846 = -579380696;    int eGJclYrpET29969737 = -137235880;    int eGJclYrpET77090009 = -26657883;    int eGJclYrpET40917915 = -434771012;    int eGJclYrpET21740903 = -627809866;    int eGJclYrpET39236861 = -840279098;    int eGJclYrpET65044311 = 20562727;    int eGJclYrpET45696049 = -849456119;    int eGJclYrpET80426624 = -906351346;    int eGJclYrpET95611098 = -105906922;    int eGJclYrpET36726641 = 98916922;    int eGJclYrpET48022704 = -662832985;    int eGJclYrpET67542247 = -565514705;    int eGJclYrpET69310551 = 84223557;    int eGJclYrpET28303123 = 89501785;    int eGJclYrpET25342765 = -806953404;    int eGJclYrpET66219058 = 43959987;    int eGJclYrpET36599696 = -371438462;    int eGJclYrpET10018879 = -517889610;    int eGJclYrpET27197097 = -598088804;    int eGJclYrpET17914028 = -425076098;    int eGJclYrpET21967502 = -352121340;    int eGJclYrpET47392205 = -204925859;    int eGJclYrpET59798607 = -198242485;    int eGJclYrpET74220308 = -786882410;    int eGJclYrpET57430221 = -982639389;    int eGJclYrpET43567472 = 99117514;    int eGJclYrpET27036635 = -967578065;    int eGJclYrpET38602179 = -711420105;    int eGJclYrpET25419551 = -601540002;    int eGJclYrpET94735189 = -435340607;    int eGJclYrpET34219326 = -892384974;    int eGJclYrpET80172343 = -369307569;    int eGJclYrpET53810192 = 81236039;    int eGJclYrpET92348424 = -671169187;    int eGJclYrpET95753164 = -44848412;    int eGJclYrpET70527964 = -14025435;    int eGJclYrpET66498445 = 88796108;     eGJclYrpET66263559 = eGJclYrpET86953770;     eGJclYrpET86953770 = eGJclYrpET54095172;     eGJclYrpET54095172 = eGJclYrpET24318421;     eGJclYrpET24318421 = eGJclYrpET82979305;     eGJclYrpET82979305 = eGJclYrpET55149116;     eGJclYrpET55149116 = eGJclYrpET80719321;     eGJclYrpET80719321 = eGJclYrpET46230667;     eGJclYrpET46230667 = eGJclYrpET68439259;     eGJclYrpET68439259 = eGJclYrpET55621126;     eGJclYrpET55621126 = eGJclYrpET69249406;     eGJclYrpET69249406 = eGJclYrpET38878528;     eGJclYrpET38878528 = eGJclYrpET17562659;     eGJclYrpET17562659 = eGJclYrpET4318481;     eGJclYrpET4318481 = eGJclYrpET98291258;     eGJclYrpET98291258 = eGJclYrpET86305928;     eGJclYrpET86305928 = eGJclYrpET57342646;     eGJclYrpET57342646 = eGJclYrpET64942666;     eGJclYrpET64942666 = eGJclYrpET26612187;     eGJclYrpET26612187 = eGJclYrpET51624070;     eGJclYrpET51624070 = eGJclYrpET71899821;     eGJclYrpET71899821 = eGJclYrpET54026024;     eGJclYrpET54026024 = eGJclYrpET72960739;     eGJclYrpET72960739 = eGJclYrpET73635419;     eGJclYrpET73635419 = eGJclYrpET57322063;     eGJclYrpET57322063 = eGJclYrpET88982606;     eGJclYrpET88982606 = eGJclYrpET56731976;     eGJclYrpET56731976 = eGJclYrpET50447690;     eGJclYrpET50447690 = eGJclYrpET35989821;     eGJclYrpET35989821 = eGJclYrpET38469523;     eGJclYrpET38469523 = eGJclYrpET78531117;     eGJclYrpET78531117 = eGJclYrpET28331491;     eGJclYrpET28331491 = eGJclYrpET17137626;     eGJclYrpET17137626 = eGJclYrpET78325797;     eGJclYrpET78325797 = eGJclYrpET39274170;     eGJclYrpET39274170 = eGJclYrpET52595210;     eGJclYrpET52595210 = eGJclYrpET5879305;     eGJclYrpET5879305 = eGJclYrpET61731547;     eGJclYrpET61731547 = eGJclYrpET28216026;     eGJclYrpET28216026 = eGJclYrpET78589483;     eGJclYrpET78589483 = eGJclYrpET84081823;     eGJclYrpET84081823 = eGJclYrpET2589271;     eGJclYrpET2589271 = eGJclYrpET25722902;     eGJclYrpET25722902 = eGJclYrpET47617974;     eGJclYrpET47617974 = eGJclYrpET7416361;     eGJclYrpET7416361 = eGJclYrpET20722368;     eGJclYrpET20722368 = eGJclYrpET78963727;     eGJclYrpET78963727 = eGJclYrpET29534879;     eGJclYrpET29534879 = eGJclYrpET32533662;     eGJclYrpET32533662 = eGJclYrpET14022320;     eGJclYrpET14022320 = eGJclYrpET91077318;     eGJclYrpET91077318 = eGJclYrpET18732510;     eGJclYrpET18732510 = eGJclYrpET54111183;     eGJclYrpET54111183 = eGJclYrpET59707404;     eGJclYrpET59707404 = eGJclYrpET34758325;     eGJclYrpET34758325 = eGJclYrpET12237535;     eGJclYrpET12237535 = eGJclYrpET13993032;     eGJclYrpET13993032 = eGJclYrpET80459753;     eGJclYrpET80459753 = eGJclYrpET66996357;     eGJclYrpET66996357 = eGJclYrpET93996699;     eGJclYrpET93996699 = eGJclYrpET98417139;     eGJclYrpET98417139 = eGJclYrpET30271632;     eGJclYrpET30271632 = eGJclYrpET10240846;     eGJclYrpET10240846 = eGJclYrpET29969737;     eGJclYrpET29969737 = eGJclYrpET77090009;     eGJclYrpET77090009 = eGJclYrpET40917915;     eGJclYrpET40917915 = eGJclYrpET21740903;     eGJclYrpET21740903 = eGJclYrpET39236861;     eGJclYrpET39236861 = eGJclYrpET65044311;     eGJclYrpET65044311 = eGJclYrpET45696049;     eGJclYrpET45696049 = eGJclYrpET80426624;     eGJclYrpET80426624 = eGJclYrpET95611098;     eGJclYrpET95611098 = eGJclYrpET36726641;     eGJclYrpET36726641 = eGJclYrpET48022704;     eGJclYrpET48022704 = eGJclYrpET67542247;     eGJclYrpET67542247 = eGJclYrpET69310551;     eGJclYrpET69310551 = eGJclYrpET28303123;     eGJclYrpET28303123 = eGJclYrpET25342765;     eGJclYrpET25342765 = eGJclYrpET66219058;     eGJclYrpET66219058 = eGJclYrpET36599696;     eGJclYrpET36599696 = eGJclYrpET10018879;     eGJclYrpET10018879 = eGJclYrpET27197097;     eGJclYrpET27197097 = eGJclYrpET17914028;     eGJclYrpET17914028 = eGJclYrpET21967502;     eGJclYrpET21967502 = eGJclYrpET47392205;     eGJclYrpET47392205 = eGJclYrpET59798607;     eGJclYrpET59798607 = eGJclYrpET74220308;     eGJclYrpET74220308 = eGJclYrpET57430221;     eGJclYrpET57430221 = eGJclYrpET43567472;     eGJclYrpET43567472 = eGJclYrpET27036635;     eGJclYrpET27036635 = eGJclYrpET38602179;     eGJclYrpET38602179 = eGJclYrpET25419551;     eGJclYrpET25419551 = eGJclYrpET94735189;     eGJclYrpET94735189 = eGJclYrpET34219326;     eGJclYrpET34219326 = eGJclYrpET80172343;     eGJclYrpET80172343 = eGJclYrpET53810192;     eGJclYrpET53810192 = eGJclYrpET92348424;     eGJclYrpET92348424 = eGJclYrpET95753164;     eGJclYrpET95753164 = eGJclYrpET70527964;     eGJclYrpET70527964 = eGJclYrpET66498445;     eGJclYrpET66498445 = eGJclYrpET66263559;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void sTXtjuBdZE35677269() {     long lNZqnZQFzE35444428 = -970633894;    long lNZqnZQFzE32467023 = -257856713;    long lNZqnZQFzE55396069 = -638860666;    long lNZqnZQFzE89959694 = -100954466;    long lNZqnZQFzE21035937 = -701083220;    long lNZqnZQFzE11412864 = -240398356;    long lNZqnZQFzE70394124 = -715309436;    long lNZqnZQFzE19005824 = -573759785;    long lNZqnZQFzE90861434 = -275408894;    long lNZqnZQFzE15789306 = -902004220;    long lNZqnZQFzE55400328 = -258882473;    long lNZqnZQFzE33231367 = -301976700;    long lNZqnZQFzE64814384 = -968611725;    long lNZqnZQFzE40293543 = -527971259;    long lNZqnZQFzE21958061 = -496044509;    long lNZqnZQFzE35302395 = -134717703;    long lNZqnZQFzE27007183 = -954824415;    long lNZqnZQFzE59530261 = -637540759;    long lNZqnZQFzE48246065 = -901355372;    long lNZqnZQFzE87587374 = -455654672;    long lNZqnZQFzE72243550 = 24315872;    long lNZqnZQFzE3023687 = -144872056;    long lNZqnZQFzE21881583 = -554403103;    long lNZqnZQFzE29483587 = -52202952;    long lNZqnZQFzE84052780 = -305956853;    long lNZqnZQFzE81817055 = 46482491;    long lNZqnZQFzE31299885 = -215724489;    long lNZqnZQFzE42626196 = -530874946;    long lNZqnZQFzE7745789 = -138454316;    long lNZqnZQFzE73573728 = -374918782;    long lNZqnZQFzE86831941 = -471268423;    long lNZqnZQFzE87128762 = -512074534;    long lNZqnZQFzE14843444 = -985333324;    long lNZqnZQFzE43869647 = -66034154;    long lNZqnZQFzE43275458 = -336418791;    long lNZqnZQFzE84724334 = -714691077;    long lNZqnZQFzE33040053 = -245178867;    long lNZqnZQFzE30808162 = -538773903;    long lNZqnZQFzE78050705 = -437515646;    long lNZqnZQFzE75018354 = -568081508;    long lNZqnZQFzE79264869 = -410504888;    long lNZqnZQFzE17718539 = -599568863;    long lNZqnZQFzE37977266 = -177451895;    long lNZqnZQFzE74752806 = -213200571;    long lNZqnZQFzE6424584 = -521715642;    long lNZqnZQFzE21057570 = -882495460;    long lNZqnZQFzE88412308 = -952985006;    long lNZqnZQFzE29848934 = -98115023;    long lNZqnZQFzE18965139 = 23194565;    long lNZqnZQFzE29436595 = -821323167;    long lNZqnZQFzE65253716 = -478709246;    long lNZqnZQFzE7360516 = -191415692;    long lNZqnZQFzE11976821 = -871059469;    long lNZqnZQFzE74194424 = 59404536;    long lNZqnZQFzE63433719 = -504632684;    long lNZqnZQFzE32420742 = -725761838;    long lNZqnZQFzE10585441 = -703453611;    long lNZqnZQFzE25912483 = -486657715;    long lNZqnZQFzE5906915 = -794997613;    long lNZqnZQFzE39218882 = -647565712;    long lNZqnZQFzE80112978 = 75326132;    long lNZqnZQFzE27767929 = -84434491;    long lNZqnZQFzE11260035 = -335305470;    long lNZqnZQFzE17287706 = -900490112;    long lNZqnZQFzE28957364 = -330735798;    long lNZqnZQFzE68271566 = -746807940;    long lNZqnZQFzE18387924 = -316643377;    long lNZqnZQFzE20944737 = -802577571;    long lNZqnZQFzE97018085 = -91552468;    long lNZqnZQFzE37233726 = -781353433;    long lNZqnZQFzE2262343 = -889538836;    long lNZqnZQFzE96199021 = -316050512;    long lNZqnZQFzE81479555 = -100025114;    long lNZqnZQFzE73227710 = -233273864;    long lNZqnZQFzE8322506 = 54850215;    long lNZqnZQFzE54525012 = -376115265;    long lNZqnZQFzE65046421 = -967420162;    long lNZqnZQFzE47128777 = -241202532;    long lNZqnZQFzE23059003 = -530487310;    long lNZqnZQFzE62995210 = -423461394;    long lNZqnZQFzE93404746 = -532503;    long lNZqnZQFzE1450952 = -17609466;    long lNZqnZQFzE23661058 = -454069511;    long lNZqnZQFzE78309194 = -317131149;    long lNZqnZQFzE8320013 = -896209537;    long lNZqnZQFzE79471425 = -179852731;    long lNZqnZQFzE75151941 = -641015066;    long lNZqnZQFzE40649019 = -944737861;    long lNZqnZQFzE80435928 = -561401470;    long lNZqnZQFzE10854717 = -610656954;    long lNZqnZQFzE74138894 = 88762533;    long lNZqnZQFzE7127570 = -758521152;    long lNZqnZQFzE24901247 = -743776290;    long lNZqnZQFzE38831823 = -789949935;    long lNZqnZQFzE94905375 = -543407641;    long lNZqnZQFzE51496940 = -226070398;    long lNZqnZQFzE6458505 = -164263393;    long lNZqnZQFzE20689560 = -276961783;    long lNZqnZQFzE45795442 = -882464773;    long lNZqnZQFzE38153017 = -970633894;     lNZqnZQFzE35444428 = lNZqnZQFzE32467023;     lNZqnZQFzE32467023 = lNZqnZQFzE55396069;     lNZqnZQFzE55396069 = lNZqnZQFzE89959694;     lNZqnZQFzE89959694 = lNZqnZQFzE21035937;     lNZqnZQFzE21035937 = lNZqnZQFzE11412864;     lNZqnZQFzE11412864 = lNZqnZQFzE70394124;     lNZqnZQFzE70394124 = lNZqnZQFzE19005824;     lNZqnZQFzE19005824 = lNZqnZQFzE90861434;     lNZqnZQFzE90861434 = lNZqnZQFzE15789306;     lNZqnZQFzE15789306 = lNZqnZQFzE55400328;     lNZqnZQFzE55400328 = lNZqnZQFzE33231367;     lNZqnZQFzE33231367 = lNZqnZQFzE64814384;     lNZqnZQFzE64814384 = lNZqnZQFzE40293543;     lNZqnZQFzE40293543 = lNZqnZQFzE21958061;     lNZqnZQFzE21958061 = lNZqnZQFzE35302395;     lNZqnZQFzE35302395 = lNZqnZQFzE27007183;     lNZqnZQFzE27007183 = lNZqnZQFzE59530261;     lNZqnZQFzE59530261 = lNZqnZQFzE48246065;     lNZqnZQFzE48246065 = lNZqnZQFzE87587374;     lNZqnZQFzE87587374 = lNZqnZQFzE72243550;     lNZqnZQFzE72243550 = lNZqnZQFzE3023687;     lNZqnZQFzE3023687 = lNZqnZQFzE21881583;     lNZqnZQFzE21881583 = lNZqnZQFzE29483587;     lNZqnZQFzE29483587 = lNZqnZQFzE84052780;     lNZqnZQFzE84052780 = lNZqnZQFzE81817055;     lNZqnZQFzE81817055 = lNZqnZQFzE31299885;     lNZqnZQFzE31299885 = lNZqnZQFzE42626196;     lNZqnZQFzE42626196 = lNZqnZQFzE7745789;     lNZqnZQFzE7745789 = lNZqnZQFzE73573728;     lNZqnZQFzE73573728 = lNZqnZQFzE86831941;     lNZqnZQFzE86831941 = lNZqnZQFzE87128762;     lNZqnZQFzE87128762 = lNZqnZQFzE14843444;     lNZqnZQFzE14843444 = lNZqnZQFzE43869647;     lNZqnZQFzE43869647 = lNZqnZQFzE43275458;     lNZqnZQFzE43275458 = lNZqnZQFzE84724334;     lNZqnZQFzE84724334 = lNZqnZQFzE33040053;     lNZqnZQFzE33040053 = lNZqnZQFzE30808162;     lNZqnZQFzE30808162 = lNZqnZQFzE78050705;     lNZqnZQFzE78050705 = lNZqnZQFzE75018354;     lNZqnZQFzE75018354 = lNZqnZQFzE79264869;     lNZqnZQFzE79264869 = lNZqnZQFzE17718539;     lNZqnZQFzE17718539 = lNZqnZQFzE37977266;     lNZqnZQFzE37977266 = lNZqnZQFzE74752806;     lNZqnZQFzE74752806 = lNZqnZQFzE6424584;     lNZqnZQFzE6424584 = lNZqnZQFzE21057570;     lNZqnZQFzE21057570 = lNZqnZQFzE88412308;     lNZqnZQFzE88412308 = lNZqnZQFzE29848934;     lNZqnZQFzE29848934 = lNZqnZQFzE18965139;     lNZqnZQFzE18965139 = lNZqnZQFzE29436595;     lNZqnZQFzE29436595 = lNZqnZQFzE65253716;     lNZqnZQFzE65253716 = lNZqnZQFzE7360516;     lNZqnZQFzE7360516 = lNZqnZQFzE11976821;     lNZqnZQFzE11976821 = lNZqnZQFzE74194424;     lNZqnZQFzE74194424 = lNZqnZQFzE63433719;     lNZqnZQFzE63433719 = lNZqnZQFzE32420742;     lNZqnZQFzE32420742 = lNZqnZQFzE10585441;     lNZqnZQFzE10585441 = lNZqnZQFzE25912483;     lNZqnZQFzE25912483 = lNZqnZQFzE5906915;     lNZqnZQFzE5906915 = lNZqnZQFzE39218882;     lNZqnZQFzE39218882 = lNZqnZQFzE80112978;     lNZqnZQFzE80112978 = lNZqnZQFzE27767929;     lNZqnZQFzE27767929 = lNZqnZQFzE11260035;     lNZqnZQFzE11260035 = lNZqnZQFzE17287706;     lNZqnZQFzE17287706 = lNZqnZQFzE28957364;     lNZqnZQFzE28957364 = lNZqnZQFzE68271566;     lNZqnZQFzE68271566 = lNZqnZQFzE18387924;     lNZqnZQFzE18387924 = lNZqnZQFzE20944737;     lNZqnZQFzE20944737 = lNZqnZQFzE97018085;     lNZqnZQFzE97018085 = lNZqnZQFzE37233726;     lNZqnZQFzE37233726 = lNZqnZQFzE2262343;     lNZqnZQFzE2262343 = lNZqnZQFzE96199021;     lNZqnZQFzE96199021 = lNZqnZQFzE81479555;     lNZqnZQFzE81479555 = lNZqnZQFzE73227710;     lNZqnZQFzE73227710 = lNZqnZQFzE8322506;     lNZqnZQFzE8322506 = lNZqnZQFzE54525012;     lNZqnZQFzE54525012 = lNZqnZQFzE65046421;     lNZqnZQFzE65046421 = lNZqnZQFzE47128777;     lNZqnZQFzE47128777 = lNZqnZQFzE23059003;     lNZqnZQFzE23059003 = lNZqnZQFzE62995210;     lNZqnZQFzE62995210 = lNZqnZQFzE93404746;     lNZqnZQFzE93404746 = lNZqnZQFzE1450952;     lNZqnZQFzE1450952 = lNZqnZQFzE23661058;     lNZqnZQFzE23661058 = lNZqnZQFzE78309194;     lNZqnZQFzE78309194 = lNZqnZQFzE8320013;     lNZqnZQFzE8320013 = lNZqnZQFzE79471425;     lNZqnZQFzE79471425 = lNZqnZQFzE75151941;     lNZqnZQFzE75151941 = lNZqnZQFzE40649019;     lNZqnZQFzE40649019 = lNZqnZQFzE80435928;     lNZqnZQFzE80435928 = lNZqnZQFzE10854717;     lNZqnZQFzE10854717 = lNZqnZQFzE74138894;     lNZqnZQFzE74138894 = lNZqnZQFzE7127570;     lNZqnZQFzE7127570 = lNZqnZQFzE24901247;     lNZqnZQFzE24901247 = lNZqnZQFzE38831823;     lNZqnZQFzE38831823 = lNZqnZQFzE94905375;     lNZqnZQFzE94905375 = lNZqnZQFzE51496940;     lNZqnZQFzE51496940 = lNZqnZQFzE6458505;     lNZqnZQFzE6458505 = lNZqnZQFzE20689560;     lNZqnZQFzE20689560 = lNZqnZQFzE45795442;     lNZqnZQFzE45795442 = lNZqnZQFzE38153017;     lNZqnZQFzE38153017 = lNZqnZQFzE35444428;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void yXCNJrzOsN41675619() {     long LNondsnXuB44759694 = -873164920;    long LNondsnXuB87889945 = -274391216;    long LNondsnXuB75548150 = -352501280;    long LNondsnXuB47749716 = -292618458;    long LNondsnXuB61099176 = -677598377;    long LNondsnXuB4956226 = -799267217;    long LNondsnXuB44048196 = -514558314;    long LNondsnXuB8141980 = -770718933;    long LNondsnXuB83670091 = -655469565;    long LNondsnXuB84413762 = -722799888;    long LNondsnXuB92302312 = -450123545;    long LNondsnXuB44469116 = -939346515;    long LNondsnXuB60596100 = -554033859;    long LNondsnXuB72382184 = -507025364;    long LNondsnXuB37234099 = -347574921;    long LNondsnXuB40272919 = -162241180;    long LNondsnXuB70817950 = -982409502;    long LNondsnXuB71722902 = -367598072;    long LNondsnXuB30258997 = -854996201;    long LNondsnXuB50782722 = -534362514;    long LNondsnXuB60922902 = -301384056;    long LNondsnXuB27232599 = 32231611;    long LNondsnXuB57942849 = -168932103;    long LNondsnXuB88690215 = -669364062;    long LNondsnXuB47115322 = -625089904;    long LNondsnXuB47724872 = -52729150;    long LNondsnXuB65058152 = 75386666;    long LNondsnXuB13720672 = 61988321;    long LNondsnXuB74862696 = -252873751;    long LNondsnXuB86354676 = -181003345;    long LNondsnXuB56830320 = -81920240;    long LNondsnXuB64388125 = -943245829;    long LNondsnXuB64702773 = -796424337;    long LNondsnXuB32898595 = -649594041;    long LNondsnXuB29925734 = -512831386;    long LNondsnXuB16549230 = -30916524;    long LNondsnXuB70507661 = -946378729;    long LNondsnXuB86844959 = -291369266;    long LNondsnXuB44276406 = -488731433;    long LNondsnXuB51118251 = -275267021;    long LNondsnXuB3328003 = -547661306;    long LNondsnXuB5370195 = -520840954;    long LNondsnXuB59694272 = -399332276;    long LNondsnXuB33509707 = -703595459;    long LNondsnXuB42374658 = -313491436;    long LNondsnXuB91296804 = -108030019;    long LNondsnXuB93661939 = -525819151;    long LNondsnXuB1319971 = -544658411;    long LNondsnXuB84992282 = -212501952;    long LNondsnXuB99074496 = -614336870;    long LNondsnXuB2221426 = -214585243;    long LNondsnXuB71577558 = -839401910;    long LNondsnXuB99984478 = -458836375;    long LNondsnXuB78775668 = -300448581;    long LNondsnXuB80752717 = -439550769;    long LNondsnXuB17527096 = -805396532;    long LNondsnXuB29947096 = -5459114;    long LNondsnXuB86857934 = -683137218;    long LNondsnXuB634394 = -667528554;    long LNondsnXuB13374304 = -524869228;    long LNondsnXuB39898074 = -774653884;    long LNondsnXuB30327525 = -476546636;    long LNondsnXuB33279284 = -417845182;    long LNondsnXuB97315414 = -374466221;    long LNondsnXuB27583443 = -540879649;    long LNondsnXuB27914187 = -506877717;    long LNondsnXuB79766343 = -42922179;    long LNondsnXuB27697505 = -904439819;    long LNondsnXuB42456451 = -994193979;    long LNondsnXuB20684870 = -216658397;    long LNondsnXuB69765258 = -215862452;    long LNondsnXuB83972990 = -591040237;    long LNondsnXuB27446496 = -878866640;    long LNondsnXuB79140745 = -479729181;    long LNondsnXuB47454719 = -986701209;    long LNondsnXuB55552707 = -780543102;    long LNondsnXuB67538327 = -568436113;    long LNondsnXuB24433143 = -465336644;    long LNondsnXuB46315558 = -255872627;    long LNondsnXuB55818518 = -417059886;    long LNondsnXuB54062933 = -526909999;    long LNondsnXuB63738181 = -379954923;    long LNondsnXuB28728389 = -725509728;    long LNondsnXuB75788199 = -638536882;    long LNondsnXuB84133251 = -966418103;    long LNondsnXuB85252762 = -242518330;    long LNondsnXuB64403647 = -384409454;    long LNondsnXuB85927105 = -395975756;    long LNondsnXuB52145878 = -110043273;    long LNondsnXuB12398639 = -707434854;    long LNondsnXuB86602133 = 74542589;    long LNondsnXuB83649726 = -163241512;    long LNondsnXuB86210566 = -623840712;    long LNondsnXuB30902103 = -963862206;    long LNondsnXuB11220178 = -500613137;    long LNondsnXuB73000478 = 28885330;    long LNondsnXuB72090911 = -2995772;    long LNondsnXuB62378857 = 75133944;    long LNondsnXuB5926265 = -62715810;    long LNondsnXuB14460471 = -873164920;     LNondsnXuB44759694 = LNondsnXuB87889945;     LNondsnXuB87889945 = LNondsnXuB75548150;     LNondsnXuB75548150 = LNondsnXuB47749716;     LNondsnXuB47749716 = LNondsnXuB61099176;     LNondsnXuB61099176 = LNondsnXuB4956226;     LNondsnXuB4956226 = LNondsnXuB44048196;     LNondsnXuB44048196 = LNondsnXuB8141980;     LNondsnXuB8141980 = LNondsnXuB83670091;     LNondsnXuB83670091 = LNondsnXuB84413762;     LNondsnXuB84413762 = LNondsnXuB92302312;     LNondsnXuB92302312 = LNondsnXuB44469116;     LNondsnXuB44469116 = LNondsnXuB60596100;     LNondsnXuB60596100 = LNondsnXuB72382184;     LNondsnXuB72382184 = LNondsnXuB37234099;     LNondsnXuB37234099 = LNondsnXuB40272919;     LNondsnXuB40272919 = LNondsnXuB70817950;     LNondsnXuB70817950 = LNondsnXuB71722902;     LNondsnXuB71722902 = LNondsnXuB30258997;     LNondsnXuB30258997 = LNondsnXuB50782722;     LNondsnXuB50782722 = LNondsnXuB60922902;     LNondsnXuB60922902 = LNondsnXuB27232599;     LNondsnXuB27232599 = LNondsnXuB57942849;     LNondsnXuB57942849 = LNondsnXuB88690215;     LNondsnXuB88690215 = LNondsnXuB47115322;     LNondsnXuB47115322 = LNondsnXuB47724872;     LNondsnXuB47724872 = LNondsnXuB65058152;     LNondsnXuB65058152 = LNondsnXuB13720672;     LNondsnXuB13720672 = LNondsnXuB74862696;     LNondsnXuB74862696 = LNondsnXuB86354676;     LNondsnXuB86354676 = LNondsnXuB56830320;     LNondsnXuB56830320 = LNondsnXuB64388125;     LNondsnXuB64388125 = LNondsnXuB64702773;     LNondsnXuB64702773 = LNondsnXuB32898595;     LNondsnXuB32898595 = LNondsnXuB29925734;     LNondsnXuB29925734 = LNondsnXuB16549230;     LNondsnXuB16549230 = LNondsnXuB70507661;     LNondsnXuB70507661 = LNondsnXuB86844959;     LNondsnXuB86844959 = LNondsnXuB44276406;     LNondsnXuB44276406 = LNondsnXuB51118251;     LNondsnXuB51118251 = LNondsnXuB3328003;     LNondsnXuB3328003 = LNondsnXuB5370195;     LNondsnXuB5370195 = LNondsnXuB59694272;     LNondsnXuB59694272 = LNondsnXuB33509707;     LNondsnXuB33509707 = LNondsnXuB42374658;     LNondsnXuB42374658 = LNondsnXuB91296804;     LNondsnXuB91296804 = LNondsnXuB93661939;     LNondsnXuB93661939 = LNondsnXuB1319971;     LNondsnXuB1319971 = LNondsnXuB84992282;     LNondsnXuB84992282 = LNondsnXuB99074496;     LNondsnXuB99074496 = LNondsnXuB2221426;     LNondsnXuB2221426 = LNondsnXuB71577558;     LNondsnXuB71577558 = LNondsnXuB99984478;     LNondsnXuB99984478 = LNondsnXuB78775668;     LNondsnXuB78775668 = LNondsnXuB80752717;     LNondsnXuB80752717 = LNondsnXuB17527096;     LNondsnXuB17527096 = LNondsnXuB29947096;     LNondsnXuB29947096 = LNondsnXuB86857934;     LNondsnXuB86857934 = LNondsnXuB634394;     LNondsnXuB634394 = LNondsnXuB13374304;     LNondsnXuB13374304 = LNondsnXuB39898074;     LNondsnXuB39898074 = LNondsnXuB30327525;     LNondsnXuB30327525 = LNondsnXuB33279284;     LNondsnXuB33279284 = LNondsnXuB97315414;     LNondsnXuB97315414 = LNondsnXuB27583443;     LNondsnXuB27583443 = LNondsnXuB27914187;     LNondsnXuB27914187 = LNondsnXuB79766343;     LNondsnXuB79766343 = LNondsnXuB27697505;     LNondsnXuB27697505 = LNondsnXuB42456451;     LNondsnXuB42456451 = LNondsnXuB20684870;     LNondsnXuB20684870 = LNondsnXuB69765258;     LNondsnXuB69765258 = LNondsnXuB83972990;     LNondsnXuB83972990 = LNondsnXuB27446496;     LNondsnXuB27446496 = LNondsnXuB79140745;     LNondsnXuB79140745 = LNondsnXuB47454719;     LNondsnXuB47454719 = LNondsnXuB55552707;     LNondsnXuB55552707 = LNondsnXuB67538327;     LNondsnXuB67538327 = LNondsnXuB24433143;     LNondsnXuB24433143 = LNondsnXuB46315558;     LNondsnXuB46315558 = LNondsnXuB55818518;     LNondsnXuB55818518 = LNondsnXuB54062933;     LNondsnXuB54062933 = LNondsnXuB63738181;     LNondsnXuB63738181 = LNondsnXuB28728389;     LNondsnXuB28728389 = LNondsnXuB75788199;     LNondsnXuB75788199 = LNondsnXuB84133251;     LNondsnXuB84133251 = LNondsnXuB85252762;     LNondsnXuB85252762 = LNondsnXuB64403647;     LNondsnXuB64403647 = LNondsnXuB85927105;     LNondsnXuB85927105 = LNondsnXuB52145878;     LNondsnXuB52145878 = LNondsnXuB12398639;     LNondsnXuB12398639 = LNondsnXuB86602133;     LNondsnXuB86602133 = LNondsnXuB83649726;     LNondsnXuB83649726 = LNondsnXuB86210566;     LNondsnXuB86210566 = LNondsnXuB30902103;     LNondsnXuB30902103 = LNondsnXuB11220178;     LNondsnXuB11220178 = LNondsnXuB73000478;     LNondsnXuB73000478 = LNondsnXuB72090911;     LNondsnXuB72090911 = LNondsnXuB62378857;     LNondsnXuB62378857 = LNondsnXuB5926265;     LNondsnXuB5926265 = LNondsnXuB14460471;     LNondsnXuB14460471 = LNondsnXuB44759694;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void miShEHzYLn99916500() {     long hxBZYwSZzR83416822 = -30070958;    long hxBZYwSZzR13211667 = -461877545;    long hxBZYwSZzR17044524 = -680976047;    long hxBZYwSZzR50688312 = -114684891;    long hxBZYwSZzR88552195 = -239062508;    long hxBZYwSZzR43305257 = -255987738;    long hxBZYwSZzR67870708 = -19567949;    long hxBZYwSZzR9649807 = -325038309;    long hxBZYwSZzR13353858 = -655828978;    long hxBZYwSZzR26711273 = -979307077;    long hxBZYwSZzR7275742 = -565934760;    long hxBZYwSZzR24451972 = -996430900;    long hxBZYwSZzR8403343 = -251577410;    long hxBZYwSZzR74468012 = 69795690;    long hxBZYwSZzR14962226 = -12068180;    long hxBZYwSZzR87279350 = -952576722;    long hxBZYwSZzR76628650 = -920424029;    long hxBZYwSZzR86622581 = -505560811;    long hxBZYwSZzR50992417 = -719431354;    long hxBZYwSZzR27265124 = -682728501;    long hxBZYwSZzR74135923 = -428871217;    long hxBZYwSZzR68342012 = 30628701;    long hxBZYwSZzR28938704 = -774575800;    long hxBZYwSZzR57697416 = -475044789;    long hxBZYwSZzR74496166 = -595012120;    long hxBZYwSZzR99609186 = 38137726;    long hxBZYwSZzR60777215 = -639124116;    long hxBZYwSZzR12436447 = -548991632;    long hxBZYwSZzR22515562 = -719005285;    long hxBZYwSZzR36053399 = -204124211;    long hxBZYwSZzR53169381 = -331231606;    long hxBZYwSZzR81859885 = -337941722;    long hxBZYwSZzR17069166 = -862497956;    long hxBZYwSZzR56845405 = -446305976;    long hxBZYwSZzR20909811 = -1611125;    long hxBZYwSZzR31209055 = -447681170;    long hxBZYwSZzR76125150 = -458638763;    long hxBZYwSZzR60006949 = -255114200;    long hxBZYwSZzR67282415 = -196900000;    long hxBZYwSZzR3987743 = -74275857;    long hxBZYwSZzR55173521 = -388742744;    long hxBZYwSZzR50236520 = -866516058;    long hxBZYwSZzR92727677 = -93588860;    long hxBZYwSZzR44990205 = -678979819;    long hxBZYwSZzR66388726 = -269880237;    long hxBZYwSZzR95558173 = -487272093;    long hxBZYwSZzR478161 = -463864119;    long hxBZYwSZzR82560379 = -47832860;    long hxBZYwSZzR64236363 = -26198360;    long hxBZYwSZzR80330335 = -811366579;    long hxBZYwSZzR84259179 = 17555875;    long hxBZYwSZzR8470353 = -800314154;    long hxBZYwSZzR8364828 = -10869248;    long hxBZYwSZzR3379879 = -531340054;    long hxBZYwSZzR25780578 = -20427217;    long hxBZYwSZzR15074811 = 39300341;    long hxBZYwSZzR84272962 = -687301745;    long hxBZYwSZzR59347107 = -105931258;    long hxBZYwSZzR76192146 = -519672771;    long hxBZYwSZzR88943009 = -177200235;    long hxBZYwSZzR82528042 = -616863622;    long hxBZYwSZzR55434262 = -470576317;    long hxBZYwSZzR87134244 = -606033024;    long hxBZYwSZzR77300459 = -351704767;    long hxBZYwSZzR73541891 = -548075472;    long hxBZYwSZzR25415856 = -127993039;    long hxBZYwSZzR7382806 = -33932945;    long hxBZYwSZzR51557937 = -805271434;    long hxBZYwSZzR53558201 = -928593185;    long hxBZYwSZzR83753171 = -564387011;    long hxBZYwSZzR11154200 = -393937960;    long hxBZYwSZzR16621702 = -565309829;    long hxBZYwSZzR19340167 = -208660812;    long hxBZYwSZzR47004674 = -545155498;    long hxBZYwSZzR72091603 = -193985758;    long hxBZYwSZzR23899403 = -562355160;    long hxBZYwSZzR75614334 = -875782439;    long hxBZYwSZzR83948499 = 4404018;    long hxBZYwSZzR91308690 = -105164552;    long hxBZYwSZzR78937992 = -7740028;    long hxBZYwSZzR99131026 = -497998156;    long hxBZYwSZzR78216835 = -491291256;    long hxBZYwSZzR48200083 = -422793273;    long hxBZYwSZzR42185227 = -907638706;    long hxBZYwSZzR51794219 = -121680086;    long hxBZYwSZzR44699028 = -530917452;    long hxBZYwSZzR73495057 = -227072475;    long hxBZYwSZzR13689288 = -231157903;    long hxBZYwSZzR31064828 = -325878759;    long hxBZYwSZzR5835001 = 59088534;    long hxBZYwSZzR46936092 = -760379425;    long hxBZYwSZzR16778043 = -252707505;    long hxBZYwSZzR83814802 = -735441429;    long hxBZYwSZzR78339405 = 80300235;    long hxBZYwSZzR21459700 = -457412235;    long hxBZYwSZzR99739258 = -918166427;    long hxBZYwSZzR63102276 = -160483035;    long hxBZYwSZzR15427218 = -741884093;    long hxBZYwSZzR71448313 = -30904348;    long hxBZYwSZzR40972870 = -30070958;     hxBZYwSZzR83416822 = hxBZYwSZzR13211667;     hxBZYwSZzR13211667 = hxBZYwSZzR17044524;     hxBZYwSZzR17044524 = hxBZYwSZzR50688312;     hxBZYwSZzR50688312 = hxBZYwSZzR88552195;     hxBZYwSZzR88552195 = hxBZYwSZzR43305257;     hxBZYwSZzR43305257 = hxBZYwSZzR67870708;     hxBZYwSZzR67870708 = hxBZYwSZzR9649807;     hxBZYwSZzR9649807 = hxBZYwSZzR13353858;     hxBZYwSZzR13353858 = hxBZYwSZzR26711273;     hxBZYwSZzR26711273 = hxBZYwSZzR7275742;     hxBZYwSZzR7275742 = hxBZYwSZzR24451972;     hxBZYwSZzR24451972 = hxBZYwSZzR8403343;     hxBZYwSZzR8403343 = hxBZYwSZzR74468012;     hxBZYwSZzR74468012 = hxBZYwSZzR14962226;     hxBZYwSZzR14962226 = hxBZYwSZzR87279350;     hxBZYwSZzR87279350 = hxBZYwSZzR76628650;     hxBZYwSZzR76628650 = hxBZYwSZzR86622581;     hxBZYwSZzR86622581 = hxBZYwSZzR50992417;     hxBZYwSZzR50992417 = hxBZYwSZzR27265124;     hxBZYwSZzR27265124 = hxBZYwSZzR74135923;     hxBZYwSZzR74135923 = hxBZYwSZzR68342012;     hxBZYwSZzR68342012 = hxBZYwSZzR28938704;     hxBZYwSZzR28938704 = hxBZYwSZzR57697416;     hxBZYwSZzR57697416 = hxBZYwSZzR74496166;     hxBZYwSZzR74496166 = hxBZYwSZzR99609186;     hxBZYwSZzR99609186 = hxBZYwSZzR60777215;     hxBZYwSZzR60777215 = hxBZYwSZzR12436447;     hxBZYwSZzR12436447 = hxBZYwSZzR22515562;     hxBZYwSZzR22515562 = hxBZYwSZzR36053399;     hxBZYwSZzR36053399 = hxBZYwSZzR53169381;     hxBZYwSZzR53169381 = hxBZYwSZzR81859885;     hxBZYwSZzR81859885 = hxBZYwSZzR17069166;     hxBZYwSZzR17069166 = hxBZYwSZzR56845405;     hxBZYwSZzR56845405 = hxBZYwSZzR20909811;     hxBZYwSZzR20909811 = hxBZYwSZzR31209055;     hxBZYwSZzR31209055 = hxBZYwSZzR76125150;     hxBZYwSZzR76125150 = hxBZYwSZzR60006949;     hxBZYwSZzR60006949 = hxBZYwSZzR67282415;     hxBZYwSZzR67282415 = hxBZYwSZzR3987743;     hxBZYwSZzR3987743 = hxBZYwSZzR55173521;     hxBZYwSZzR55173521 = hxBZYwSZzR50236520;     hxBZYwSZzR50236520 = hxBZYwSZzR92727677;     hxBZYwSZzR92727677 = hxBZYwSZzR44990205;     hxBZYwSZzR44990205 = hxBZYwSZzR66388726;     hxBZYwSZzR66388726 = hxBZYwSZzR95558173;     hxBZYwSZzR95558173 = hxBZYwSZzR478161;     hxBZYwSZzR478161 = hxBZYwSZzR82560379;     hxBZYwSZzR82560379 = hxBZYwSZzR64236363;     hxBZYwSZzR64236363 = hxBZYwSZzR80330335;     hxBZYwSZzR80330335 = hxBZYwSZzR84259179;     hxBZYwSZzR84259179 = hxBZYwSZzR8470353;     hxBZYwSZzR8470353 = hxBZYwSZzR8364828;     hxBZYwSZzR8364828 = hxBZYwSZzR3379879;     hxBZYwSZzR3379879 = hxBZYwSZzR25780578;     hxBZYwSZzR25780578 = hxBZYwSZzR15074811;     hxBZYwSZzR15074811 = hxBZYwSZzR84272962;     hxBZYwSZzR84272962 = hxBZYwSZzR59347107;     hxBZYwSZzR59347107 = hxBZYwSZzR76192146;     hxBZYwSZzR76192146 = hxBZYwSZzR88943009;     hxBZYwSZzR88943009 = hxBZYwSZzR82528042;     hxBZYwSZzR82528042 = hxBZYwSZzR55434262;     hxBZYwSZzR55434262 = hxBZYwSZzR87134244;     hxBZYwSZzR87134244 = hxBZYwSZzR77300459;     hxBZYwSZzR77300459 = hxBZYwSZzR73541891;     hxBZYwSZzR73541891 = hxBZYwSZzR25415856;     hxBZYwSZzR25415856 = hxBZYwSZzR7382806;     hxBZYwSZzR7382806 = hxBZYwSZzR51557937;     hxBZYwSZzR51557937 = hxBZYwSZzR53558201;     hxBZYwSZzR53558201 = hxBZYwSZzR83753171;     hxBZYwSZzR83753171 = hxBZYwSZzR11154200;     hxBZYwSZzR11154200 = hxBZYwSZzR16621702;     hxBZYwSZzR16621702 = hxBZYwSZzR19340167;     hxBZYwSZzR19340167 = hxBZYwSZzR47004674;     hxBZYwSZzR47004674 = hxBZYwSZzR72091603;     hxBZYwSZzR72091603 = hxBZYwSZzR23899403;     hxBZYwSZzR23899403 = hxBZYwSZzR75614334;     hxBZYwSZzR75614334 = hxBZYwSZzR83948499;     hxBZYwSZzR83948499 = hxBZYwSZzR91308690;     hxBZYwSZzR91308690 = hxBZYwSZzR78937992;     hxBZYwSZzR78937992 = hxBZYwSZzR99131026;     hxBZYwSZzR99131026 = hxBZYwSZzR78216835;     hxBZYwSZzR78216835 = hxBZYwSZzR48200083;     hxBZYwSZzR48200083 = hxBZYwSZzR42185227;     hxBZYwSZzR42185227 = hxBZYwSZzR51794219;     hxBZYwSZzR51794219 = hxBZYwSZzR44699028;     hxBZYwSZzR44699028 = hxBZYwSZzR73495057;     hxBZYwSZzR73495057 = hxBZYwSZzR13689288;     hxBZYwSZzR13689288 = hxBZYwSZzR31064828;     hxBZYwSZzR31064828 = hxBZYwSZzR5835001;     hxBZYwSZzR5835001 = hxBZYwSZzR46936092;     hxBZYwSZzR46936092 = hxBZYwSZzR16778043;     hxBZYwSZzR16778043 = hxBZYwSZzR83814802;     hxBZYwSZzR83814802 = hxBZYwSZzR78339405;     hxBZYwSZzR78339405 = hxBZYwSZzR21459700;     hxBZYwSZzR21459700 = hxBZYwSZzR99739258;     hxBZYwSZzR99739258 = hxBZYwSZzR63102276;     hxBZYwSZzR63102276 = hxBZYwSZzR15427218;     hxBZYwSZzR15427218 = hxBZYwSZzR71448313;     hxBZYwSZzR71448313 = hxBZYwSZzR40972870;     hxBZYwSZzR40972870 = hxBZYwSZzR83416822;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void cTXFrPEisZ57827185() {     long pmpshPwGAn76734819 = -851980438;    long pmpshPwGAn46958691 = -86333562;    long pmpshPwGAn17960332 = -337706813;    long pmpshPwGAn39981989 = -122312905;    long pmpshPwGAn81616784 = -104606557;    long pmpshPwGAn61023254 = -142426283;    long pmpshPwGAn44246588 = -121933790;    long pmpshPwGAn71118685 = -675748599;    long pmpshPwGAn48071871 = -989395691;    long pmpshPwGAn55001254 = -777808664;    long pmpshPwGAn80539860 = -247630476;    long pmpshPwGAn52907863 = -404461011;    long pmpshPwGAn88174986 = -831002790;    long pmpshPwGAn15676051 = -820333782;    long pmpshPwGAn77742317 = -720970220;    long pmpshPwGAn49488770 = 59723823;    long pmpshPwGAn26418355 = -412423815;    long pmpshPwGAn46118315 = -310016395;    long pmpshPwGAn52518168 = -251695789;    long pmpshPwGAn49308318 = -564436184;    long pmpshPwGAn41853908 = -313975156;    long pmpshPwGAn82407748 = -727426434;    long pmpshPwGAn99525993 = -408005076;    long pmpshPwGAn84482877 = -98845809;    long pmpshPwGAn69186936 = -511153935;    long pmpshPwGAn76160370 = 33501745;    long pmpshPwGAn88264620 = -507679465;    long pmpshPwGAn28997697 = -559056458;    long pmpshPwGAn97387658 = -919311379;    long pmpshPwGAn4097660 = -842571671;    long pmpshPwGAn67801291 = -742322263;    long pmpshPwGAn45599399 = -852312383;    long pmpshPwGAn51639011 = -183144974;    long pmpshPwGAn75165271 = -46456988;    long pmpshPwGAn64040007 = -548940200;    long pmpshPwGAn90367232 = 67324334;    long pmpshPwGAn44505760 = -821672038;    long pmpshPwGAn76228497 = -97525476;    long pmpshPwGAn5744477 = -674335752;    long pmpshPwGAn8970737 = 77838394;    long pmpshPwGAn75122772 = -254430441;    long pmpshPwGAn46079843 = -525931166;    long pmpshPwGAn56477906 = -169220507;    long pmpshPwGAn6233204 = -571079401;    long pmpshPwGAn66368805 = -985527234;    long pmpshPwGAn14725176 = -23259111;    long pmpshPwGAn96070299 = 52314151;    long pmpshPwGAn11844516 = -19898326;    long pmpshPwGAn67164822 = -175861096;    long pmpshPwGAn86382413 = -561390697;    long pmpshPwGAn28151104 = -928963503;    long pmpshPwGAn20198040 = -894146632;    long pmpshPwGAn73024831 = -510763569;    long pmpshPwGAn75149575 = -126198159;    long pmpshPwGAn15973277 = -240313068;    long pmpshPwGAn94327071 = -24554004;    long pmpshPwGAn47432698 = -678328486;    long pmpshPwGAn33477455 = -138861005;    long pmpshPwGAn70795053 = -611158970;    long pmpshPwGAn5456414 = -38108303;    long pmpshPwGAn72758634 = -634746818;    long pmpshPwGAn15248892 = -562877332;    long pmpshPwGAn73731027 = -756437220;    long pmpshPwGAn43974211 = -46824020;    long pmpshPwGAn87199962 = 64513599;    long pmpshPwGAn34940462 = -395318094;    long pmpshPwGAn1268852 = -121316038;    long pmpshPwGAn13009715 = -684545803;    long pmpshPwGAn51636044 = -171393583;    long pmpshPwGAn87375085 = -688294555;    long pmpshPwGAn4983010 = -118604140;    long pmpshPwGAn50189857 = -214898339;    long pmpshPwGAn40373839 = -635680644;    long pmpshPwGAn43547432 = -229534184;    long pmpshPwGAn74185546 = -210005744;    long pmpshPwGAn95774064 = -788043990;    long pmpshPwGAn25929842 = -458205927;    long pmpshPwGAn93292789 = -836925676;    long pmpshPwGAn18114072 = -113318576;    long pmpshPwGAn54461760 = -387894824;    long pmpshPwGAn80090070 = 81187593;    long pmpshPwGAn76420104 = -387781139;    long pmpshPwGAn61832874 = -283195362;    long pmpshPwGAn11005245 = -257920682;    long pmpshPwGAn75946555 = -913608169;    long pmpshPwGAn47603252 = -848175631;    long pmpshPwGAn72574567 = -241548814;    long pmpshPwGAn76489436 = 43053185;    long pmpshPwGAn59191994 = -806143920;    long pmpshPwGAn69712935 = -424386196;    long pmpshPwGAn42934534 = -254347180;    long pmpshPwGAn11028306 = -582811034;    long pmpshPwGAn5433445 = -486366507;    long pmpshPwGAn288063 = -536227449;    long pmpshPwGAn36212103 = -287414788;    long pmpshPwGAn59873880 = -691553109;    long pmpshPwGAn72348816 = -769493946;    long pmpshPwGAn12503695 = -22396488;    long pmpshPwGAn19033242 = -535593000;    long pmpshPwGAn31428344 = -851980438;     pmpshPwGAn76734819 = pmpshPwGAn46958691;     pmpshPwGAn46958691 = pmpshPwGAn17960332;     pmpshPwGAn17960332 = pmpshPwGAn39981989;     pmpshPwGAn39981989 = pmpshPwGAn81616784;     pmpshPwGAn81616784 = pmpshPwGAn61023254;     pmpshPwGAn61023254 = pmpshPwGAn44246588;     pmpshPwGAn44246588 = pmpshPwGAn71118685;     pmpshPwGAn71118685 = pmpshPwGAn48071871;     pmpshPwGAn48071871 = pmpshPwGAn55001254;     pmpshPwGAn55001254 = pmpshPwGAn80539860;     pmpshPwGAn80539860 = pmpshPwGAn52907863;     pmpshPwGAn52907863 = pmpshPwGAn88174986;     pmpshPwGAn88174986 = pmpshPwGAn15676051;     pmpshPwGAn15676051 = pmpshPwGAn77742317;     pmpshPwGAn77742317 = pmpshPwGAn49488770;     pmpshPwGAn49488770 = pmpshPwGAn26418355;     pmpshPwGAn26418355 = pmpshPwGAn46118315;     pmpshPwGAn46118315 = pmpshPwGAn52518168;     pmpshPwGAn52518168 = pmpshPwGAn49308318;     pmpshPwGAn49308318 = pmpshPwGAn41853908;     pmpshPwGAn41853908 = pmpshPwGAn82407748;     pmpshPwGAn82407748 = pmpshPwGAn99525993;     pmpshPwGAn99525993 = pmpshPwGAn84482877;     pmpshPwGAn84482877 = pmpshPwGAn69186936;     pmpshPwGAn69186936 = pmpshPwGAn76160370;     pmpshPwGAn76160370 = pmpshPwGAn88264620;     pmpshPwGAn88264620 = pmpshPwGAn28997697;     pmpshPwGAn28997697 = pmpshPwGAn97387658;     pmpshPwGAn97387658 = pmpshPwGAn4097660;     pmpshPwGAn4097660 = pmpshPwGAn67801291;     pmpshPwGAn67801291 = pmpshPwGAn45599399;     pmpshPwGAn45599399 = pmpshPwGAn51639011;     pmpshPwGAn51639011 = pmpshPwGAn75165271;     pmpshPwGAn75165271 = pmpshPwGAn64040007;     pmpshPwGAn64040007 = pmpshPwGAn90367232;     pmpshPwGAn90367232 = pmpshPwGAn44505760;     pmpshPwGAn44505760 = pmpshPwGAn76228497;     pmpshPwGAn76228497 = pmpshPwGAn5744477;     pmpshPwGAn5744477 = pmpshPwGAn8970737;     pmpshPwGAn8970737 = pmpshPwGAn75122772;     pmpshPwGAn75122772 = pmpshPwGAn46079843;     pmpshPwGAn46079843 = pmpshPwGAn56477906;     pmpshPwGAn56477906 = pmpshPwGAn6233204;     pmpshPwGAn6233204 = pmpshPwGAn66368805;     pmpshPwGAn66368805 = pmpshPwGAn14725176;     pmpshPwGAn14725176 = pmpshPwGAn96070299;     pmpshPwGAn96070299 = pmpshPwGAn11844516;     pmpshPwGAn11844516 = pmpshPwGAn67164822;     pmpshPwGAn67164822 = pmpshPwGAn86382413;     pmpshPwGAn86382413 = pmpshPwGAn28151104;     pmpshPwGAn28151104 = pmpshPwGAn20198040;     pmpshPwGAn20198040 = pmpshPwGAn73024831;     pmpshPwGAn73024831 = pmpshPwGAn75149575;     pmpshPwGAn75149575 = pmpshPwGAn15973277;     pmpshPwGAn15973277 = pmpshPwGAn94327071;     pmpshPwGAn94327071 = pmpshPwGAn47432698;     pmpshPwGAn47432698 = pmpshPwGAn33477455;     pmpshPwGAn33477455 = pmpshPwGAn70795053;     pmpshPwGAn70795053 = pmpshPwGAn5456414;     pmpshPwGAn5456414 = pmpshPwGAn72758634;     pmpshPwGAn72758634 = pmpshPwGAn15248892;     pmpshPwGAn15248892 = pmpshPwGAn73731027;     pmpshPwGAn73731027 = pmpshPwGAn43974211;     pmpshPwGAn43974211 = pmpshPwGAn87199962;     pmpshPwGAn87199962 = pmpshPwGAn34940462;     pmpshPwGAn34940462 = pmpshPwGAn1268852;     pmpshPwGAn1268852 = pmpshPwGAn13009715;     pmpshPwGAn13009715 = pmpshPwGAn51636044;     pmpshPwGAn51636044 = pmpshPwGAn87375085;     pmpshPwGAn87375085 = pmpshPwGAn4983010;     pmpshPwGAn4983010 = pmpshPwGAn50189857;     pmpshPwGAn50189857 = pmpshPwGAn40373839;     pmpshPwGAn40373839 = pmpshPwGAn43547432;     pmpshPwGAn43547432 = pmpshPwGAn74185546;     pmpshPwGAn74185546 = pmpshPwGAn95774064;     pmpshPwGAn95774064 = pmpshPwGAn25929842;     pmpshPwGAn25929842 = pmpshPwGAn93292789;     pmpshPwGAn93292789 = pmpshPwGAn18114072;     pmpshPwGAn18114072 = pmpshPwGAn54461760;     pmpshPwGAn54461760 = pmpshPwGAn80090070;     pmpshPwGAn80090070 = pmpshPwGAn76420104;     pmpshPwGAn76420104 = pmpshPwGAn61832874;     pmpshPwGAn61832874 = pmpshPwGAn11005245;     pmpshPwGAn11005245 = pmpshPwGAn75946555;     pmpshPwGAn75946555 = pmpshPwGAn47603252;     pmpshPwGAn47603252 = pmpshPwGAn72574567;     pmpshPwGAn72574567 = pmpshPwGAn76489436;     pmpshPwGAn76489436 = pmpshPwGAn59191994;     pmpshPwGAn59191994 = pmpshPwGAn69712935;     pmpshPwGAn69712935 = pmpshPwGAn42934534;     pmpshPwGAn42934534 = pmpshPwGAn11028306;     pmpshPwGAn11028306 = pmpshPwGAn5433445;     pmpshPwGAn5433445 = pmpshPwGAn288063;     pmpshPwGAn288063 = pmpshPwGAn36212103;     pmpshPwGAn36212103 = pmpshPwGAn59873880;     pmpshPwGAn59873880 = pmpshPwGAn72348816;     pmpshPwGAn72348816 = pmpshPwGAn12503695;     pmpshPwGAn12503695 = pmpshPwGAn19033242;     pmpshPwGAn19033242 = pmpshPwGAn31428344;     pmpshPwGAn31428344 = pmpshPwGAn76734819;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void CvjNTdapQR94141962() {     long TpJRTJXPLa53782806 = 48612157;    long TpJRTJXPLa39731494 = -571846306;    long TpJRTJXPLa6149045 = 94383343;    long TpJRTJXPLa41343758 = -314282017;    long TpJRTJXPLa45402607 = -999743476;    long TpJRTJXPLa55275335 = -432752685;    long TpJRTJXPLa88955694 = -541277302;    long TpJRTJXPLa6713598 = -842736159;    long TpJRTJXPLa50269249 = -106799031;    long TpJRTJXPLa32757311 = -62544395;    long TpJRTJXPLa76372409 = -910139377;    long TpJRTJXPLa57283848 = -6152031;    long TpJRTJXPLa87147567 = -131601940;    long TpJRTJXPLa97413013 = -614993066;    long TpJRTJXPLa99529559 = -292856712;    long TpJRTJXPLa8947671 = -719307633;    long TpJRTJXPLa20220711 = -243688893;    long TpJRTJXPLa96690785 = -428251931;    long TpJRTJXPLa74592129 = -54627196;    long TpJRTJXPLa73385394 = -550412333;    long TpJRTJXPLa97241978 = -63079241;    long TpJRTJXPLa19179290 = -888644972;    long TpJRTJXPLa2410752 = -711871247;    long TpJRTJXPLa68760924 = -480958960;    long TpJRTJXPLa72037109 = -78932658;    long TpJRTJXPLa89130235 = -505895335;    long TpJRTJXPLa67122383 = -519310524;    long TpJRTJXPLa52754622 = -846595784;    long TpJRTJXPLa31499450 = -997743059;    long TpJRTJXPLa31600379 = -278194132;    long TpJRTJXPLa30384947 = -985417706;    long TpJRTJXPLa49408342 = -424058504;    long TpJRTJXPLa14881136 = 32938133;    long TpJRTJXPLa28927014 = -174022915;    long TpJRTJXPLa92415490 = -571245957;    long TpJRTJXPLa76558453 = -416300892;    long TpJRTJXPLa708593 = -305393232;    long TpJRTJXPLa72914158 = -943817290;    long TpJRTJXPLa69508660 = -744648969;    long TpJRTJXPLa81269953 = -107262548;    long TpJRTJXPLa71983876 = -782214367;    long TpJRTJXPLa85565231 = -697579862;    long TpJRTJXPLa92744920 = -438126155;    long TpJRTJXPLa55439824 = -485158272;    long TpJRTJXPLa10318083 = -453928908;    long TpJRTJXPLa37731091 = -462233151;    long TpJRTJXPLa97143616 = -71872863;    long TpJRTJXPLa4486919 = -245324333;    long TpJRTJXPLa85309104 = -461544122;    long TpJRTJXPLa8262399 = -256405364;    long TpJRTJXPLa98874490 = -262700274;    long TpJRTJXPLa68884189 = -357886149;    long TpJRTJXPLa7618890 = -30536248;    long TpJRTJXPLa82601606 = -865845602;    long TpJRTJXPLa96899983 = 79973413;    long TpJRTJXPLa34603516 = -62742872;    long TpJRTJXPLa37320743 = -859975059;    long TpJRTJXPLa37388120 = -424657697;    long TpJRTJXPLa69306649 = -135349359;    long TpJRTJXPLa56272372 = -393848142;    long TpJRTJXPLa88152952 = -913442162;    long TpJRTJXPLa36201073 = -694681518;    long TpJRTJXPLa75214147 = -844993101;    long TpJRTJXPLa18668871 = -828604899;    long TpJRTJXPLa2372364 = -77126690;    long TpJRTJXPLa26964067 = -386080873;    long TpJRTJXPLa42402713 = 60909835;    long TpJRTJXPLa58220553 = -957579025;    long TpJRTJXPLa4997524 = 56252891;    long TpJRTJXPLa22971106 = -876555820;    long TpJRTJXPLa8239079 = -313914402;    long TpJRTJXPLa47306553 = -299871603;    long TpJRTJXPLa27182126 = -683602963;    long TpJRTJXPLa93322176 = -947364648;    long TpJRTJXPLa1401519 = -768197967;    long TpJRTJXPLa11676747 = -365499380;    long TpJRTJXPLa26434369 = -350518818;    long TpJRTJXPLa46970927 = -126712976;    long TpJRTJXPLa58442842 = 72969947;    long TpJRTJXPLa34306018 = -616699508;    long TpJRTJXPLa91986618 = -334022473;    long TpJRTJXPLa62635465 = -173986191;    long TpJRTJXPLa67445517 = -285051662;    long TpJRTJXPLa23237051 = -641337695;    long TpJRTJXPLa32725888 = 84506142;    long TpJRTJXPLa61500757 = -527531557;    long TpJRTJXPLa41789453 = -293522256;    long TpJRTJXPLa32279529 = -101216266;    long TpJRTJXPLa32027031 = -153996329;    long TpJRTJXPLa57811974 = -408503086;    long TpJRTJXPLa39237711 = -556325834;    long TpJRTJXPLa63320472 = -880735535;    long TpJRTJXPLa3607510 = -708467931;    long TpJRTJXPLa13236288 = -250800827;    long TpJRTJXPLa93117000 = -193820387;    long TpJRTJXPLa35782803 = 12467151;    long TpJRTJXPLa10351084 = -852586762;    long TpJRTJXPLa74076050 = -609521256;    long TpJRTJXPLa53067461 = -308031583;    long TpJRTJXPLa83354015 = 48612157;     TpJRTJXPLa53782806 = TpJRTJXPLa39731494;     TpJRTJXPLa39731494 = TpJRTJXPLa6149045;     TpJRTJXPLa6149045 = TpJRTJXPLa41343758;     TpJRTJXPLa41343758 = TpJRTJXPLa45402607;     TpJRTJXPLa45402607 = TpJRTJXPLa55275335;     TpJRTJXPLa55275335 = TpJRTJXPLa88955694;     TpJRTJXPLa88955694 = TpJRTJXPLa6713598;     TpJRTJXPLa6713598 = TpJRTJXPLa50269249;     TpJRTJXPLa50269249 = TpJRTJXPLa32757311;     TpJRTJXPLa32757311 = TpJRTJXPLa76372409;     TpJRTJXPLa76372409 = TpJRTJXPLa57283848;     TpJRTJXPLa57283848 = TpJRTJXPLa87147567;     TpJRTJXPLa87147567 = TpJRTJXPLa97413013;     TpJRTJXPLa97413013 = TpJRTJXPLa99529559;     TpJRTJXPLa99529559 = TpJRTJXPLa8947671;     TpJRTJXPLa8947671 = TpJRTJXPLa20220711;     TpJRTJXPLa20220711 = TpJRTJXPLa96690785;     TpJRTJXPLa96690785 = TpJRTJXPLa74592129;     TpJRTJXPLa74592129 = TpJRTJXPLa73385394;     TpJRTJXPLa73385394 = TpJRTJXPLa97241978;     TpJRTJXPLa97241978 = TpJRTJXPLa19179290;     TpJRTJXPLa19179290 = TpJRTJXPLa2410752;     TpJRTJXPLa2410752 = TpJRTJXPLa68760924;     TpJRTJXPLa68760924 = TpJRTJXPLa72037109;     TpJRTJXPLa72037109 = TpJRTJXPLa89130235;     TpJRTJXPLa89130235 = TpJRTJXPLa67122383;     TpJRTJXPLa67122383 = TpJRTJXPLa52754622;     TpJRTJXPLa52754622 = TpJRTJXPLa31499450;     TpJRTJXPLa31499450 = TpJRTJXPLa31600379;     TpJRTJXPLa31600379 = TpJRTJXPLa30384947;     TpJRTJXPLa30384947 = TpJRTJXPLa49408342;     TpJRTJXPLa49408342 = TpJRTJXPLa14881136;     TpJRTJXPLa14881136 = TpJRTJXPLa28927014;     TpJRTJXPLa28927014 = TpJRTJXPLa92415490;     TpJRTJXPLa92415490 = TpJRTJXPLa76558453;     TpJRTJXPLa76558453 = TpJRTJXPLa708593;     TpJRTJXPLa708593 = TpJRTJXPLa72914158;     TpJRTJXPLa72914158 = TpJRTJXPLa69508660;     TpJRTJXPLa69508660 = TpJRTJXPLa81269953;     TpJRTJXPLa81269953 = TpJRTJXPLa71983876;     TpJRTJXPLa71983876 = TpJRTJXPLa85565231;     TpJRTJXPLa85565231 = TpJRTJXPLa92744920;     TpJRTJXPLa92744920 = TpJRTJXPLa55439824;     TpJRTJXPLa55439824 = TpJRTJXPLa10318083;     TpJRTJXPLa10318083 = TpJRTJXPLa37731091;     TpJRTJXPLa37731091 = TpJRTJXPLa97143616;     TpJRTJXPLa97143616 = TpJRTJXPLa4486919;     TpJRTJXPLa4486919 = TpJRTJXPLa85309104;     TpJRTJXPLa85309104 = TpJRTJXPLa8262399;     TpJRTJXPLa8262399 = TpJRTJXPLa98874490;     TpJRTJXPLa98874490 = TpJRTJXPLa68884189;     TpJRTJXPLa68884189 = TpJRTJXPLa7618890;     TpJRTJXPLa7618890 = TpJRTJXPLa82601606;     TpJRTJXPLa82601606 = TpJRTJXPLa96899983;     TpJRTJXPLa96899983 = TpJRTJXPLa34603516;     TpJRTJXPLa34603516 = TpJRTJXPLa37320743;     TpJRTJXPLa37320743 = TpJRTJXPLa37388120;     TpJRTJXPLa37388120 = TpJRTJXPLa69306649;     TpJRTJXPLa69306649 = TpJRTJXPLa56272372;     TpJRTJXPLa56272372 = TpJRTJXPLa88152952;     TpJRTJXPLa88152952 = TpJRTJXPLa36201073;     TpJRTJXPLa36201073 = TpJRTJXPLa75214147;     TpJRTJXPLa75214147 = TpJRTJXPLa18668871;     TpJRTJXPLa18668871 = TpJRTJXPLa2372364;     TpJRTJXPLa2372364 = TpJRTJXPLa26964067;     TpJRTJXPLa26964067 = TpJRTJXPLa42402713;     TpJRTJXPLa42402713 = TpJRTJXPLa58220553;     TpJRTJXPLa58220553 = TpJRTJXPLa4997524;     TpJRTJXPLa4997524 = TpJRTJXPLa22971106;     TpJRTJXPLa22971106 = TpJRTJXPLa8239079;     TpJRTJXPLa8239079 = TpJRTJXPLa47306553;     TpJRTJXPLa47306553 = TpJRTJXPLa27182126;     TpJRTJXPLa27182126 = TpJRTJXPLa93322176;     TpJRTJXPLa93322176 = TpJRTJXPLa1401519;     TpJRTJXPLa1401519 = TpJRTJXPLa11676747;     TpJRTJXPLa11676747 = TpJRTJXPLa26434369;     TpJRTJXPLa26434369 = TpJRTJXPLa46970927;     TpJRTJXPLa46970927 = TpJRTJXPLa58442842;     TpJRTJXPLa58442842 = TpJRTJXPLa34306018;     TpJRTJXPLa34306018 = TpJRTJXPLa91986618;     TpJRTJXPLa91986618 = TpJRTJXPLa62635465;     TpJRTJXPLa62635465 = TpJRTJXPLa67445517;     TpJRTJXPLa67445517 = TpJRTJXPLa23237051;     TpJRTJXPLa23237051 = TpJRTJXPLa32725888;     TpJRTJXPLa32725888 = TpJRTJXPLa61500757;     TpJRTJXPLa61500757 = TpJRTJXPLa41789453;     TpJRTJXPLa41789453 = TpJRTJXPLa32279529;     TpJRTJXPLa32279529 = TpJRTJXPLa32027031;     TpJRTJXPLa32027031 = TpJRTJXPLa57811974;     TpJRTJXPLa57811974 = TpJRTJXPLa39237711;     TpJRTJXPLa39237711 = TpJRTJXPLa63320472;     TpJRTJXPLa63320472 = TpJRTJXPLa3607510;     TpJRTJXPLa3607510 = TpJRTJXPLa13236288;     TpJRTJXPLa13236288 = TpJRTJXPLa93117000;     TpJRTJXPLa93117000 = TpJRTJXPLa35782803;     TpJRTJXPLa35782803 = TpJRTJXPLa10351084;     TpJRTJXPLa10351084 = TpJRTJXPLa74076050;     TpJRTJXPLa74076050 = TpJRTJXPLa53067461;     TpJRTJXPLa53067461 = TpJRTJXPLa83354015;     TpJRTJXPLa83354015 = TpJRTJXPLa53782806;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void zlxOZaQQWe75830063() {     float uqEWjElhKT32282176 = -427086801;    float uqEWjElhKT71381511 = -261935485;    float uqEWjElhKT76945416 = -538949883;    float uqEWjElhKT72235123 = -101843802;    float uqEWjElhKT19102426 = -812716840;    float uqEWjElhKT87679729 = -332431907;    float uqEWjElhKT68010901 = -555631935;    float uqEWjElhKT34017885 = -828339704;    float uqEWjElhKT24129999 = -369262288;    float uqEWjElhKT63230189 = -136866359;    float uqEWjElhKT89265013 = -203805418;    float uqEWjElhKT55728598 = 47087702;    float uqEWjElhKT14254552 = -381454204;    float uqEWjElhKT88554607 = -283805179;    float uqEWjElhKT15886345 = -582831231;    float uqEWjElhKT75632387 = 51428543;    float uqEWjElhKT5769636 = -452224753;    float uqEWjElhKT41970755 = -18320981;    float uqEWjElhKT18024805 = -508985118;    float uqEWjElhKT36507165 = 31031956;    float uqEWjElhKT46430347 = -242903786;    float uqEWjElhKT3237731 = -858834914;    float uqEWjElhKT68953664 = -153687708;    float uqEWjElhKT61990667 = -627947197;    float uqEWjElhKT97609023 = -118708206;    float uqEWjElhKT89585462 = -209599329;    float uqEWjElhKT90337767 = -403159004;    float uqEWjElhKT98325840 = -607704432;    float uqEWjElhKT64033101 = -123935619;    float uqEWjElhKT91440597 = -872534396;    float uqEWjElhKT91977810 = -720374722;    float uqEWjElhKT93103048 = -529442996;    float uqEWjElhKT19720072 = -815476894;    float uqEWjElhKT83358393 = -432202830;    float uqEWjElhKT41888036 = -368220928;    float uqEWjElhKT19098378 = -730546686;    float uqEWjElhKT9366265 = -341717087;    float uqEWjElhKT63068731 = -579395781;    float uqEWjElhKT67912242 = -299129559;    float uqEWjElhKT67862275 = -698999565;    float uqEWjElhKT97302221 = -591623851;    float uqEWjElhKT95497325 = -870974893;    float uqEWjElhKT46574611 = -275809013;    float uqEWjElhKT61715607 = -169588192;    float uqEWjElhKT3267354 = -554915091;    float uqEWjElhKT88091544 = -763711865;    float uqEWjElhKT97992932 = -580019120;    float uqEWjElhKT88580219 = -743325873;    float uqEWjElhKT78438530 = -994820705;    float uqEWjElhKT83291864 = -175705096;    float uqEWjElhKT3388913 = -143986478;    float uqEWjElhKT14455609 = -601041062;    float uqEWjElhKT65768011 = -238277572;    float uqEWjElhKT96544735 = 64063556;    float uqEWjElhKT29399716 = -578936943;    float uqEWjElhKT29044446 = -568251888;    float uqEWjElhKT2427848 = -8247778;    float uqEWjElhKT14954749 = -911002686;    float uqEWjElhKT74626099 = -983135597;    float uqEWjElhKT29516964 = -503117512;    float uqEWjElhKT97341961 = -929272903;    float uqEWjElhKT69685060 = -947927503;    float uqEWjElhKT69984784 = -604404086;    float uqEWjElhKT32689401 = -496727892;    float uqEWjElhKT71252379 = -416491637;    float uqEWjElhKT96161965 = -674362423;    float uqEWjElhKT36008526 = -137435405;    float uqEWjElhKT30896159 = -949251375;    float uqEWjElhKT46666572 = -915584251;    float uqEWjElhKT96787966 = -852284545;    float uqEWjElhKT66266122 = -606854371;    float uqEWjElhKT42700904 = -872828973;    float uqEWjElhKT74058512 = -719191422;    float uqEWjElhKT50162530 = -809985553;    float uqEWjElhKT39204943 = -377344194;    float uqEWjElhKT50933021 = -371928893;    float uqEWjElhKT56663120 = -483025901;    float uqEWjElhKT7238057 = -984099516;    float uqEWjElhKT58723313 = 26967894;    float uqEWjElhKT9517480 = -354996341;    float uqEWjElhKT91592530 = -629580209;    float uqEWjElhKT1757549 = -659833132;    float uqEWjElhKT19887311 = -612883728;    float uqEWjElhKT80741236 = -948230523;    float uqEWjElhKT88051684 = -628547919;    float uqEWjElhKT77522201 = -19333660;    float uqEWjElhKT27335037 = -191165424;    float uqEWjElhKT23175337 = -779540451;    float uqEWjElhKT53958677 = -853265887;    float uqEWjElhKT12843591 = -799969041;    float uqEWjElhKT16670531 = -622298909;    float uqEWjElhKT94411516 = -430714401;    float uqEWjElhKT88442631 = -596260185;    float uqEWjElhKT38395279 = -796012048;    float uqEWjElhKT70520313 = -769726662;    float uqEWjElhKT27617161 = -643696348;    float uqEWjElhKT25512542 = -166570807;    float uqEWjElhKT13885210 = -779081122;    float uqEWjElhKT90463228 = -753096555;    float uqEWjElhKT7105389 = -427086801;     uqEWjElhKT32282176 = uqEWjElhKT71381511;     uqEWjElhKT71381511 = uqEWjElhKT76945416;     uqEWjElhKT76945416 = uqEWjElhKT72235123;     uqEWjElhKT72235123 = uqEWjElhKT19102426;     uqEWjElhKT19102426 = uqEWjElhKT87679729;     uqEWjElhKT87679729 = uqEWjElhKT68010901;     uqEWjElhKT68010901 = uqEWjElhKT34017885;     uqEWjElhKT34017885 = uqEWjElhKT24129999;     uqEWjElhKT24129999 = uqEWjElhKT63230189;     uqEWjElhKT63230189 = uqEWjElhKT89265013;     uqEWjElhKT89265013 = uqEWjElhKT55728598;     uqEWjElhKT55728598 = uqEWjElhKT14254552;     uqEWjElhKT14254552 = uqEWjElhKT88554607;     uqEWjElhKT88554607 = uqEWjElhKT15886345;     uqEWjElhKT15886345 = uqEWjElhKT75632387;     uqEWjElhKT75632387 = uqEWjElhKT5769636;     uqEWjElhKT5769636 = uqEWjElhKT41970755;     uqEWjElhKT41970755 = uqEWjElhKT18024805;     uqEWjElhKT18024805 = uqEWjElhKT36507165;     uqEWjElhKT36507165 = uqEWjElhKT46430347;     uqEWjElhKT46430347 = uqEWjElhKT3237731;     uqEWjElhKT3237731 = uqEWjElhKT68953664;     uqEWjElhKT68953664 = uqEWjElhKT61990667;     uqEWjElhKT61990667 = uqEWjElhKT97609023;     uqEWjElhKT97609023 = uqEWjElhKT89585462;     uqEWjElhKT89585462 = uqEWjElhKT90337767;     uqEWjElhKT90337767 = uqEWjElhKT98325840;     uqEWjElhKT98325840 = uqEWjElhKT64033101;     uqEWjElhKT64033101 = uqEWjElhKT91440597;     uqEWjElhKT91440597 = uqEWjElhKT91977810;     uqEWjElhKT91977810 = uqEWjElhKT93103048;     uqEWjElhKT93103048 = uqEWjElhKT19720072;     uqEWjElhKT19720072 = uqEWjElhKT83358393;     uqEWjElhKT83358393 = uqEWjElhKT41888036;     uqEWjElhKT41888036 = uqEWjElhKT19098378;     uqEWjElhKT19098378 = uqEWjElhKT9366265;     uqEWjElhKT9366265 = uqEWjElhKT63068731;     uqEWjElhKT63068731 = uqEWjElhKT67912242;     uqEWjElhKT67912242 = uqEWjElhKT67862275;     uqEWjElhKT67862275 = uqEWjElhKT97302221;     uqEWjElhKT97302221 = uqEWjElhKT95497325;     uqEWjElhKT95497325 = uqEWjElhKT46574611;     uqEWjElhKT46574611 = uqEWjElhKT61715607;     uqEWjElhKT61715607 = uqEWjElhKT3267354;     uqEWjElhKT3267354 = uqEWjElhKT88091544;     uqEWjElhKT88091544 = uqEWjElhKT97992932;     uqEWjElhKT97992932 = uqEWjElhKT88580219;     uqEWjElhKT88580219 = uqEWjElhKT78438530;     uqEWjElhKT78438530 = uqEWjElhKT83291864;     uqEWjElhKT83291864 = uqEWjElhKT3388913;     uqEWjElhKT3388913 = uqEWjElhKT14455609;     uqEWjElhKT14455609 = uqEWjElhKT65768011;     uqEWjElhKT65768011 = uqEWjElhKT96544735;     uqEWjElhKT96544735 = uqEWjElhKT29399716;     uqEWjElhKT29399716 = uqEWjElhKT29044446;     uqEWjElhKT29044446 = uqEWjElhKT2427848;     uqEWjElhKT2427848 = uqEWjElhKT14954749;     uqEWjElhKT14954749 = uqEWjElhKT74626099;     uqEWjElhKT74626099 = uqEWjElhKT29516964;     uqEWjElhKT29516964 = uqEWjElhKT97341961;     uqEWjElhKT97341961 = uqEWjElhKT69685060;     uqEWjElhKT69685060 = uqEWjElhKT69984784;     uqEWjElhKT69984784 = uqEWjElhKT32689401;     uqEWjElhKT32689401 = uqEWjElhKT71252379;     uqEWjElhKT71252379 = uqEWjElhKT96161965;     uqEWjElhKT96161965 = uqEWjElhKT36008526;     uqEWjElhKT36008526 = uqEWjElhKT30896159;     uqEWjElhKT30896159 = uqEWjElhKT46666572;     uqEWjElhKT46666572 = uqEWjElhKT96787966;     uqEWjElhKT96787966 = uqEWjElhKT66266122;     uqEWjElhKT66266122 = uqEWjElhKT42700904;     uqEWjElhKT42700904 = uqEWjElhKT74058512;     uqEWjElhKT74058512 = uqEWjElhKT50162530;     uqEWjElhKT50162530 = uqEWjElhKT39204943;     uqEWjElhKT39204943 = uqEWjElhKT50933021;     uqEWjElhKT50933021 = uqEWjElhKT56663120;     uqEWjElhKT56663120 = uqEWjElhKT7238057;     uqEWjElhKT7238057 = uqEWjElhKT58723313;     uqEWjElhKT58723313 = uqEWjElhKT9517480;     uqEWjElhKT9517480 = uqEWjElhKT91592530;     uqEWjElhKT91592530 = uqEWjElhKT1757549;     uqEWjElhKT1757549 = uqEWjElhKT19887311;     uqEWjElhKT19887311 = uqEWjElhKT80741236;     uqEWjElhKT80741236 = uqEWjElhKT88051684;     uqEWjElhKT88051684 = uqEWjElhKT77522201;     uqEWjElhKT77522201 = uqEWjElhKT27335037;     uqEWjElhKT27335037 = uqEWjElhKT23175337;     uqEWjElhKT23175337 = uqEWjElhKT53958677;     uqEWjElhKT53958677 = uqEWjElhKT12843591;     uqEWjElhKT12843591 = uqEWjElhKT16670531;     uqEWjElhKT16670531 = uqEWjElhKT94411516;     uqEWjElhKT94411516 = uqEWjElhKT88442631;     uqEWjElhKT88442631 = uqEWjElhKT38395279;     uqEWjElhKT38395279 = uqEWjElhKT70520313;     uqEWjElhKT70520313 = uqEWjElhKT27617161;     uqEWjElhKT27617161 = uqEWjElhKT25512542;     uqEWjElhKT25512542 = uqEWjElhKT13885210;     uqEWjElhKT13885210 = uqEWjElhKT90463228;     uqEWjElhKT90463228 = uqEWjElhKT7105389;     uqEWjElhKT7105389 = uqEWjElhKT32282176;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void mOfYdyhpLV34070945() {     float MwNkPqKmZy70939304 = -683992839;    float MwNkPqKmZy96703232 = -449421814;    float MwNkPqKmZy18441790 = -867424650;    float MwNkPqKmZy75173719 = 76089765;    float MwNkPqKmZy46555446 = -374180971;    float MwNkPqKmZy26028761 = -889152427;    float MwNkPqKmZy91833413 = -60641570;    float MwNkPqKmZy35525711 = -382659079;    float MwNkPqKmZy53813765 = -369621700;    float MwNkPqKmZy5527700 = -393373547;    float MwNkPqKmZy4238443 = -319616633;    float MwNkPqKmZy35711453 = -9996683;    float MwNkPqKmZy62061794 = -78997755;    float MwNkPqKmZy90640435 = -806984125;    float MwNkPqKmZy93614472 = -247324491;    float MwNkPqKmZy22638819 = -738906999;    float MwNkPqKmZy11580336 = -390239280;    float MwNkPqKmZy56870434 = -156283719;    float MwNkPqKmZy38758225 = -373420271;    float MwNkPqKmZy12989567 = -117334032;    float MwNkPqKmZy59643368 = -370390947;    float MwNkPqKmZy44347144 = -860437824;    float MwNkPqKmZy39949518 = -759331405;    float MwNkPqKmZy30997867 = -433627924;    float MwNkPqKmZy24989867 = -88630422;    float MwNkPqKmZy41469776 = -118732453;    float MwNkPqKmZy86056830 = -17669787;    float MwNkPqKmZy97041615 = -118684386;    float MwNkPqKmZy11685967 = -590067152;    float MwNkPqKmZy41139320 = -895655262;    float MwNkPqKmZy88316871 = -969686088;    float MwNkPqKmZy10574809 = 75861111;    float MwNkPqKmZy72086464 = -881550514;    float MwNkPqKmZy7305204 = -228914765;    float MwNkPqKmZy32872113 = -957000668;    float MwNkPqKmZy33758203 = -47311332;    float MwNkPqKmZy14983755 = -953977120;    float MwNkPqKmZy36230720 = -543140715;    float MwNkPqKmZy90918251 = -7298126;    float MwNkPqKmZy20731767 = -498008401;    float MwNkPqKmZy49147739 = -432705289;    float MwNkPqKmZy40363651 = -116649997;    float MwNkPqKmZy79608017 = 29934403;    float MwNkPqKmZy73196105 = -144972552;    float MwNkPqKmZy27281422 = -511303892;    float MwNkPqKmZy92352913 = -42953940;    float MwNkPqKmZy4809154 = -518064088;    float MwNkPqKmZy69820627 = -246500322;    float MwNkPqKmZy57682611 = -808517113;    float MwNkPqKmZy64547703 = -372734806;    float MwNkPqKmZy85426667 = 88154639;    float MwNkPqKmZy51348404 = -561953306;    float MwNkPqKmZy74148360 = -890310445;    float MwNkPqKmZy21148946 = -166827916;    float MwNkPqKmZy74427576 = -159813391;    float MwNkPqKmZy26592161 = -823555015;    float MwNkPqKmZy56753714 = -690090409;    float MwNkPqKmZy87443922 = -333796726;    float MwNkPqKmZy50183852 = -835279813;    float MwNkPqKmZy5085670 = -155448519;    float MwNkPqKmZy39971930 = -771482641;    float MwNkPqKmZy94791797 = -941957185;    float MwNkPqKmZy23839744 = -792591927;    float MwNkPqKmZy12674446 = -473966439;    float MwNkPqKmZy17210829 = -423687460;    float MwNkPqKmZy93663634 = -295477745;    float MwNkPqKmZy63624989 = -128446170;    float MwNkPqKmZy54756591 = -850082991;    float MwNkPqKmZy57768323 = -849983458;    float MwNkPqKmZy59856269 = -100013160;    float MwNkPqKmZy7655065 = -784929879;    float MwNkPqKmZy75349615 = -847098565;    float MwNkPqKmZy65952183 = -48985594;    float MwNkPqKmZy18026459 = -875411871;    float MwNkPqKmZy63841827 = -684628744;    float MwNkPqKmZy19279717 = -153740951;    float MwNkPqKmZy64739127 = -790372228;    float MwNkPqKmZy66753413 = -514358854;    float MwNkPqKmZy3716446 = -922324032;    float MwNkPqKmZy32636954 = 54323518;    float MwNkPqKmZy36660623 = -600668365;    float MwNkPqKmZy16236203 = -771169465;    float MwNkPqKmZy39359005 = -310167273;    float MwNkPqKmZy47138264 = -117332347;    float MwNkPqKmZy55712652 = -883809902;    float MwNkPqKmZy36968468 = -307732782;    float MwNkPqKmZy36426448 = -33828445;    float MwNkPqKmZy50937519 = -614722598;    float MwNkPqKmZy32877628 = 30898626;    float MwNkPqKmZy6279953 = -33445653;    float MwNkPqKmZy77004489 = -357220923;    float MwNkPqKmZy27539833 = -520180394;    float MwNkPqKmZy86046868 = -707860902;    float MwNkPqKmZy85832581 = -851849608;    float MwNkPqKmZy80759836 = -726525760;    float MwNkPqKmZy54355942 = -490748105;    float MwNkPqKmZy16523907 = -324058070;    float MwNkPqKmZy66933571 = -496099158;    float MwNkPqKmZy55985277 = -721285093;    float MwNkPqKmZy33617788 = -683992839;     MwNkPqKmZy70939304 = MwNkPqKmZy96703232;     MwNkPqKmZy96703232 = MwNkPqKmZy18441790;     MwNkPqKmZy18441790 = MwNkPqKmZy75173719;     MwNkPqKmZy75173719 = MwNkPqKmZy46555446;     MwNkPqKmZy46555446 = MwNkPqKmZy26028761;     MwNkPqKmZy26028761 = MwNkPqKmZy91833413;     MwNkPqKmZy91833413 = MwNkPqKmZy35525711;     MwNkPqKmZy35525711 = MwNkPqKmZy53813765;     MwNkPqKmZy53813765 = MwNkPqKmZy5527700;     MwNkPqKmZy5527700 = MwNkPqKmZy4238443;     MwNkPqKmZy4238443 = MwNkPqKmZy35711453;     MwNkPqKmZy35711453 = MwNkPqKmZy62061794;     MwNkPqKmZy62061794 = MwNkPqKmZy90640435;     MwNkPqKmZy90640435 = MwNkPqKmZy93614472;     MwNkPqKmZy93614472 = MwNkPqKmZy22638819;     MwNkPqKmZy22638819 = MwNkPqKmZy11580336;     MwNkPqKmZy11580336 = MwNkPqKmZy56870434;     MwNkPqKmZy56870434 = MwNkPqKmZy38758225;     MwNkPqKmZy38758225 = MwNkPqKmZy12989567;     MwNkPqKmZy12989567 = MwNkPqKmZy59643368;     MwNkPqKmZy59643368 = MwNkPqKmZy44347144;     MwNkPqKmZy44347144 = MwNkPqKmZy39949518;     MwNkPqKmZy39949518 = MwNkPqKmZy30997867;     MwNkPqKmZy30997867 = MwNkPqKmZy24989867;     MwNkPqKmZy24989867 = MwNkPqKmZy41469776;     MwNkPqKmZy41469776 = MwNkPqKmZy86056830;     MwNkPqKmZy86056830 = MwNkPqKmZy97041615;     MwNkPqKmZy97041615 = MwNkPqKmZy11685967;     MwNkPqKmZy11685967 = MwNkPqKmZy41139320;     MwNkPqKmZy41139320 = MwNkPqKmZy88316871;     MwNkPqKmZy88316871 = MwNkPqKmZy10574809;     MwNkPqKmZy10574809 = MwNkPqKmZy72086464;     MwNkPqKmZy72086464 = MwNkPqKmZy7305204;     MwNkPqKmZy7305204 = MwNkPqKmZy32872113;     MwNkPqKmZy32872113 = MwNkPqKmZy33758203;     MwNkPqKmZy33758203 = MwNkPqKmZy14983755;     MwNkPqKmZy14983755 = MwNkPqKmZy36230720;     MwNkPqKmZy36230720 = MwNkPqKmZy90918251;     MwNkPqKmZy90918251 = MwNkPqKmZy20731767;     MwNkPqKmZy20731767 = MwNkPqKmZy49147739;     MwNkPqKmZy49147739 = MwNkPqKmZy40363651;     MwNkPqKmZy40363651 = MwNkPqKmZy79608017;     MwNkPqKmZy79608017 = MwNkPqKmZy73196105;     MwNkPqKmZy73196105 = MwNkPqKmZy27281422;     MwNkPqKmZy27281422 = MwNkPqKmZy92352913;     MwNkPqKmZy92352913 = MwNkPqKmZy4809154;     MwNkPqKmZy4809154 = MwNkPqKmZy69820627;     MwNkPqKmZy69820627 = MwNkPqKmZy57682611;     MwNkPqKmZy57682611 = MwNkPqKmZy64547703;     MwNkPqKmZy64547703 = MwNkPqKmZy85426667;     MwNkPqKmZy85426667 = MwNkPqKmZy51348404;     MwNkPqKmZy51348404 = MwNkPqKmZy74148360;     MwNkPqKmZy74148360 = MwNkPqKmZy21148946;     MwNkPqKmZy21148946 = MwNkPqKmZy74427576;     MwNkPqKmZy74427576 = MwNkPqKmZy26592161;     MwNkPqKmZy26592161 = MwNkPqKmZy56753714;     MwNkPqKmZy56753714 = MwNkPqKmZy87443922;     MwNkPqKmZy87443922 = MwNkPqKmZy50183852;     MwNkPqKmZy50183852 = MwNkPqKmZy5085670;     MwNkPqKmZy5085670 = MwNkPqKmZy39971930;     MwNkPqKmZy39971930 = MwNkPqKmZy94791797;     MwNkPqKmZy94791797 = MwNkPqKmZy23839744;     MwNkPqKmZy23839744 = MwNkPqKmZy12674446;     MwNkPqKmZy12674446 = MwNkPqKmZy17210829;     MwNkPqKmZy17210829 = MwNkPqKmZy93663634;     MwNkPqKmZy93663634 = MwNkPqKmZy63624989;     MwNkPqKmZy63624989 = MwNkPqKmZy54756591;     MwNkPqKmZy54756591 = MwNkPqKmZy57768323;     MwNkPqKmZy57768323 = MwNkPqKmZy59856269;     MwNkPqKmZy59856269 = MwNkPqKmZy7655065;     MwNkPqKmZy7655065 = MwNkPqKmZy75349615;     MwNkPqKmZy75349615 = MwNkPqKmZy65952183;     MwNkPqKmZy65952183 = MwNkPqKmZy18026459;     MwNkPqKmZy18026459 = MwNkPqKmZy63841827;     MwNkPqKmZy63841827 = MwNkPqKmZy19279717;     MwNkPqKmZy19279717 = MwNkPqKmZy64739127;     MwNkPqKmZy64739127 = MwNkPqKmZy66753413;     MwNkPqKmZy66753413 = MwNkPqKmZy3716446;     MwNkPqKmZy3716446 = MwNkPqKmZy32636954;     MwNkPqKmZy32636954 = MwNkPqKmZy36660623;     MwNkPqKmZy36660623 = MwNkPqKmZy16236203;     MwNkPqKmZy16236203 = MwNkPqKmZy39359005;     MwNkPqKmZy39359005 = MwNkPqKmZy47138264;     MwNkPqKmZy47138264 = MwNkPqKmZy55712652;     MwNkPqKmZy55712652 = MwNkPqKmZy36968468;     MwNkPqKmZy36968468 = MwNkPqKmZy36426448;     MwNkPqKmZy36426448 = MwNkPqKmZy50937519;     MwNkPqKmZy50937519 = MwNkPqKmZy32877628;     MwNkPqKmZy32877628 = MwNkPqKmZy6279953;     MwNkPqKmZy6279953 = MwNkPqKmZy77004489;     MwNkPqKmZy77004489 = MwNkPqKmZy27539833;     MwNkPqKmZy27539833 = MwNkPqKmZy86046868;     MwNkPqKmZy86046868 = MwNkPqKmZy85832581;     MwNkPqKmZy85832581 = MwNkPqKmZy80759836;     MwNkPqKmZy80759836 = MwNkPqKmZy54355942;     MwNkPqKmZy54355942 = MwNkPqKmZy16523907;     MwNkPqKmZy16523907 = MwNkPqKmZy66933571;     MwNkPqKmZy66933571 = MwNkPqKmZy55985277;     MwNkPqKmZy55985277 = MwNkPqKmZy33617788;     MwNkPqKmZy33617788 = MwNkPqKmZy70939304;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void BVLgzalnUQ31348774() {     float WuXKWnGwEH28791862 = -912149560;    float WuXKWnGwEH55750495 = -235921350;    float WuXKWnGwEH83284332 = -815616955;    float WuXKWnGwEH77323901 = 69071992;    float WuXKWnGwEH92174867 = -602481497;    float WuXKWnGwEH42329317 = -212675889;    float WuXKWnGwEH26099223 = -22818143;    float WuXKWnGwEH4077081 = -793312547;    float WuXKWnGwEH69754337 = 71496924;    float WuXKWnGwEH15554483 = -163995007;    float WuXKWnGwEH59641432 = -158776691;    float WuXKWnGwEH77890873 = -389384385;    float WuXKWnGwEH35451707 = -128069105;    float WuXKWnGwEH32551831 = -965903240;    float WuXKWnGwEH43372156 = -415514367;    float WuXKWnGwEH75871485 = -423590498;    float WuXKWnGwEH61386864 = -274879083;    float WuXKWnGwEH39606509 = -284382857;    float WuXKWnGwEH60161916 = -207103551;    float WuXKWnGwEH13269306 = -184505101;    float WuXKWnGwEH93943913 = -308686571;    float WuXKWnGwEH33287621 = -941848548;    float WuXKWnGwEH76889824 = -114086339;    float WuXKWnGwEH7640492 = -527524863;    float WuXKWnGwEH40105376 = -407480892;    float WuXKWnGwEH23896866 = -342997555;    float WuXKWnGwEH23345244 = -380740707;    float WuXKWnGwEH8277966 = -567944025;    float WuXKWnGwEH52568296 = -862348759;    float WuXKWnGwEH79740040 = -75026925;    float WuXKWnGwEH17778230 = -115889493;    float WuXKWnGwEH21215160 = 42640103;    float WuXKWnGwEH79890722 = -256545770;    float WuXKWnGwEH96159480 = -741053696;    float WuXKWnGwEH92551892 = -712543416;    float WuXKWnGwEH84183726 = -497506268;    float WuXKWnGwEH45893916 = -451967734;    float WuXKWnGwEH71154545 = -398159089;    float WuXKWnGwEH34303348 = -446539018;    float WuXKWnGwEH33316121 = -490063290;    float WuXKWnGwEH23501050 = -617137971;    float WuXKWnGwEH32539508 = -375311896;    float WuXKWnGwEH14258228 = 48353288;    float WuXKWnGwEH53539665 = -89704167;    float WuXKWnGwEH11263095 = -773699130;    float WuXKWnGwEH5986555 = -452061997;    float WuXKWnGwEH8753922 = 819920;    float WuXKWnGwEH56762033 = -660800550;    float WuXKWnGwEH56376793 = -858206830;    float WuXKWnGwEH66115615 = -318756994;    float WuXKWnGwEH61807238 = -562643188;    float WuXKWnGwEH94137876 = -824279187;    float WuXKWnGwEH45635564 = -426213221;    float WuXKWnGwEH87177066 = -102097373;    float WuXKWnGwEH37404860 = -890108374;    float WuXKWnGwEH95504240 = -970301012;    float WuXKWnGwEH78860670 = -21835011;    float WuXKWnGwEH75643841 = -188092093;    float WuXKWnGwEH37218526 = -523447116;    float WuXKWnGwEH68278001 = -159483942;    float WuXKWnGwEH18984074 = -831935182;    float WuXKWnGwEH17821257 = -454874119;    float WuXKWnGwEH51508785 = -930963788;    float WuXKWnGwEH90014297 = -853476152;    float WuXKWnGwEH97776253 = 51894485;    float WuXKWnGwEH38426272 = -101416795;    float WuXKWnGwEH98000151 = -32838616;    float WuXKWnGwEH39292226 = -387015410;    float WuXKWnGwEH39999938 = -153359824;    float WuXKWnGwEH59188430 = -918008100;    float WuXKWnGwEH29977569 = -971622765;    float WuXKWnGwEH90232318 = -876719994;    float WuXKWnGwEH5303162 = -837843840;    float WuXKWnGwEH26845796 = -717040262;    float WuXKWnGwEH89768255 = -567367130;    float WuXKWnGwEH61404406 = -933374675;    float WuXKWnGwEH19029394 = -890201836;    float WuXKWnGwEH23350160 = 75617827;    float WuXKWnGwEH96377396 = -753825733;    float WuXKWnGwEH34118821 = -955418895;    float WuXKWnGwEH15142944 = -243817476;    float WuXKWnGwEH66583210 = -719940158;    float WuXKWnGwEH51901173 = -709737195;    float WuXKWnGwEH86452680 = -443591765;    float WuXKWnGwEH17932802 = -512383738;    float WuXKWnGwEH23640354 = -291610306;    float WuXKWnGwEH75579596 = -531146676;    float WuXKWnGwEH92713655 = -54448397;    float WuXKWnGwEH58754621 = -850945322;    float WuXKWnGwEH97047652 = -742242405;    float WuXKWnGwEH5323057 = -375671257;    float WuXKWnGwEH70250074 = -163875641;    float WuXKWnGwEH33936019 = -874711973;    float WuXKWnGwEH66025346 = -187055077;    float WuXKWnGwEH14332047 = -658128108;    float WuXKWnGwEH5679794 = -62263853;    float WuXKWnGwEH81030723 = -444348109;    float WuXKWnGwEH24243930 = -98170561;    float WuXKWnGwEH55763411 = -41598653;    float WuXKWnGwEH72836823 = -912149560;     WuXKWnGwEH28791862 = WuXKWnGwEH55750495;     WuXKWnGwEH55750495 = WuXKWnGwEH83284332;     WuXKWnGwEH83284332 = WuXKWnGwEH77323901;     WuXKWnGwEH77323901 = WuXKWnGwEH92174867;     WuXKWnGwEH92174867 = WuXKWnGwEH42329317;     WuXKWnGwEH42329317 = WuXKWnGwEH26099223;     WuXKWnGwEH26099223 = WuXKWnGwEH4077081;     WuXKWnGwEH4077081 = WuXKWnGwEH69754337;     WuXKWnGwEH69754337 = WuXKWnGwEH15554483;     WuXKWnGwEH15554483 = WuXKWnGwEH59641432;     WuXKWnGwEH59641432 = WuXKWnGwEH77890873;     WuXKWnGwEH77890873 = WuXKWnGwEH35451707;     WuXKWnGwEH35451707 = WuXKWnGwEH32551831;     WuXKWnGwEH32551831 = WuXKWnGwEH43372156;     WuXKWnGwEH43372156 = WuXKWnGwEH75871485;     WuXKWnGwEH75871485 = WuXKWnGwEH61386864;     WuXKWnGwEH61386864 = WuXKWnGwEH39606509;     WuXKWnGwEH39606509 = WuXKWnGwEH60161916;     WuXKWnGwEH60161916 = WuXKWnGwEH13269306;     WuXKWnGwEH13269306 = WuXKWnGwEH93943913;     WuXKWnGwEH93943913 = WuXKWnGwEH33287621;     WuXKWnGwEH33287621 = WuXKWnGwEH76889824;     WuXKWnGwEH76889824 = WuXKWnGwEH7640492;     WuXKWnGwEH7640492 = WuXKWnGwEH40105376;     WuXKWnGwEH40105376 = WuXKWnGwEH23896866;     WuXKWnGwEH23896866 = WuXKWnGwEH23345244;     WuXKWnGwEH23345244 = WuXKWnGwEH8277966;     WuXKWnGwEH8277966 = WuXKWnGwEH52568296;     WuXKWnGwEH52568296 = WuXKWnGwEH79740040;     WuXKWnGwEH79740040 = WuXKWnGwEH17778230;     WuXKWnGwEH17778230 = WuXKWnGwEH21215160;     WuXKWnGwEH21215160 = WuXKWnGwEH79890722;     WuXKWnGwEH79890722 = WuXKWnGwEH96159480;     WuXKWnGwEH96159480 = WuXKWnGwEH92551892;     WuXKWnGwEH92551892 = WuXKWnGwEH84183726;     WuXKWnGwEH84183726 = WuXKWnGwEH45893916;     WuXKWnGwEH45893916 = WuXKWnGwEH71154545;     WuXKWnGwEH71154545 = WuXKWnGwEH34303348;     WuXKWnGwEH34303348 = WuXKWnGwEH33316121;     WuXKWnGwEH33316121 = WuXKWnGwEH23501050;     WuXKWnGwEH23501050 = WuXKWnGwEH32539508;     WuXKWnGwEH32539508 = WuXKWnGwEH14258228;     WuXKWnGwEH14258228 = WuXKWnGwEH53539665;     WuXKWnGwEH53539665 = WuXKWnGwEH11263095;     WuXKWnGwEH11263095 = WuXKWnGwEH5986555;     WuXKWnGwEH5986555 = WuXKWnGwEH8753922;     WuXKWnGwEH8753922 = WuXKWnGwEH56762033;     WuXKWnGwEH56762033 = WuXKWnGwEH56376793;     WuXKWnGwEH56376793 = WuXKWnGwEH66115615;     WuXKWnGwEH66115615 = WuXKWnGwEH61807238;     WuXKWnGwEH61807238 = WuXKWnGwEH94137876;     WuXKWnGwEH94137876 = WuXKWnGwEH45635564;     WuXKWnGwEH45635564 = WuXKWnGwEH87177066;     WuXKWnGwEH87177066 = WuXKWnGwEH37404860;     WuXKWnGwEH37404860 = WuXKWnGwEH95504240;     WuXKWnGwEH95504240 = WuXKWnGwEH78860670;     WuXKWnGwEH78860670 = WuXKWnGwEH75643841;     WuXKWnGwEH75643841 = WuXKWnGwEH37218526;     WuXKWnGwEH37218526 = WuXKWnGwEH68278001;     WuXKWnGwEH68278001 = WuXKWnGwEH18984074;     WuXKWnGwEH18984074 = WuXKWnGwEH17821257;     WuXKWnGwEH17821257 = WuXKWnGwEH51508785;     WuXKWnGwEH51508785 = WuXKWnGwEH90014297;     WuXKWnGwEH90014297 = WuXKWnGwEH97776253;     WuXKWnGwEH97776253 = WuXKWnGwEH38426272;     WuXKWnGwEH38426272 = WuXKWnGwEH98000151;     WuXKWnGwEH98000151 = WuXKWnGwEH39292226;     WuXKWnGwEH39292226 = WuXKWnGwEH39999938;     WuXKWnGwEH39999938 = WuXKWnGwEH59188430;     WuXKWnGwEH59188430 = WuXKWnGwEH29977569;     WuXKWnGwEH29977569 = WuXKWnGwEH90232318;     WuXKWnGwEH90232318 = WuXKWnGwEH5303162;     WuXKWnGwEH5303162 = WuXKWnGwEH26845796;     WuXKWnGwEH26845796 = WuXKWnGwEH89768255;     WuXKWnGwEH89768255 = WuXKWnGwEH61404406;     WuXKWnGwEH61404406 = WuXKWnGwEH19029394;     WuXKWnGwEH19029394 = WuXKWnGwEH23350160;     WuXKWnGwEH23350160 = WuXKWnGwEH96377396;     WuXKWnGwEH96377396 = WuXKWnGwEH34118821;     WuXKWnGwEH34118821 = WuXKWnGwEH15142944;     WuXKWnGwEH15142944 = WuXKWnGwEH66583210;     WuXKWnGwEH66583210 = WuXKWnGwEH51901173;     WuXKWnGwEH51901173 = WuXKWnGwEH86452680;     WuXKWnGwEH86452680 = WuXKWnGwEH17932802;     WuXKWnGwEH17932802 = WuXKWnGwEH23640354;     WuXKWnGwEH23640354 = WuXKWnGwEH75579596;     WuXKWnGwEH75579596 = WuXKWnGwEH92713655;     WuXKWnGwEH92713655 = WuXKWnGwEH58754621;     WuXKWnGwEH58754621 = WuXKWnGwEH97047652;     WuXKWnGwEH97047652 = WuXKWnGwEH5323057;     WuXKWnGwEH5323057 = WuXKWnGwEH70250074;     WuXKWnGwEH70250074 = WuXKWnGwEH33936019;     WuXKWnGwEH33936019 = WuXKWnGwEH66025346;     WuXKWnGwEH66025346 = WuXKWnGwEH14332047;     WuXKWnGwEH14332047 = WuXKWnGwEH5679794;     WuXKWnGwEH5679794 = WuXKWnGwEH81030723;     WuXKWnGwEH81030723 = WuXKWnGwEH24243930;     WuXKWnGwEH24243930 = WuXKWnGwEH55763411;     WuXKWnGwEH55763411 = WuXKWnGwEH72836823;     WuXKWnGwEH72836823 = WuXKWnGwEH28791862;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void jDsUxvWWgH15436677() {     double YtMMVcEpri31701102 = -492208523;    double YtMMVcEpri94932606 = -802479096;    double YtMMVcEpri47186438 = -789335105;    double YtMMVcEpri51303232 = -401572522;    double YtMMVcEpri3162759 = -169799064;    double YtMMVcEpri83828508 = -831948716;    double YtMMVcEpri26244087 = -608303920;    double YtMMVcEpri18281782 = -726367201;    double YtMMVcEpri30340917 = -304976841;    double YtMMVcEpri24348615 = 93639399;    double YtMMVcEpri73440936 = -761618900;    double YtMMVcEpri51736724 = -244044518;    double YtMMVcEpri59867599 = -904916860;    double YtMMVcEpri97516873 = -369055848;    double YtMMVcEpri87339135 = -788977566;    double YtMMVcEpri15758570 = -291259582;    double YtMMVcEpri89027241 = -109806076;    double YtMMVcEpri6020974 = 69021738;    double YtMMVcEpri82451072 = -891974316;    double YtMMVcEpri19284931 = -637639775;    double YtMMVcEpri1875086 = -359661114;    double YtMMVcEpri49565155 = -49799473;    double YtMMVcEpri67327736 = -131029076;    double YtMMVcEpri81893772 = -176354512;    double YtMMVcEpri57106167 = -924780687;    double YtMMVcEpri82293514 = -963405454;    double YtMMVcEpri41838905 = -924375838;    double YtMMVcEpri16503029 = -6115682;    double YtMMVcEpri36733570 = -511800205;    double YtMMVcEpri68325170 = -517031158;    double YtMMVcEpri82383967 = -912519404;    double YtMMVcEpri16262827 = -632814875;    double YtMMVcEpri77631924 = 16402178;    double YtMMVcEpri97215916 = -25599938;    double YtMMVcEpri52387978 = -2567920;    double YtMMVcEpri54052943 = -78981655;    double YtMMVcEpri38862229 = -357519678;    double YtMMVcEpri88174691 = -181469764;    double YtMMVcEpri68268549 = -902192349;    double YtMMVcEpri90029438 = -593903752;    double YtMMVcEpri92559865 = -192445189;    double YtMMVcEpri87735257 = -185012485;    double YtMMVcEpri89100218 = -730070880;    double YtMMVcEpri8446093 = -73441821;    double YtMMVcEpri25245029 = -465608747;    double YtMMVcEpri20267663 = -365026677;    double YtMMVcEpri80608128 = -285817943;    double YtMMVcEpri64280249 = -597190261;    double YtMMVcEpri87688690 = -689630082;    double YtMMVcEpri52369153 = -80302238;    double YtMMVcEpri16129940 = -979187045;    double YtMMVcEpri73361159 = -900964232;    double YtMMVcEpri17297344 = -810096502;    double YtMMVcEpri89908989 = -631064323;    double YtMMVcEpri13561592 = -891602771;    double YtMMVcEpri82135946 = -342409051;    double YtMMVcEpri27604870 = -571450021;    double YtMMVcEpri65292665 = -512980593;    double YtMMVcEpri94197064 = -476791836;    double YtMMVcEpri20869244 = -206393611;    double YtMMVcEpri41989603 = -907572879;    double YtMMVcEpri9741059 = -502188238;    double YtMMVcEpri81548211 = -114566997;    double YtMMVcEpri62015747 = -787945684;    double YtMMVcEpri41964647 = 6158802;    double YtMMVcEpri57178109 = -28804025;    double YtMMVcEpri74104800 = -160446696;    double YtMMVcEpri62651682 = -779316922;    double YtMMVcEpri45128895 = -266487929;    double YtMMVcEpri33286193 = -609995912;    double YtMMVcEpri76896340 = -933739905;    double YtMMVcEpri852551 = -928336312;    double YtMMVcEpri37752425 = -28785914;    double YtMMVcEpri92421634 = -198070565;    double YtMMVcEpri26725066 = -345194586;    double YtMMVcEpri14139828 = -74648630;    double YtMMVcEpri60464937 = -319728594;    double YtMMVcEpri58881644 = 42412744;    double YtMMVcEpri56648743 = -710745766;    double YtMMVcEpri36838505 = -459754010;    double YtMMVcEpri1685387 = -577587511;    double YtMMVcEpri77558656 = -227185577;    double YtMMVcEpri28814338 = -316485601;    double YtMMVcEpri84364417 = -331497967;    double YtMMVcEpri52195230 = -537844114;    double YtMMVcEpri9022809 = 88444828;    double YtMMVcEpri98965483 = -822718373;    double YtMMVcEpri87722934 = -352533500;    double YtMMVcEpri83654325 = -133997168;    double YtMMVcEpri70252032 = -660158870;    double YtMMVcEpri26448074 = -507531635;    double YtMMVcEpri73569563 = -844539085;    double YtMMVcEpri93977626 = -704677929;    double YtMMVcEpri47399305 = -595798738;    double YtMMVcEpri48039835 = -686330873;    double YtMMVcEpri82818807 = -690256951;    double YtMMVcEpri6187047 = 29554511;    double YtMMVcEpri27084471 = -942125197;    double YtMMVcEpri66481446 = 20399376;    double YtMMVcEpri68066919 = -492208523;     YtMMVcEpri31701102 = YtMMVcEpri94932606;     YtMMVcEpri94932606 = YtMMVcEpri47186438;     YtMMVcEpri47186438 = YtMMVcEpri51303232;     YtMMVcEpri51303232 = YtMMVcEpri3162759;     YtMMVcEpri3162759 = YtMMVcEpri83828508;     YtMMVcEpri83828508 = YtMMVcEpri26244087;     YtMMVcEpri26244087 = YtMMVcEpri18281782;     YtMMVcEpri18281782 = YtMMVcEpri30340917;     YtMMVcEpri30340917 = YtMMVcEpri24348615;     YtMMVcEpri24348615 = YtMMVcEpri73440936;     YtMMVcEpri73440936 = YtMMVcEpri51736724;     YtMMVcEpri51736724 = YtMMVcEpri59867599;     YtMMVcEpri59867599 = YtMMVcEpri97516873;     YtMMVcEpri97516873 = YtMMVcEpri87339135;     YtMMVcEpri87339135 = YtMMVcEpri15758570;     YtMMVcEpri15758570 = YtMMVcEpri89027241;     YtMMVcEpri89027241 = YtMMVcEpri6020974;     YtMMVcEpri6020974 = YtMMVcEpri82451072;     YtMMVcEpri82451072 = YtMMVcEpri19284931;     YtMMVcEpri19284931 = YtMMVcEpri1875086;     YtMMVcEpri1875086 = YtMMVcEpri49565155;     YtMMVcEpri49565155 = YtMMVcEpri67327736;     YtMMVcEpri67327736 = YtMMVcEpri81893772;     YtMMVcEpri81893772 = YtMMVcEpri57106167;     YtMMVcEpri57106167 = YtMMVcEpri82293514;     YtMMVcEpri82293514 = YtMMVcEpri41838905;     YtMMVcEpri41838905 = YtMMVcEpri16503029;     YtMMVcEpri16503029 = YtMMVcEpri36733570;     YtMMVcEpri36733570 = YtMMVcEpri68325170;     YtMMVcEpri68325170 = YtMMVcEpri82383967;     YtMMVcEpri82383967 = YtMMVcEpri16262827;     YtMMVcEpri16262827 = YtMMVcEpri77631924;     YtMMVcEpri77631924 = YtMMVcEpri97215916;     YtMMVcEpri97215916 = YtMMVcEpri52387978;     YtMMVcEpri52387978 = YtMMVcEpri54052943;     YtMMVcEpri54052943 = YtMMVcEpri38862229;     YtMMVcEpri38862229 = YtMMVcEpri88174691;     YtMMVcEpri88174691 = YtMMVcEpri68268549;     YtMMVcEpri68268549 = YtMMVcEpri90029438;     YtMMVcEpri90029438 = YtMMVcEpri92559865;     YtMMVcEpri92559865 = YtMMVcEpri87735257;     YtMMVcEpri87735257 = YtMMVcEpri89100218;     YtMMVcEpri89100218 = YtMMVcEpri8446093;     YtMMVcEpri8446093 = YtMMVcEpri25245029;     YtMMVcEpri25245029 = YtMMVcEpri20267663;     YtMMVcEpri20267663 = YtMMVcEpri80608128;     YtMMVcEpri80608128 = YtMMVcEpri64280249;     YtMMVcEpri64280249 = YtMMVcEpri87688690;     YtMMVcEpri87688690 = YtMMVcEpri52369153;     YtMMVcEpri52369153 = YtMMVcEpri16129940;     YtMMVcEpri16129940 = YtMMVcEpri73361159;     YtMMVcEpri73361159 = YtMMVcEpri17297344;     YtMMVcEpri17297344 = YtMMVcEpri89908989;     YtMMVcEpri89908989 = YtMMVcEpri13561592;     YtMMVcEpri13561592 = YtMMVcEpri82135946;     YtMMVcEpri82135946 = YtMMVcEpri27604870;     YtMMVcEpri27604870 = YtMMVcEpri65292665;     YtMMVcEpri65292665 = YtMMVcEpri94197064;     YtMMVcEpri94197064 = YtMMVcEpri20869244;     YtMMVcEpri20869244 = YtMMVcEpri41989603;     YtMMVcEpri41989603 = YtMMVcEpri9741059;     YtMMVcEpri9741059 = YtMMVcEpri81548211;     YtMMVcEpri81548211 = YtMMVcEpri62015747;     YtMMVcEpri62015747 = YtMMVcEpri41964647;     YtMMVcEpri41964647 = YtMMVcEpri57178109;     YtMMVcEpri57178109 = YtMMVcEpri74104800;     YtMMVcEpri74104800 = YtMMVcEpri62651682;     YtMMVcEpri62651682 = YtMMVcEpri45128895;     YtMMVcEpri45128895 = YtMMVcEpri33286193;     YtMMVcEpri33286193 = YtMMVcEpri76896340;     YtMMVcEpri76896340 = YtMMVcEpri852551;     YtMMVcEpri852551 = YtMMVcEpri37752425;     YtMMVcEpri37752425 = YtMMVcEpri92421634;     YtMMVcEpri92421634 = YtMMVcEpri26725066;     YtMMVcEpri26725066 = YtMMVcEpri14139828;     YtMMVcEpri14139828 = YtMMVcEpri60464937;     YtMMVcEpri60464937 = YtMMVcEpri58881644;     YtMMVcEpri58881644 = YtMMVcEpri56648743;     YtMMVcEpri56648743 = YtMMVcEpri36838505;     YtMMVcEpri36838505 = YtMMVcEpri1685387;     YtMMVcEpri1685387 = YtMMVcEpri77558656;     YtMMVcEpri77558656 = YtMMVcEpri28814338;     YtMMVcEpri28814338 = YtMMVcEpri84364417;     YtMMVcEpri84364417 = YtMMVcEpri52195230;     YtMMVcEpri52195230 = YtMMVcEpri9022809;     YtMMVcEpri9022809 = YtMMVcEpri98965483;     YtMMVcEpri98965483 = YtMMVcEpri87722934;     YtMMVcEpri87722934 = YtMMVcEpri83654325;     YtMMVcEpri83654325 = YtMMVcEpri70252032;     YtMMVcEpri70252032 = YtMMVcEpri26448074;     YtMMVcEpri26448074 = YtMMVcEpri73569563;     YtMMVcEpri73569563 = YtMMVcEpri93977626;     YtMMVcEpri93977626 = YtMMVcEpri47399305;     YtMMVcEpri47399305 = YtMMVcEpri48039835;     YtMMVcEpri48039835 = YtMMVcEpri82818807;     YtMMVcEpri82818807 = YtMMVcEpri6187047;     YtMMVcEpri6187047 = YtMMVcEpri27084471;     YtMMVcEpri27084471 = YtMMVcEpri66481446;     YtMMVcEpri66481446 = YtMMVcEpri68066919;     YtMMVcEpri68066919 = YtMMVcEpri31701102;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void VXyExEmOmG21435027() {     double doTINRulPu41016368 = -394739550;    double doTINRulPu50355528 = -819013599;    double doTINRulPu67338518 = -502975718;    double doTINRulPu9093254 = -593236514;    double doTINRulPu43225998 = -146314221;    double doTINRulPu77371870 = -290817577;    double doTINRulPu99898158 = -407552798;    double doTINRulPu7417938 = -923326349;    double doTINRulPu23149575 = -685037513;    double doTINRulPu92973071 = -827156269;    double doTINRulPu10342921 = -952859973;    double doTINRulPu62974474 = -881414333;    double doTINRulPu55649314 = -490338994;    double doTINRulPu29605515 = -348109953;    double doTINRulPu2615174 = -640507977;    double doTINRulPu20729094 = -318783060;    double doTINRulPu32838009 = -137391163;    double doTINRulPu18213615 = -761035575;    double doTINRulPu64464004 = -845615145;    double doTINRulPu82480279 = -716347616;    double doTINRulPu90554436 = -685361042;    double doTINRulPu73774067 = -972695806;    double doTINRulPu3389004 = -845558076;    double doTINRulPu41100402 = -793515622;    double doTINRulPu20168710 = -143913737;    double doTINRulPu48201332 = 37382905;    double doTINRulPu75597172 = -633264683;    double doTINRulPu87597504 = -513252415;    double doTINRulPu3850478 = -626219641;    double doTINRulPu81106118 = -323115721;    double doTINRulPu52382346 = -523171221;    double doTINRulPu93522189 = 36013830;    double doTINRulPu27491254 = -894688834;    double doTINRulPu86244864 = -609159825;    double doTINRulPu39038254 = -178980514;    double doTINRulPu85877837 = -495207102;    double doTINRulPu76329836 = 41280460;    double doTINRulPu44211489 = 65934873;    double doTINRulPu34494250 = -953408136;    double doTINRulPu66129335 = -301089264;    double doTINRulPu16623000 = -329601607;    double doTINRulPu75386913 = -106284576;    double doTINRulPu10817224 = -951951261;    double doTINRulPu67202993 = -563836709;    double doTINRulPu61195103 = -257384541;    double doTINRulPu90506897 = -690561236;    double doTINRulPu85857759 = -958652088;    double doTINRulPu35751286 = 56266351;    double doTINRulPu53715835 = -925326599;    double doTINRulPu22007055 = -973315940;    double doTINRulPu53097649 = -715063042;    double doTINRulPu37578201 = -448950449;    double doTINRulPu5305002 = -397873409;    double doTINRulPu94490233 = -990917441;    double doTINRulPu30880590 = -826520856;    double doTINRulPu67242300 = -422043745;    double doTINRulPu46966525 = -973455523;    double doTINRulPu26238117 = -709460096;    double doTINRulPu88924544 = -349322777;    double doTINRulPu95024665 = -83697127;    double doTINRulPu1774699 = -657552894;    double doTINRulPu12300655 = -894300384;    double doTINRulPu3567461 = -197106709;    double doTINRulPu42043456 = -261921792;    double doTINRulPu40590726 = -203985049;    double doTINRulPu16820731 = -888873803;    double doTINRulPu35483220 = -986725499;    double doTINRulPu69404450 = -881179170;    double doTINRulPu90567260 = -69129439;    double doTINRulPu16737336 = -45300876;    double doTINRulPu44399257 = -260063520;    double doTINRulPu88626519 = -103326036;    double doTINRulPu83719364 = -807627440;    double doTINRulPu98334669 = -444525881;    double doTINRulPu65857279 = -286746010;    double doTINRulPu15167524 = -479076467;    double doTINRulPu62956843 = 79255455;    double doTINRulPu36186010 = -181721368;    double doTINRulPu79905298 = -436131082;    double doTINRulPu29661813 = -453352502;    double doTINRulPu62343573 = -3965007;    double doTINRulPu39845886 = -589531034;    double doTINRulPu33881669 = -587925817;    double doTINRulPu81843422 = -652903701;    double doTINRulPu28008469 = -608052680;    double doTINRulPu14804145 = 25779228;    double doTINRulPu88217188 = -566112762;    double doTINRulPu33001021 = -903771394;    double doTINRulPu55364275 = -782638970;    double doTINRulPu71795953 = -756936770;    double doTINRulPu38911312 = -521751579;    double doTINRulPu50091720 = -249259444;    double doTINRulPu55286945 = -584742351;    double doTINRulPu39469584 = -769711009;    double doTINRulPu64354636 = -643536370;    double doTINRulPu4322346 = -435301224;    double doTINRulPu71819452 = -909177868;    double doTINRulPu68773768 = -590029469;    double doTINRulPu26612268 = -259851661;    double doTINRulPu44374373 = -394739550;     doTINRulPu41016368 = doTINRulPu50355528;     doTINRulPu50355528 = doTINRulPu67338518;     doTINRulPu67338518 = doTINRulPu9093254;     doTINRulPu9093254 = doTINRulPu43225998;     doTINRulPu43225998 = doTINRulPu77371870;     doTINRulPu77371870 = doTINRulPu99898158;     doTINRulPu99898158 = doTINRulPu7417938;     doTINRulPu7417938 = doTINRulPu23149575;     doTINRulPu23149575 = doTINRulPu92973071;     doTINRulPu92973071 = doTINRulPu10342921;     doTINRulPu10342921 = doTINRulPu62974474;     doTINRulPu62974474 = doTINRulPu55649314;     doTINRulPu55649314 = doTINRulPu29605515;     doTINRulPu29605515 = doTINRulPu2615174;     doTINRulPu2615174 = doTINRulPu20729094;     doTINRulPu20729094 = doTINRulPu32838009;     doTINRulPu32838009 = doTINRulPu18213615;     doTINRulPu18213615 = doTINRulPu64464004;     doTINRulPu64464004 = doTINRulPu82480279;     doTINRulPu82480279 = doTINRulPu90554436;     doTINRulPu90554436 = doTINRulPu73774067;     doTINRulPu73774067 = doTINRulPu3389004;     doTINRulPu3389004 = doTINRulPu41100402;     doTINRulPu41100402 = doTINRulPu20168710;     doTINRulPu20168710 = doTINRulPu48201332;     doTINRulPu48201332 = doTINRulPu75597172;     doTINRulPu75597172 = doTINRulPu87597504;     doTINRulPu87597504 = doTINRulPu3850478;     doTINRulPu3850478 = doTINRulPu81106118;     doTINRulPu81106118 = doTINRulPu52382346;     doTINRulPu52382346 = doTINRulPu93522189;     doTINRulPu93522189 = doTINRulPu27491254;     doTINRulPu27491254 = doTINRulPu86244864;     doTINRulPu86244864 = doTINRulPu39038254;     doTINRulPu39038254 = doTINRulPu85877837;     doTINRulPu85877837 = doTINRulPu76329836;     doTINRulPu76329836 = doTINRulPu44211489;     doTINRulPu44211489 = doTINRulPu34494250;     doTINRulPu34494250 = doTINRulPu66129335;     doTINRulPu66129335 = doTINRulPu16623000;     doTINRulPu16623000 = doTINRulPu75386913;     doTINRulPu75386913 = doTINRulPu10817224;     doTINRulPu10817224 = doTINRulPu67202993;     doTINRulPu67202993 = doTINRulPu61195103;     doTINRulPu61195103 = doTINRulPu90506897;     doTINRulPu90506897 = doTINRulPu85857759;     doTINRulPu85857759 = doTINRulPu35751286;     doTINRulPu35751286 = doTINRulPu53715835;     doTINRulPu53715835 = doTINRulPu22007055;     doTINRulPu22007055 = doTINRulPu53097649;     doTINRulPu53097649 = doTINRulPu37578201;     doTINRulPu37578201 = doTINRulPu5305002;     doTINRulPu5305002 = doTINRulPu94490233;     doTINRulPu94490233 = doTINRulPu30880590;     doTINRulPu30880590 = doTINRulPu67242300;     doTINRulPu67242300 = doTINRulPu46966525;     doTINRulPu46966525 = doTINRulPu26238117;     doTINRulPu26238117 = doTINRulPu88924544;     doTINRulPu88924544 = doTINRulPu95024665;     doTINRulPu95024665 = doTINRulPu1774699;     doTINRulPu1774699 = doTINRulPu12300655;     doTINRulPu12300655 = doTINRulPu3567461;     doTINRulPu3567461 = doTINRulPu42043456;     doTINRulPu42043456 = doTINRulPu40590726;     doTINRulPu40590726 = doTINRulPu16820731;     doTINRulPu16820731 = doTINRulPu35483220;     doTINRulPu35483220 = doTINRulPu69404450;     doTINRulPu69404450 = doTINRulPu90567260;     doTINRulPu90567260 = doTINRulPu16737336;     doTINRulPu16737336 = doTINRulPu44399257;     doTINRulPu44399257 = doTINRulPu88626519;     doTINRulPu88626519 = doTINRulPu83719364;     doTINRulPu83719364 = doTINRulPu98334669;     doTINRulPu98334669 = doTINRulPu65857279;     doTINRulPu65857279 = doTINRulPu15167524;     doTINRulPu15167524 = doTINRulPu62956843;     doTINRulPu62956843 = doTINRulPu36186010;     doTINRulPu36186010 = doTINRulPu79905298;     doTINRulPu79905298 = doTINRulPu29661813;     doTINRulPu29661813 = doTINRulPu62343573;     doTINRulPu62343573 = doTINRulPu39845886;     doTINRulPu39845886 = doTINRulPu33881669;     doTINRulPu33881669 = doTINRulPu81843422;     doTINRulPu81843422 = doTINRulPu28008469;     doTINRulPu28008469 = doTINRulPu14804145;     doTINRulPu14804145 = doTINRulPu88217188;     doTINRulPu88217188 = doTINRulPu33001021;     doTINRulPu33001021 = doTINRulPu55364275;     doTINRulPu55364275 = doTINRulPu71795953;     doTINRulPu71795953 = doTINRulPu38911312;     doTINRulPu38911312 = doTINRulPu50091720;     doTINRulPu50091720 = doTINRulPu55286945;     doTINRulPu55286945 = doTINRulPu39469584;     doTINRulPu39469584 = doTINRulPu64354636;     doTINRulPu64354636 = doTINRulPu4322346;     doTINRulPu4322346 = doTINRulPu71819452;     doTINRulPu71819452 = doTINRulPu68773768;     doTINRulPu68773768 = doTINRulPu26612268;     doTINRulPu26612268 = doTINRulPu44374373;     doTINRulPu44374373 = doTINRulPu41016368;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void dbxGoFpqok79675908() {     double yFVSTORjjK79673496 = -651645587;    double yFVSTORjjK75677250 = 93500072;    double yFVSTORjjK8834892 = -831450485;    double yFVSTORjjK12031850 = -415302946;    double yFVSTORjjK70679017 = -807778353;    double yFVSTORjjK15720902 = -847538097;    double yFVSTORjjK23720670 = 87437567;    double yFVSTORjjK8925765 = -477645724;    double yFVSTORjjK52833341 = -685396925;    double yFVSTORjjK35270582 = 16336543;    double yFVSTORjjK25316350 = 31328812;    double yFVSTORjjK42957329 = -938498718;    double yFVSTORjjK3456558 = -187882545;    double yFVSTORjjK31691343 = -871288899;    double yFVSTORjjK80343300 = -305001237;    double yFVSTORjjK67735525 = -9118602;    double yFVSTORjjK38648710 = -75405690;    double yFVSTORjjK33113294 = -898998314;    double yFVSTORjjK85197424 = -710050298;    double yFVSTORjjK58962681 = -864713604;    double yFVSTORjjK3767458 = -812848203;    double yFVSTORjjK14883481 = -974298716;    double yFVSTORjjK74384857 = -351201773;    double yFVSTORjjK10107602 = -599196349;    double yFVSTORjjK47549554 = -113835953;    double yFVSTORjjK85646 = -971750219;    double yFVSTORjjK71316234 = -247775465;    double yFVSTORjjK86313279 = -24232368;    double yFVSTORjjK51503343 = 7648826;    double yFVSTORjjK30804841 = -346236587;    double yFVSTORjjK48721407 = -772482586;    double yFVSTORjjK10993950 = -458682064;    double yFVSTORjjK79857646 = -960762454;    double yFVSTORjjK10191676 = -405871760;    double yFVSTORjjK30022331 = -767760254;    double yFVSTORjjK537663 = -911971747;    double yFVSTORjjK81947326 = -570979574;    double yFVSTORjjK17373479 = -997810061;    double yFVSTORjjK57500259 = -661576702;    double yFVSTORjjK18998827 = -100098100;    double yFVSTORjjK68468517 = -170683045;    double yFVSTORjjK20253239 = -451959680;    double yFVSTORjjK43850630 = -646207845;    double yFVSTORjjK78683491 = -539221069;    double yFVSTORjjK85209171 = -213773342;    double yFVSTORjjK94768266 = 30196690;    double yFVSTORjjK92673979 = -896697056;    double yFVSTORjjK16991695 = -546908099;    double yFVSTORjjK32959916 = -739023007;    double yFVSTORjjK3262894 = -70345650;    double yFVSTORjjK35135404 = -482921924;    double yFVSTORjjK74470996 = -409862693;    double yFVSTORjjK13685351 = 50093719;    double yFVSTORjjK19094444 = -121808913;    double yFVSTORjjK75908450 = -407397304;    double yFVSTORjjK64790015 = -677346872;    double yFVSTORjjK1292393 = -555298155;    double yFVSTORjjK98727290 = -132254137;    double yFVSTORjjK64482296 = -201466994;    double yFVSTORjjK70593372 = -836028134;    double yFVSTORjjK44404667 = -499762632;    double yFVSTORjjK37407391 = -888330065;    double yFVSTORjjK57422421 = -385294550;    double yFVSTORjjK22028501 = -239160339;    double yFVSTORjjK86549174 = -211180871;    double yFVSTORjjK14322400 = -509989124;    double yFVSTORjjK63099683 = -977736264;    double yFVSTORjjK93264882 = -782010785;    double yFVSTORjjK1669012 = -3528645;    double yFVSTORjjK79805638 = -393029490;    double yFVSTORjjK85788198 = -438139029;    double yFVSTORjjK21275231 = -77595629;    double yFVSTORjjK75613035 = -137421612;    double yFVSTORjjK66198598 = -509952199;    double yFVSTORjjK90494163 = -594030560;    double yFVSTORjjK83514219 = -260888524;    double yFVSTORjjK71032850 = -228090871;    double yFVSTORjjK95701366 = -811980705;    double yFVSTORjjK24898431 = -285423007;    double yFVSTORjjK52781287 = -44032644;    double yFVSTORjjK7411666 = 24946837;    double yFVSTORjjK54324540 = -700867367;    double yFVSTORjjK53353363 = -285209362;    double yFVSTORjjK48240450 = -922005525;    double yFVSTORjjK95669436 = -863314663;    double yFVSTORjjK74250411 = -262619894;    double yFVSTORjjK97308599 = -408775783;    double yFVSTORjjK60763203 = -738953541;    double yFVSTORjjK34283225 = -998474457;    double yFVSTORjjK65232315 = 9586617;    double yFVSTORjjK99245270 = -256673593;    double yFVSTORjjK83220036 = -338725437;    double yFVSTORjjK52891182 = -696343068;    double yFVSTORjjK86906887 = -825548569;    double yFVSTORjjK74594159 = -600335468;    double yFVSTORjjK31061126 = -282352980;    double yFVSTORjjK62830818 = 33334870;    double yFVSTORjjK21822129 = -307047506;    double yFVSTORjjK92134316 = -228040198;    double yFVSTORjjK70886772 = -651645587;     yFVSTORjjK79673496 = yFVSTORjjK75677250;     yFVSTORjjK75677250 = yFVSTORjjK8834892;     yFVSTORjjK8834892 = yFVSTORjjK12031850;     yFVSTORjjK12031850 = yFVSTORjjK70679017;     yFVSTORjjK70679017 = yFVSTORjjK15720902;     yFVSTORjjK15720902 = yFVSTORjjK23720670;     yFVSTORjjK23720670 = yFVSTORjjK8925765;     yFVSTORjjK8925765 = yFVSTORjjK52833341;     yFVSTORjjK52833341 = yFVSTORjjK35270582;     yFVSTORjjK35270582 = yFVSTORjjK25316350;     yFVSTORjjK25316350 = yFVSTORjjK42957329;     yFVSTORjjK42957329 = yFVSTORjjK3456558;     yFVSTORjjK3456558 = yFVSTORjjK31691343;     yFVSTORjjK31691343 = yFVSTORjjK80343300;     yFVSTORjjK80343300 = yFVSTORjjK67735525;     yFVSTORjjK67735525 = yFVSTORjjK38648710;     yFVSTORjjK38648710 = yFVSTORjjK33113294;     yFVSTORjjK33113294 = yFVSTORjjK85197424;     yFVSTORjjK85197424 = yFVSTORjjK58962681;     yFVSTORjjK58962681 = yFVSTORjjK3767458;     yFVSTORjjK3767458 = yFVSTORjjK14883481;     yFVSTORjjK14883481 = yFVSTORjjK74384857;     yFVSTORjjK74384857 = yFVSTORjjK10107602;     yFVSTORjjK10107602 = yFVSTORjjK47549554;     yFVSTORjjK47549554 = yFVSTORjjK85646;     yFVSTORjjK85646 = yFVSTORjjK71316234;     yFVSTORjjK71316234 = yFVSTORjjK86313279;     yFVSTORjjK86313279 = yFVSTORjjK51503343;     yFVSTORjjK51503343 = yFVSTORjjK30804841;     yFVSTORjjK30804841 = yFVSTORjjK48721407;     yFVSTORjjK48721407 = yFVSTORjjK10993950;     yFVSTORjjK10993950 = yFVSTORjjK79857646;     yFVSTORjjK79857646 = yFVSTORjjK10191676;     yFVSTORjjK10191676 = yFVSTORjjK30022331;     yFVSTORjjK30022331 = yFVSTORjjK537663;     yFVSTORjjK537663 = yFVSTORjjK81947326;     yFVSTORjjK81947326 = yFVSTORjjK17373479;     yFVSTORjjK17373479 = yFVSTORjjK57500259;     yFVSTORjjK57500259 = yFVSTORjjK18998827;     yFVSTORjjK18998827 = yFVSTORjjK68468517;     yFVSTORjjK68468517 = yFVSTORjjK20253239;     yFVSTORjjK20253239 = yFVSTORjjK43850630;     yFVSTORjjK43850630 = yFVSTORjjK78683491;     yFVSTORjjK78683491 = yFVSTORjjK85209171;     yFVSTORjjK85209171 = yFVSTORjjK94768266;     yFVSTORjjK94768266 = yFVSTORjjK92673979;     yFVSTORjjK92673979 = yFVSTORjjK16991695;     yFVSTORjjK16991695 = yFVSTORjjK32959916;     yFVSTORjjK32959916 = yFVSTORjjK3262894;     yFVSTORjjK3262894 = yFVSTORjjK35135404;     yFVSTORjjK35135404 = yFVSTORjjK74470996;     yFVSTORjjK74470996 = yFVSTORjjK13685351;     yFVSTORjjK13685351 = yFVSTORjjK19094444;     yFVSTORjjK19094444 = yFVSTORjjK75908450;     yFVSTORjjK75908450 = yFVSTORjjK64790015;     yFVSTORjjK64790015 = yFVSTORjjK1292393;     yFVSTORjjK1292393 = yFVSTORjjK98727290;     yFVSTORjjK98727290 = yFVSTORjjK64482296;     yFVSTORjjK64482296 = yFVSTORjjK70593372;     yFVSTORjjK70593372 = yFVSTORjjK44404667;     yFVSTORjjK44404667 = yFVSTORjjK37407391;     yFVSTORjjK37407391 = yFVSTORjjK57422421;     yFVSTORjjK57422421 = yFVSTORjjK22028501;     yFVSTORjjK22028501 = yFVSTORjjK86549174;     yFVSTORjjK86549174 = yFVSTORjjK14322400;     yFVSTORjjK14322400 = yFVSTORjjK63099683;     yFVSTORjjK63099683 = yFVSTORjjK93264882;     yFVSTORjjK93264882 = yFVSTORjjK1669012;     yFVSTORjjK1669012 = yFVSTORjjK79805638;     yFVSTORjjK79805638 = yFVSTORjjK85788198;     yFVSTORjjK85788198 = yFVSTORjjK21275231;     yFVSTORjjK21275231 = yFVSTORjjK75613035;     yFVSTORjjK75613035 = yFVSTORjjK66198598;     yFVSTORjjK66198598 = yFVSTORjjK90494163;     yFVSTORjjK90494163 = yFVSTORjjK83514219;     yFVSTORjjK83514219 = yFVSTORjjK71032850;     yFVSTORjjK71032850 = yFVSTORjjK95701366;     yFVSTORjjK95701366 = yFVSTORjjK24898431;     yFVSTORjjK24898431 = yFVSTORjjK52781287;     yFVSTORjjK52781287 = yFVSTORjjK7411666;     yFVSTORjjK7411666 = yFVSTORjjK54324540;     yFVSTORjjK54324540 = yFVSTORjjK53353363;     yFVSTORjjK53353363 = yFVSTORjjK48240450;     yFVSTORjjK48240450 = yFVSTORjjK95669436;     yFVSTORjjK95669436 = yFVSTORjjK74250411;     yFVSTORjjK74250411 = yFVSTORjjK97308599;     yFVSTORjjK97308599 = yFVSTORjjK60763203;     yFVSTORjjK60763203 = yFVSTORjjK34283225;     yFVSTORjjK34283225 = yFVSTORjjK65232315;     yFVSTORjjK65232315 = yFVSTORjjK99245270;     yFVSTORjjK99245270 = yFVSTORjjK83220036;     yFVSTORjjK83220036 = yFVSTORjjK52891182;     yFVSTORjjK52891182 = yFVSTORjjK86906887;     yFVSTORjjK86906887 = yFVSTORjjK74594159;     yFVSTORjjK74594159 = yFVSTORjjK31061126;     yFVSTORjjK31061126 = yFVSTORjjK62830818;     yFVSTORjjK62830818 = yFVSTORjjK21822129;     yFVSTORjjK21822129 = yFVSTORjjK92134316;     yFVSTORjjK92134316 = yFVSTORjjK70886772;     yFVSTORjjK70886772 = yFVSTORjjK79673496;}
// Junk Finished
