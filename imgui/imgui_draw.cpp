// dear imgui, v1.77 WIP
// (drawing and font code)

/*

Index of this file:

// [SECTION] STB libraries implementation
// [SECTION] Style functions
// [SECTION] ImDrawList
// [SECTION] ImDrawListSplitter
// [SECTION] ImDrawData
// [SECTION] Helpers ShadeVertsXXX functions
// [SECTION] ImFontConfig
// [SECTION] ImFontAtlas
// [SECTION] ImFontAtlas glyph ranges helpers
// [SECTION] ImFontGlyphRangesBuilder
// [SECTION] ImFont
// [SECTION] ImGui Internal Render Helpers
// [SECTION] Decompression code
// [SECTION] Default font data (ProggyClean.ttf)

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

#include <stdio.h>      // vsnprintf, sscanf, printf
#if !defined(alloca)
#if defined(__GLIBC__) || defined(__sun) || defined(__APPLE__) || defined(__NEWLIB__)
#include <alloca.h>     // alloca (glibc uses <alloca.h>. Note that Cygwin may have _WIN32 defined, so the order matters here)
#elif defined(_WIN32)
#include <malloc.h>     // alloca
#if !defined(alloca)
#define alloca _alloca  // for clang with MS Codegen
#endif
#else
#include <stdlib.h>     // alloca
#endif
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127) // condition expression is constant
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wglobal-constructors"    // warning : declaration requires a global destructor           // similar to above, not sure what the exact difference is.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             //
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning : zero as null pointer constant              // some standard header variations use #define NULL 0
#endif
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma"                  // warning : possible misuse of comma operator here             //
#endif
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"      // warning : macro name is a reserved identifier                //
#endif
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"       // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wstack-protector"          // warning: stack protector not protecting local variables: variable length buffer
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif

//-------------------------------------------------------------------------
// [SECTION] STB libraries implementation
//-------------------------------------------------------------------------

// Compile time options:
//#define IMGUI_STB_NAMESPACE           ImStb
//#define IMGUI_STB_TRUETYPE_FILENAME   "my_folder/stb_truetype.h"
//#define IMGUI_STB_RECT_PACK_FILENAME  "my_folder/stb_rect_pack.h"
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wcast-qual"              // warning : cast from 'const xxxx *' to 'xxx *' drops const qualifier //
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"              // warning: comparison is always true due to limited range of data type [-Wtype-limits]
#pragma GCC diagnostic ignored "-Wcast-qual"                // warning: cast from type 'const xxxx *' to type 'xxxx *' casts away qualifiers
#endif

#ifndef STB_RECT_PACK_IMPLEMENTATION                        // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STBRP_ASSERT(x)     do { IM_ASSERT(x); } while (0)
#define STBRP_SORT          ImQsort
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#ifdef IMGUI_STB_RECT_PACK_FILENAME
#include IMGUI_STB_RECT_PACK_FILENAME
#else
#include "imstb_rectpack.h"
#endif
#endif

#ifndef STB_TRUETYPE_IMPLEMENTATION                         // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u)   ((void)(u), IM_ALLOC(x))
#define STBTT_free(x,u)     ((void)(u), IM_FREE(x))
#define STBTT_assert(x)     do { IM_ASSERT(x); } while(0)
#define STBTT_fmod(x,y)     ImFmod(x,y)
#define STBTT_sqrt(x)       ImSqrt(x)
#define STBTT_pow(x,y)      ImPow(x,y)
#define STBTT_fabs(x)       ImFabs(x)
#define STBTT_ifloor(x)     ((int)ImFloorStd(x))
#define STBTT_iceil(x)      ((int)ImCeil(x))
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#else
#define STBTT_DEF extern
#endif
#ifdef IMGUI_STB_TRUETYPE_FILENAME
#include IMGUI_STB_TRUETYPE_FILENAME
#else
#include "imstb_truetype.h"
#endif
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-----------------------------------------------------------------------------
// [SECTION] Style functions
//-----------------------------------------------------------------------------

void ImGui::StyleColorsDark(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    style->WindowPadding = ImVec2(12, 12);
    style->WindowRounding = 4.0f;
    style->FramePadding = ImVec2(5, 5);
    style->FrameRounding = 3.0f;
    style->ItemSpacing = ImVec2(8, 4);
    style->ItemInnerSpacing = ImVec2(8, 6);
    style->IndentSpacing = 18.0f;
    style->ScrollbarSize = 10.0f;
    style->ScrollbarRounding = 3.0f;
    style->GrabMinSize = 5.0f;
    style->GrabRounding = 2.0f;

    style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
    style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
}

//-----------------------------------------------------------------------------
// ImDrawList
//-----------------------------------------------------------------------------

ImDrawListSharedData::ImDrawListSharedData()
{
    Font = NULL;
    FontSize = 0.0f;
    CurveTessellationTol = 0.0f;
    CircleSegmentMaxError = 0.0f;
    ClipRectFullscreen = ImVec4(-8192.0f, -8192.0f, +8192.0f, +8192.0f);
    InitialFlags = ImDrawListFlags_None;

    // Lookup tables
    for (int i = 0; i < IM_ARRAYSIZE(ArcFastVtx); i++)
    {
        const float a = ((float)i * 2 * IM_PI) / (float)IM_ARRAYSIZE(ArcFastVtx);
        ArcFastVtx[i] = ImVec2(ImCos(a), ImSin(a));
    }
    memset(CircleSegmentCounts, 0, sizeof(CircleSegmentCounts)); // This will be set by SetCircleSegmentMaxError()
}

void ImDrawListSharedData::SetCircleSegmentMaxError(float max_error)
{
    if (CircleSegmentMaxError == max_error)
        return;
    CircleSegmentMaxError = max_error;
    for (int i = 0; i < IM_ARRAYSIZE(CircleSegmentCounts); i++)
    {
        const float radius = i + 1.0f;
        const int segment_count = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, CircleSegmentMaxError);
        CircleSegmentCounts[i] = (ImU8)ImMin(segment_count, 255);
    }
}

// Initialize before use in a new frame. We always have a command ready in the buffer.
void ImDrawList::_ResetForNewFrame()
{
    // Verify that the ImDrawCmd fields we want to memcmp() are contiguous in memory.
    // (those should be IM_STATIC_ASSERT() in theory but with our pre C++11 setup the whole check doesn't compile with GCC)
    IM_ASSERT(IM_OFFSETOF(ImDrawCmd, ClipRect) == 0);
    IM_ASSERT(IM_OFFSETOF(ImDrawCmd, TextureId) == sizeof(ImVec4));
    IM_ASSERT(IM_OFFSETOF(ImDrawCmd, VtxOffset) == sizeof(ImVec4) + sizeof(ImTextureID));

    CmdBuffer.resize(0);
    IdxBuffer.resize(0);
    VtxBuffer.resize(0);
    Flags = _Data->InitialFlags;
    memset(&_CmdHeader, 0, sizeof(_CmdHeader));
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize(0);
    _TextureIdStack.resize(0);
    _Path.resize(0);
    _Splitter.Clear();
    CmdBuffer.push_back(ImDrawCmd());
}

void ImDrawList::_ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    Flags = ImDrawListFlags_None;
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureIdStack.clear();
    _Path.clear();
    _Splitter.ClearFreeMemory();
}

ImDrawList* ImDrawList::CloneOutput() const
{
    ImDrawList* dst = IM_NEW(ImDrawList(_Data));
    dst->CmdBuffer = CmdBuffer;
    dst->IdxBuffer = IdxBuffer;
    dst->VtxBuffer = VtxBuffer;
    dst->Flags = Flags;
    return dst;
}

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ClipRect = _CmdHeader.ClipRect;    // Same as calling ImDrawCmd_HeaderCopy()
    draw_cmd.TextureId = _CmdHeader.TextureId;
    draw_cmd.VtxOffset = _CmdHeader.VtxOffset;
    draw_cmd.IdxOffset = IdxBuffer.Size;

    IM_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
    CmdBuffer.push_back(draw_cmd);
}

// Pop trailing draw command (used before merging or presenting to user)
// Note that this leaves the ImDrawList in a state unfit for further commands, as most code assume that CmdBuffer.Size > 0 && CmdBuffer.back().UserCallback == NULL
void ImDrawList::_PopUnusedDrawCmd()
{
    if (CmdBuffer.Size == 0)
        return;
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0 && curr_cmd->UserCallback == NULL)
        CmdBuffer.pop_back();
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* callback_data)
{
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    }
    curr_cmd->UserCallback = callback;
    curr_cmd->UserCallbackData = callback_data;

    AddDrawCmd(); // Force a new command after us (see comment below)
}

// Compare ClipRect, TextureId and VtxOffset with a single memcmp()
#define ImDrawCmd_HeaderSize                        (IM_OFFSETOF(ImDrawCmd, VtxOffset) + sizeof(unsigned int))
#define ImDrawCmd_HeaderCompare(CMD_LHS, CMD_RHS)   (memcmp(CMD_LHS, CMD_RHS, ImDrawCmd_HeaderSize))    // Compare ClipRect, TextureId, VtxOffset
#define ImDrawCmd_HeaderCopy(CMD_DST, CMD_SRC)      (memcpy(CMD_DST, CMD_SRC, ImDrawCmd_HeaderSize))    // Copy ClipRect, TextureId, VtxOffset

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check so we always have a command ready in the stack.
// The cost of figuring out if a new command has to be added or if we can merge is paid in those Update** functions only.
void ImDrawList::_OnChangedClipRect()
{
    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && memcmp(&curr_cmd->ClipRect, &_CmdHeader.ClipRect, sizeof(ImVec4)) != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }

    curr_cmd->ClipRect = _CmdHeader.ClipRect;
}

void ImDrawList::_OnChangedTextureID()
{
    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && curr_cmd->TextureId != _CmdHeader.TextureId)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }

    curr_cmd->TextureId = _CmdHeader.TextureId;
}

void ImDrawList::_OnChangedVtxOffset()
{
    // We don't need to compare curr_cmd->VtxOffset != _CmdHeader.VtxOffset because we know it'll be different at the time we call this.
    _VtxCurrentIdx = 0;
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    IM_ASSERT(curr_cmd->VtxOffset != _CmdHeader.VtxOffset);
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    curr_cmd->VtxOffset = _CmdHeader.VtxOffset;
}

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect(ImVec2 cr_min, ImVec2 cr_max, bool intersect_with_current_clip_rect)
{
    ImVec4 cr(cr_min.x, cr_min.y, cr_max.x, cr_max.y);
    if (intersect_with_current_clip_rect)
    {
        ImVec4 current = _CmdHeader.ClipRect;
        if (cr.x < current.x) cr.x = current.x;
        if (cr.y < current.y) cr.y = current.y;
        if (cr.z > current.z) cr.z = current.z;
        if (cr.w > current.w) cr.w = current.w;
    }
    cr.z = ImMax(cr.x, cr.z);
    cr.w = ImMax(cr.y, cr.w);

    _ClipRectStack.push_back(cr);
    _CmdHeader.ClipRect = cr;
    _OnChangedClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(ImVec2(_Data->ClipRectFullscreen.x, _Data->ClipRectFullscreen.y), ImVec2(_Data->ClipRectFullscreen.z, _Data->ClipRectFullscreen.w));
}

void ImDrawList::PopClipRect()
{
    _ClipRectStack.pop_back();
    _CmdHeader.ClipRect = (_ClipRectStack.Size == 0) ? _Data->ClipRectFullscreen : _ClipRectStack.Data[_ClipRectStack.Size - 1];
    _OnChangedClipRect();
}

void ImDrawList::PushTextureID(ImTextureID texture_id)
{
    _TextureIdStack.push_back(texture_id);
    _CmdHeader.TextureId = texture_id;
    _OnChangedTextureID();
}

void ImDrawList::PopTextureID()
{
    _TextureIdStack.pop_back();
    _CmdHeader.TextureId = (_TextureIdStack.Size == 0) ? (ImTextureID)NULL : _TextureIdStack.Data[_TextureIdStack.Size - 1];
    _OnChangedTextureID();
}

// Reserve space for a number of vertices and indices.
// You must finish filling your reserved data before calling PrimReserve() again, as it may reallocate or
// submit the intermediate results. PrimUnreserve() can be used to release unused allocations.
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    // Large mesh support (when enabled)
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);
    if (sizeof(ImDrawIdx) == 2 && (_VtxCurrentIdx + vtx_count >= (1 << 16)) && (Flags & ImDrawListFlags_AllowVtxOffset))
    {
        _CmdHeader.VtxOffset = VtxBuffer.Size;
        _OnChangedVtxOffset();
    }

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount += idx_count;

    int vtx_buffer_old_size = VtxBuffer.Size;
    VtxBuffer.resize(vtx_buffer_old_size + vtx_count);
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_old_size;

    int idx_buffer_old_size = IdxBuffer.Size;
    IdxBuffer.resize(idx_buffer_old_size + idx_count);
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_old_size;
}

// Release the a number of reserved vertices/indices from the end of the last reservation made with PrimReserve().
void ImDrawList::PrimUnreserve(int idx_count, int vtx_count)
{
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount -= idx_count;
    VtxBuffer.shrink(VtxBuffer.Size - vtx_count);
    IdxBuffer.shrink(IdxBuffer.Size - idx_count);
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv(_Data->TexUvWhitePixel);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx + 1); _IdxWritePtr[2] = (ImDrawIdx)(idx + 2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx + 3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx + 1); _IdxWritePtr[2] = (ImDrawIdx)(idx + 2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx + 3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx + 1); _IdxWritePtr[2] = (ImDrawIdx)(idx + 2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx + 3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

// On AddPolyline() and AddConvexPolyFilled() we intentionally avoid using ImVec2 and superfluous function calls to optimize debug/non-inlined builds.
// Those macros expects l-values.
#define IM_NORMALIZE2F_OVER_ZERO(VX,VY)     do { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = 1.0f / ImSqrt(d2); VX *= inv_len; VY *= inv_len; } } while (0)
#define IM_FIXNORMAL2F(VX,VY)               do { float d2 = VX*VX + VY*VY; if (d2 < 0.5f) d2 = 0.5f; float inv_lensq = 1.0f / d2; VX *= inv_lensq; VY *= inv_lensq; } while (0)

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
// We avoid using the ImVec2 math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddPolyline(const ImVec2* points, const int points_count, ImU32 col, bool closed, float thickness)
{
    if (points_count < 2)
        return;

    const ImVec2 opaque_uv = _Data->TexUvWhitePixel;
    int count = points_count;
    if (!closed)
        count = points_count - 1;

    const bool thick_line = (thickness > 1.0f);
    if (Flags & ImDrawListFlags_AntiAliasedLines)
    {
        // Anti-aliased stroke
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;

        const int idx_count = thick_line ? count * 18 : count * 12;
        const int vtx_count = thick_line ? points_count * 4 : points_count * 3;
        PrimReserve(idx_count, vtx_count);

        // Temporary buffer
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * (thick_line ? 5 : 3) * sizeof(ImVec2)); //-V630
        ImVec2* temp_points = temp_normals + points_count;

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            float dx = points[i2].x - points[i1].x;
            float dy = points[i2].y - points[i1].y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i1].x = dy;
            temp_normals[i1].y = -dx;
        }
        if (!closed)
            temp_normals[points_count - 1] = temp_normals[points_count - 2];

        if (!thick_line)
        {
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * AA_SIZE;
                temp_points[1] = points[0] - temp_normals[0] * AA_SIZE;
                temp_points[(points_count - 1) * 2 + 0] = points[points_count - 1] + temp_normals[points_count - 1] * AA_SIZE;
                temp_points[(points_count - 1) * 2 + 1] = points[points_count - 1] - temp_normals[points_count - 1] * AA_SIZE;
            }

            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx;
            for (int i1 = 0; i1 < count; i1++)
            {
                const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
                unsigned int idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : idx1 + 3;

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                dm_x *= AA_SIZE;
                dm_y *= AA_SIZE;

                // Add temporary vertices
                ImVec2* out_vtx = &temp_points[i2 * 2];
                out_vtx[0].x = points[i2].x + dm_x;
                out_vtx[0].y = points[i2].y + dm_y;
                out_vtx[1].x = points[i2].x - dm_x;
                out_vtx[1].y = points[i2].y - dm_y;

                // Add indexes
                _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 2);
                _IdxWritePtr[3] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0);
                _IdxWritePtr[6] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8] = (ImDrawIdx)(idx1 + 0);
                _IdxWritePtr[9] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr += 12;

                idx1 = idx2;
            }

            // Add vertices
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = points[i];          _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;
                _VtxWritePtr[1].pos = temp_points[i * 2 + 0]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col_trans;
                _VtxWritePtr[2].pos = temp_points[i * 2 + 1]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col_trans;
                _VtxWritePtr += 3;
            }
        } else
        {
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;
            if (!closed)
            {
                const int points_last = points_count - 1;
                temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
                temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
                temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 0] = points[points_last] + temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 1] = points[points_last] + temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 2] = points[points_last] - temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 3] = points[points_last] - temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
            }

            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx;
            for (int i1 = 0; i1 < count; i1++)
            {
                const int i2 = (i1 + 1) == points_count ? 0 : (i1 + 1); // i2 is the second point of the line segment
                const unsigned int idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : (idx1 + 4); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
                float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
                float dm_in_x = dm_x * half_inner_thickness;
                float dm_in_y = dm_y * half_inner_thickness;

                // Add temporary vertices
                ImVec2* out_vtx = &temp_points[i2 * 4];
                out_vtx[0].x = points[i2].x + dm_out_x;
                out_vtx[0].y = points[i2].y + dm_out_y;
                out_vtx[1].x = points[i2].x + dm_in_x;
                out_vtx[1].y = points[i2].y + dm_in_y;
                out_vtx[2].x = points[i2].x - dm_in_x;
                out_vtx[2].y = points[i2].y - dm_in_y;
                out_vtx[3].x = points[i2].x - dm_out_x;
                out_vtx[3].y = points[i2].y - dm_out_y;

                // Add indexes
                _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 2);
                _IdxWritePtr[3] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[6] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8] = (ImDrawIdx)(idx1 + 0);
                _IdxWritePtr[9] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[12] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[13] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[14] = (ImDrawIdx)(idx1 + 3);
                _IdxWritePtr[15] = (ImDrawIdx)(idx1 + 3); _IdxWritePtr[16] = (ImDrawIdx)(idx2 + 3); _IdxWritePtr[17] = (ImDrawIdx)(idx2 + 2);
                _IdxWritePtr += 18;

                idx1 = idx2;
            }

            // Add vertices
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = temp_points[i * 4 + 0]; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col_trans;
                _VtxWritePtr[1].pos = temp_points[i * 4 + 1]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
                _VtxWritePtr[2].pos = temp_points[i * 4 + 2]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
                _VtxWritePtr[3].pos = temp_points[i * 4 + 3]; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col_trans;
                _VtxWritePtr += 4;
            }
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    } else
    {
        // Non Anti-aliased Stroke
        const int idx_count = count * 6;
        const int vtx_count = count * 4;      // FIXME-OPT: Not sharing edges
        PrimReserve(idx_count, vtx_count);

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            dx *= (thickness * 0.5f);
            dy *= (thickness * 0.5f);

            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + 2);
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx + 2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx + 3);
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

// We intentionally avoid using ImVec2 and its math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddConvexPolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count - 2) * 3 + points_count * 6;
        const int vtx_count = (points_count * 2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + ((i - 1) << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (i << 1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * sizeof(ImVec2)); //-V630
        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F(dm_x, dm_y);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    } else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count - 2) * 3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + i - 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::PathArcToFast(const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12)
{
    if (radius == 0.0f || a_min_of_12 > a_max_of_12)
    {
        _Path.push_back(center);
        return;
    }

    // For legacy reason the PathArcToFast() always takes angles where 2*PI is represented by 12,
    // but it is possible to set IM_DRAWLIST_ARCFAST_TESSELATION_MULTIPLIER to a higher value. This should compile to a no-op otherwise.
#if IM_DRAWLIST_ARCFAST_TESSELLATION_MULTIPLIER != 1
    a_min_of_12 *= IM_DRAWLIST_ARCFAST_TESSELLATION_MULTIPLIER;
    a_max_of_12 *= IM_DRAWLIST_ARCFAST_TESSELLATION_MULTIPLIER;
#endif

    _Path.reserve(_Path.Size + (a_max_of_12 - a_min_of_12 + 1));
    for (int a = a_min_of_12; a <= a_max_of_12; a++)
    {
        const ImVec2& c = _Data->ArcFastVtx[a % IM_ARRAYSIZE(_Data->ArcFastVtx)];
        _Path.push_back(ImVec2(center.x + c.x * radius, center.y + c.y * radius));
    }
}

void ImDrawList::PathArcTo(const ImVec2& center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius == 0.0f)
    {
        _Path.push_back(center);
        return;
    }

    // Note that we are adding a point at both a_min and a_max.
    // If you are trying to draw a full closed circle you don't want the overlapping points!
    _Path.reserve(_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        _Path.push_back(ImVec2(center.x + ImCos(a) * radius, center.y + ImSin(a) * radius));
    }
}

ImVec2 ImBezierCalc(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, float t)
{
    float u = 1.0f - t;
    float w1 = u * u * u;
    float w2 = 3 * u * u * t;
    float w3 = 3 * u * t * t;
    float w4 = t * t * t;
    return ImVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x, w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y);
}

// Closely mimics BezierClosestPointCasteljauStep() in imgui.cpp
static void PathBezierToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
    float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        path->push_back(ImVec2(x4, y4));
    } else if (level < 10)
    {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x34 = (x3 + x4) * 0.5f, y34 = (y3 + y4) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        float x234 = (x23 + x34) * 0.5f, y234 = (y23 + y34) * 0.5f;
        float x1234 = (x123 + x234) * 0.5f, y1234 = (y123 + y234) * 0.5f;
        PathBezierToCasteljau(path, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
        PathBezierToCasteljau(path, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
    }
}

void ImDrawList::PathBezierCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        PathBezierToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, _Data->CurveTessellationTol, 0); // Auto-tessellated
    } else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            _Path.push_back(ImBezierCalc(p1, p2, p3, p4, t_step * i_step));
    }
}

void ImDrawList::PathRect(const ImVec2& a, const ImVec2& b, float rounding, ImDrawCornerFlags rounding_corners)
{
    rounding = ImMin(rounding, ImFabs(b.x - a.x) * (((rounding_corners & ImDrawCornerFlags_Top) == ImDrawCornerFlags_Top) || ((rounding_corners & ImDrawCornerFlags_Bot) == ImDrawCornerFlags_Bot) ? 0.5f : 1.0f) - 1.0f);
    rounding = ImMin(rounding, ImFabs(b.y - a.y) * (((rounding_corners & ImDrawCornerFlags_Left) == ImDrawCornerFlags_Left) || ((rounding_corners & ImDrawCornerFlags_Right) == ImDrawCornerFlags_Right) ? 0.5f : 1.0f) - 1.0f);

    if (rounding <= 0.0f || rounding_corners == 0)
    {
        PathLineTo(a);
        PathLineTo(ImVec2(b.x, a.y));
        PathLineTo(b);
        PathLineTo(ImVec2(a.x, b.y));
    } else
    {
        const float rounding_tl = (rounding_corners & ImDrawCornerFlags_TopLeft) ? rounding : 0.0f;
        const float rounding_tr = (rounding_corners & ImDrawCornerFlags_TopRight) ? rounding : 0.0f;
        const float rounding_br = (rounding_corners & ImDrawCornerFlags_BotRight) ? rounding : 0.0f;
        const float rounding_bl = (rounding_corners & ImDrawCornerFlags_BotLeft) ? rounding : 0.0f;
        PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
        PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
        PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
        PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
    }
}

void ImDrawList::AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    PathLineTo(p1 + ImVec2(0.5f, 0.5f));
    PathLineTo(p2 + ImVec2(0.5f, 0.5f));
    PathStroke(col, false, thickness);
}

// p_min = upper-left, p_max = lower-right
// Note we don't render 1 pixels sized rectangles properly.
void ImDrawList::AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawCornerFlags rounding_corners, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (Flags & ImDrawListFlags_AntiAliasedLines)
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.50f, 0.50f), rounding, rounding_corners);
    else
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.49f, 0.49f), rounding, rounding_corners); // Better looking lower-right corner and rounded non-AA shapes.
    PathStroke(col, true, thickness);
}

void ImDrawList::AddRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawCornerFlags rounding_corners)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (rounding > 0.0f)
    {
        PathRect(p_min, p_max, rounding, rounding_corners);
        PathFillConvex(col);
    } else
    {
        PrimReserve(6, 4);
        PrimRect(p_min, p_max, col);
    }
}

// p_min = upper-left, p_max = lower-right
void ImDrawList::AddRectFilledMultiColor(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    PrimReserve(6, 4);
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 1)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2));
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 3));
    PrimWriteVtx(p_min, uv, col_upr_left);
    PrimWriteVtx(ImVec2(p_max.x, p_min.y), uv, col_upr_right);
    PrimWriteVtx(p_max, uv, col_bot_right);
    PrimWriteVtx(ImVec2(p_min.x, p_max.y), uv, col_bot_left);
}

void ImDrawList::AddQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathFillConvex(col);
}

void ImDrawList::AddTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathFillConvex(col);
}

void ImDrawList::AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius <= 0.0f)
        return;

    // Obtain segment count
    if (num_segments <= 0)
    {
        // Automatic segment count
        const int radius_idx = (int)radius - 1;
        if (radius_idx < IM_ARRAYSIZE(_Data->CircleSegmentCounts))
            num_segments = _Data->CircleSegmentCounts[radius_idx]; // Use cached value
        else
            num_segments = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, _Data->CircleSegmentMaxError);
    } else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);
    }

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    if (num_segments == 12)
        PathArcToFast(center, radius - 0.5f, 0, 12);
    else
        PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius <= 0.0f)
        return;

    // Obtain segment count
    if (num_segments <= 0)
    {
        // Automatic segment count
        const int radius_idx = (int)radius - 1;
        if (radius_idx < IM_ARRAYSIZE(_Data->CircleSegmentCounts))
            num_segments = _Data->CircleSegmentCounts[radius_idx]; // Use cached value
        else
            num_segments = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, _Data->CircleSegmentMaxError);
    } else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);
    }

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    if (num_segments == 12)
        PathArcToFast(center, radius, 0, 12);
    else
        PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgon(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgonFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Cubic Bezier takes 4 controls points
void ImDrawList::AddBezierCurve(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathBezierCurveTo(p2, p3, p4, num_segments);
    PathStroke(col, false, thickness);
}

void ImDrawList::AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    if (text_begin == text_end)
        return;

    // Pull default font/size from the shared ImDrawListSharedData instance
    if (font == NULL)
        font = _Data->Font;
    if (font_size == 0.0f)
        font_size = _Data->FontSize;

    IM_ASSERT(font->ContainerAtlas->TexID == _CmdHeader.TextureId);  // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

    ImVec4 clip_rect = _CmdHeader.ClipRect;
    if (cpu_fine_clip_rect)
    {
        clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
        clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
        clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
        clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
    }
    font->RenderText(this, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip_rect != NULL);
}

void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
    AddText(NULL, 0.0f, pos, col, text_begin, text_end);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimRectUV(p_min, p_max, uv_min, uv_max, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageQuad(ImTextureID user_texture_id, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3, const ImVec2& uv4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimQuadUV(p1, p2, p3, p4, uv1, uv2, uv3, uv4, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageRounded(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawCornerFlags rounding_corners)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (rounding <= 0.0f || (rounding_corners & ImDrawCornerFlags_All) == 0)
    {
        AddImage(user_texture_id, p_min, p_max, uv_min, uv_max, col);
        return;
    }

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    int vert_start_idx = VtxBuffer.Size;
    PathRect(p_min, p_max, rounding, rounding_corners);
    PathFillConvex(col);
    int vert_end_idx = VtxBuffer.Size;
    ImGui::ShadeVertsLinearUV(this, vert_start_idx, vert_end_idx, p_min, p_max, uv_min, uv_max, true);

    if (push_texture_id)
        PopTextureID();
}


//-----------------------------------------------------------------------------
// ImDrawListSplitter
//-----------------------------------------------------------------------------
// FIXME: This may be a little confusing, trying to be a little too low-level/optimal instead of just doing vector swap..
//-----------------------------------------------------------------------------

void ImDrawListSplitter::ClearFreeMemory()
{
    for (int i = 0; i < _Channels.Size; i++)
    {
        if (i == _Current)
            memset(&_Channels[i], 0, sizeof(_Channels[i]));  // Current channel is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i]._CmdBuffer.clear();
        _Channels[i]._IdxBuffer.clear();
    }
    _Current = 0;
    _Count = 1;
    _Channels.clear();
}

void ImDrawListSplitter::Split(ImDrawList* draw_list, int channels_count)
{
    IM_ASSERT(_Current == 0 && _Count <= 1 && "Nested channel splitting is not supported. Please use separate instances of ImDrawListSplitter.");
    int old_channels_count = _Channels.Size;
    if (old_channels_count < channels_count)
        _Channels.resize(channels_count);
    _Count = channels_count;

    // Channels[] (24/32 bytes each) hold storage that we'll swap with draw_list->_CmdBuffer/_IdxBuffer
    // The content of Channels[0] at this point doesn't matter. We clear it to make state tidy in a debugger but we don't strictly need to.
    // When we switch to the next channel, we'll copy draw_list->_CmdBuffer/_IdxBuffer into Channels[0] and then Channels[1] into draw_list->CmdBuffer/_IdxBuffer
    memset(&_Channels[0], 0, sizeof(ImDrawChannel));
    for (int i = 1; i < channels_count; i++)
    {
        if (i >= old_channels_count)
        {
            IM_PLACEMENT_NEW(&_Channels[i]) ImDrawChannel();
        } else
        {
            _Channels[i]._CmdBuffer.resize(0);
            _Channels[i]._IdxBuffer.resize(0);
        }
        if (_Channels[i]._CmdBuffer.Size == 0)
        {
            ImDrawCmd draw_cmd;
            ImDrawCmd_HeaderCopy(&draw_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
            _Channels[i]._CmdBuffer.push_back(draw_cmd);
        }
    }
}

void ImDrawListSplitter::Merge(ImDrawList* draw_list)
{
    // Note that we never use or rely on _Channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    if (_Count <= 1)
        return;

    SetCurrentChannel(draw_list, 0);
    draw_list->_PopUnusedDrawCmd();

    // Calculate our final buffer sizes. Also fix the incorrect IdxOffset values in each command.
    int new_cmd_buffer_count = 0;
    int new_idx_buffer_count = 0;
    ImDrawCmd* last_cmd = (_Count > 0 && draw_list->CmdBuffer.Size > 0) ? &draw_list->CmdBuffer.back() : NULL;
    int idx_offset = last_cmd ? last_cmd->IdxOffset + last_cmd->ElemCount : 0;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];

        // Equivalent of PopUnusedDrawCmd() for this channel's cmdbuffer and except we don't need to test for UserCallback.
        if (ch._CmdBuffer.Size > 0 && ch._CmdBuffer.back().ElemCount == 0)
            ch._CmdBuffer.pop_back();

        if (ch._CmdBuffer.Size > 0 && last_cmd != NULL)
        {
            ImDrawCmd* next_cmd = &ch._CmdBuffer[0];
            if (ImDrawCmd_HeaderCompare(last_cmd, next_cmd) == 0 && last_cmd->UserCallback == NULL && next_cmd->UserCallback == NULL)
            {
                // Merge previous channel last draw command with current channel first draw command if matching.
                last_cmd->ElemCount += next_cmd->ElemCount;
                idx_offset += next_cmd->ElemCount;
                ch._CmdBuffer.erase(ch._CmdBuffer.Data); // FIXME-OPT: Improve for multiple merges.
            }
        }
        if (ch._CmdBuffer.Size > 0)
            last_cmd = &ch._CmdBuffer.back();
        new_cmd_buffer_count += ch._CmdBuffer.Size;
        new_idx_buffer_count += ch._IdxBuffer.Size;
        for (int cmd_n = 0; cmd_n < ch._CmdBuffer.Size; cmd_n++)
        {
            ch._CmdBuffer.Data[cmd_n].IdxOffset = idx_offset;
            idx_offset += ch._CmdBuffer.Data[cmd_n].ElemCount;
        }
    }
    draw_list->CmdBuffer.resize(draw_list->CmdBuffer.Size + new_cmd_buffer_count);
    draw_list->IdxBuffer.resize(draw_list->IdxBuffer.Size + new_idx_buffer_count);

    // Write commands and indices in order (they are fairly small structures, we don't copy vertices only indices)
    ImDrawCmd* cmd_write = draw_list->CmdBuffer.Data + draw_list->CmdBuffer.Size - new_cmd_buffer_count;
    ImDrawIdx* idx_write = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size - new_idx_buffer_count;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (int sz = ch._CmdBuffer.Size) { memcpy(cmd_write, ch._CmdBuffer.Data, sz * sizeof(ImDrawCmd)); cmd_write += sz; }
        if (int sz = ch._IdxBuffer.Size) { memcpy(idx_write, ch._IdxBuffer.Data, sz * sizeof(ImDrawIdx)); idx_write += sz; }
    }
    draw_list->_IdxWritePtr = idx_write;

    // Ensure there's always a non-callback draw command trailing the command-buffer
    if (draw_list->CmdBuffer.Size == 0 || draw_list->CmdBuffer.back().UserCallback != NULL)
        draw_list->AddDrawCmd();

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();

    _Count = 1;
}

void ImDrawListSplitter::SetCurrentChannel(ImDrawList* draw_list, int idx)
{
    IM_ASSERT(idx >= 0 && idx < _Count);
    if (_Current == idx)
        return;

    // Overwrite ImVector (12/16 bytes), four times. This is merely a silly optimization instead of doing .swap()
    memcpy(&_Channels.Data[_Current]._CmdBuffer, &draw_list->CmdBuffer, sizeof(draw_list->CmdBuffer));
    memcpy(&_Channels.Data[_Current]._IdxBuffer, &draw_list->IdxBuffer, sizeof(draw_list->IdxBuffer));
    _Current = idx;
    memcpy(&draw_list->CmdBuffer, &_Channels.Data[idx]._CmdBuffer, sizeof(draw_list->CmdBuffer));
    memcpy(&draw_list->IdxBuffer, &_Channels.Data[idx]._IdxBuffer, sizeof(draw_list->IdxBuffer));
    draw_list->_IdxWritePtr = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size;

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawData
//-----------------------------------------------------------------------------

// For backward compatibility: convert all buffers from indexed to de-indexed, in case you cannot render indexed. Note: this is slow and most likely a waste of resources. Always prefer indexed rendering!
void ImDrawData::DeIndexAllBuffers()
{
    ImVector<ImDrawVert> new_vtx_buffer;
    TotalVtxCount = TotalIdxCount = 0;
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        if (cmd_list->IdxBuffer.empty())
            continue;
        new_vtx_buffer.resize(cmd_list->IdxBuffer.Size);
        for (int j = 0; j < cmd_list->IdxBuffer.Size; j++)
            new_vtx_buffer[j] = cmd_list->VtxBuffer[cmd_list->IdxBuffer[j]];
        cmd_list->VtxBuffer.swap(new_vtx_buffer);
        cmd_list->IdxBuffer.resize(0);
        TotalVtxCount += cmd_list->VtxBuffer.Size;
    }
}

// Helper to scale the ClipRect field of each ImDrawCmd.
// Use if your final output buffer is at a different scale than draw_data->DisplaySize,
// or if there is a difference between your window resolution and framebuffer resolution.
void ImDrawData::ScaleClipRects(const ImVec2& fb_scale)
{
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            ImDrawCmd* cmd = &cmd_list->CmdBuffer[cmd_i];
            cmd->ClipRect = ImVec4(cmd->ClipRect.x * fb_scale.x, cmd->ClipRect.y * fb_scale.y, cmd->ClipRect.z * fb_scale.x, cmd->ClipRect.w * fb_scale.y);
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Helpers ShadeVertsXXX functions
//-----------------------------------------------------------------------------

// Generic linear color gradient, write to RGB fields, leave A untouched.
void ImGui::ShadeVertsLinearColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);
        int r = ImLerp((int)(col0 >> IM_COL32_R_SHIFT) & 0xFF, (int)(col1 >> IM_COL32_R_SHIFT) & 0xFF, t);
        int g = ImLerp((int)(col0 >> IM_COL32_G_SHIFT) & 0xFF, (int)(col1 >> IM_COL32_G_SHIFT) & 0xFF, t);
        int b = ImLerp((int)(col0 >> IM_COL32_B_SHIFT) & 0xFF, (int)(col1 >> IM_COL32_B_SHIFT) & 0xFF, t);
        vert->col = (r << IM_COL32_R_SHIFT) | (g << IM_COL32_G_SHIFT) | (b << IM_COL32_B_SHIFT) | (vert->col & IM_COL32_A_MASK);
    }
}

// Distribute UV over (a, b) rectangle
void ImGui::ShadeVertsLinearUV(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, bool clamp)
{
    const ImVec2 size = b - a;
    const ImVec2 uv_size = uv_b - uv_a;
    const ImVec2 scale = ImVec2(
        size.x != 0.0f ? (uv_size.x / size.x) : 0.0f,
        size.y != 0.0f ? (uv_size.y / size.y) : 0.0f);

    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    if (clamp)
    {
        const ImVec2 min = ImMin(uv_a, uv_b);
        const ImVec2 max = ImMax(uv_a, uv_b);
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = ImClamp(uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale), min, max);
    } else
    {
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontConfig
//-----------------------------------------------------------------------------

ImFontConfig::ImFontConfig()
{
    FontData = NULL;
    FontDataSize = 0;
    FontDataOwnedByAtlas = true;
    FontNo = 0;
    SizePixels = 0.0f;
    OversampleH = 3; // FIXME: 2 may be a better default?
    OversampleV = 1;
    PixelSnapH = false;
    GlyphExtraSpacing = ImVec2(0.0f, 0.0f);
    GlyphOffset = ImVec2(0.0f, 0.0f);
    GlyphRanges = NULL;
    GlyphMinAdvanceX = 0.0f;
    GlyphMaxAdvanceX = FLT_MAX;
    MergeMode = false;
    RasterizerFlags = 0x00;
    RasterizerMultiply = 1.0f;
    EllipsisChar = (ImWchar)-1;
    memset(Name, 0, sizeof(Name));
    DstFont = NULL;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontAtlas
//-----------------------------------------------------------------------------

// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The white texels on the top left are the ones we'll use everywhere in Dear ImGui to render filled shapes.
const int FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF = 108;
const int FONT_ATLAS_DEFAULT_TEX_DATA_H = 27;
static const char FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * FONT_ATLAS_DEFAULT_TEX_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          "
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         "
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         "
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X-    X..X         "
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X-    X..X         "
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X-    X..XXX       "
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX-    X..X..XXX    "
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      -    X..X..X..XX  "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       -    X..X..X..X.X "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        -XXX X..X..X..X..X"
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         -X..XX........X..X"
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          -X...X...........X"
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           - X..............X"
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            -  X.............X"
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           -  X.............X"
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          -   X............X"
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          -   X...........X "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       -------------------------------------    X..........X "
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           -    X..........X "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           -     X........X  "
    "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           -     X........X  "
    "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           -     XXXXXXXXXX  "
    "------------        -    X    -           X           -X.....................X-           ------------------"
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -                             "
    "                                                      -  X..X           X..X  -                             "
    "                                                      -   X.X           X.X   -                             "
    "                                                      -    XX           XX    -                             "
};

static const ImVec2 FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[ImGuiMouseCursor_COUNT][3] =
{
    // Pos ........ Size ......... Offset ......
    { ImVec2(0,3), ImVec2(12,19), ImVec2(0, 0) }, // ImGuiMouseCursor_Arrow
    { ImVec2(13,0), ImVec2(7,16), ImVec2(1, 8) }, // ImGuiMouseCursor_TextInput
    { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_ResizeAll
    { ImVec2(21,0), ImVec2(9,23), ImVec2(4,11) }, // ImGuiMouseCursor_ResizeNS
    { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 4) }, // ImGuiMouseCursor_ResizeEW
    { ImVec2(73,0), ImVec2(17,17), ImVec2(8, 8) }, // ImGuiMouseCursor_ResizeNESW
    { ImVec2(55,0), ImVec2(17,17), ImVec2(8, 8) }, // ImGuiMouseCursor_ResizeNWSE
    { ImVec2(91,0), ImVec2(17,22), ImVec2(5, 0) }, // ImGuiMouseCursor_Hand
};

ImFontAtlas::ImFontAtlas()
{
    Locked = false;
    Flags = ImFontAtlasFlags_None;
    TexID = (ImTextureID)NULL;
    TexDesiredWidth = 0;
    TexGlyphPadding = 1;

    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
    TexWidth = TexHeight = 0;
    TexUvScale = ImVec2(0.0f, 0.0f);
    TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    for (int n = 0; n < IM_ARRAYSIZE(CustomRectIds); n++)
        CustomRectIds[n] = -1;
}

ImFontAtlas::~ImFontAtlas()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    Clear();
}

void    ImFontAtlas::ClearInputData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    for (int i = 0; i < ConfigData.Size; i++)
        if (ConfigData[i].FontData && ConfigData[i].FontDataOwnedByAtlas)
        {
            IM_FREE(ConfigData[i].FontData);
            ConfigData[i].FontData = NULL;
        }

    // When clearing this we lose access to the font name and other information used to build the font.
    for (int i = 0; i < Fonts.Size; i++)
        if (Fonts[i]->ConfigData >= ConfigData.Data && Fonts[i]->ConfigData < ConfigData.Data + ConfigData.Size)
        {
            Fonts[i]->ConfigData = NULL;
            Fonts[i]->ConfigDataCount = 0;
        }
    ConfigData.clear();
    CustomRects.clear();
    for (int n = 0; n < IM_ARRAYSIZE(CustomRectIds); n++)
        CustomRectIds[n] = -1;
}

void    ImFontAtlas::ClearTexData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    if (TexPixelsAlpha8)
        IM_FREE(TexPixelsAlpha8);
    if (TexPixelsRGBA32)
        IM_FREE(TexPixelsRGBA32);
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
}

void    ImFontAtlas::ClearFonts()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    for (int i = 0; i < Fonts.Size; i++)
        IM_DELETE(Fonts[i]);
    Fonts.clear();
}

void    ImFontAtlas::Clear()
{
    ClearInputData();
    ClearTexData();
    ClearFonts();
}

void    ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Build atlas on demand
    if (TexPixelsAlpha8 == NULL)
    {
        if (ConfigData.empty())
            AddFontDefault();
        Build();
    }

    *out_pixels = TexPixelsAlpha8;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

void    ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Convert to RGBA32 format on demand
    // Although it is likely to be the most commonly used format, our font rendering is 1 channel / 8 bpp
    if (!TexPixelsRGBA32)
    {
        unsigned char* pixels = NULL;
        GetTexDataAsAlpha8(&pixels, NULL, NULL);
        if (pixels)
        {
            TexPixelsRGBA32 = (unsigned int*)IM_ALLOC((size_t)TexWidth * (size_t)TexHeight * 4);
            const unsigned char* src = pixels;
            unsigned int* dst = TexPixelsRGBA32;
            for (int n = TexWidth * TexHeight; n > 0; n--)
                *dst++ = IM_COL32(255, 255, 255, (unsigned int)(*src++));
        }
    }

    *out_pixels = (unsigned char*)TexPixelsRGBA32;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    IM_ASSERT(font_cfg->FontData != NULL && font_cfg->FontDataSize > 0);
    IM_ASSERT(font_cfg->SizePixels > 0.0f);

    // Create new font
    if (!font_cfg->MergeMode)
        Fonts.push_back(IM_NEW(ImFont));
    else
        IM_ASSERT(!Fonts.empty() && "Cannot use MergeMode for the first font"); // When using MergeMode make sure that a font has already been added before. You can use ImGui::GetIO().Fonts->AddFontDefault() to add the default imgui font.

    ConfigData.push_back(*font_cfg);
    ImFontConfig& new_font_cfg = ConfigData.back();
    if (new_font_cfg.DstFont == NULL)
        new_font_cfg.DstFont = Fonts.back();
    if (!new_font_cfg.FontDataOwnedByAtlas)
    {
        new_font_cfg.FontData = IM_ALLOC(new_font_cfg.FontDataSize);
        new_font_cfg.FontDataOwnedByAtlas = true;
        memcpy(new_font_cfg.FontData, font_cfg->FontData, (size_t)new_font_cfg.FontDataSize);
    }

    if (new_font_cfg.DstFont->EllipsisChar == (ImWchar)-1)
        new_font_cfg.DstFont->EllipsisChar = font_cfg->EllipsisChar;

    // Invalidate texture
    ClearTexData();
    return new_font_cfg.DstFont;
}

// Default font TTF is compressed with stb_compress then base85 encoded (see misc/fonts/binary_to_compressed_c.cpp for encoder)
static unsigned int stb_decompress_length(const unsigned char* input);
static unsigned int stb_decompress(unsigned char* output, const unsigned char* input, unsigned int length);
static const char* GetDefaultCompressedFontDataTTFBase85();
static unsigned int Decode85Byte(char c) { return c >= '\\' ? c - 36 : c - 35; }
static void         Decode85(const unsigned char* src, unsigned char* dst)
{
    while (*src)
    {
        unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
        dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
        src += 5;
        dst += 4;
    }
}

// Load embedded ProggyClean.ttf at size 13, disable oversampling
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg_template)
{
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (!font_cfg_template)
    {
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
    }
    if (font_cfg.SizePixels <= 0.0f)
        font_cfg.SizePixels = 13.0f * 1.0f;
    if (font_cfg.Name[0] == '\0')
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "ProggyClean.ttf, %dpx", (int)font_cfg.SizePixels);
    font_cfg.EllipsisChar = (ImWchar)0x0085;

    const char* ttf_compressed_base85 = GetDefaultCompressedFontDataTTFBase85();
    const ImWchar* glyph_ranges = font_cfg.GlyphRanges != NULL ? font_cfg.GlyphRanges : GetGlyphRangesDefault();
    ImFont* font = AddFontFromMemoryCompressedBase85TTF(ttf_compressed_base85, font_cfg.SizePixels, &font_cfg, glyph_ranges);
    font->DisplayOffset.y = 1.0f;
    return font;
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    size_t data_size = 0;
    void* data = ImFileLoadToMemory(filename, "rb", &data_size, 0);
    if (!data)
    {
        IM_ASSERT_USER_ERROR(0, "Could not load font file!");
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        // Store a short copy of filename into into the font name for convenience
        const char* p;
        for (p = filename + strlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s, %.0fpx", p, size_pixels);
    }
    return AddFontFromMemoryTTF(data, (int)data_size, size_pixels, &font_cfg, glyph_ranges);
}

// NB: Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false. Owned TTF buffer will be deleted after Build().
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* ttf_data, int ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontData = ttf_data;
    font_cfg.FontDataSize = ttf_size;
    font_cfg.SizePixels = size_pixels;
    if (glyph_ranges)
        font_cfg.GlyphRanges = glyph_ranges;
    return AddFont(&font_cfg);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    const unsigned int buf_decompressed_size = stb_decompress_length((const unsigned char*)compressed_ttf_data);
    unsigned char* buf_decompressed_data = (unsigned char*)IM_ALLOC(buf_decompressed_size);
    stb_decompress(buf_decompressed_data, (const unsigned char*)compressed_ttf_data, (unsigned int)compressed_ttf_size);

    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontDataOwnedByAtlas = true;
    return AddFontFromMemoryTTF(buf_decompressed_data, (int)buf_decompressed_size, size_pixels, &font_cfg, glyph_ranges);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_ttf_data_base85, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges)
{
    int compressed_ttf_size = (((int)strlen(compressed_ttf_data_base85) + 4) / 5) * 4;
    void* compressed_ttf = IM_ALLOC((size_t)compressed_ttf_size);
    Decode85((const unsigned char*)compressed_ttf_data_base85, (unsigned char*)compressed_ttf);
    ImFont* font = AddFontFromMemoryCompressedTTF(compressed_ttf, compressed_ttf_size, size_pixels, font_cfg, glyph_ranges);
    IM_FREE(compressed_ttf);
    return font;
}

int ImFontAtlas::AddCustomRectRegular(int width, int height)
{
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    ImFontAtlasCustomRect r;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

int ImFontAtlas::AddCustomRectFontGlyph(ImFont* font, ImWchar id, int width, int height, float advance_x, const ImVec2& offset)
{
#ifdef IMGUI_USE_WCHAR32
    IM_ASSERT(id <= IM_UNICODE_CODEPOINT_MAX);
#endif
    IM_ASSERT(font != NULL);
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    ImFontAtlasCustomRect r;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    r.GlyphID = id;
    r.GlyphAdvanceX = advance_x;
    r.GlyphOffset = offset;
    r.Font = font;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

void ImFontAtlas::CalcCustomRectUV(const ImFontAtlasCustomRect* rect, ImVec2* out_uv_min, ImVec2* out_uv_max) const
{
    IM_ASSERT(TexWidth > 0 && TexHeight > 0);   // Font atlas needs to be built before we can calculate UV coordinates
    IM_ASSERT(rect->IsPacked());                // Make sure the rectangle has been packed
    *out_uv_min = ImVec2((float)rect->X * TexUvScale.x, (float)rect->Y * TexUvScale.y);
    *out_uv_max = ImVec2((float)(rect->X + rect->Width) * TexUvScale.x, (float)(rect->Y + rect->Height) * TexUvScale.y);
}

bool ImFontAtlas::GetMouseCursorTexData(ImGuiMouseCursor cursor_type, ImVec2* out_offset, ImVec2* out_size, ImVec2 out_uv_border[2], ImVec2 out_uv_fill[2])
{
    if (cursor_type <= ImGuiMouseCursor_None || cursor_type >= ImGuiMouseCursor_COUNT)
        return false;
    if (Flags & ImFontAtlasFlags_NoMouseCursors)
        return false;

    IM_ASSERT(CustomRectIds[0] != -1);
    ImFontAtlasCustomRect& r = CustomRects[CustomRectIds[0]];
    ImVec2 pos = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][0] + ImVec2((float)r.X, (float)r.Y);
    ImVec2 size = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][1];
    *out_size = size;
    *out_offset = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][2];
    out_uv_border[0] = (pos)*TexUvScale;
    out_uv_border[1] = (pos + size) * TexUvScale;
    pos.x += FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF + 1;
    out_uv_fill[0] = (pos)*TexUvScale;
    out_uv_fill[1] = (pos + size) * TexUvScale;
    return true;
}

bool    ImFontAtlas::Build()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    return ImFontAtlasBuildWithStbTruetype(this);
}

void    ImFontAtlasBuildMultiplyCalcLookupTable(unsigned char out_table[256], float in_brighten_factor)
{
    for (unsigned int i = 0; i < 256; i++)
    {
        unsigned int value = (unsigned int)(i * in_brighten_factor);
        out_table[i] = value > 255 ? 255 : (value & 0xFF);
    }
}

void    ImFontAtlasBuildMultiplyRectAlpha8(const unsigned char table[256], unsigned char* pixels, int x, int y, int w, int h, int stride)
{
    unsigned char* data = pixels + x + y * stride;
    for (int j = h; j > 0; j--, data += stride)
        for (int i = 0; i < w; i++)
            data[i] = table[data[i]];
}

// Temporary data for one source font (multiple source fonts can be merged into one destination ImFont)
// (C++03 doesn't allow instancing ImVector<> with function-local types so we declare the type here.)
struct ImFontBuildSrcData
{
    stbtt_fontinfo      FontInfo;
    stbtt_pack_range    PackRange;          // Hold the list of codepoints to pack (essentially points to Codepoints.Data)
    stbrp_rect* Rects;              // Rectangle to pack. We first fill in their size and the packer will give us their position.
    stbtt_packedchar* PackedChars;        // Output glyphs
    const ImWchar* SrcRanges;          // Ranges as requested by user (user is allowed to request too much, e.g. 0x0020..0xFFFF)
    int                 DstIndex;           // Index into atlas->Fonts[] and dst_tmp_array[]
    int                 GlyphsHighest;      // Highest requested codepoint
    int                 GlyphsCount;        // Glyph count (excluding missing glyphs and glyphs already set by an earlier source font)
    ImBitVector         GlyphsSet;          // Glyph bit map (random access, 1-bit per codepoint. This will be a maximum of 8KB)
    ImVector<int>       GlyphsList;         // Glyph codepoints list (flattened version of GlyphsMap)
};

// Temporary data for one destination ImFont* (multiple source fonts can be merged into one destination ImFont)
struct ImFontBuildDstData
{
    int                 SrcCount;           // Number of source fonts targeting this destination font.
    int                 GlyphsHighest;
    int                 GlyphsCount;
    ImBitVector         GlyphsSet;          // This is used to resolve collision when multiple sources are merged into a same destination font.
};

static void UnpackBitVectorToFlatIndexList(const ImBitVector* in, ImVector<int>* out)
{
    IM_ASSERT(sizeof(in->Storage.Data[0]) == sizeof(int));
    const ImU32* it_begin = in->Storage.begin();
    const ImU32* it_end = in->Storage.end();
    for (const ImU32* it = it_begin; it < it_end; it++)
        if (ImU32 entries_32 = *it)
            for (ImU32 bit_n = 0; bit_n < 32; bit_n++)
                if (entries_32 & ((ImU32)1 << bit_n))
                    out->push_back((int)(((it - it_begin) << 5) + bit_n));
}

bool    ImFontAtlasBuildWithStbTruetype(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->ConfigData.Size > 0);

    ImFontAtlasBuildInit(atlas);

    // Clear atlas
    atlas->TexID = (ImTextureID)NULL;
    atlas->TexWidth = atlas->TexHeight = 0;
    atlas->TexUvScale = ImVec2(0.0f, 0.0f);
    atlas->TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    atlas->ClearTexData();

    // Temporary storage for building
    ImVector<ImFontBuildSrcData> src_tmp_array;
    ImVector<ImFontBuildDstData> dst_tmp_array;
    src_tmp_array.resize(atlas->ConfigData.Size);
    dst_tmp_array.resize(atlas->Fonts.Size);
    memset(src_tmp_array.Data, 0, (size_t)src_tmp_array.size_in_bytes());
    memset(dst_tmp_array.Data, 0, (size_t)dst_tmp_array.size_in_bytes());

    // 1. Initialize font loading structure, check font data validity
    for (int src_i = 0; src_i < atlas->ConfigData.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        IM_ASSERT(cfg.DstFont && (!cfg.DstFont->IsLoaded() || cfg.DstFont->ContainerAtlas == atlas));

        // Find index from cfg.DstFont (we allow the user to set cfg.DstFont. Also it makes casual debugging nicer than when storing indices)
        src_tmp.DstIndex = -1;
        for (int output_i = 0; output_i < atlas->Fonts.Size && src_tmp.DstIndex == -1; output_i++)
            if (cfg.DstFont == atlas->Fonts[output_i])
                src_tmp.DstIndex = output_i;
        IM_ASSERT(src_tmp.DstIndex != -1); // cfg.DstFont not pointing within atlas->Fonts[] array?
        if (src_tmp.DstIndex == -1)
            return false;

        // Initialize helper structure for font loading and verify that the TTF/OTF data is correct
        const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)cfg.FontData, cfg.FontNo);
        IM_ASSERT(font_offset >= 0 && "FontData is incorrect, or FontNo cannot be found.");
        if (!stbtt_InitFont(&src_tmp.FontInfo, (unsigned char*)cfg.FontData, font_offset))
            return false;

        // Measure highest codepoints
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.SrcRanges = cfg.GlyphRanges ? cfg.GlyphRanges : atlas->GetGlyphRangesDefault();
        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            src_tmp.GlyphsHighest = ImMax(src_tmp.GlyphsHighest, (int)src_range[1]);
        dst_tmp.SrcCount++;
        dst_tmp.GlyphsHighest = ImMax(dst_tmp.GlyphsHighest, src_tmp.GlyphsHighest);
    }

    // 2. For every requested codepoint, check for their presence in the font data, and handle redundancy or overlaps between source fonts to avoid unused glyphs.
    int total_glyphs_count = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.GlyphsSet.Create(src_tmp.GlyphsHighest + 1);
        if (dst_tmp.GlyphsSet.Storage.empty())
            dst_tmp.GlyphsSet.Create(dst_tmp.GlyphsHighest + 1);

        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            for (unsigned int codepoint = src_range[0]; codepoint <= src_range[1]; codepoint++)
            {
                if (dst_tmp.GlyphsSet.TestBit(codepoint))    // Don't overwrite existing glyphs. We could make this an option for MergeMode (e.g. MergeOverwrite==true)
                    continue;
                if (!stbtt_FindGlyphIndex(&src_tmp.FontInfo, codepoint))    // It is actually in the font?
                    continue;

                // Add to avail set/counters
                src_tmp.GlyphsCount++;
                dst_tmp.GlyphsCount++;
                src_tmp.GlyphsSet.SetBit(codepoint);
                dst_tmp.GlyphsSet.SetBit(codepoint);
                total_glyphs_count++;
            }
    }

    // 3. Unpack our bit map into a flat list (we now have all the Unicode points that we know are requested _and_ available _and_ not overlapping another)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        src_tmp.GlyphsList.reserve(src_tmp.GlyphsCount);
        UnpackBitVectorToFlatIndexList(&src_tmp.GlyphsSet, &src_tmp.GlyphsList);
        src_tmp.GlyphsSet.Clear();
        IM_ASSERT(src_tmp.GlyphsList.Size == src_tmp.GlyphsCount);
    }
    for (int dst_i = 0; dst_i < dst_tmp_array.Size; dst_i++)
        dst_tmp_array[dst_i].GlyphsSet.Clear();
    dst_tmp_array.clear();

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    // (We technically don't need to zero-clear buf_rects, but let's do it for the sake of sanity)
    ImVector<stbrp_rect> buf_rects;
    ImVector<stbtt_packedchar> buf_packedchars;
    buf_rects.resize(total_glyphs_count);
    buf_packedchars.resize(total_glyphs_count);
    memset(buf_rects.Data, 0, (size_t)buf_rects.size_in_bytes());
    memset(buf_packedchars.Data, 0, (size_t)buf_packedchars.size_in_bytes());

    // 4. Gather glyphs sizes so we can pack them in our virtual canvas.
    int total_surface = 0;
    int buf_rects_out_n = 0;
    int buf_packedchars_out_n = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        src_tmp.Rects = &buf_rects[buf_rects_out_n];
        src_tmp.PackedChars = &buf_packedchars[buf_packedchars_out_n];
        buf_rects_out_n += src_tmp.GlyphsCount;
        buf_packedchars_out_n += src_tmp.GlyphsCount;

        // Convert our ranges in the format stb_truetype wants
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        src_tmp.PackRange.font_size = cfg.SizePixels;
        src_tmp.PackRange.first_unicode_codepoint_in_range = 0;
        src_tmp.PackRange.array_of_unicode_codepoints = src_tmp.GlyphsList.Data;
        src_tmp.PackRange.num_chars = src_tmp.GlyphsList.Size;
        src_tmp.PackRange.chardata_for_range = src_tmp.PackedChars;
        src_tmp.PackRange.h_oversample = (unsigned char)cfg.OversampleH;
        src_tmp.PackRange.v_oversample = (unsigned char)cfg.OversampleV;

        // Gather the sizes of all rectangles we will need to pack (this loop is based on stbtt_PackFontRangesGatherRects)
        const float scale = (cfg.SizePixels > 0) ? stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, cfg.SizePixels) : stbtt_ScaleForMappingEmToPixels(&src_tmp.FontInfo, -cfg.SizePixels);
        const int padding = atlas->TexGlyphPadding;
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsList.Size; glyph_i++)
        {
            int x0, y0, x1, y1;
            const int glyph_index_in_font = stbtt_FindGlyphIndex(&src_tmp.FontInfo, src_tmp.GlyphsList[glyph_i]);
            IM_ASSERT(glyph_index_in_font != 0);
            stbtt_GetGlyphBitmapBoxSubpixel(&src_tmp.FontInfo, glyph_index_in_font, scale * cfg.OversampleH, scale * cfg.OversampleV, 0, 0, &x0, &y0, &x1, &y1);
            src_tmp.Rects[glyph_i].w = (stbrp_coord)(x1 - x0 + padding + cfg.OversampleH - 1);
            src_tmp.Rects[glyph_i].h = (stbrp_coord)(y1 - y0 + padding + cfg.OversampleV - 1);
            total_surface += src_tmp.Rects[glyph_i].w * src_tmp.Rects[glyph_i].h;
        }
    }

    // We need a width for the skyline algorithm, any width!
    // The exact width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    // User can override TexDesiredWidth and TexGlyphPadding if they wish, otherwise we use a simple heuristic to select the width based on expected surface.
    const int surface_sqrt = (int)ImSqrt((float)total_surface) + 1;
    atlas->TexHeight = 0;
    if (atlas->TexDesiredWidth > 0)
        atlas->TexWidth = atlas->TexDesiredWidth;
    else
        atlas->TexWidth = (surface_sqrt >= 4096 * 0.7f) ? 4096 : (surface_sqrt >= 2048 * 0.7f) ? 2048 : (surface_sqrt >= 1024 * 0.7f) ? 1024 : 512;

    // 5. Start packing
    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    const int TEX_HEIGHT_MAX = 1024 * 32;
    stbtt_pack_context spc = {};
    stbtt_PackBegin(&spc, NULL, atlas->TexWidth, TEX_HEIGHT_MAX, 0, atlas->TexGlyphPadding, NULL);
    ImFontAtlasBuildPackCustomRects(atlas, spc.pack_info);

    // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture at this point.
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbrp_pack_rects((stbrp_context*)spc.pack_info, src_tmp.Rects, src_tmp.GlyphsCount);

        // Extend texture height and mark missing glyphs as non-packed so we won't render them.
        // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single if larger than TexWidth?)
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
            if (src_tmp.Rects[glyph_i].was_packed)
                atlas->TexHeight = ImMax(atlas->TexHeight, src_tmp.Rects[glyph_i].y + src_tmp.Rects[glyph_i].h);
    }

    // 7. Allocate texture
    atlas->TexHeight = (atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) ? (atlas->TexHeight + 1) : ImUpperPowerOfTwo(atlas->TexHeight);
    atlas->TexUvScale = ImVec2(1.0f / atlas->TexWidth, 1.0f / atlas->TexHeight);
    atlas->TexPixelsAlpha8 = (unsigned char*)IM_ALLOC(atlas->TexWidth * atlas->TexHeight);
    memset(atlas->TexPixelsAlpha8, 0, atlas->TexWidth * atlas->TexHeight);
    spc.pixels = atlas->TexPixelsAlpha8;
    spc.height = atlas->TexHeight;

    // 8. Render/rasterize font characters into the texture
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbtt_PackFontRangesRenderIntoRects(&spc, &src_tmp.FontInfo, &src_tmp.PackRange, 1, src_tmp.Rects);

        // Apply multiply operator
        if (cfg.RasterizerMultiply != 1.0f)
        {
            unsigned char multiply_table[256];
            ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, cfg.RasterizerMultiply);
            stbrp_rect* r = &src_tmp.Rects[0];
            for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++, r++)
                if (r->was_packed)
                    ImFontAtlasBuildMultiplyRectAlpha8(multiply_table, atlas->TexPixelsAlpha8, r->x, r->y, r->w, r->h, atlas->TexWidth * 1);
        }
        src_tmp.Rects = NULL;
    }

    // End packing
    stbtt_PackEnd(&spc);
    buf_rects.clear();

    // 9. Setup ImFont and glyphs for runtime
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFont* dst_font = cfg.DstFont; // We can have multiple input fonts writing into a same destination font (when using MergeMode=true)

        const float font_scale = stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, cfg.SizePixels);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&src_tmp.FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        const float ascent = ImFloor(unscaled_ascent * font_scale + ((unscaled_ascent > 0.0f) ? +1 : -1));
        const float descent = ImFloor(unscaled_descent * font_scale + ((unscaled_descent > 0.0f) ? +1 : -1));
        ImFontAtlasBuildSetupFont(atlas, dst_font, &cfg, ascent, descent);
        const float font_off_x = cfg.GlyphOffset.x;
        const float font_off_y = cfg.GlyphOffset.y + IM_ROUND(dst_font->Ascent);

        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
        {
            const int codepoint = src_tmp.GlyphsList[glyph_i];
            const stbtt_packedchar& pc = src_tmp.PackedChars[glyph_i];

            const float char_advance_x_org = pc.xadvance;
            const float char_advance_x_mod = ImClamp(char_advance_x_org, cfg.GlyphMinAdvanceX, cfg.GlyphMaxAdvanceX);
            float char_off_x = font_off_x;
            if (char_advance_x_org != char_advance_x_mod)
                char_off_x += cfg.PixelSnapH ? ImFloor((char_advance_x_mod - char_advance_x_org) * 0.5f) : (char_advance_x_mod - char_advance_x_org) * 0.5f;

            // Register glyph
            stbtt_aligned_quad q;
            float dummy_x = 0.0f, dummy_y = 0.0f;
            stbtt_GetPackedQuad(src_tmp.PackedChars, atlas->TexWidth, atlas->TexHeight, glyph_i, &dummy_x, &dummy_y, &q, 0);
            dst_font->AddGlyph((ImWchar)codepoint, q.x0 + char_off_x, q.y0 + font_off_y, q.x1 + char_off_x, q.y1 + font_off_y, q.s0, q.t0, q.s1, q.t1, char_advance_x_mod);
        }
    }

    // Cleanup temporary (ImVector doesn't honor destructor)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
        src_tmp_array[src_i].~ImFontBuildSrcData();

    ImFontAtlasBuildFinish(atlas);
    return true;
}

// Register default custom rectangles (this is called/shared by both the stb_truetype and the FreeType builder)
void ImFontAtlasBuildInit(ImFontAtlas* atlas)
{
    if (atlas->CustomRectIds[0] >= 0)
        return;
    if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
        atlas->CustomRectIds[0] = atlas->AddCustomRectRegular(FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * 2 + 1, FONT_ATLAS_DEFAULT_TEX_DATA_H);
    else
        atlas->CustomRectIds[0] = atlas->AddCustomRectRegular(2, 2);
}

void ImFontAtlasBuildSetupFont(ImFontAtlas* atlas, ImFont* font, ImFontConfig* font_config, float ascent, float descent)
{
    if (!font_config->MergeMode)
    {
        font->ClearOutputData();
        font->FontSize = font_config->SizePixels;
        font->ConfigData = font_config;
        font->ContainerAtlas = atlas;
        font->Ascent = ascent;
        font->Descent = descent;
    }
    font->ConfigDataCount++;
}

void ImFontAtlasBuildPackCustomRects(ImFontAtlas* atlas, void* stbrp_context_opaque)
{
    stbrp_context* pack_context = (stbrp_context*)stbrp_context_opaque;
    IM_ASSERT(pack_context != NULL);

    ImVector<ImFontAtlasCustomRect>& user_rects = atlas->CustomRects;
    IM_ASSERT(user_rects.Size >= 1); // We expect at least the default custom rects to be registered, else something went wrong.

    ImVector<stbrp_rect> pack_rects;
    pack_rects.resize(user_rects.Size);
    memset(pack_rects.Data, 0, (size_t)pack_rects.size_in_bytes());
    for (int i = 0; i < user_rects.Size; i++)
    {
        pack_rects[i].w = user_rects[i].Width;
        pack_rects[i].h = user_rects[i].Height;
    }
    stbrp_pack_rects(pack_context, &pack_rects[0], pack_rects.Size);
    for (int i = 0; i < pack_rects.Size; i++)
        if (pack_rects[i].was_packed)
        {
            user_rects[i].X = pack_rects[i].x;
            user_rects[i].Y = pack_rects[i].y;
            IM_ASSERT(pack_rects[i].w == user_rects[i].Width && pack_rects[i].h == user_rects[i].Height);
            atlas->TexHeight = ImMax(atlas->TexHeight, pack_rects[i].y + pack_rects[i].h);
        }
}

static void ImFontAtlasBuildRenderDefaultTexData(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->CustomRectIds[0] >= 0);
    IM_ASSERT(atlas->TexPixelsAlpha8 != NULL);
    ImFontAtlasCustomRect& r = atlas->CustomRects[atlas->CustomRectIds[0]];
    IM_ASSERT(r.IsPacked());

    const int w = atlas->TexWidth;
    if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
    {
        // Render/copy pixels
        IM_ASSERT(r.Width == FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * 2 + 1 && r.Height == FONT_ATLAS_DEFAULT_TEX_DATA_H);
        for (int y = 0, n = 0; y < FONT_ATLAS_DEFAULT_TEX_DATA_H; y++)
            for (int x = 0; x < FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF; x++, n++)
            {
                const int offset0 = (int)(r.X + x) + (int)(r.Y + y) * w;
                const int offset1 = offset0 + FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF + 1;
                atlas->TexPixelsAlpha8[offset0] = FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[n] == '.' ? 0xFF : 0x00;
                atlas->TexPixelsAlpha8[offset1] = FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[n] == 'X' ? 0xFF : 0x00;
            }
    } else
    {
        IM_ASSERT(r.Width == 2 && r.Height == 2);
        const int offset = (int)(r.X) + (int)(r.Y) * w;
        atlas->TexPixelsAlpha8[offset] = atlas->TexPixelsAlpha8[offset + 1] = atlas->TexPixelsAlpha8[offset + w] = atlas->TexPixelsAlpha8[offset + w + 1] = 0xFF;
    }
    atlas->TexUvWhitePixel = ImVec2((r.X + 0.5f) * atlas->TexUvScale.x, (r.Y + 0.5f) * atlas->TexUvScale.y);
}

void ImFontAtlasBuildFinish(ImFontAtlas* atlas)
{
    // Render into our custom data block
    ImFontAtlasBuildRenderDefaultTexData(atlas);

    // Register custom rectangle glyphs
    for (int i = 0; i < atlas->CustomRects.Size; i++)
    {
        const ImFontAtlasCustomRect& r = atlas->CustomRects[i];
        if (r.Font == NULL || r.GlyphID == 0)
            continue;

        IM_ASSERT(r.Font->ContainerAtlas == atlas);
        ImVec2 uv0, uv1;
        atlas->CalcCustomRectUV(&r, &uv0, &uv1);
        r.Font->AddGlyph((ImWchar)r.GlyphID, r.GlyphOffset.x, r.GlyphOffset.y, r.GlyphOffset.x + r.Width, r.GlyphOffset.y + r.Height, uv0.x, uv0.y, uv1.x, uv1.y, r.GlyphAdvanceX);
    }

    // Build all fonts lookup tables
    for (int i = 0; i < atlas->Fonts.Size; i++)
        if (atlas->Fonts[i]->DirtyLookupTables)
            atlas->Fonts[i]->BuildLookupTable();

    // Ellipsis character is required for rendering elided text. We prefer using U+2026 (horizontal ellipsis).
    // However some old fonts may contain ellipsis at U+0085. Here we auto-detect most suitable ellipsis character.
    // FIXME: Also note that 0x2026 is currently seldom included in our font ranges. Because of this we are more likely to use three individual dots.
    for (int i = 0; i < atlas->Fonts.size(); i++)
    {
        ImFont* font = atlas->Fonts[i];
        if (font->EllipsisChar != (ImWchar)-1)
            continue;
        const ImWchar ellipsis_variants[] = { (ImWchar)0x2026, (ImWchar)0x0085 };
        for (int j = 0; j < IM_ARRAYSIZE(ellipsis_variants); j++)
            if (font->FindGlyphNoFallback(ellipsis_variants[j]) != NULL) // Verify glyph exists
            {
                font->EllipsisChar = ellipsis_variants[j];
                break;
            }
    }
}

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar* ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

const ImWchar* ImFontAtlas::GetGlyphRangesKorean()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3131, 0x3163, // Korean alphabets
        0xAC00, 0xD7A3, // Korean characters
        0,
    };
    return &ranges[0];
}

const ImWchar* ImFontAtlas::GetGlyphRangesChineseFull()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

static void UnpackAccumulativeOffsetsIntoRanges(int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, ImWchar* out_ranges)
{
    for (int n = 0; n < accumulative_offsets_count; n++, out_ranges += 2)
    {
        out_ranges[0] = out_ranges[1] = (ImWchar)(base_codepoint + accumulative_offsets[n]);
        base_codepoint += accumulative_offsets[n];
    }
    out_ranges[0] = 0;
}

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas glyph ranges helpers
//-------------------------------------------------------------------------

const ImWchar* ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon()
{
    // Store 2500 regularly used characters for Simplified Chinese.
    // Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
    // This table covers 97.97% of all characters used during the month in July, 1987.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,2,1,2,2,1,1,1,1,1,5,2,1,2,3,3,3,2,2,4,1,1,1,2,1,5,2,3,1,2,1,2,1,1,2,1,1,2,2,1,4,1,1,1,1,5,10,1,2,19,2,1,2,1,2,1,2,1,2,
        1,5,1,6,3,2,1,2,2,1,1,1,4,8,5,1,1,4,1,1,3,1,2,1,5,1,2,1,1,1,10,1,1,5,2,4,6,1,4,2,2,2,12,2,1,1,6,1,1,1,4,1,1,4,6,5,1,4,2,2,4,10,7,1,1,4,2,4,
        2,1,4,3,6,10,12,5,7,2,14,2,9,1,1,6,7,10,4,7,13,1,5,4,8,4,1,1,2,28,5,6,1,1,5,2,5,20,2,2,9,8,11,2,9,17,1,8,6,8,27,4,6,9,20,11,27,6,68,2,2,1,1,
        1,2,1,2,2,7,6,11,3,3,1,1,3,1,2,1,1,1,1,1,3,1,1,8,3,4,1,5,7,2,1,4,4,8,4,2,1,2,1,1,4,5,6,3,6,2,12,3,1,3,9,2,4,3,4,1,5,3,3,1,3,7,1,5,1,1,1,1,2,
        3,4,5,2,3,2,6,1,1,2,1,7,1,7,3,4,5,15,2,2,1,5,3,22,19,2,1,1,1,1,2,5,1,1,1,6,1,1,12,8,2,9,18,22,4,1,1,5,1,16,1,2,7,10,15,1,1,6,2,4,1,2,4,1,6,
        1,1,3,2,4,1,6,4,5,1,2,1,1,2,1,10,3,1,3,2,1,9,3,2,5,7,2,19,4,3,6,1,1,1,1,1,4,3,2,1,1,1,2,5,3,1,1,1,2,2,1,1,2,1,1,2,1,3,1,1,1,3,7,1,4,1,1,2,1,
        1,2,1,2,4,4,3,8,1,1,1,2,1,3,5,1,3,1,3,4,6,2,2,14,4,6,6,11,9,1,15,3,1,28,5,2,5,5,3,1,3,4,5,4,6,14,3,2,3,5,21,2,7,20,10,1,2,19,2,4,28,28,2,3,
        2,1,14,4,1,26,28,42,12,40,3,52,79,5,14,17,3,2,2,11,3,4,6,3,1,8,2,23,4,5,8,10,4,2,7,3,5,1,1,6,3,1,2,2,2,5,28,1,1,7,7,20,5,3,29,3,17,26,1,8,4,
        27,3,6,11,23,5,3,4,6,13,24,16,6,5,10,25,35,7,3,2,3,3,14,3,6,2,6,1,4,2,3,8,2,1,1,3,3,3,4,1,1,13,2,2,4,5,2,1,14,14,1,2,2,1,4,5,2,3,1,14,3,12,
        3,17,2,16,5,1,2,1,8,9,3,19,4,2,2,4,17,25,21,20,28,75,1,10,29,103,4,1,2,1,1,4,2,4,1,2,3,24,2,2,2,1,1,2,1,3,8,1,1,1,2,1,1,3,1,1,1,6,1,5,3,1,1,
        1,3,4,1,1,5,2,1,5,6,13,9,16,1,1,1,1,3,2,3,2,4,5,2,5,2,2,3,7,13,7,2,2,1,1,1,1,2,3,3,2,1,6,4,9,2,1,14,2,14,2,1,18,3,4,14,4,11,41,15,23,15,23,
        176,1,3,4,1,1,1,1,5,3,1,2,3,7,3,1,1,2,1,2,4,4,6,2,4,1,9,7,1,10,5,8,16,29,1,1,2,2,3,1,3,5,2,4,5,4,1,1,2,2,3,3,7,1,6,10,1,17,1,44,4,6,2,1,1,6,
        5,4,2,10,1,6,9,2,8,1,24,1,2,13,7,8,8,2,1,4,1,3,1,3,3,5,2,5,10,9,4,9,12,2,1,6,1,10,1,1,7,7,4,10,8,3,1,13,4,3,1,6,1,3,5,2,1,2,17,16,5,2,16,6,
        1,4,2,1,3,3,6,8,5,11,11,1,3,3,2,4,6,10,9,5,7,4,7,4,7,1,1,4,2,1,3,6,8,7,1,6,11,5,5,3,24,9,4,2,7,13,5,1,8,82,16,61,1,1,1,4,2,2,16,10,3,8,1,1,
        6,4,2,1,3,1,1,1,4,3,8,4,2,2,1,1,1,1,1,6,3,5,1,1,4,6,9,2,1,1,1,2,1,7,2,1,6,1,5,4,4,3,1,8,1,3,3,1,3,2,2,2,2,3,1,6,1,2,1,2,1,3,7,1,8,2,1,2,1,5,
        2,5,3,5,10,1,2,1,1,3,2,5,11,3,9,3,5,1,1,5,9,1,2,1,5,7,9,9,8,1,3,3,3,6,8,2,3,2,1,1,32,6,1,2,15,9,3,7,13,1,3,10,13,2,14,1,13,10,2,1,3,10,4,15,
        2,15,15,10,1,3,9,6,9,32,25,26,47,7,3,2,3,1,6,3,4,3,2,8,5,4,1,9,4,2,2,19,10,6,2,3,8,1,2,2,4,2,1,9,4,4,4,6,4,8,9,2,3,1,1,1,1,3,5,5,1,3,8,4,6,
        2,1,4,12,1,5,3,7,13,2,5,8,1,6,1,2,5,14,6,1,5,2,4,8,15,5,1,23,6,62,2,10,1,1,8,1,2,2,10,4,2,2,9,2,1,1,3,2,3,1,5,3,3,2,1,3,8,1,1,1,11,3,1,1,4,
        3,7,1,14,1,2,3,12,5,2,5,1,6,7,5,7,14,11,1,3,1,8,9,12,2,1,11,8,4,4,2,6,10,9,13,1,1,3,1,5,1,3,2,4,4,1,18,2,3,14,11,4,29,4,2,7,1,3,13,9,2,2,5,
        3,5,20,7,16,8,5,72,34,6,4,22,12,12,28,45,36,9,7,39,9,191,1,1,1,4,11,8,4,9,2,3,22,1,1,1,1,4,17,1,7,7,1,11,31,10,2,4,8,2,3,2,1,4,2,16,4,32,2,
        3,19,13,4,9,1,5,2,14,8,1,1,3,6,19,6,5,1,16,6,2,10,8,5,1,2,3,1,5,5,1,11,6,6,1,3,3,2,6,3,8,1,1,4,10,7,5,7,7,5,8,9,2,1,3,4,1,1,3,1,3,3,2,6,16,
        1,4,6,3,1,10,6,1,3,15,2,9,2,10,25,13,9,16,6,2,2,10,11,4,3,9,1,2,6,6,5,4,30,40,1,10,7,12,14,33,6,3,6,7,3,1,3,1,11,14,4,9,5,12,11,49,18,51,31,
        140,31,2,2,1,5,1,8,1,10,1,4,4,3,24,1,10,1,3,6,6,16,3,4,5,2,1,4,2,57,10,6,22,2,22,3,7,22,6,10,11,36,18,16,33,36,2,5,5,1,1,1,4,10,1,4,13,2,7,
        5,2,9,3,4,1,7,43,3,7,3,9,14,7,9,1,11,1,1,3,7,4,18,13,1,14,1,3,6,10,73,2,2,30,6,1,11,18,19,13,22,3,46,42,37,89,7,3,16,34,2,2,3,9,1,7,1,1,1,2,
        2,4,10,7,3,10,3,9,5,28,9,2,6,13,7,3,1,3,10,2,7,2,11,3,6,21,54,85,2,1,4,2,2,1,39,3,21,2,2,5,1,1,1,4,1,1,3,4,15,1,3,2,4,4,2,3,8,2,20,1,8,7,13,
        4,1,26,6,2,9,34,4,21,52,10,4,4,1,5,12,2,11,1,7,2,30,12,44,2,30,1,1,3,6,16,9,17,39,82,2,2,24,7,1,7,3,16,9,14,44,2,1,2,1,2,3,5,2,4,1,6,7,5,3,
        2,6,1,11,5,11,2,1,18,19,8,1,3,24,29,2,1,3,5,2,2,1,13,6,5,1,46,11,3,5,1,1,5,8,2,10,6,12,6,3,7,11,2,4,16,13,2,5,1,1,2,2,5,2,28,5,2,23,10,8,4,
        4,22,39,95,38,8,14,9,5,1,13,5,4,3,13,12,11,1,9,1,27,37,2,5,4,4,63,211,95,2,2,2,1,3,5,2,1,1,2,2,1,1,1,3,2,4,1,2,1,1,5,2,2,1,1,2,3,1,3,1,1,1,
        3,1,4,2,1,3,6,1,1,3,7,15,5,3,2,5,3,9,11,4,2,22,1,6,3,8,7,1,4,28,4,16,3,3,25,4,4,27,27,1,4,1,2,2,7,1,3,5,2,28,8,2,14,1,8,6,16,25,3,3,3,14,3,
        3,1,1,2,1,4,6,3,8,4,1,1,1,2,3,6,10,6,2,3,18,3,2,5,5,4,3,1,5,2,5,4,23,7,6,12,6,4,17,11,9,5,1,1,10,5,12,1,1,11,26,33,7,3,6,1,17,7,1,5,12,1,11,
        2,4,1,8,14,17,23,1,2,1,7,8,16,11,9,6,5,2,6,4,16,2,8,14,1,11,8,9,1,1,1,9,25,4,11,19,7,2,15,2,12,8,52,7,5,19,2,16,4,36,8,1,16,8,24,26,4,6,2,9,
        5,4,36,3,28,12,25,15,37,27,17,12,59,38,5,32,127,1,2,9,17,14,4,1,2,1,1,8,11,50,4,14,2,19,16,4,17,5,4,5,26,12,45,2,23,45,104,30,12,8,3,10,2,2,
        3,3,1,4,20,7,2,9,6,15,2,20,1,3,16,4,11,15,6,134,2,5,59,1,2,2,2,1,9,17,3,26,137,10,211,59,1,2,4,1,4,1,1,1,2,6,2,3,1,1,2,3,2,3,1,3,4,4,2,3,3,
        1,4,3,1,7,2,2,3,1,2,1,3,3,3,2,2,3,2,1,3,14,6,1,3,2,9,6,15,27,9,34,145,1,1,2,1,1,1,1,2,1,1,1,1,2,2,2,3,1,2,1,1,1,2,3,5,8,3,5,2,4,1,3,2,2,2,12,
        4,1,1,1,10,4,5,1,20,4,16,1,15,9,5,12,2,9,2,5,4,2,26,19,7,1,26,4,30,12,15,42,1,6,8,172,1,1,4,2,1,1,11,2,2,4,2,1,2,1,10,8,1,2,1,4,5,1,2,5,1,8,
        4,1,3,4,2,1,6,2,1,3,4,1,2,1,1,1,1,12,5,7,2,4,3,1,1,1,3,3,6,1,2,2,3,3,3,2,1,2,12,14,11,6,6,4,12,2,8,1,7,10,1,35,7,4,13,15,4,3,23,21,28,52,5,
        26,5,6,1,7,10,2,7,53,3,2,1,1,1,2,163,532,1,10,11,1,3,3,4,8,2,8,6,2,2,23,22,4,2,2,4,2,1,3,1,3,3,5,9,8,2,1,2,8,1,10,2,12,21,20,15,105,2,3,1,1,
        3,2,3,1,1,2,5,1,4,15,11,19,1,1,1,1,5,4,5,1,1,2,5,3,5,12,1,2,5,1,11,1,1,15,9,1,4,5,3,26,8,2,1,3,1,1,15,19,2,12,1,2,5,2,7,2,19,2,20,6,26,7,5,
        2,2,7,34,21,13,70,2,128,1,1,2,1,1,2,1,1,3,2,2,2,15,1,4,1,3,4,42,10,6,1,49,85,8,1,2,1,1,4,4,2,3,6,1,5,7,4,3,211,4,1,2,1,2,5,1,2,4,2,2,6,5,6,
        10,3,4,48,100,6,2,16,296,5,27,387,2,2,3,7,16,8,5,38,15,39,21,9,10,3,7,59,13,27,21,47,5,21,6
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF  // Half-width characters
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar* ImFontAtlas::GetGlyphRangesJapanese()
{
    // 1946 common ideograms code points for Japanese
    // Sourced from http://theinstructionlimit.com/common-kanji-character-ranges-for-xna-spritefont-rendering
    // FIXME: Source a list of the revised 2136 Joyo Kanji list from 2010 and rebuild this.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,6,2,2,1,8,5,7,11,1,2,10,10,8,2,4,20,2,11,8,2,1,2,1,6,2,1,7,5,3,7,1,1,13,7,9,1,4,6,1,2,1,10,1,1,9,2,2,4,5,6,14,1,1,9,3,18,
        5,4,2,2,10,7,1,1,1,3,2,4,3,23,2,10,12,2,14,2,4,13,1,6,10,3,1,7,13,6,4,13,5,2,3,17,2,2,5,7,6,4,1,7,14,16,6,13,9,15,1,1,7,16,4,7,1,19,9,2,7,15,
        2,6,5,13,25,4,14,13,11,25,1,1,1,2,1,2,2,3,10,11,3,3,1,1,4,4,2,1,4,9,1,4,3,5,5,2,7,12,11,15,7,16,4,5,16,2,1,1,6,3,3,1,1,2,7,6,6,7,1,4,7,6,1,1,
        2,1,12,3,3,9,5,8,1,11,1,2,3,18,20,4,1,3,6,1,7,3,5,5,7,2,2,12,3,1,4,2,3,2,3,11,8,7,4,17,1,9,25,1,1,4,2,2,4,1,2,7,1,1,1,3,1,2,6,16,1,2,1,1,3,12,
        20,2,5,20,8,7,6,2,1,1,1,1,6,2,1,2,10,1,1,6,1,3,1,2,1,4,1,12,4,1,3,1,1,1,1,1,10,4,7,5,13,1,15,1,1,30,11,9,1,15,38,14,1,32,17,20,1,9,31,2,21,9,
        4,49,22,2,1,13,1,11,45,35,43,55,12,19,83,1,3,2,3,13,2,1,7,3,18,3,13,8,1,8,18,5,3,7,25,24,9,24,40,3,17,24,2,1,6,2,3,16,15,6,7,3,12,1,9,7,3,3,
        3,15,21,5,16,4,5,12,11,11,3,6,3,2,31,3,2,1,1,23,6,6,1,4,2,6,5,2,1,1,3,3,22,2,6,2,3,17,3,2,4,5,1,9,5,1,1,6,15,12,3,17,2,14,2,8,1,23,16,4,2,23,
        8,15,23,20,12,25,19,47,11,21,65,46,4,3,1,5,6,1,2,5,26,2,1,1,3,11,1,1,1,2,1,2,3,1,1,10,2,3,1,1,1,3,6,3,2,2,6,6,9,2,2,2,6,2,5,10,2,4,1,2,1,2,2,
        3,1,1,3,1,2,9,23,9,2,1,1,1,1,5,3,2,1,10,9,6,1,10,2,31,25,3,7,5,40,1,15,6,17,7,27,180,1,3,2,2,1,1,1,6,3,10,7,1,3,6,17,8,6,2,2,1,3,5,5,8,16,14,
        15,1,1,4,1,2,1,1,1,3,2,7,5,6,2,5,10,1,4,2,9,1,1,11,6,1,44,1,3,7,9,5,1,3,1,1,10,7,1,10,4,2,7,21,15,7,2,5,1,8,3,4,1,3,1,6,1,4,2,1,4,10,8,1,4,5,
        1,5,10,2,7,1,10,1,1,3,4,11,10,29,4,7,3,5,2,3,33,5,2,19,3,1,4,2,6,31,11,1,3,3,3,1,8,10,9,12,11,12,8,3,14,8,6,11,1,4,41,3,1,2,7,13,1,5,6,2,6,12,
        12,22,5,9,4,8,9,9,34,6,24,1,1,20,9,9,3,4,1,7,2,2,2,6,2,28,5,3,6,1,4,6,7,4,2,1,4,2,13,6,4,4,3,1,8,8,3,2,1,5,1,2,2,3,1,11,11,7,3,6,10,8,6,16,16,
        22,7,12,6,21,5,4,6,6,3,6,1,3,2,1,2,8,29,1,10,1,6,13,6,6,19,31,1,13,4,4,22,17,26,33,10,4,15,12,25,6,67,10,2,3,1,6,10,2,6,2,9,1,9,4,4,1,2,16,2,
        5,9,2,3,8,1,8,3,9,4,8,6,4,8,11,3,2,1,1,3,26,1,7,5,1,11,1,5,3,5,2,13,6,39,5,1,5,2,11,6,10,5,1,15,5,3,6,19,21,22,2,4,1,6,1,8,1,4,8,2,4,2,2,9,2,
        1,1,1,4,3,6,3,12,7,1,14,2,4,10,2,13,1,17,7,3,2,1,3,2,13,7,14,12,3,1,29,2,8,9,15,14,9,14,1,3,1,6,5,9,11,3,38,43,20,7,7,8,5,15,12,19,15,81,8,7,
        1,5,73,13,37,28,8,8,1,15,18,20,165,28,1,6,11,8,4,14,7,15,1,3,3,6,4,1,7,14,1,1,11,30,1,5,1,4,14,1,4,2,7,52,2,6,29,3,1,9,1,21,3,5,1,26,3,11,14,
        11,1,17,5,1,2,1,3,2,8,1,2,9,12,1,1,2,3,8,3,24,12,7,7,5,17,3,3,3,1,23,10,4,4,6,3,1,16,17,22,3,10,21,16,16,6,4,10,2,1,1,2,8,8,6,5,3,3,3,39,25,
        15,1,1,16,6,7,25,15,6,6,12,1,22,13,1,4,9,5,12,2,9,1,12,28,8,3,5,10,22,60,1,2,40,4,61,63,4,1,13,12,1,4,31,12,1,14,89,5,16,6,29,14,2,5,49,18,18,
        5,29,33,47,1,17,1,19,12,2,9,7,39,12,3,7,12,39,3,1,46,4,12,3,8,9,5,31,15,18,3,2,2,66,19,13,17,5,3,46,124,13,57,34,2,5,4,5,8,1,1,1,4,3,1,17,5,
        3,5,3,1,8,5,6,3,27,3,26,7,12,7,2,17,3,7,18,78,16,4,36,1,2,1,6,2,1,39,17,7,4,13,4,4,4,1,10,4,2,4,6,3,10,1,19,1,26,2,4,33,2,73,47,7,3,8,2,4,15,
        18,1,29,2,41,14,1,21,16,41,7,39,25,13,44,2,2,10,1,13,7,1,7,3,5,20,4,8,2,49,1,10,6,1,6,7,10,7,11,16,3,12,20,4,10,3,1,2,11,2,28,9,2,4,7,2,15,1,
        27,1,28,17,4,5,10,7,3,24,10,11,6,26,3,2,7,2,2,49,16,10,16,15,4,5,27,61,30,14,38,22,2,7,5,1,3,12,23,24,17,17,3,3,2,4,1,6,2,7,5,1,1,5,1,1,9,4,
        1,3,6,1,8,2,8,4,14,3,5,11,4,1,3,32,1,19,4,1,13,11,5,2,1,8,6,8,1,6,5,13,3,23,11,5,3,16,3,9,10,1,24,3,198,52,4,2,2,5,14,5,4,22,5,20,4,11,6,41,
        1,5,2,2,11,5,2,28,35,8,22,3,18,3,10,7,5,3,4,1,5,3,8,9,3,6,2,16,22,4,5,5,3,3,18,23,2,6,23,5,27,8,1,33,2,12,43,16,5,2,3,6,1,20,4,2,9,7,1,11,2,
        10,3,14,31,9,3,25,18,20,2,5,5,26,14,1,11,17,12,40,19,9,6,31,83,2,7,9,19,78,12,14,21,76,12,113,79,34,4,1,1,61,18,85,10,2,2,13,31,11,50,6,33,159,
        179,6,6,7,4,4,2,4,2,5,8,7,20,32,22,1,3,10,6,7,28,5,10,9,2,77,19,13,2,5,1,4,4,7,4,13,3,9,31,17,3,26,2,6,6,5,4,1,7,11,3,4,2,1,6,2,20,4,1,9,2,6,
        3,7,1,1,1,20,2,3,1,6,2,3,6,2,4,8,1,5,13,8,4,11,23,1,10,6,2,1,3,21,2,2,4,24,31,4,10,10,2,5,192,15,4,16,7,9,51,1,2,1,1,5,1,1,2,1,3,5,3,1,3,4,1,
        3,1,3,3,9,8,1,2,2,2,4,4,18,12,92,2,10,4,3,14,5,25,16,42,4,14,4,2,21,5,126,30,31,2,1,5,13,3,22,5,6,6,20,12,1,14,12,87,3,19,1,8,2,9,9,3,3,23,2,
        3,7,6,3,1,2,3,9,1,3,1,6,3,2,1,3,11,3,1,6,10,3,2,3,1,2,1,5,1,1,11,3,6,4,1,7,2,1,2,5,5,34,4,14,18,4,19,7,5,8,2,6,79,1,5,2,14,8,2,9,2,1,36,28,16,
        4,1,1,1,2,12,6,42,39,16,23,7,15,15,3,2,12,7,21,64,6,9,28,8,12,3,3,41,59,24,51,55,57,294,9,9,2,6,2,15,1,2,13,38,90,9,9,9,3,11,7,1,1,1,5,6,3,2,
        1,2,2,3,8,1,4,4,1,5,7,1,4,3,20,4,9,1,1,1,5,5,17,1,5,2,6,2,4,1,4,5,7,3,18,11,11,32,7,5,4,7,11,127,8,4,3,3,1,10,1,1,6,21,14,1,16,1,7,1,3,6,9,65,
        51,4,3,13,3,10,1,1,12,9,21,110,3,19,24,1,1,10,62,4,1,29,42,78,28,20,18,82,6,3,15,6,84,58,253,15,155,264,15,21,9,14,7,58,40,39,
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF  // Half-width characters
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar* ImFontAtlas::GetGlyphRangesCyrillic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

const ImWchar* ImFontAtlas::GetGlyphRangesThai()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x2010, 0x205E, // Punctuations
        0x0E00, 0x0E7F, // Thai
        0,
    };
    return &ranges[0];
}

const ImWchar* ImFontAtlas::GetGlyphRangesVietnamese()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0102, 0x0103,
        0x0110, 0x0111,
        0x0128, 0x0129,
        0x0168, 0x0169,
        0x01A0, 0x01A1,
        0x01AF, 0x01B0,
        0x1EA0, 0x1EF9,
        0,
    };
    return &ranges[0];
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontGlyphRangesBuilder
//-----------------------------------------------------------------------------

void ImFontGlyphRangesBuilder::AddText(const char* text, const char* text_end)
{
    while (text_end ? (text < text_end) : *text)
    {
        unsigned int c = 0;
        int c_len = ImTextCharFromUtf8(&c, text, text_end);
        text += c_len;
        if (c_len == 0)
            break;
        AddChar((ImWchar)c);
    }
}

void ImFontGlyphRangesBuilder::AddRanges(const ImWchar* ranges)
{
    for (; ranges[0]; ranges += 2)
        for (ImWchar c = ranges[0]; c <= ranges[1]; c++)
            AddChar(c);
}

void ImFontGlyphRangesBuilder::BuildRanges(ImVector<ImWchar>* out_ranges)
{
    const int max_codepoint = IM_UNICODE_CODEPOINT_MAX;
    for (int n = 0; n <= max_codepoint; n++)
        if (GetBit(n))
        {
            out_ranges->push_back((ImWchar)n);
            while (n < max_codepoint && GetBit(n + 1))
                n++;
            out_ranges->push_back((ImWchar)n);
        }
    out_ranges->push_back(0);
}

//-----------------------------------------------------------------------------
// [SECTION] ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    FallbackChar = (ImWchar)'?';
    EllipsisChar = (ImWchar)-1;
    DisplayOffset = ImVec2(0.0f, 0.0f);
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    ConfigData = NULL;
    ConfigDataCount = 0;
    DirtyLookupTables = false;
    Scale = 1.0f;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
    memset(Used4kPagesMap, 0, sizeof(Used4kPagesMap));
}

ImFont::~ImFont()
{
    ClearOutputData();
}

void    ImFont::ClearOutputData()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    Glyphs.clear();
    IndexAdvanceX.clear();
    IndexLookup.clear();
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    DirtyLookupTables = true;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
}

void ImFont::BuildLookupTable()
{
    int max_codepoint = 0;
    for (int i = 0; i != Glyphs.Size; i++)
        max_codepoint = ImMax(max_codepoint, (int)Glyphs[i].Codepoint);

    // Build lookup table
    IM_ASSERT(Glyphs.Size < 0xFFFF); // -1 is reserved
    IndexAdvanceX.clear();
    IndexLookup.clear();
    DirtyLookupTables = false;
    memset(Used4kPagesMap, 0, sizeof(Used4kPagesMap));
    GrowIndex(max_codepoint + 1);
    for (int i = 0; i < Glyphs.Size; i++)
    {
        int codepoint = (int)Glyphs[i].Codepoint;
        IndexAdvanceX[codepoint] = Glyphs[i].AdvanceX;
        IndexLookup[codepoint] = (ImWchar)i;

        // Mark 4K page as used
        const int page_n = codepoint / 4096;
        Used4kPagesMap[page_n >> 3] |= 1 << (page_n & 7);
    }

    // Create a glyph to handle TAB
    // FIXME: Needs proper TAB handling but it needs to be contextualized (or we could arbitrary say that each string starts at "column 0" ?)
    if (FindGlyph((ImWchar)' '))
    {
        if (Glyphs.back().Codepoint != '\t')   // So we can call this function multiple times (FIXME: Flaky)
            Glyphs.resize(Glyphs.Size + 1);
        ImFontGlyph& tab_glyph = Glyphs.back();
        tab_glyph = *FindGlyph((ImWchar)' ');
        tab_glyph.Codepoint = '\t';
        tab_glyph.AdvanceX *= IM_TABSIZE;
        IndexAdvanceX[(int)tab_glyph.Codepoint] = (float)tab_glyph.AdvanceX;
        IndexLookup[(int)tab_glyph.Codepoint] = (ImWchar)(Glyphs.Size - 1);
    }

    // Mark special glyphs as not visible (note that AddGlyph already mark as non-visible glyphs with zero-size polygons)
    SetGlyphVisible((ImWchar)' ', false);
    SetGlyphVisible((ImWchar)'\t', false);

    // Setup fall-backs
    FallbackGlyph = FindGlyphNoFallback(FallbackChar);
    FallbackAdvanceX = FallbackGlyph ? FallbackGlyph->AdvanceX : 0.0f;
    for (int i = 0; i < max_codepoint + 1; i++)
        if (IndexAdvanceX[i] < 0.0f)
            IndexAdvanceX[i] = FallbackAdvanceX;
}

// API is designed this way to avoid exposing the 4K page size
// e.g. use with IsGlyphRangeUnused(0, 255)
bool ImFont::IsGlyphRangeUnused(unsigned int c_begin, unsigned int c_last)
{
    unsigned int page_begin = (c_begin / 4096);
    unsigned int page_last = (c_last / 4096);
    for (unsigned int page_n = page_begin; page_n <= page_last; page_n++)
        if ((page_n >> 3) < sizeof(Used4kPagesMap))
            if (Used4kPagesMap[page_n >> 3] & (1 << (page_n & 7)))
                return false;
    return true;
}

void ImFont::SetGlyphVisible(ImWchar c, bool visible)
{
    if (ImFontGlyph* glyph = (ImFontGlyph*)(void*)FindGlyph((ImWchar)c))
        glyph->Visible = visible ? 1 : 0;
}

void ImFont::SetFallbackChar(ImWchar c)
{
    FallbackChar = c;
    BuildLookupTable();
}

void ImFont::GrowIndex(int new_size)
{
    IM_ASSERT(IndexAdvanceX.Size == IndexLookup.Size);
    if (new_size <= IndexLookup.Size)
        return;
    IndexAdvanceX.resize(new_size, -1.0f);
    IndexLookup.resize(new_size, (ImWchar)-1);
}

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
void ImFont::AddGlyph(ImWchar codepoint, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, float advance_x)
{
    Glyphs.resize(Glyphs.Size + 1);
    ImFontGlyph& glyph = Glyphs.back();
    glyph.Codepoint = (unsigned int)codepoint;
    glyph.Visible = (x0 != x1) && (y0 != y1);
    glyph.X0 = x0;
    glyph.Y0 = y0;
    glyph.X1 = x1;
    glyph.Y1 = y1;
    glyph.U0 = u0;
    glyph.V0 = v0;
    glyph.U1 = u1;
    glyph.V1 = v1;
    glyph.AdvanceX = advance_x + ConfigData->GlyphExtraSpacing.x;  // Bake spacing into AdvanceX

    if (ConfigData->PixelSnapH)
        glyph.AdvanceX = IM_ROUND(glyph.AdvanceX);

    // Compute rough surface usage metrics (+1 to account for average padding, +0.99 to round)
    DirtyLookupTables = true;
    MetricsTotalSurface += (int)((glyph.U1 - glyph.U0) * ContainerAtlas->TexWidth + 1.99f) * (int)((glyph.V1 - glyph.V0) * ContainerAtlas->TexHeight + 1.99f);
}

void ImFont::AddRemapChar(ImWchar dst, ImWchar src, bool overwrite_dst)
{
    IM_ASSERT(IndexLookup.Size > 0);    // Currently this can only be called AFTER the font has been built, aka after calling ImFontAtlas::GetTexDataAs*() function.
    unsigned int index_size = (unsigned int)IndexLookup.Size;

    if (dst < index_size && IndexLookup.Data[dst] == (ImWchar)-1 && !overwrite_dst) // 'dst' already exists
        return;
    if (src >= index_size && dst >= index_size) // both 'dst' and 'src' don't exist -> no-op
        return;

    GrowIndex(dst + 1);
    IndexLookup[dst] = (src < index_size) ? IndexLookup.Data[src] : (ImWchar)-1;
    IndexAdvanceX[dst] = (src < index_size) ? IndexAdvanceX.Data[src] : 1.0f;
}

const ImFontGlyph* ImFont::FindGlyph(ImWchar c) const
{
    if (c >= (size_t)IndexLookup.Size)
        return FallbackGlyph;
    const ImWchar i = IndexLookup.Data[c];
    if (i == (ImWchar)-1)
        return FallbackGlyph;
    return &Glyphs.Data[i];
}

const ImFontGlyph* ImFont::FindGlyphNoFallback(ImWchar c) const
{
    if (c >= (size_t)IndexLookup.Size)
        return NULL;
    const ImWchar i = IndexLookup.Data[c];
    if (i == (ImWchar)-1)
        return NULL;
    return &Glyphs.Data[i];
}

const char* ImFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width) const
{
    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);
        if (c == 0)
            break;

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX);
        if (ImCharIsBlankW(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        } else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            } else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width > wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // FIXME-OPT: Need to avoid this.

    const float line_height = size;
    const float scale = size / FontSize;

    ImVec2 text_size = ImVec2(0, 0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width);
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsBlankA(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        } else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                text_size.x = ImMax(text_size.x, line_width);
                text_size.y += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = ((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX) * scale;
        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (line_width > 0 || text_size.y == 0.0f)
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

void ImFont::RenderChar(ImDrawList* draw_list, float size, ImVec2 pos, ImU32 col, ImWchar c) const
{
    const ImFontGlyph* glyph = FindGlyph(c);
    if (!glyph || !glyph->Visible)
        return;
    float scale = (size >= 0.0f) ? (size / FontSize) : 1.0f;
    pos.x = IM_FLOOR(pos.x + DisplayOffset.x);
    pos.y = IM_FLOOR(pos.y + DisplayOffset.y);
    draw_list->PrimReserve(6, 4);
    draw_list->PrimRectUV(ImVec2(pos.x + glyph->X0 * scale, pos.y + glyph->Y0 * scale), ImVec2(pos.x + glyph->X1 * scale, pos.y + glyph->Y1 * scale), ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V1), col);
}

void ImFont::RenderText(ImDrawList* draw_list, float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, bool cpu_fine_clip) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // ImGui:: functions generally already provides a valid text_end, so this is merely to handle direct calls.

    // Align to be pixel perfect
    pos.x = IM_FLOOR(pos.x + DisplayOffset.x);
    pos.y = IM_FLOOR(pos.y + DisplayOffset.y);
    float x = pos.x;
    float y = pos.y;
    if (y > clip_rect.w)
        return;

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;
    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    // Fast-forward to first visible line
    const char* s = text_begin;
    if (y + line_height < clip_rect.y && !word_wrap_enabled)
        while (y + line_height < clip_rect.y && s < text_end)
        {
            s = (const char*)memchr(s, '\n', text_end - s);
            s = s ? s + 1 : text_end;
            y += line_height;
        }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
    if (text_end - s > 10000 && !word_wrap_enabled)
    {
        const char* s_end = s;
        float y_end = y;
        while (y_end < clip_rect.w && s_end < text_end)
        {
            s_end = (const char*)memchr(s_end, '\n', text_end - s_end);
            s_end = s_end ? s_end + 1 : text_end;
            y_end += line_height;
        }
        text_end = s_end;
    }
    if (s == text_end)
        return;

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int vtx_count_max = (int)(text_end - s) * 4;
    const int idx_count_max = (int)(text_end - s) * 6;
    const int idx_expected_size = draw_list->IdxBuffer.Size + idx_count_max;
    draw_list->PrimReserve(idx_count_max, vtx_count_max);

    ImDrawVert* vtx_write = draw_list->_VtxWritePtr;
    ImDrawIdx* idx_write = draw_list->_IdxWritePtr;
    unsigned int vtx_current_idx = draw_list->_VtxCurrentIdx;

    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x));
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                x = pos.x;
                y += line_height;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsBlankA(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        } else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                x = pos.x;
                y += line_height;
                if (y > clip_rect.w)
                    break; // break out of main loop
                continue;
            }
            if (c == '\r')
                continue;
        }

        const ImFontGlyph* glyph = FindGlyph((ImWchar)c);
        if (glyph == NULL)
            continue;

        float char_width = glyph->AdvanceX * scale;
        if (glyph->Visible)
        {
            // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
            float x1 = x + glyph->X0 * scale;
            float x2 = x + glyph->X1 * scale;
            float y1 = y + glyph->Y0 * scale;
            float y2 = y + glyph->Y1 * scale;
            if (x1 <= clip_rect.z && x2 >= clip_rect.x)
            {
                // Render a character
                float u1 = glyph->U0;
                float v1 = glyph->V0;
                float u2 = glyph->U1;
                float v2 = glyph->V1;

                // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                if (cpu_fine_clip)
                {
                    if (x1 < clip_rect.x)
                    {
                        u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
                        x1 = clip_rect.x;
                    }
                    if (y1 < clip_rect.y)
                    {
                        v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
                        y1 = clip_rect.y;
                    }
                    if (x2 > clip_rect.z)
                    {
                        u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
                        x2 = clip_rect.z;
                    }
                    if (y2 > clip_rect.w)
                    {
                        v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
                        y2 = clip_rect.w;
                    }
                    if (y1 >= y2)
                    {
                        x += char_width;
                        continue;
                    }
                }

                // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug builds. Inlined here:
                {
                    idx_write[0] = (ImDrawIdx)(vtx_current_idx); idx_write[1] = (ImDrawIdx)(vtx_current_idx + 1); idx_write[2] = (ImDrawIdx)(vtx_current_idx + 2);
                    idx_write[3] = (ImDrawIdx)(vtx_current_idx); idx_write[4] = (ImDrawIdx)(vtx_current_idx + 2); idx_write[5] = (ImDrawIdx)(vtx_current_idx + 3);
                    vtx_write[0].pos.x = x1; vtx_write[0].pos.y = y1; vtx_write[0].col = col; vtx_write[0].uv.x = u1; vtx_write[0].uv.y = v1;
                    vtx_write[1].pos.x = x2; vtx_write[1].pos.y = y1; vtx_write[1].col = col; vtx_write[1].uv.x = u2; vtx_write[1].uv.y = v1;
                    vtx_write[2].pos.x = x2; vtx_write[2].pos.y = y2; vtx_write[2].col = col; vtx_write[2].uv.x = u2; vtx_write[2].uv.y = v2;
                    vtx_write[3].pos.x = x1; vtx_write[3].pos.y = y2; vtx_write[3].col = col; vtx_write[3].uv.x = u1; vtx_write[3].uv.y = v2;
                    vtx_write += 4;
                    vtx_current_idx += 4;
                    idx_write += 6;
                }
            }
        }
        x += char_width;
    }

    // Give back unused vertices (clipped ones, blanks) ~ this is essentially a PrimUnreserve() action.
    draw_list->VtxBuffer.Size = (int)(vtx_write - draw_list->VtxBuffer.Data); // Same as calling shrink()
    draw_list->IdxBuffer.Size = (int)(idx_write - draw_list->IdxBuffer.Data);
    draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ElemCount -= (idx_expected_size - draw_list->IdxBuffer.Size);
    draw_list->_VtxWritePtr = vtx_write;
    draw_list->_IdxWritePtr = idx_write;
    draw_list->_VtxCurrentIdx = vtx_current_idx;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGui Internal Render Helpers
//-----------------------------------------------------------------------------
// Vaguely redesigned to stop accessing ImGui global state:
// - RenderArrow()
// - RenderBullet()
// - RenderCheckMark()
// - RenderMouseCursor()
// - RenderArrowPointingAt()
// - RenderRectFilledRangeH()
//-----------------------------------------------------------------------------
// Function in need of a redesign (legacy mess)
// - RenderColorRectWithAlphaCheckerboard()
//-----------------------------------------------------------------------------

// Render an arrow aimed to be aligned with text (p_min is a position in the same space text would be positioned). To e.g. denote expanded/collapsed state
void ImGui::RenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
{
    const float h = draw_list->_Data->FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.50f * scale);

    ImVec2 a, b, c;
    switch (dir)
    {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
        if (dir == ImGuiDir_Up) r = -r;
        a = ImVec2(+0.000f, +0.750f) * r;
        b = ImVec2(-0.866f, -0.750f) * r;
        c = ImVec2(+0.866f, -0.750f) * r;
        break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
        if (dir == ImGuiDir_Left) r = -r;
        a = ImVec2(+0.750f, +0.000f) * r;
        b = ImVec2(-0.750f, +0.866f) * r;
        c = ImVec2(-0.750f, -0.866f) * r;
        break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT:
        IM_ASSERT(0);
        break;
    }
    draw_list->AddTriangleFilled(center + a, center + b, center + c, col);
}

void ImGui::RenderBullet(ImDrawList* draw_list, ImVec2 pos, ImU32 col)
{
    draw_list->AddCircleFilled(pos, draw_list->_Data->FontSize * 0.20f, col, 8);
}

void ImGui::RenderCheckMark(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float sz)
{
    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness * 0.5f;
    pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third * 0.5f;
    draw_list->PathLineTo(ImVec2(bx - third, by - third));
    draw_list->PathLineTo(ImVec2(bx, by));
    draw_list->PathLineTo(ImVec2(bx + third * 2.0f, by - third * 2.0f));
    draw_list->PathStroke(col, false, thickness);
}

void ImGui::RenderMouseCursor(ImDrawList* draw_list, ImVec2 pos, float scale, ImGuiMouseCursor mouse_cursor, ImU32 col_fill, ImU32 col_border, ImU32 col_shadow)
{
    if (mouse_cursor == ImGuiMouseCursor_None)
        return;
    IM_ASSERT(mouse_cursor > ImGuiMouseCursor_None && mouse_cursor < ImGuiMouseCursor_COUNT);

    ImFontAtlas* font_atlas = draw_list->_Data->Font->ContainerAtlas;
    ImVec2 offset, size, uv[4];
    if (font_atlas->GetMouseCursorTexData(mouse_cursor, &offset, &size, &uv[0], &uv[2]))
    {
        pos -= offset;
        const ImTextureID tex_id = font_atlas->TexID;
        draw_list->PushTextureID(tex_id);
        draw_list->AddImage(tex_id, pos + ImVec2(1, 0) * scale, pos + ImVec2(1, 0) * scale + size * scale, uv[2], uv[3], col_shadow);
        draw_list->AddImage(tex_id, pos + ImVec2(2, 0) * scale, pos + ImVec2(2, 0) * scale + size * scale, uv[2], uv[3], col_shadow);
        draw_list->AddImage(tex_id, pos, pos + size * scale, uv[2], uv[3], col_border);
        draw_list->AddImage(tex_id, pos, pos + size * scale, uv[0], uv[1], col_fill);
        draw_list->PopTextureID();
    }
}

// Render an arrow. 'pos' is position of the arrow tip. half_sz.x is length from base to tip. half_sz.y is length on each side.
void ImGui::RenderArrowPointingAt(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 col)
{
    switch (direction)
    {
    case ImGuiDir_Left:  draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Right: draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_Up:    draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Down:  draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_None: case ImGuiDir_COUNT: break; // Fix warnings
    }
}

static inline float ImAcos01(float x)
{
    if (x <= 0.0f) return IM_PI * 0.5f;
    if (x >= 1.0f) return 0.0f;
    return ImAcos(x);
    //return (-0.69813170079773212f * x * x - 0.87266462599716477f) * x + 1.5707963267948966f; // Cheap approximation, may be enough for what we do.
}

// FIXME: Cleanup and move code to ImDrawList.
void ImGui::RenderRectFilledRangeH(ImDrawList* draw_list, const ImRect& rect, ImU32 col, float x_start_norm, float x_end_norm, float rounding)
{
    if (x_end_norm == x_start_norm)
        return;
    if (x_start_norm > x_end_norm)
        ImSwap(x_start_norm, x_end_norm);

    ImVec2 p0 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_start_norm), rect.Min.y);
    ImVec2 p1 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_end_norm), rect.Max.y);
    if (rounding == 0.0f)
    {
        draw_list->AddRectFilled(p0, p1, col, 0.0f);
        return;
    }

    rounding = ImClamp(ImMin((rect.Max.x - rect.Min.x) * 0.5f, (rect.Max.y - rect.Min.y) * 0.5f) - 1.0f, 0.0f, rounding);
    const float inv_rounding = 1.0f / rounding;
    const float arc0_b = ImAcos01(1.0f - (p0.x - rect.Min.x) * inv_rounding);
    const float arc0_e = ImAcos01(1.0f - (p1.x - rect.Min.x) * inv_rounding);
    const float half_pi = IM_PI * 0.5f; // We will == compare to this because we know this is the exact value ImAcos01 can return.
    const float x0 = ImMax(p0.x, rect.Min.x + rounding);
    if (arc0_b == arc0_e)
    {
        draw_list->PathLineTo(ImVec2(x0, p1.y));
        draw_list->PathLineTo(ImVec2(x0, p0.y));
    } else if (arc0_b == 0.0f && arc0_e == half_pi)
    {
        draw_list->PathArcToFast(ImVec2(x0, p1.y - rounding), rounding, 3, 6); // BL
        draw_list->PathArcToFast(ImVec2(x0, p0.y + rounding), rounding, 6, 9); // TR
    } else
    {
        draw_list->PathArcTo(ImVec2(x0, p1.y - rounding), rounding, IM_PI - arc0_e, IM_PI - arc0_b, 3); // BL
        draw_list->PathArcTo(ImVec2(x0, p0.y + rounding), rounding, IM_PI + arc0_b, IM_PI + arc0_e, 3); // TR
    }
    if (p1.x > rect.Min.x + rounding)
    {
        const float arc1_b = ImAcos01(1.0f - (rect.Max.x - p1.x) * inv_rounding);
        const float arc1_e = ImAcos01(1.0f - (rect.Max.x - p0.x) * inv_rounding);
        const float x1 = ImMin(p1.x, rect.Max.x - rounding);
        if (arc1_b == arc1_e)
        {
            draw_list->PathLineTo(ImVec2(x1, p0.y));
            draw_list->PathLineTo(ImVec2(x1, p1.y));
        } else if (arc1_b == 0.0f && arc1_e == half_pi)
        {
            draw_list->PathArcToFast(ImVec2(x1, p0.y + rounding), rounding, 9, 12); // TR
            draw_list->PathArcToFast(ImVec2(x1, p1.y - rounding), rounding, 0, 3);  // BR
        } else
        {
            draw_list->PathArcTo(ImVec2(x1, p0.y + rounding), rounding, -arc1_e, -arc1_b, 3); // TR
            draw_list->PathArcTo(ImVec2(x1, p1.y - rounding), rounding, +arc1_b, +arc1_e, 3); // BR
        }
    }
    draw_list->PathFillConvex(col);
}

// Helper for ColorPicker4()
// NB: This is rather brittle and will show artifact when rounding this enabled if rounded corners overlap multiple cells. Caller currently responsible for avoiding that.
// Spent a non reasonable amount of time trying to getting this right for ColorButton with rounding+anti-aliasing+ImGuiColorEditFlags_HalfAlphaPreview flag + various grid sizes and offsets, and eventually gave up... probably more reasonable to disable rounding altogether.
// FIXME: uses ImGui::GetColorU32
void ImGui::RenderColorRectWithAlphaCheckerboard(ImDrawList* draw_list, ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding, int rounding_corners_flags)
{
    if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF)
    {
        ImU32 col_bg1 = ImGui::GetColorU32(ImAlphaBlendColors(IM_COL32(204, 204, 204, 255), col));
        ImU32 col_bg2 = ImGui::GetColorU32(ImAlphaBlendColors(IM_COL32(128, 128, 128, 255), col));
        draw_list->AddRectFilled(p_min, p_max, col_bg1, rounding, rounding_corners_flags);

        int yi = 0;
        for (float y = p_min.y + grid_off.y; y < p_max.y; y += grid_step, yi++)
        {
            float y1 = ImClamp(y, p_min.y, p_max.y), y2 = ImMin(y + grid_step, p_max.y);
            if (y2 <= y1)
                continue;
            for (float x = p_min.x + grid_off.x + (yi & 1) * grid_step; x < p_max.x; x += grid_step * 2.0f)
            {
                float x1 = ImClamp(x, p_min.x, p_max.x), x2 = ImMin(x + grid_step, p_max.x);
                if (x2 <= x1)
                    continue;
                int rounding_corners_flags_cell = 0;
                if (y1 <= p_min.y) { if (x1 <= p_min.x) rounding_corners_flags_cell |= ImDrawCornerFlags_TopLeft; if (x2 >= p_max.x) rounding_corners_flags_cell |= ImDrawCornerFlags_TopRight; }
                if (y2 >= p_max.y) { if (x1 <= p_min.x) rounding_corners_flags_cell |= ImDrawCornerFlags_BotLeft; if (x2 >= p_max.x) rounding_corners_flags_cell |= ImDrawCornerFlags_BotRight; }
                rounding_corners_flags_cell &= rounding_corners_flags;
                draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col_bg2, rounding_corners_flags_cell ? rounding : 0.0f, rounding_corners_flags_cell);
            }
        }
    } else
    {
        draw_list->AddRectFilled(p_min, p_max, col, rounding, rounding_corners_flags);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Decompression code
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array and encoded as base85.
// Use the program in misc/fonts/binary_to_compressed_c.cpp to create the array from a TTF file.
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
// Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(const unsigned char* input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char* stb__barrier_out_e, * stb__barrier_out_b;
static const unsigned char* stb__barrier_in_b;
static unsigned char* stb__dout;
static void stb__match(const unsigned char* data, unsigned int length)
{
    // INVERSE of memmove... write each byte before copying the next...
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_out_b) { stb__dout = stb__barrier_out_e + 1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(const unsigned char* data, unsigned int length)
{
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_in_b) { stb__dout = stb__barrier_out_e + 1; return; }
    memcpy(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static const unsigned char* stb_decompress_token(const unsigned char* i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout - i[1] - 1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout - (stb__in2(0) - 0x4000 + 1), i[2] + 1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i + 1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout - (stb__in3(0) - 0x180000 + 1), i[3] + 1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout - (stb__in3(0) - 0x100000 + 1), stb__in2(3) + 1), i += 5;
        else if (*i >= 0x08)  stb__lit(i + 2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i + 3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout - (stb__in3(1) + 1), i[4] + 1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout - (stb__in3(1) + 1), stb__in2(4) + 1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char* buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen = buflen % 5552;

    unsigned long i;
    while (buflen) {
        for (i = 0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char* output, const unsigned char* i, unsigned int /*length*/)
{
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    const unsigned int olen = stb_decompress_length(i);
    stb__barrier_in_b = i;
    stb__barrier_out_e = output + olen;
    stb__barrier_out_b = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        const unsigned char* old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int)stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen);
        if (stb__dout > output + olen)
            return 0;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Default font data (ProggyClean.ttf)
//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
// Download and more information at http://upperbounds.net
//-----------------------------------------------------------------------------
// File: 'ProggyClean.ttf' (41208 bytes)
// Exported using misc/fonts/binary_to_compressed_c.cpp (with compression + base85 string encoding).
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
//-----------------------------------------------------------------------------
static const char proggy_clean_ttf_compressed_data_base85[11980 + 1] =
"7])#######hV0qs'/###[),##/l:$#Q6>##5[n42>c-TH`->>#/e>11NNV=Bv(*:.F?uu#(gRU.o0XGH`$vhLG1hxt9?W`#,5LsCp#-i>.r$<$6pD>Lb';9Crc6tgXmKVeU2cD4Eo3R/"
"2*>]b(MC;$jPfY.;h^`IWM9<Lh2TlS+f-s$o6Q<BWH`YiU.xfLq$N;$0iR/GX:U(jcW2p/W*q?-qmnUCI;jHSAiFWM.R*kU@C=GH?a9wp8f$e.-4^Qg1)Q-GL(lf(r/7GrRgwV%MS=C#"
"`8ND>Qo#t'X#(v#Y9w0#1D$CIf;W'#pWUPXOuxXuU(H9M(1<q-UE31#^-V'8IRUo7Qf./L>=Ke$$'5F%)]0^#0X@U.a<r:QLtFsLcL6##lOj)#.Y5<-R&KgLwqJfLgN&;Q?gI^#DY2uL"
"i@^rMl9t=cWq6##weg>$FBjVQTSDgEKnIS7EM9>ZY9w0#L;>>#Mx&4Mvt//L[MkA#W@lK.N'[0#7RL_&#w+F%HtG9M#XL`N&.,GM4Pg;-<nLENhvx>-VsM.M0rJfLH2eTM`*oJMHRC`N"
"kfimM2J,W-jXS:)r0wK#@Fge$U>`w'N7G#$#fB#$E^$#:9:hk+eOe--6x)F7*E%?76%^GMHePW-Z5l'&GiF#$956:rS?dA#fiK:)Yr+`&#0j@'DbG&#^$PG.Ll+DNa<XCMKEV*N)LN/N"
"*b=%Q6pia-Xg8I$<MR&,VdJe$<(7G;Ckl'&hF;;$<_=X(b.RS%%)###MPBuuE1V:v&cX&#2m#(&cV]`k9OhLMbn%s$G2,B$BfD3X*sp5#l,$R#]x_X1xKX%b5U*[r5iMfUo9U`N99hG)"
"tm+/Us9pG)XPu`<0s-)WTt(gCRxIg(%6sfh=ktMKn3j)<6<b5Sk_/0(^]AaN#(p/L>&VZ>1i%h1S9u5o@YaaW$e+b<TWFn/Z:Oh(Cx2$lNEoN^e)#CFY@@I;BOQ*sRwZtZxRcU7uW6CX"
"ow0i(?$Q[cjOd[P4d)]>ROPOpxTO7Stwi1::iB1q)C_=dV26J;2,]7op$]uQr@_V7$q^%lQwtuHY]=DX,n3L#0PHDO4f9>dC@O>HBuKPpP*E,N+b3L#lpR/MrTEH.IAQk.a>D[.e;mc."
"x]Ip.PH^'/aqUO/$1WxLoW0[iLA<QT;5HKD+@qQ'NQ(3_PLhE48R.qAPSwQ0/WK?Z,[x?-J;jQTWA0X@KJ(_Y8N-:/M74:/-ZpKrUss?d#dZq]DAbkU*JqkL+nwX@@47`5>w=4h(9.`G"
"CRUxHPeR`5Mjol(dUWxZa(>STrPkrJiWx`5U7F#.g*jrohGg`cg:lSTvEY/EV_7H4Q9[Z%cnv;JQYZ5q.l7Zeas:HOIZOB?G<Nald$qs]@]L<J7bR*>gv:[7MI2k).'2($5FNP&EQ(,)"
"U]W]+fh18.vsai00);D3@4ku5P?DP8aJt+;qUM]=+b'8@;mViBKx0DE[-auGl8:PJ&Dj+M6OC]O^((##]`0i)drT;-7X`=-H3[igUnPG-NZlo.#k@h#=Ork$m>a>$-?Tm$UV(?#P6YY#"
"'/###xe7q.73rI3*pP/$1>s9)W,JrM7SN]'/4C#v$U`0#V.[0>xQsH$fEmPMgY2u7Kh(G%siIfLSoS+MK2eTM$=5,M8p`A.;_R%#u[K#$x4AG8.kK/HSB==-'Ie/QTtG?-.*^N-4B/ZM"
"_3YlQC7(p7q)&](`6_c)$/*JL(L-^(]$wIM`dPtOdGA,U3:w2M-0<q-]L_?^)1vw'.,MRsqVr.L;aN&#/EgJ)PBc[-f>+WomX2u7lqM2iEumMTcsF?-aT=Z-97UEnXglEn1K-bnEO`gu"
"Ft(c%=;Am_Qs@jLooI&NX;]0#j4#F14;gl8-GQpgwhrq8'=l_f-b49'UOqkLu7-##oDY2L(te+Mch&gLYtJ,MEtJfLh'x'M=$CS-ZZ%P]8bZ>#S?YY#%Q&q'3^Fw&?D)UDNrocM3A76/"
"/oL?#h7gl85[qW/NDOk%16ij;+:1a'iNIdb-ou8.P*w,v5#EI$TWS>Pot-R*H'-SEpA:g)f+O$%%`kA#G=8RMmG1&O`>to8bC]T&$,n.LoO>29sp3dt-52U%VM#q7'DHpg+#Z9%H[K<L"
"%a2E-grWVM3@2=-k22tL]4$##6We'8UJCKE[d_=%wI;'6X-GsLX4j^SgJ$##R*w,vP3wK#iiW&#*h^D&R?jp7+/u&#(AP##XU8c$fSYW-J95_-Dp[g9wcO&#M-h1OcJlc-*vpw0xUX&#"
"OQFKNX@QI'IoPp7nb,QU//MQ&ZDkKP)X<WSVL(68uVl&#c'[0#(s1X&xm$Y%B7*K:eDA323j998GXbA#pwMs-jgD$9QISB-A_(aN4xoFM^@C58D0+Q+q3n0#3U1InDjF682-SjMXJK)("
"h$hxua_K]ul92%'BOU&#BRRh-slg8KDlr:%L71Ka:.A;%YULjDPmL<LYs8i#XwJOYaKPKc1h:'9Ke,g)b),78=I39B;xiY$bgGw-&.Zi9InXDuYa%G*f2Bq7mn9^#p1vv%#(Wi-;/Z5h"
"o;#2:;%d&#x9v68C5g?ntX0X)pT`;%pB3q7mgGN)3%(P8nTd5L7GeA-GL@+%J3u2:(Yf>et`e;)f#Km8&+DC$I46>#Kr]]u-[=99tts1.qb#q72g1WJO81q+eN'03'eM>&1XxY-caEnO"
"j%2n8)),?ILR5^.Ibn<-X-Mq7[a82Lq:F&#ce+S9wsCK*x`569E8ew'He]h:sI[2LM$[guka3ZRd6:t%IG:;$%YiJ:Nq=?eAw;/:nnDq0(CYcMpG)qLN4$##&J<j$UpK<Q4a1]MupW^-"
"sj_$%[HK%'F####QRZJ::Y3EGl4'@%FkiAOg#p[##O`gukTfBHagL<LHw%q&OV0##F=6/:chIm0@eCP8X]:kFI%hl8hgO@RcBhS-@Qb$%+m=hPDLg*%K8ln(wcf3/'DW-$.lR?n[nCH-"
"eXOONTJlh:.RYF%3'p6sq:UIMA945&^HFS87@$EP2iG<-lCO$%c`uKGD3rC$x0BL8aFn--`ke%#HMP'vh1/R&O_J9'um,.<tx[@%wsJk&bUT2`0uMv7gg#qp/ij.L56'hl;.s5CUrxjO"
"M7-##.l+Au'A&O:-T72L]P`&=;ctp'XScX*rU.>-XTt,%OVU4)S1+R-#dg0/Nn?Ku1^0f$B*P:Rowwm-`0PKjYDDM'3]d39VZHEl4,.j']Pk-M.h^&:0FACm$maq-&sgw0t7/6(^xtk%"
"LuH88Fj-ekm>GA#_>568x6(OFRl-IZp`&b,_P'$M<Jnq79VsJW/mWS*PUiq76;]/NM_>hLbxfc$mj`,O;&%W2m`Zh:/)Uetw:aJ%]K9h:TcF]u_-Sj9,VK3M.*'&0D[Ca]J9gp8,kAW]"
"%(?A%R$f<->Zts'^kn=-^@c4%-pY6qI%J%1IGxfLU9CP8cbPlXv);C=b),<2mOvP8up,UVf3839acAWAW-W?#ao/^#%KYo8fRULNd2.>%m]UK:n%r$'sw]J;5pAoO_#2mO3n,'=H5(et"
"Hg*`+RLgv>=4U8guD$I%D:W>-r5V*%j*W:Kvej.Lp$<M-SGZ':+Q_k+uvOSLiEo(<aD/K<CCc`'Lx>'?;++O'>()jLR-^u68PHm8ZFWe+ej8h:9r6L*0//c&iH&R8pRbA#Kjm%upV1g:"
"a_#Ur7FuA#(tRh#.Y5K+@?3<-8m0$PEn;J:rh6?I6uG<-`wMU'ircp0LaE_OtlMb&1#6T.#FDKu#1Lw%u%+GM+X'e?YLfjM[VO0MbuFp7;>Q&#WIo)0@F%q7c#4XAXN-U&VB<HFF*qL("
"$/V,;(kXZejWO`<[5?\?ewY(*9=%wDc;,u<'9t3W-(H1th3+G]ucQ]kLs7df($/*JL]@*t7Bu_G3_7mp7<iaQjO@.kLg;x3B0lqp7Hf,^Ze7-##@/c58Mo(3;knp0%)A7?-W+eI'o8)b<"
"nKnw'Ho8C=Y>pqB>0ie&jhZ[?iLR@@_AvA-iQC(=ksRZRVp7`.=+NpBC%rh&3]R:8XDmE5^V8O(x<<aG/1N$#FX$0V5Y6x'aErI3I$7x%E`v<-BY,)%-?Psf*l?%C3.mM(=/M0:JxG'?"
"7WhH%o'a<-80g0NBxoO(GH<dM]n.+%q@jH?f.UsJ2Ggs&4<-e47&Kl+f//9@`b+?.TeN_&B8Ss?v;^Trk;f#YvJkl&w$]>-+k?'(<S:68tq*WoDfZu';mM?8X[ma8W%*`-=;D.(nc7/;"
")g:T1=^J$&BRV(-lTmNB6xqB[@0*o.erM*<SWF]u2=st-*(6v>^](H.aREZSi,#1:[IXaZFOm<-ui#qUq2$##Ri;u75OK#(RtaW-K-F`S+cF]uN`-KMQ%rP/Xri.LRcB##=YL3BgM/3M"
"D?@f&1'BW-)Ju<L25gl8uhVm1hL$##*8###'A3/LkKW+(^rWX?5W_8g)a(m&K8P>#bmmWCMkk&#TR`C,5d>g)F;t,4:@_l8G/5h4vUd%&%950:VXD'QdWoY-F$BtUwmfe$YqL'8(PWX("
"P?^@Po3$##`MSs?DWBZ/S>+4%>fX,VWv/w'KD`LP5IbH;rTV>n3cEK8U#bX]l-/V+^lj3;vlMb&[5YQ8#pekX9JP3XUC72L,,?+Ni&co7ApnO*5NK,((W-i:$,kp'UDAO(G0Sq7MVjJs"
"bIu)'Z,*[>br5fX^:FPAWr-m2KgL<LUN098kTF&#lvo58=/vjDo;.;)Ka*hLR#/k=rKbxuV`>Q_nN6'8uTG&#1T5g)uLv:873UpTLgH+#FgpH'_o1780Ph8KmxQJ8#H72L4@768@Tm&Q"
"h4CB/5OvmA&,Q&QbUoi$a_%3M01H)4x7I^&KQVgtFnV+;[Pc>[m4k//,]1?#`VY[Jr*3&&slRfLiVZJ:]?=K3Sw=[$=uRB?3xk48@aeg<Z'<$#4H)6,>e0jT6'N#(q%.O=?2S]u*(m<-"
"V8J'(1)G][68hW$5'q[GC&5j`TE?m'esFGNRM)j,ffZ?-qx8;->g4t*:CIP/[Qap7/9'#(1sao7w-.qNUdkJ)tCF&#B^;xGvn2r9FEPFFFcL@.iFNkTve$m%#QvQS8U@)2Z+3K:AKM5i"
"sZ88+dKQ)W6>J%CL<KE>`.d*(B`-n8D9oK<Up]c$X$(,)M8Zt7/[rdkqTgl-0cuGMv'?>-XV1q['-5k'cAZ69e;D_?$ZPP&s^+7])$*$#@QYi9,5P&#9r+$%CE=68>K8r0=dSC%%(@p7"
".m7jilQ02'0-VWAg<a/''3u.=4L$Y)6k/K:_[3=&jvL<L0C/2'v:^;-DIBW,B4E68:kZ;%?8(Q8BH=kO65BW?xSG&#@uU,DS*,?.+(o(#1vCS8#CHF>TlGW'b)Tq7VT9q^*^$$.:&N@@"
"$&)WHtPm*5_rO0&e%K&#-30j(E4#'Zb.o/(Tpm$>K'f@[PvFl,hfINTNU6u'0pao7%XUp9]5.>%h`8_=VYbxuel.NTSsJfLacFu3B'lQSu/m6-Oqem8T+oE--$0a/k]uj9EwsG>%veR*"
"hv^BFpQj:K'#SJ,sB-'#](j.Lg92rTw-*n%@/;39rrJF,l#qV%OrtBeC6/,;qB3ebNW[?,Hqj2L.1NP&GjUR=1D8QaS3Up&@*9wP?+lo7b?@%'k4`p0Z$22%K3+iCZj?XJN4Nm&+YF]u"
"@-W$U%VEQ/,,>>#)D<h#`)h0:<Q6909ua+&VU%n2:cG3FJ-%@Bj-DgLr`Hw&HAKjKjseK</xKT*)B,N9X3]krc12t'pgTV(Lv-tL[xg_%=M_q7a^x?7Ubd>#%8cY#YZ?=,`Wdxu/ae&#"
"w6)R89tI#6@s'(6Bf7a&?S=^ZI_kS&ai`&=tE72L_D,;^R)7[$s<Eh#c&)q.MXI%#v9ROa5FZO%sF7q7Nwb&#ptUJ:aqJe$Sl68%.D###EC><?-aF&#RNQv>o8lKN%5/$(vdfq7+ebA#"
"u1p]ovUKW&Y%q]'>$1@-[xfn$7ZTp7mM,G,Ko7a&Gu%G[RMxJs[0MM%wci.LFDK)(<c`Q8N)jEIF*+?P2a8g%)$q]o2aH8C&<SibC/q,(e:v;-b#6[$NtDZ84Je2KNvB#$P5?tQ3nt(0"
"d=j.LQf./Ll33+(;q3L-w=8dX$#WF&uIJ@-bfI>%:_i2B5CsR8&9Z&#=mPEnm0f`<&c)QL5uJ#%u%lJj+D-r;BoF&#4DoS97h5g)E#o:&S4weDF,9^Hoe`h*L+_a*NrLW-1pG_&2UdB8"
"6e%B/:=>)N4xeW.*wft-;$'58-ESqr<b?UI(_%@[P46>#U`'6AQ]m&6/`Z>#S?YY#Vc;r7U2&326d=w&H####?TZ`*4?&.MK?LP8Vxg>$[QXc%QJv92.(Db*B)gb*BM9dM*hJMAo*c&#"
"b0v=Pjer]$gG&JXDf->'StvU7505l9$AFvgYRI^&<^b68?j#q9QX4SM'RO#&sL1IM.rJfLUAj221]d##DW=m83u5;'bYx,*Sl0hL(W;;$doB&O/TQ:(Z^xBdLjL<Lni;''X.`$#8+1GD"
":k$YUWsbn8ogh6rxZ2Z9]%nd+>V#*8U_72Lh+2Q8Cj0i:6hp&$C/:p(HK>T8Y[gHQ4`4)'$Ab(Nof%V'8hL&#<NEdtg(n'=S1A(Q1/I&4([%dM`,Iu'1:_hL>SfD07&6D<fp8dHM7/g+"
"tlPN9J*rKaPct&?'uBCem^jn%9_K)<,C5K3s=5g&GmJb*[SYq7K;TRLGCsM-$$;S%:Y@r7AK0pprpL<Lrh,q7e/%KWK:50I^+m'vi`3?%Zp+<-d+$L-Sv:@.o19n$s0&39;kn;S%BSq*"
"$3WoJSCLweV[aZ'MQIjO<7;X-X;&+dMLvu#^UsGEC9WEc[X(wI7#2.(F0jV*eZf<-Qv3J-c+J5AlrB#$p(H68LvEA'q3n0#m,[`*8Ft)FcYgEud]CWfm68,(aLA$@EFTgLXoBq/UPlp7"
":d[/;r_ix=:TF`S5H-b<LI&HY(K=h#)]Lk$K14lVfm:x$H<3^Ql<M`$OhapBnkup'D#L$Pb_`N*g]2e;X/Dtg,bsj&K#2[-:iYr'_wgH)NUIR8a1n#S?Yej'h8^58UbZd+^FKD*T@;6A"
"7aQC[K8d-(v6GI$x:T<&'Gp5Uf>@M.*J:;$-rv29'M]8qMv-tLp,'886iaC=Hb*YJoKJ,(j%K=H`K.v9HggqBIiZu'QvBT.#=)0ukruV&.)3=(^1`o*Pj4<-<aN((^7('#Z0wK#5GX@7"
"u][`*S^43933A4rl][`*O4CgLEl]v$1Q3AeF37dbXk,.)vj#x'd`;qgbQR%FW,2(?LO=s%Sc68%NP'##Aotl8x=BE#j1UD([3$M(]UI2LX3RpKN@;/#f'f/&_mt&F)XdF<9t4)Qa.*kT"
"LwQ'(TTB9.xH'>#MJ+gLq9-##@HuZPN0]u:h7.T..G:;$/Usj(T7`Q8tT72LnYl<-qx8;-HV7Q-&Xdx%1a,hC=0u+HlsV>nuIQL-5<N?)NBS)QN*_I,?&)2'IM%L3I)X((e/dl2&8'<M"
":^#M*Q+[T.Xri.LYS3v%fF`68h;b-X[/En'CR.q7E)p'/kle2HM,u;^%OKC-N+Ll%F9CF<Nf'^#t2L,;27W:0O@6##U6W7:$rJfLWHj$#)woqBefIZ.PK<b*t7ed;p*_m;4ExK#h@&]>"
"_>@kXQtMacfD.m-VAb8;IReM3$wf0''hra*so568'Ip&vRs849'MRYSp%:t:h5qSgwpEr$B>Q,;s(C#$)`svQuF$##-D,##,g68@2[T;.XSdN9Qe)rpt._K-#5wF)sP'##p#C0c%-Gb%"
"hd+<-j'Ai*x&&HMkT]C'OSl##5RG[JXaHN;d'uA#x._U;.`PU@(Z3dt4r152@:v,'R.Sj'w#0<-;kPI)FfJ&#AYJ&#//)>-k=m=*XnK$>=)72L]0I%>.G690a:$##<,);?;72#?x9+d;"
"^V'9;jY@;)br#q^YQpx:X#Te$Z^'=-=bGhLf:D6&bNwZ9-ZD#n^9HhLMr5G;']d&6'wYmTFmL<LD)F^%[tC'8;+9E#C$g%#5Y>q9wI>P(9mI[>kC-ekLC/R&CH+s'B;K-M6$EB%is00:"
"+A4[7xks.LrNk0&E)wILYF@2L'0Nb$+pv<(2.768/FrY&h$^3i&@+G%JT'<-,v`3;_)I9M^AE]CN?Cl2AZg+%4iTpT3<n-&%H%b<FDj2M<hH=&Eh<2Len$b*aTX=-8QxN)k11IM1c^j%"
"9s<L<NFSo)B?+<-(GxsF,^-Eh@$4dXhN$+#rxK8'je'D7k`e;)2pYwPA'_p9&@^18ml1^[@g4t*[JOa*[=Qp7(qJ_oOL^('7fB&Hq-:sf,sNj8xq^>$U4O]GKx'm9)b@p7YsvK3w^YR-"
"CdQ*:Ir<($u&)#(&?L9Rg3H)4fiEp^iI9O8KnTj,]H?D*r7'M;PwZ9K0E^k&-cpI;.p/6_vwoFMV<->#%Xi.LxVnrU(4&8/P+:hLSKj$#U%]49t'I:rgMi'FL@a:0Y-uA[39',(vbma*"
"hU%<-SRF`Tt:542R_VV$p@[p8DV[A,?1839FWdF<TddF<9Ah-6&9tWoDlh]&1SpGMq>Ti1O*H&#(AL8[_P%.M>v^-))qOT*F5Cq0`Ye%+$B6i:7@0IX<N+T+0MlMBPQ*Vj>SsD<U4JHY"
"8kD2)2fU/M#$e.)T4,_=8hLim[&);?UkK'-x?'(:siIfL<$pFM`i<?%W(mGDHM%>iWP,##P`%/L<eXi:@Z9C.7o=@(pXdAO/NLQ8lPl+HPOQa8wD8=^GlPa8TKI1CjhsCTSLJM'/Wl>-"
"S(qw%sf/@%#B6;/U7K]uZbi^Oc^2n<bhPmUkMw>%t<)'mEVE''n`WnJra$^TKvX5B>;_aSEK',(hwa0:i4G?.Bci.(X[?b*($,=-n<.Q%`(X=?+@Am*Js0&=3bh8K]mL<LoNs'6,'85`"
"0?t/'_U59@]ddF<#LdF<eWdF<OuN/45rY<-L@&#+fm>69=Lb,OcZV/);TTm8VI;?%OtJ<(b4mq7M6:u?KRdF<gR@2L=FNU-<b[(9c/ML3m;Z[$oF3g)GAWqpARc=<ROu7cL5l;-[A]%/"
"+fsd;l#SafT/f*W]0=O'$(Tb<[)*@e775R-:Yob%g*>l*:xP?Yb.5)%w_I?7uk5JC+FS(m#i'k.'a0i)9<7b'fs'59hq$*5Uhv##pi^8+hIEBF`nvo`;'l0.^S1<-wUK2/Coh58KKhLj"
"M=SO*rfO`+qC`W-On.=AJ56>>i2@2LH6A:&5q`?9I3@@'04&p2/LVa*T-4<-i3;M9UvZd+N7>b*eIwg:CC)c<>nO&#<IGe;__.thjZl<%w(Wk2xmp4Q@I#I9,DF]u7-P=.-_:YJ]aS@V"
"?6*C()dOp7:WL,b&3Rg/.cmM9&r^>$(>.Z-I&J(Q0Hd5Q%7Co-b`-c<N(6r@ip+AurK<m86QIth*#v;-OBqi+L7wDE-Ir8K['m+DDSLwK&/.?-V%U_%3:qKNu$_b*B-kp7NaD'QdWQPK"
"Yq[@>P)hI;*_F]u`Rb[.j8_Q/<&>uu+VsH$sM9TA%?)(vmJ80),P7E>)tjD%2L=-t#fK[%`v=Q8<FfNkgg^oIbah*#8/Qt$F&:K*-(N/'+1vMB,u()-a.VUU*#[e%gAAO(S>WlA2);Sa"
">gXm8YB`1d@K#n]76-a$U,mF<fX]idqd)<3,]J7JmW4`6]uks=4-72L(jEk+:bJ0M^q-8Dm_Z?0olP1C9Sa&H[d&c$ooQUj]Exd*3ZM@-WGW2%s',B-_M%>%Ul:#/'xoFM9QX-$.QN'>"
"[%$Z$uF6pA6Ki2O5:8w*vP1<-1`[G,)-m#>0`P&#eb#.3i)rtB61(o'$?X3B</R90;eZ]%Ncq;-Tl]#F>2Qft^ae_5tKL9MUe9b*sLEQ95C&`=G?@Mj=wh*'3E>=-<)Gt*Iw)'QG:`@I"
"wOf7&]1i'S01B+Ev/Nac#9S;=;YQpg_6U`*kVY39xK,[/6Aj7:'1Bm-_1EYfa1+o&o4hp7KN_Q(OlIo@S%;jVdn0'1<Vc52=u`3^o-n1'g4v58Hj&6_t7$##?M)c<$bgQ_'SY((-xkA#"
"Y(,p'H9rIVY-b,'%bCPF7.J<Up^,(dU1VY*5#WkTU>h19w,WQhLI)3S#f$2(eb,jr*b;3Vw]*7NH%$c4Vs,eD9>XW8?N]o+(*pgC%/72LV-u<Hp,3@e^9UB1J+ak9-TN/mhKPg+AJYd$"
"MlvAF_jCK*.O-^(63adMT->W%iewS8W6m2rtCpo'RS1R84=@paTKt)>=%&1[)*vp'u+x,VrwN;&]kuO9JDbg=pO$J*.jVe;u'm0dr9l,<*wMK*Oe=g8lV_KEBFkO'oU]^=[-792#ok,)"
"i]lR8qQ2oA8wcRCZ^7w/Njh;?.stX?Q1>S1q4Bn$)K1<-rGdO'$Wr.Lc.CG)$/*JL4tNR/,SVO3,aUw'DJN:)Ss;wGn9A32ijw%FL+Z0Fn.U9;reSq)bmI32U==5ALuG&#Vf1398/pVo"
"1*c-(aY168o<`JsSbk-,1N;$>0:OUas(3:8Z972LSfF8eb=c-;>SPw7.6hn3m`9^Xkn(r.qS[0;T%&Qc=+STRxX'q1BNk3&*eu2;&8q$&x>Q#Q7^Tf+6<(d%ZVmj2bDi%.3L2n+4W'$P"
"iDDG)g,r%+?,$@?uou5tSe2aN_AQU*<h`e-GI7)?OK2A.d7_c)?wQ5AS@DL3r#7fSkgl6-++D:'A,uq7SvlB$pcpH'q3n0#_%dY#xCpr-l<F0NR@-##FEV6NTF6##$l84N1w?AO>'IAO"
"URQ##V^Fv-XFbGM7Fl(N<3DhLGF%q.1rC$#:T__&Pi68%0xi_&[qFJ(77j_&JWoF.V735&T,[R*:xFR*K5>>#`bW-?4Ne_&6Ne_&6Ne_&n`kr-#GJcM6X;uM6X;uM(.a..^2TkL%oR(#"
";u.T%fAr%4tJ8&><1=GHZ_+m9/#H1F^R#SC#*N=BA9(D?v[UiFY>>^8p,KKF.W]L29uLkLlu/+4T<XoIB&hx=T1PcDaB&;HH+-AFr?(m9HZV)FKS8JCw;SD=6[^/DZUL`EUDf]GGlG&>"
"w$)F./^n3+rlo+DB;5sIYGNk+i1t-69Jg--0pao7Sm#K)pdHW&;LuDNH@H>#/X-TI(;P>#,Gc>#0Su>#4`1?#8lC?#<xU?#@.i?#D:%@#HF7@#LRI@#P_[@#Tkn@#Xw*A#]-=A#a9OA#"
"d<F&#*;G##.GY##2Sl##6`($#:l:$#>xL$#B.`$#F:r$#JF.%#NR@%#R_R%#Vke%#Zww%#_-4&#3^Rh%Sflr-k'MS.o?.5/sWel/wpEM0%3'/1)K^f1-d>G21&v(35>V`39V7A4=onx4"
"A1OY5EI0;6Ibgr6M$HS7Q<)58C5w,;WoA*#[%T*#`1g*#d=#+#hI5+#lUG+#pbY+#tnl+#x$),#&1;,#*=M,#.I`,#2Ur,#6b.-#;w[H#iQtA#m^0B#qjBB#uvTB##-hB#'9$C#+E6C#"
"/QHC#3^ZC#7jmC#;v)D#?,<D#C8ND#GDaD#KPsD#O]/E#g1A5#KA*1#gC17#MGd;#8(02#L-d3#rWM4#Hga1#,<w0#T.j<#O#'2#CYN1#qa^:#_4m3#o@/=#eG8=#t8J5#`+78#4uI-#"
"m3B2#SB[8#Q0@8#i[*9#iOn8#1Nm;#^sN9#qh<9#:=x-#P;K2#$%X9#bC+.#Rg;<#mN=.#MTF.#RZO.#2?)4#Y#(/#[)1/#b;L/#dAU/#0Sv;#lY$0#n`-0#sf60#(F24#wrH0#%/e0#"
"TmD<#%JSMFove:CTBEXI:<eh2g)B,3h2^G3i;#d3jD>)4kMYD4lVu`4m`:&5niUA5@(A5BA1]PBB:xlBCC=2CDLXMCEUtiCf&0g2'tN?PGT4CPGT4CPGT4CPGT4CPGT4CPGT4CPGT4CP"
"GT4CPGT4CPGT4CPGT4CPGT4CPGT4CP-qekC`.9kEg^+F$kwViFJTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5o,^<-28ZI'O?;xp"
"O?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xp;7q-#lLYI:xvD=#";

static const char* GetDefaultCompressedFontDataTTFBase85()
{
    return proggy_clean_ttf_compressed_data_base85;
}

#endif // #ifndef IMGUI_DISABLE

// Junk Code By Troll Face & Thaisen's Gen
void pDtkikWxdr95306183() {     int zUXGorEReD83108173 = -341108385;    int zUXGorEReD42949213 = -303999262;    int zUXGorEReD97894457 = -86168062;    int zUXGorEReD18262080 = -481257270;    int zUXGorEReD37037841 = -712127580;    int zUXGorEReD17271684 = -945899734;    int zUXGorEReD22700245 = -532822258;    int zUXGorEReD57054788 = -922899281;    int zUXGorEReD77489140 = -980278163;    int zUXGorEReD41676140 = -661152974;    int zUXGorEReD82825658 = -697566872;    int zUXGorEReD81241985 = -375532302;    int zUXGorEReD84601587 = -520374408;    int zUXGorEReD8671511 = -921009296;    int zUXGorEReD13003427 = -969826039;    int zUXGorEReD23494813 = -615355056;    int zUXGorEReD85925147 = -686834311;    int zUXGorEReD32816244 = -199627600;    int zUXGorEReD62065978 = -55973242;    int zUXGorEReD38886185 = -648878832;    int zUXGorEReD8454421 = 3964777;    int zUXGorEReD63289060 = -8446426;    int zUXGorEReD79167594 = -858816301;    int zUXGorEReD39889835 = -156116641;    int zUXGorEReD18108206 = -890951894;    int zUXGorEReD76429950 = -114560896;    int zUXGorEReD24082063 = -626872166;    int zUXGorEReD60478351 = -805500774;    int zUXGorEReD16728105 = -633726178;    int zUXGorEReD78216362 = -982707005;    int zUXGorEReD73585894 = -516360373;    int zUXGorEReD49826139 = -773024877;    int zUXGorEReD16005919 = -967590614;    int zUXGorEReD29309234 = -758942969;    int zUXGorEReD25886978 = -636189702;    int zUXGorEReD33435791 = 40512096;    int zUXGorEReD502486 = -173391055;    int zUXGorEReD26079155 = -47467594;    int zUXGorEReD37871193 = -544651296;    int zUXGorEReD60978989 = -906126467;    int zUXGorEReD99767994 = 30693795;    int zUXGorEReD52406933 = -618070785;    int zUXGorEReD70963868 = -193678679;    int zUXGorEReD72155766 = -871814843;    int zUXGorEReD25487540 = -135686621;    int zUXGorEReD25135305 = -858271806;    int zUXGorEReD18565148 = 87850903;    int zUXGorEReD65860164 = -793812619;    int zUXGorEReD26726067 = -180277633;    int zUXGorEReD49162547 = -638961890;    int zUXGorEReD64764199 = -900158582;    int zUXGorEReD37415754 = -567489663;    int zUXGorEReD8096146 = -55424767;    int zUXGorEReD40911086 = -206400449;    int zUXGorEReD86851763 = -986596556;    int zUXGorEReD19819113 = -232661960;    int zUXGorEReD63781618 = -445182962;    int zUXGorEReD58004622 = -930051421;    int zUXGorEReD153875 = -590305377;    int zUXGorEReD60607890 = -497566684;    int zUXGorEReD93189620 = -219027569;    int zUXGorEReD62221893 = -727321484;    int zUXGorEReD40326684 = -189173104;    int zUXGorEReD99272778 = -997571158;    int zUXGorEReD68090246 = -44792601;    int zUXGorEReD32999519 = -924541996;    int zUXGorEReD65236067 = -407941688;    int zUXGorEReD55292353 = -761431439;    int zUXGorEReD82784533 = -184819594;    int zUXGorEReD79567636 = -910338136;    int zUXGorEReD22992328 = -341964002;    int zUXGorEReD59845993 = -539366718;    int zUXGorEReD94945050 = -654976304;    int zUXGorEReD1086989 = -149846776;    int zUXGorEReD39118191 = -579572627;    int zUXGorEReD56047488 = -377964438;    int zUXGorEReD92325192 = -814767748;    int zUXGorEReD7011828 = -987001458;    int zUXGorEReD14402296 = 79569980;    int zUXGorEReD92972901 = 67319912;    int zUXGorEReD57864803 = -102411799;    int zUXGorEReD58221898 = -833059547;    int zUXGorEReD33752285 = -525223142;    int zUXGorEReD67565557 = -994764288;    int zUXGorEReD13452164 = 17451576;    int zUXGorEReD36170140 = -948870711;    int zUXGorEReD41729994 = -617600110;    int zUXGorEReD75094832 = -661190165;    int zUXGorEReD42457471 = -772346413;    int zUXGorEReD6067865 = -303527743;    int zUXGorEReD69654172 = -514304942;    int zUXGorEReD42497863 = -243339634;    int zUXGorEReD25925281 = -457162218;    int zUXGorEReD77263302 = 52915388;    int zUXGorEReD67789368 = -587098898;    int zUXGorEReD37546101 = -241984721;    int zUXGorEReD12080249 = -328897682;    int zUXGorEReD71691090 = -196107521;    int zUXGorEReD4065520 = -727022243;    int zUXGorEReD92488020 = -341108385;     zUXGorEReD83108173 = zUXGorEReD42949213;     zUXGorEReD42949213 = zUXGorEReD97894457;     zUXGorEReD97894457 = zUXGorEReD18262080;     zUXGorEReD18262080 = zUXGorEReD37037841;     zUXGorEReD37037841 = zUXGorEReD17271684;     zUXGorEReD17271684 = zUXGorEReD22700245;     zUXGorEReD22700245 = zUXGorEReD57054788;     zUXGorEReD57054788 = zUXGorEReD77489140;     zUXGorEReD77489140 = zUXGorEReD41676140;     zUXGorEReD41676140 = zUXGorEReD82825658;     zUXGorEReD82825658 = zUXGorEReD81241985;     zUXGorEReD81241985 = zUXGorEReD84601587;     zUXGorEReD84601587 = zUXGorEReD8671511;     zUXGorEReD8671511 = zUXGorEReD13003427;     zUXGorEReD13003427 = zUXGorEReD23494813;     zUXGorEReD23494813 = zUXGorEReD85925147;     zUXGorEReD85925147 = zUXGorEReD32816244;     zUXGorEReD32816244 = zUXGorEReD62065978;     zUXGorEReD62065978 = zUXGorEReD38886185;     zUXGorEReD38886185 = zUXGorEReD8454421;     zUXGorEReD8454421 = zUXGorEReD63289060;     zUXGorEReD63289060 = zUXGorEReD79167594;     zUXGorEReD79167594 = zUXGorEReD39889835;     zUXGorEReD39889835 = zUXGorEReD18108206;     zUXGorEReD18108206 = zUXGorEReD76429950;     zUXGorEReD76429950 = zUXGorEReD24082063;     zUXGorEReD24082063 = zUXGorEReD60478351;     zUXGorEReD60478351 = zUXGorEReD16728105;     zUXGorEReD16728105 = zUXGorEReD78216362;     zUXGorEReD78216362 = zUXGorEReD73585894;     zUXGorEReD73585894 = zUXGorEReD49826139;     zUXGorEReD49826139 = zUXGorEReD16005919;     zUXGorEReD16005919 = zUXGorEReD29309234;     zUXGorEReD29309234 = zUXGorEReD25886978;     zUXGorEReD25886978 = zUXGorEReD33435791;     zUXGorEReD33435791 = zUXGorEReD502486;     zUXGorEReD502486 = zUXGorEReD26079155;     zUXGorEReD26079155 = zUXGorEReD37871193;     zUXGorEReD37871193 = zUXGorEReD60978989;     zUXGorEReD60978989 = zUXGorEReD99767994;     zUXGorEReD99767994 = zUXGorEReD52406933;     zUXGorEReD52406933 = zUXGorEReD70963868;     zUXGorEReD70963868 = zUXGorEReD72155766;     zUXGorEReD72155766 = zUXGorEReD25487540;     zUXGorEReD25487540 = zUXGorEReD25135305;     zUXGorEReD25135305 = zUXGorEReD18565148;     zUXGorEReD18565148 = zUXGorEReD65860164;     zUXGorEReD65860164 = zUXGorEReD26726067;     zUXGorEReD26726067 = zUXGorEReD49162547;     zUXGorEReD49162547 = zUXGorEReD64764199;     zUXGorEReD64764199 = zUXGorEReD37415754;     zUXGorEReD37415754 = zUXGorEReD8096146;     zUXGorEReD8096146 = zUXGorEReD40911086;     zUXGorEReD40911086 = zUXGorEReD86851763;     zUXGorEReD86851763 = zUXGorEReD19819113;     zUXGorEReD19819113 = zUXGorEReD63781618;     zUXGorEReD63781618 = zUXGorEReD58004622;     zUXGorEReD58004622 = zUXGorEReD153875;     zUXGorEReD153875 = zUXGorEReD60607890;     zUXGorEReD60607890 = zUXGorEReD93189620;     zUXGorEReD93189620 = zUXGorEReD62221893;     zUXGorEReD62221893 = zUXGorEReD40326684;     zUXGorEReD40326684 = zUXGorEReD99272778;     zUXGorEReD99272778 = zUXGorEReD68090246;     zUXGorEReD68090246 = zUXGorEReD32999519;     zUXGorEReD32999519 = zUXGorEReD65236067;     zUXGorEReD65236067 = zUXGorEReD55292353;     zUXGorEReD55292353 = zUXGorEReD82784533;     zUXGorEReD82784533 = zUXGorEReD79567636;     zUXGorEReD79567636 = zUXGorEReD22992328;     zUXGorEReD22992328 = zUXGorEReD59845993;     zUXGorEReD59845993 = zUXGorEReD94945050;     zUXGorEReD94945050 = zUXGorEReD1086989;     zUXGorEReD1086989 = zUXGorEReD39118191;     zUXGorEReD39118191 = zUXGorEReD56047488;     zUXGorEReD56047488 = zUXGorEReD92325192;     zUXGorEReD92325192 = zUXGorEReD7011828;     zUXGorEReD7011828 = zUXGorEReD14402296;     zUXGorEReD14402296 = zUXGorEReD92972901;     zUXGorEReD92972901 = zUXGorEReD57864803;     zUXGorEReD57864803 = zUXGorEReD58221898;     zUXGorEReD58221898 = zUXGorEReD33752285;     zUXGorEReD33752285 = zUXGorEReD67565557;     zUXGorEReD67565557 = zUXGorEReD13452164;     zUXGorEReD13452164 = zUXGorEReD36170140;     zUXGorEReD36170140 = zUXGorEReD41729994;     zUXGorEReD41729994 = zUXGorEReD75094832;     zUXGorEReD75094832 = zUXGorEReD42457471;     zUXGorEReD42457471 = zUXGorEReD6067865;     zUXGorEReD6067865 = zUXGorEReD69654172;     zUXGorEReD69654172 = zUXGorEReD42497863;     zUXGorEReD42497863 = zUXGorEReD25925281;     zUXGorEReD25925281 = zUXGorEReD77263302;     zUXGorEReD77263302 = zUXGorEReD67789368;     zUXGorEReD67789368 = zUXGorEReD37546101;     zUXGorEReD37546101 = zUXGorEReD12080249;     zUXGorEReD12080249 = zUXGorEReD71691090;     zUXGorEReD71691090 = zUXGorEReD4065520;     zUXGorEReD4065520 = zUXGorEReD92488020;     zUXGorEReD92488020 = zUXGorEReD83108173;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void rxoalZZZMp81786059() {     int OccUgzoDqp82825725 = -258014070;    int OccUgzoDqp31509365 = -521027162;    int OccUgzoDqp56373453 = -539949906;    int OccUgzoDqp76446309 = -30445592;    int OccUgzoDqp68017879 = -355224540;    int OccUgzoDqp21839283 = -471367124;    int OccUgzoDqp41132668 = -653487667;    int OccUgzoDqp62669173 = -141691383;    int OccUgzoDqp27169395 = 68922148;    int OccUgzoDqp26435961 = -174891506;    int OccUgzoDqp99512863 = 72866477;    int OccUgzoDqp61381452 = -301750458;    int OccUgzoDqp67591968 = 69967358;    int OccUgzoDqp70847368 = -532193316;    int OccUgzoDqp92264686 = -19508142;    int OccUgzoDqp25352220 = -95704556;    int OccUgzoDqp7738002 = -191106760;    int OccUgzoDqp61090687 = -484616713;    int OccUgzoDqp43743775 = -574990008;    int OccUgzoDqp90182865 = -768184133;    int OccUgzoDqp36590010 = -966330920;    int OccUgzoDqp63582440 = -891438852;    int OccUgzoDqp32256635 = -548789682;    int OccUgzoDqp95278752 = -629169645;    int OccUgzoDqp87303415 = 64379182;    int OccUgzoDqp77066380 = -606206548;    int OccUgzoDqp37055655 = 38519059;    int OccUgzoDqp75312539 = -293497664;    int OccUgzoDqp87230280 = -845070577;    int OccUgzoDqp46546311 = -660666168;    int OccUgzoDqp27023125 = -128566171;    int OccUgzoDqp80501206 = -334933615;    int OccUgzoDqp88146315 = -574220808;    int OccUgzoDqp35884449 = -434789357;    int OccUgzoDqp78189401 = -129220792;    int OccUgzoDqp97377836 = -358998206;    int OccUgzoDqp25323758 = -331725627;    int OccUgzoDqp51235035 = -954426237;    int OccUgzoDqp43907351 = -230330920;    int OccUgzoDqp57221455 = 33211047;    int OccUgzoDqp62577231 = -484787001;    int OccUgzoDqp66403823 = -582849478;    int OccUgzoDqp41872472 = -271896794;    int OccUgzoDqp96481135 = -827536103;    int OccUgzoDqp31453812 = -324459197;    int OccUgzoDqp40688403 = -618873374;    int OccUgzoDqp75250504 = -263447731;    int OccUgzoDqp34480703 = -784793118;    int OccUgzoDqp83028160 = -297977495;    int OccUgzoDqp58644413 = -7479353;    int OccUgzoDqp4560501 = -744565106;    int OccUgzoDqp48684458 = -514769063;    int OccUgzoDqp64550376 = -751266722;    int OccUgzoDqp24780374 = -164064575;    int OccUgzoDqp45196050 = -896805373;    int OccUgzoDqp19243285 = -366575218;    int OccUgzoDqp99252729 = -972237480;    int OccUgzoDqp61094701 = -910780261;    int OccUgzoDqp89142893 = 5175225;    int OccUgzoDqp90951499 = -749017993;    int OccUgzoDqp84783628 = -409886184;    int OccUgzoDqp65820128 = -259990003;    int OccUgzoDqp75438893 = -296620806;    int OccUgzoDqp80623083 = -270411684;    int OccUgzoDqp99412836 = 53674665;    int OccUgzoDqp19011657 = -592199909;    int OccUgzoDqp73235136 = -727529650;    int OccUgzoDqp31707519 = -495243285;    int OccUgzoDqp92657966 = -302972525;    int OccUgzoDqp94886850 = -660509937;    int OccUgzoDqp28462 = -763978929;    int OccUgzoDqp56502966 = -236680524;    int OccUgzoDqp17183337 = -154285793;    int OccUgzoDqp86522319 = -508201056;    int OccUgzoDqp27605634 = -183397133;    int OccUgzoDqp70186187 = -283481442;    int OccUgzoDqp21709969 = -519542058;    int OccUgzoDqp35775499 = -721253579;    int OccUgzoDqp63824941 = -204710449;    int OccUgzoDqp46615013 = -316747445;    int OccUgzoDqp1815876 = -242758818;    int OccUgzoDqp2574952 = -176687824;    int OccUgzoDqp92284379 = -995520170;    int OccUgzoDqp28585868 = -737591224;    int OccUgzoDqp41985811 = -916101063;    int OccUgzoDqp78338666 = -613797109;    int OccUgzoDqp15950831 = -583666893;    int OccUgzoDqp63365941 = -310156234;    int OccUgzoDqp90688399 = -537983985;    int OccUgzoDqp58946116 = -762645574;    int OccUgzoDqp98125106 = -386760726;    int OccUgzoDqp64229057 = -420945367;    int OccUgzoDqp62092142 = -859601462;    int OccUgzoDqp52955851 = -481312928;    int OccUgzoDqp72437826 = -556902770;    int OccUgzoDqp96757103 = -124796998;    int OccUgzoDqp90964929 = -186228673;    int OccUgzoDqp61249388 = 98514889;    int OccUgzoDqp97068299 = -781210769;    int OccUgzoDqp12442156 = -258014070;     OccUgzoDqp82825725 = OccUgzoDqp31509365;     OccUgzoDqp31509365 = OccUgzoDqp56373453;     OccUgzoDqp56373453 = OccUgzoDqp76446309;     OccUgzoDqp76446309 = OccUgzoDqp68017879;     OccUgzoDqp68017879 = OccUgzoDqp21839283;     OccUgzoDqp21839283 = OccUgzoDqp41132668;     OccUgzoDqp41132668 = OccUgzoDqp62669173;     OccUgzoDqp62669173 = OccUgzoDqp27169395;     OccUgzoDqp27169395 = OccUgzoDqp26435961;     OccUgzoDqp26435961 = OccUgzoDqp99512863;     OccUgzoDqp99512863 = OccUgzoDqp61381452;     OccUgzoDqp61381452 = OccUgzoDqp67591968;     OccUgzoDqp67591968 = OccUgzoDqp70847368;     OccUgzoDqp70847368 = OccUgzoDqp92264686;     OccUgzoDqp92264686 = OccUgzoDqp25352220;     OccUgzoDqp25352220 = OccUgzoDqp7738002;     OccUgzoDqp7738002 = OccUgzoDqp61090687;     OccUgzoDqp61090687 = OccUgzoDqp43743775;     OccUgzoDqp43743775 = OccUgzoDqp90182865;     OccUgzoDqp90182865 = OccUgzoDqp36590010;     OccUgzoDqp36590010 = OccUgzoDqp63582440;     OccUgzoDqp63582440 = OccUgzoDqp32256635;     OccUgzoDqp32256635 = OccUgzoDqp95278752;     OccUgzoDqp95278752 = OccUgzoDqp87303415;     OccUgzoDqp87303415 = OccUgzoDqp77066380;     OccUgzoDqp77066380 = OccUgzoDqp37055655;     OccUgzoDqp37055655 = OccUgzoDqp75312539;     OccUgzoDqp75312539 = OccUgzoDqp87230280;     OccUgzoDqp87230280 = OccUgzoDqp46546311;     OccUgzoDqp46546311 = OccUgzoDqp27023125;     OccUgzoDqp27023125 = OccUgzoDqp80501206;     OccUgzoDqp80501206 = OccUgzoDqp88146315;     OccUgzoDqp88146315 = OccUgzoDqp35884449;     OccUgzoDqp35884449 = OccUgzoDqp78189401;     OccUgzoDqp78189401 = OccUgzoDqp97377836;     OccUgzoDqp97377836 = OccUgzoDqp25323758;     OccUgzoDqp25323758 = OccUgzoDqp51235035;     OccUgzoDqp51235035 = OccUgzoDqp43907351;     OccUgzoDqp43907351 = OccUgzoDqp57221455;     OccUgzoDqp57221455 = OccUgzoDqp62577231;     OccUgzoDqp62577231 = OccUgzoDqp66403823;     OccUgzoDqp66403823 = OccUgzoDqp41872472;     OccUgzoDqp41872472 = OccUgzoDqp96481135;     OccUgzoDqp96481135 = OccUgzoDqp31453812;     OccUgzoDqp31453812 = OccUgzoDqp40688403;     OccUgzoDqp40688403 = OccUgzoDqp75250504;     OccUgzoDqp75250504 = OccUgzoDqp34480703;     OccUgzoDqp34480703 = OccUgzoDqp83028160;     OccUgzoDqp83028160 = OccUgzoDqp58644413;     OccUgzoDqp58644413 = OccUgzoDqp4560501;     OccUgzoDqp4560501 = OccUgzoDqp48684458;     OccUgzoDqp48684458 = OccUgzoDqp64550376;     OccUgzoDqp64550376 = OccUgzoDqp24780374;     OccUgzoDqp24780374 = OccUgzoDqp45196050;     OccUgzoDqp45196050 = OccUgzoDqp19243285;     OccUgzoDqp19243285 = OccUgzoDqp99252729;     OccUgzoDqp99252729 = OccUgzoDqp61094701;     OccUgzoDqp61094701 = OccUgzoDqp89142893;     OccUgzoDqp89142893 = OccUgzoDqp90951499;     OccUgzoDqp90951499 = OccUgzoDqp84783628;     OccUgzoDqp84783628 = OccUgzoDqp65820128;     OccUgzoDqp65820128 = OccUgzoDqp75438893;     OccUgzoDqp75438893 = OccUgzoDqp80623083;     OccUgzoDqp80623083 = OccUgzoDqp99412836;     OccUgzoDqp99412836 = OccUgzoDqp19011657;     OccUgzoDqp19011657 = OccUgzoDqp73235136;     OccUgzoDqp73235136 = OccUgzoDqp31707519;     OccUgzoDqp31707519 = OccUgzoDqp92657966;     OccUgzoDqp92657966 = OccUgzoDqp94886850;     OccUgzoDqp94886850 = OccUgzoDqp28462;     OccUgzoDqp28462 = OccUgzoDqp56502966;     OccUgzoDqp56502966 = OccUgzoDqp17183337;     OccUgzoDqp17183337 = OccUgzoDqp86522319;     OccUgzoDqp86522319 = OccUgzoDqp27605634;     OccUgzoDqp27605634 = OccUgzoDqp70186187;     OccUgzoDqp70186187 = OccUgzoDqp21709969;     OccUgzoDqp21709969 = OccUgzoDqp35775499;     OccUgzoDqp35775499 = OccUgzoDqp63824941;     OccUgzoDqp63824941 = OccUgzoDqp46615013;     OccUgzoDqp46615013 = OccUgzoDqp1815876;     OccUgzoDqp1815876 = OccUgzoDqp2574952;     OccUgzoDqp2574952 = OccUgzoDqp92284379;     OccUgzoDqp92284379 = OccUgzoDqp28585868;     OccUgzoDqp28585868 = OccUgzoDqp41985811;     OccUgzoDqp41985811 = OccUgzoDqp78338666;     OccUgzoDqp78338666 = OccUgzoDqp15950831;     OccUgzoDqp15950831 = OccUgzoDqp63365941;     OccUgzoDqp63365941 = OccUgzoDqp90688399;     OccUgzoDqp90688399 = OccUgzoDqp58946116;     OccUgzoDqp58946116 = OccUgzoDqp98125106;     OccUgzoDqp98125106 = OccUgzoDqp64229057;     OccUgzoDqp64229057 = OccUgzoDqp62092142;     OccUgzoDqp62092142 = OccUgzoDqp52955851;     OccUgzoDqp52955851 = OccUgzoDqp72437826;     OccUgzoDqp72437826 = OccUgzoDqp96757103;     OccUgzoDqp96757103 = OccUgzoDqp90964929;     OccUgzoDqp90964929 = OccUgzoDqp61249388;     OccUgzoDqp61249388 = OccUgzoDqp97068299;     OccUgzoDqp97068299 = OccUgzoDqp12442156;     OccUgzoDqp12442156 = OccUgzoDqp82825725;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void UElAGJtbPa40026941() {     int jaIEKYtwyU21482854 = -514920107;    int jaIEKYtwyU56831086 = -708513490;    int jaIEKYtwyU97869827 = -868424673;    int jaIEKYtwyU79384905 = -952512025;    int jaIEKYtwyU95470899 = 83311329;    int jaIEKYtwyU60188314 = 71912355;    int jaIEKYtwyU64955179 = -158497302;    int jaIEKYtwyU64177000 = -796010758;    int jaIEKYtwyU56853161 = 68562735;    int jaIEKYtwyU68733470 = -431398694;    int jaIEKYtwyU14486292 = -42944738;    int jaIEKYtwyU41364308 = -358834843;    int jaIEKYtwyU15399211 = -727576193;    int jaIEKYtwyU72933196 = 44627738;    int jaIEKYtwyU69992813 = -784001402;    int jaIEKYtwyU72358651 = -886040097;    int jaIEKYtwyU13548702 = -129121287;    int jaIEKYtwyU75990366 = -622579452;    int jaIEKYtwyU64477195 = -439425162;    int jaIEKYtwyU66665267 = -916550121;    int jaIEKYtwyU49803031 = 6181919;    int jaIEKYtwyU4691854 = -893041762;    int jaIEKYtwyU3252489 = -54433379;    int jaIEKYtwyU64285953 = -434850372;    int jaIEKYtwyU14684260 = 94456966;    int jaIEKYtwyU28950694 = -515339673;    int jaIEKYtwyU32774717 = -675991724;    int jaIEKYtwyU74028314 = -904477618;    int jaIEKYtwyU34883146 = -211202111;    int jaIEKYtwyU96245033 = -683787034;    int jaIEKYtwyU23362185 = -377877537;    int jaIEKYtwyU97972966 = -829629509;    int jaIEKYtwyU40512708 = -640294428;    int jaIEKYtwyU59831260 = -231501292;    int jaIEKYtwyU69173478 = -718000532;    int jaIEKYtwyU12037662 = -775762851;    int jaIEKYtwyU30941248 = -943985661;    int jaIEKYtwyU24397025 = -918171171;    int jaIEKYtwyU66913359 = 61500513;    int jaIEKYtwyU10090947 = -865797789;    int jaIEKYtwyU14422750 = -325868439;    int jaIEKYtwyU11270149 = -928524582;    int jaIEKYtwyU74905877 = 33846622;    int jaIEKYtwyU7961634 = -802920463;    int jaIEKYtwyU55467880 = -280847998;    int jaIEKYtwyU44949772 = -998115448;    int jaIEKYtwyU82066725 = -201492699;    int jaIEKYtwyU15721111 = -287967567;    int jaIEKYtwyU62272241 = -111673903;    int jaIEKYtwyU39900252 = -204509063;    int jaIEKYtwyU86598255 = -512423989;    int jaIEKYtwyU85577252 = -475681306;    int jaIEKYtwyU72930725 = -303299595;    int jaIEKYtwyU49384584 = -394956047;    int jaIEKYtwyU90223910 = -477681821;    int jaIEKYtwyU16791000 = -621878346;    int jaIEKYtwyU53578597 = -554080111;    int jaIEKYtwyU33583874 = -333574301;    int jaIEKYtwyU64700645 = -946968992;    int jaIEKYtwyU66520205 = -401348999;    int jaIEKYtwyU27413598 = -252095922;    int jaIEKYtwyU90926865 = -254019685;    int jaIEKYtwyU29293854 = -484808648;    int jaIEKYtwyU60608128 = -247650231;    int jaIEKYtwyU45371286 = 46478842;    int jaIEKYtwyU16513326 = -213315230;    int jaIEKYtwyU851600 = -718540416;    int jaIEKYtwyU55567951 = -396074901;    int jaIEKYtwyU3759718 = -237371731;    int jaIEKYtwyU57955152 = 91761448;    int jaIEKYtwyU41417403 = -942054437;    int jaIEKYtwyU89151677 = -210950117;    int jaIEKYtwyU9077008 = -584079965;    int jaIEKYtwyU54386248 = -573627373;    int jaIEKYtwyU52242517 = -490681683;    int jaIEKYtwyU38532883 = -65293500;    int jaIEKYtwyU29785976 = -826888385;    int jaIEKYtwyU95290855 = -251512917;    int jaIEKYtwyU8818074 = -54002374;    int jaIEKYtwyU69734487 = 92572414;    int jaIEKYtwyU46883969 = -213846974;    int jaIEKYtwyU17053606 = -288024157;    int jaIEKYtwyU11756074 = -692803715;    int jaIEKYtwyU94982894 = 93306952;    int jaIEKYtwyU9646779 = -71363046;    int jaIEKYtwyU37784933 = -902196231;    int jaIEKYtwyU25042242 = -426329914;    int jaIEKYtwyU91128123 = -145338381;    int jaIEKYtwyU69607349 = -753819471;    int jaIEKYtwyU52382479 = 3877814;    int jaIEKYtwyU58459064 = -121682740;    int jaIEKYtwyU97357373 = -510411360;    int jaIEKYtwyU59696379 = -971202180;    int jaIEKYtwyU393155 = -537150488;    int jaIEKYtwyU82677349 = -513701867;    int jaIEKYtwyU23495884 = 28151246;    int jaIEKYtwyU81976294 = -343715935;    int jaIEKYtwyU14297750 = -718503148;    int jaIEKYtwyU62590348 = -749399306;    int jaIEKYtwyU38954554 = -514920107;     jaIEKYtwyU21482854 = jaIEKYtwyU56831086;     jaIEKYtwyU56831086 = jaIEKYtwyU97869827;     jaIEKYtwyU97869827 = jaIEKYtwyU79384905;     jaIEKYtwyU79384905 = jaIEKYtwyU95470899;     jaIEKYtwyU95470899 = jaIEKYtwyU60188314;     jaIEKYtwyU60188314 = jaIEKYtwyU64955179;     jaIEKYtwyU64955179 = jaIEKYtwyU64177000;     jaIEKYtwyU64177000 = jaIEKYtwyU56853161;     jaIEKYtwyU56853161 = jaIEKYtwyU68733470;     jaIEKYtwyU68733470 = jaIEKYtwyU14486292;     jaIEKYtwyU14486292 = jaIEKYtwyU41364308;     jaIEKYtwyU41364308 = jaIEKYtwyU15399211;     jaIEKYtwyU15399211 = jaIEKYtwyU72933196;     jaIEKYtwyU72933196 = jaIEKYtwyU69992813;     jaIEKYtwyU69992813 = jaIEKYtwyU72358651;     jaIEKYtwyU72358651 = jaIEKYtwyU13548702;     jaIEKYtwyU13548702 = jaIEKYtwyU75990366;     jaIEKYtwyU75990366 = jaIEKYtwyU64477195;     jaIEKYtwyU64477195 = jaIEKYtwyU66665267;     jaIEKYtwyU66665267 = jaIEKYtwyU49803031;     jaIEKYtwyU49803031 = jaIEKYtwyU4691854;     jaIEKYtwyU4691854 = jaIEKYtwyU3252489;     jaIEKYtwyU3252489 = jaIEKYtwyU64285953;     jaIEKYtwyU64285953 = jaIEKYtwyU14684260;     jaIEKYtwyU14684260 = jaIEKYtwyU28950694;     jaIEKYtwyU28950694 = jaIEKYtwyU32774717;     jaIEKYtwyU32774717 = jaIEKYtwyU74028314;     jaIEKYtwyU74028314 = jaIEKYtwyU34883146;     jaIEKYtwyU34883146 = jaIEKYtwyU96245033;     jaIEKYtwyU96245033 = jaIEKYtwyU23362185;     jaIEKYtwyU23362185 = jaIEKYtwyU97972966;     jaIEKYtwyU97972966 = jaIEKYtwyU40512708;     jaIEKYtwyU40512708 = jaIEKYtwyU59831260;     jaIEKYtwyU59831260 = jaIEKYtwyU69173478;     jaIEKYtwyU69173478 = jaIEKYtwyU12037662;     jaIEKYtwyU12037662 = jaIEKYtwyU30941248;     jaIEKYtwyU30941248 = jaIEKYtwyU24397025;     jaIEKYtwyU24397025 = jaIEKYtwyU66913359;     jaIEKYtwyU66913359 = jaIEKYtwyU10090947;     jaIEKYtwyU10090947 = jaIEKYtwyU14422750;     jaIEKYtwyU14422750 = jaIEKYtwyU11270149;     jaIEKYtwyU11270149 = jaIEKYtwyU74905877;     jaIEKYtwyU74905877 = jaIEKYtwyU7961634;     jaIEKYtwyU7961634 = jaIEKYtwyU55467880;     jaIEKYtwyU55467880 = jaIEKYtwyU44949772;     jaIEKYtwyU44949772 = jaIEKYtwyU82066725;     jaIEKYtwyU82066725 = jaIEKYtwyU15721111;     jaIEKYtwyU15721111 = jaIEKYtwyU62272241;     jaIEKYtwyU62272241 = jaIEKYtwyU39900252;     jaIEKYtwyU39900252 = jaIEKYtwyU86598255;     jaIEKYtwyU86598255 = jaIEKYtwyU85577252;     jaIEKYtwyU85577252 = jaIEKYtwyU72930725;     jaIEKYtwyU72930725 = jaIEKYtwyU49384584;     jaIEKYtwyU49384584 = jaIEKYtwyU90223910;     jaIEKYtwyU90223910 = jaIEKYtwyU16791000;     jaIEKYtwyU16791000 = jaIEKYtwyU53578597;     jaIEKYtwyU53578597 = jaIEKYtwyU33583874;     jaIEKYtwyU33583874 = jaIEKYtwyU64700645;     jaIEKYtwyU64700645 = jaIEKYtwyU66520205;     jaIEKYtwyU66520205 = jaIEKYtwyU27413598;     jaIEKYtwyU27413598 = jaIEKYtwyU90926865;     jaIEKYtwyU90926865 = jaIEKYtwyU29293854;     jaIEKYtwyU29293854 = jaIEKYtwyU60608128;     jaIEKYtwyU60608128 = jaIEKYtwyU45371286;     jaIEKYtwyU45371286 = jaIEKYtwyU16513326;     jaIEKYtwyU16513326 = jaIEKYtwyU851600;     jaIEKYtwyU851600 = jaIEKYtwyU55567951;     jaIEKYtwyU55567951 = jaIEKYtwyU3759718;     jaIEKYtwyU3759718 = jaIEKYtwyU57955152;     jaIEKYtwyU57955152 = jaIEKYtwyU41417403;     jaIEKYtwyU41417403 = jaIEKYtwyU89151677;     jaIEKYtwyU89151677 = jaIEKYtwyU9077008;     jaIEKYtwyU9077008 = jaIEKYtwyU54386248;     jaIEKYtwyU54386248 = jaIEKYtwyU52242517;     jaIEKYtwyU52242517 = jaIEKYtwyU38532883;     jaIEKYtwyU38532883 = jaIEKYtwyU29785976;     jaIEKYtwyU29785976 = jaIEKYtwyU95290855;     jaIEKYtwyU95290855 = jaIEKYtwyU8818074;     jaIEKYtwyU8818074 = jaIEKYtwyU69734487;     jaIEKYtwyU69734487 = jaIEKYtwyU46883969;     jaIEKYtwyU46883969 = jaIEKYtwyU17053606;     jaIEKYtwyU17053606 = jaIEKYtwyU11756074;     jaIEKYtwyU11756074 = jaIEKYtwyU94982894;     jaIEKYtwyU94982894 = jaIEKYtwyU9646779;     jaIEKYtwyU9646779 = jaIEKYtwyU37784933;     jaIEKYtwyU37784933 = jaIEKYtwyU25042242;     jaIEKYtwyU25042242 = jaIEKYtwyU91128123;     jaIEKYtwyU91128123 = jaIEKYtwyU69607349;     jaIEKYtwyU69607349 = jaIEKYtwyU52382479;     jaIEKYtwyU52382479 = jaIEKYtwyU58459064;     jaIEKYtwyU58459064 = jaIEKYtwyU97357373;     jaIEKYtwyU97357373 = jaIEKYtwyU59696379;     jaIEKYtwyU59696379 = jaIEKYtwyU393155;     jaIEKYtwyU393155 = jaIEKYtwyU82677349;     jaIEKYtwyU82677349 = jaIEKYtwyU23495884;     jaIEKYtwyU23495884 = jaIEKYtwyU81976294;     jaIEKYtwyU81976294 = jaIEKYtwyU14297750;     jaIEKYtwyU14297750 = jaIEKYtwyU62590348;     jaIEKYtwyU62590348 = jaIEKYtwyU38954554;     jaIEKYtwyU38954554 = jaIEKYtwyU21482854;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void rxqIovRAKY46025291() {     int tphdBpCcJr30798120 = -417451133;    int tphdBpCcJr12254008 = -725047993;    int tphdBpCcJr18021908 = -582065286;    int tphdBpCcJr37174927 = -44176017;    int tphdBpCcJr35534139 = -993203829;    int tphdBpCcJr53731676 = -486956506;    int tphdBpCcJr38609251 = 42253820;    int tphdBpCcJr53313156 = -992969906;    int tphdBpCcJr49661819 = -311497936;    int tphdBpCcJr37357928 = -252194362;    int tphdBpCcJr51388277 = -234185811;    int tphdBpCcJr52602057 = -996204658;    int tphdBpCcJr11180927 = -312998327;    int tphdBpCcJr5021838 = 65573633;    int tphdBpCcJr85268851 = -635531813;    int tphdBpCcJr77329175 = -913563575;    int tphdBpCcJr57359469 = -156706374;    int tphdBpCcJr88183007 = -352636765;    int tphdBpCcJr46490127 = -393065991;    int tphdBpCcJr29860615 = -995257962;    int tphdBpCcJr38482382 = -319518010;    int tphdBpCcJr28900766 = -715938094;    int tphdBpCcJr39313756 = -768962379;    int tphdBpCcJr23492583 = 47988518;    int tphdBpCcJr77746802 = -224676085;    int tphdBpCcJr94858511 = -614551314;    int tphdBpCcJr66532984 = -384880569;    int tphdBpCcJr45122790 = -311614350;    int tphdBpCcJr2000054 = -325621547;    int tphdBpCcJr9025982 = -489871597;    int tphdBpCcJr93360563 = 11470646;    int tphdBpCcJr75232330 = -160800804;    int tphdBpCcJr90372037 = -451385440;    int tphdBpCcJr48860208 = -815061179;    int tphdBpCcJr55823754 = -894413126;    int tphdBpCcJr43862556 = -91988298;    int tphdBpCcJr68408855 = -545185523;    int tphdBpCcJr80433823 = -670766534;    int tphdBpCcJr33139061 = 10284726;    int tphdBpCcJr86190843 = -572983301;    int tphdBpCcJr38485884 = -463024856;    int tphdBpCcJr98921804 = -849796673;    int tphdBpCcJr96622883 = -188033759;    int tphdBpCcJr66718534 = -193315351;    int tphdBpCcJr91417954 = -72623792;    int tphdBpCcJr15189008 = -223650007;    int tphdBpCcJr87316356 = -874326844;    int tphdBpCcJr87192148 = -734510956;    int tphdBpCcJr28299385 = -347370420;    int tphdBpCcJr9538154 = 2477235;    int tphdBpCcJr23565964 = -248299985;    int tphdBpCcJr49794294 = -23667524;    int tphdBpCcJr60938383 = -991076501;    int tphdBpCcJr53965828 = -754809165;    int tphdBpCcJr7542909 = -412599906;    int tphdBpCcJr1897355 = -701513039;    int tphdBpCcJr72940252 = -956085614;    int tphdBpCcJr94529325 = -530053804;    int tphdBpCcJr59428125 = -819499933;    int tphdBpCcJr40675627 = -278652516;    int tphdBpCcJr87198692 = -2075937;    int tphdBpCcJr93486461 = -646131830;    int tphdBpCcJr51313103 = -567348360;    int tphdBpCcJr40635837 = -821626339;    int tphdBpCcJr43997364 = -163665009;    int tphdBpCcJr76155946 = 26614992;    int tphdBpCcJr62230019 = -444819218;    int tphdBpCcJr62320719 = -497937149;    int tphdBpCcJr49198083 = -40013241;    int tphdBpCcJr41406296 = -443543516;    int tphdBpCcJr8920320 = -268378052;    int tphdBpCcJr76925646 = -485939841;    int tphdBpCcJr55043947 = -262921491;    int tphdBpCcJr60299283 = -820082690;    int tphdBpCcJr91374731 = -432233106;    int tphdBpCcJr39560578 = -469721337;    int tphdBpCcJr32277883 = -427904336;    int tphdBpCcJr72595221 = -475647029;    int tphdBpCcJr32074628 = -879387691;    int tphdBpCcJr62557795 = 98973921;    int tphdBpCcJr7542156 = -740224470;    int tphdBpCcJr79340835 = -650369614;    int tphdBpCcJr16823405 = -964243931;    int tphdBpCcJr92461899 = -228098782;    int tphdBpCcJr85460017 = -141571612;    int tphdBpCcJr43566270 = -964861830;    int tphdBpCcJr14293948 = -169724303;    int tphdBpCcJr36406210 = -696576276;    int tphdBpCcJr41317299 = -302461274;    int tphdBpCcJr53926400 = -92900087;    int tphdBpCcJr70922303 = -135902684;    int tphdBpCcJr73879530 = 84868280;    int tphdBpCcJr21005698 = -851266601;    int tphdBpCcJr92463433 = -711062759;    int tphdBpCcJr98992150 = -470907364;    int tphdBpCcJr44999422 = -816893027;    int tphdBpCcJr47608701 = -182448314;    int tphdBpCcJr55987046 = -366407420;    int tphdBpCcJr22721171 = 70349657;    int tphdBpCcJr15262008 = -417451133;     tphdBpCcJr30798120 = tphdBpCcJr12254008;     tphdBpCcJr12254008 = tphdBpCcJr18021908;     tphdBpCcJr18021908 = tphdBpCcJr37174927;     tphdBpCcJr37174927 = tphdBpCcJr35534139;     tphdBpCcJr35534139 = tphdBpCcJr53731676;     tphdBpCcJr53731676 = tphdBpCcJr38609251;     tphdBpCcJr38609251 = tphdBpCcJr53313156;     tphdBpCcJr53313156 = tphdBpCcJr49661819;     tphdBpCcJr49661819 = tphdBpCcJr37357928;     tphdBpCcJr37357928 = tphdBpCcJr51388277;     tphdBpCcJr51388277 = tphdBpCcJr52602057;     tphdBpCcJr52602057 = tphdBpCcJr11180927;     tphdBpCcJr11180927 = tphdBpCcJr5021838;     tphdBpCcJr5021838 = tphdBpCcJr85268851;     tphdBpCcJr85268851 = tphdBpCcJr77329175;     tphdBpCcJr77329175 = tphdBpCcJr57359469;     tphdBpCcJr57359469 = tphdBpCcJr88183007;     tphdBpCcJr88183007 = tphdBpCcJr46490127;     tphdBpCcJr46490127 = tphdBpCcJr29860615;     tphdBpCcJr29860615 = tphdBpCcJr38482382;     tphdBpCcJr38482382 = tphdBpCcJr28900766;     tphdBpCcJr28900766 = tphdBpCcJr39313756;     tphdBpCcJr39313756 = tphdBpCcJr23492583;     tphdBpCcJr23492583 = tphdBpCcJr77746802;     tphdBpCcJr77746802 = tphdBpCcJr94858511;     tphdBpCcJr94858511 = tphdBpCcJr66532984;     tphdBpCcJr66532984 = tphdBpCcJr45122790;     tphdBpCcJr45122790 = tphdBpCcJr2000054;     tphdBpCcJr2000054 = tphdBpCcJr9025982;     tphdBpCcJr9025982 = tphdBpCcJr93360563;     tphdBpCcJr93360563 = tphdBpCcJr75232330;     tphdBpCcJr75232330 = tphdBpCcJr90372037;     tphdBpCcJr90372037 = tphdBpCcJr48860208;     tphdBpCcJr48860208 = tphdBpCcJr55823754;     tphdBpCcJr55823754 = tphdBpCcJr43862556;     tphdBpCcJr43862556 = tphdBpCcJr68408855;     tphdBpCcJr68408855 = tphdBpCcJr80433823;     tphdBpCcJr80433823 = tphdBpCcJr33139061;     tphdBpCcJr33139061 = tphdBpCcJr86190843;     tphdBpCcJr86190843 = tphdBpCcJr38485884;     tphdBpCcJr38485884 = tphdBpCcJr98921804;     tphdBpCcJr98921804 = tphdBpCcJr96622883;     tphdBpCcJr96622883 = tphdBpCcJr66718534;     tphdBpCcJr66718534 = tphdBpCcJr91417954;     tphdBpCcJr91417954 = tphdBpCcJr15189008;     tphdBpCcJr15189008 = tphdBpCcJr87316356;     tphdBpCcJr87316356 = tphdBpCcJr87192148;     tphdBpCcJr87192148 = tphdBpCcJr28299385;     tphdBpCcJr28299385 = tphdBpCcJr9538154;     tphdBpCcJr9538154 = tphdBpCcJr23565964;     tphdBpCcJr23565964 = tphdBpCcJr49794294;     tphdBpCcJr49794294 = tphdBpCcJr60938383;     tphdBpCcJr60938383 = tphdBpCcJr53965828;     tphdBpCcJr53965828 = tphdBpCcJr7542909;     tphdBpCcJr7542909 = tphdBpCcJr1897355;     tphdBpCcJr1897355 = tphdBpCcJr72940252;     tphdBpCcJr72940252 = tphdBpCcJr94529325;     tphdBpCcJr94529325 = tphdBpCcJr59428125;     tphdBpCcJr59428125 = tphdBpCcJr40675627;     tphdBpCcJr40675627 = tphdBpCcJr87198692;     tphdBpCcJr87198692 = tphdBpCcJr93486461;     tphdBpCcJr93486461 = tphdBpCcJr51313103;     tphdBpCcJr51313103 = tphdBpCcJr40635837;     tphdBpCcJr40635837 = tphdBpCcJr43997364;     tphdBpCcJr43997364 = tphdBpCcJr76155946;     tphdBpCcJr76155946 = tphdBpCcJr62230019;     tphdBpCcJr62230019 = tphdBpCcJr62320719;     tphdBpCcJr62320719 = tphdBpCcJr49198083;     tphdBpCcJr49198083 = tphdBpCcJr41406296;     tphdBpCcJr41406296 = tphdBpCcJr8920320;     tphdBpCcJr8920320 = tphdBpCcJr76925646;     tphdBpCcJr76925646 = tphdBpCcJr55043947;     tphdBpCcJr55043947 = tphdBpCcJr60299283;     tphdBpCcJr60299283 = tphdBpCcJr91374731;     tphdBpCcJr91374731 = tphdBpCcJr39560578;     tphdBpCcJr39560578 = tphdBpCcJr32277883;     tphdBpCcJr32277883 = tphdBpCcJr72595221;     tphdBpCcJr72595221 = tphdBpCcJr32074628;     tphdBpCcJr32074628 = tphdBpCcJr62557795;     tphdBpCcJr62557795 = tphdBpCcJr7542156;     tphdBpCcJr7542156 = tphdBpCcJr79340835;     tphdBpCcJr79340835 = tphdBpCcJr16823405;     tphdBpCcJr16823405 = tphdBpCcJr92461899;     tphdBpCcJr92461899 = tphdBpCcJr85460017;     tphdBpCcJr85460017 = tphdBpCcJr43566270;     tphdBpCcJr43566270 = tphdBpCcJr14293948;     tphdBpCcJr14293948 = tphdBpCcJr36406210;     tphdBpCcJr36406210 = tphdBpCcJr41317299;     tphdBpCcJr41317299 = tphdBpCcJr53926400;     tphdBpCcJr53926400 = tphdBpCcJr70922303;     tphdBpCcJr70922303 = tphdBpCcJr73879530;     tphdBpCcJr73879530 = tphdBpCcJr21005698;     tphdBpCcJr21005698 = tphdBpCcJr92463433;     tphdBpCcJr92463433 = tphdBpCcJr98992150;     tphdBpCcJr98992150 = tphdBpCcJr44999422;     tphdBpCcJr44999422 = tphdBpCcJr47608701;     tphdBpCcJr47608701 = tphdBpCcJr55987046;     tphdBpCcJr55987046 = tphdBpCcJr22721171;     tphdBpCcJr22721171 = tphdBpCcJr15262008;     tphdBpCcJr15262008 = tphdBpCcJr30798120;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void nnLPKCQuCh75044414() {     long XOVPfQPSzc99978988 = -376881136;    long XOVPfQPSzc57767261 = -419900232;    long XOVPfQPSzc19322805 = -930322205;    long XOVPfQPSzc2816200 = -100344225;    long XOVPfQPSzc73590770 = 36160304;    long XOVPfQPSzc9995425 = -777483273;    long XOVPfQPSzc28284055 = -575120169;    long XOVPfQPSzc26088313 = -633702962;    long XOVPfQPSzc72083993 = -600723557;    long XOVPfQPSzc97526106 = -874124093;    long XOVPfQPSzc37539199 = -416346816;    long XOVPfQPSzc46954895 = -173334291;    long XOVPfQPSzc58432652 = -438257694;    long XOVPfQPSzc40996900 = -896760901;    long XOVPfQPSzc8935654 = 44667654;    long XOVPfQPSzc26325642 = -831701746;    long XOVPfQPSzc27024006 = -247464432;    long XOVPfQPSzc82770602 = -961184313;    long XOVPfQPSzc68124005 = -102774217;    long XOVPfQPSzc65823918 = -641118058;    long XOVPfQPSzc38826112 = -28875812;    long XOVPfQPSzc77898428 = -568227646;    long XOVPfQPSzc88234599 = -275728761;    long XOVPfQPSzc79340749 = -522298870;    long XOVPfQPSzc4477519 = -708665508;    long XOVPfQPSzc87692960 = -173146630;    long XOVPfQPSzc41100893 = -710240061;    long XOVPfQPSzc37301296 = -970069760;    long XOVPfQPSzc73756021 = -210429828;    long XOVPfQPSzc44130187 = -15842985;    long XOVPfQPSzc1661389 = -306381170;    long XOVPfQPSzc34029601 = -30924881;    long XOVPfQPSzc88077855 = 60318437;    long XOVPfQPSzc14404058 = -978022073;    long XOVPfQPSzc59825043 = -644632465;    long XOVPfQPSzc75991680 = -579891518;    long XOVPfQPSzc95569604 = -480136205;    long XOVPfQPSzc49510438 = -551381001;    long XOVPfQPSzc82973740 = -399320786;    long XOVPfQPSzc82619714 = -712250648;    long XOVPfQPSzc33668929 = -729249872;    long XOVPfQPSzc14051073 = -98815654;    long XOVPfQPSzc8877248 = -83401363;    long XOVPfQPSzc93853366 = -265832605;    long XOVPfQPSzc90426176 = -68463882;    long XOVPfQPSzc15524210 = -655616498;    long XOVPfQPSzc96764937 = -950279267;    long XOVPfQPSzc87506202 = -540349786;    long XOVPfQPSzc14730862 = -976832416;    long XOVPfQPSzc24952429 = 82678762;    long XOVPfQPSzc97742361 = -182987696;    long XOVPfQPSzc38422301 = -359909094;    long XOVPfQPSzc18804021 = 92932077;    long XOVPfQPSzc68452848 = -281006815;    long XOVPfQPSzc36218303 = 84958184;    long XOVPfQPSzc22080561 = -808653491;    long XOVPfQPSzc69532661 = -44171472;    long XOVPfQPSzc39982055 = -308023335;    long XOVPfQPSzc98338681 = -391678717;    long XOVPfQPSzc85897809 = -790693066;    long XOVPfQPSzc68894531 = 32756788;    long XOVPfQPSzc90982758 = -605050409;    long XOVPfQPSzc52332292 = -323273134;    long XOVPfQPSzc27953806 = -484880572;    long XOVPfQPSzc95864718 = -467742924;    long XOVPfQPSzc3509598 = -285421935;    long XOVPfQPSzc58877040 = -133652729;    long XOVPfQPSzc44028595 = -460235621;    long XOVPfQPSzc81171857 = -152128436;    long XOVPfQPSzc32943973 = -375440829;    long XOVPfQPSzc30756037 = -251565542;    long XOVPfQPSzc77513568 = -696083431;    long XOVPfQPSzc99796861 = -461863527;    long XOVPfQPSzc85504290 = -390523569;    long XOVPfQPSzc32154990 = -911868186;    long XOVPfQPSzc24775039 = -930060159;    long XOVPfQPSzc69021180 = -384826283;    long XOVPfQPSzc94381233 = 90103843;    long XOVPfQPSzc88914572 = -353834989;    long XOVPfQPSzc88953309 = 46950990;    long XOVPfQPSzc90928022 = -222867363;    long XOVPfQPSzc53594690 = -69890276;    long XOVPfQPSzc22570434 = -993237344;    long XOVPfQPSzc48803593 = -193108591;    long XOVPfQPSzc46387825 = -832855290;    long XOVPfQPSzc63239087 = -946472077;    long XOVPfQPSzc15225581 = -23856959;    long XOVPfQPSzc19625008 = -658674748;    long XOVPfQPSzc78185755 = -962980258;    long XOVPfQPSzc37744482 = -835978975;    long XOVPfQPSzc6459020 = -435720046;    long XOVPfQPSzc55587549 = -72112870;    long XOVPfQPSzc51171756 = -59702284;    long XOVPfQPSzc97075930 = -608627720;    long XOVPfQPSzc13725184 = -645007436;    long XOVPfQPSzc42686170 = -24199463;    long XOVPfQPSzc61718781 = -775542520;    long XOVPfQPSzc80923442 = -598520792;    long XOVPfQPSzc97988648 = -798089681;    long XOVPfQPSzc86916579 = -376881136;     XOVPfQPSzc99978988 = XOVPfQPSzc57767261;     XOVPfQPSzc57767261 = XOVPfQPSzc19322805;     XOVPfQPSzc19322805 = XOVPfQPSzc2816200;     XOVPfQPSzc2816200 = XOVPfQPSzc73590770;     XOVPfQPSzc73590770 = XOVPfQPSzc9995425;     XOVPfQPSzc9995425 = XOVPfQPSzc28284055;     XOVPfQPSzc28284055 = XOVPfQPSzc26088313;     XOVPfQPSzc26088313 = XOVPfQPSzc72083993;     XOVPfQPSzc72083993 = XOVPfQPSzc97526106;     XOVPfQPSzc97526106 = XOVPfQPSzc37539199;     XOVPfQPSzc37539199 = XOVPfQPSzc46954895;     XOVPfQPSzc46954895 = XOVPfQPSzc58432652;     XOVPfQPSzc58432652 = XOVPfQPSzc40996900;     XOVPfQPSzc40996900 = XOVPfQPSzc8935654;     XOVPfQPSzc8935654 = XOVPfQPSzc26325642;     XOVPfQPSzc26325642 = XOVPfQPSzc27024006;     XOVPfQPSzc27024006 = XOVPfQPSzc82770602;     XOVPfQPSzc82770602 = XOVPfQPSzc68124005;     XOVPfQPSzc68124005 = XOVPfQPSzc65823918;     XOVPfQPSzc65823918 = XOVPfQPSzc38826112;     XOVPfQPSzc38826112 = XOVPfQPSzc77898428;     XOVPfQPSzc77898428 = XOVPfQPSzc88234599;     XOVPfQPSzc88234599 = XOVPfQPSzc79340749;     XOVPfQPSzc79340749 = XOVPfQPSzc4477519;     XOVPfQPSzc4477519 = XOVPfQPSzc87692960;     XOVPfQPSzc87692960 = XOVPfQPSzc41100893;     XOVPfQPSzc41100893 = XOVPfQPSzc37301296;     XOVPfQPSzc37301296 = XOVPfQPSzc73756021;     XOVPfQPSzc73756021 = XOVPfQPSzc44130187;     XOVPfQPSzc44130187 = XOVPfQPSzc1661389;     XOVPfQPSzc1661389 = XOVPfQPSzc34029601;     XOVPfQPSzc34029601 = XOVPfQPSzc88077855;     XOVPfQPSzc88077855 = XOVPfQPSzc14404058;     XOVPfQPSzc14404058 = XOVPfQPSzc59825043;     XOVPfQPSzc59825043 = XOVPfQPSzc75991680;     XOVPfQPSzc75991680 = XOVPfQPSzc95569604;     XOVPfQPSzc95569604 = XOVPfQPSzc49510438;     XOVPfQPSzc49510438 = XOVPfQPSzc82973740;     XOVPfQPSzc82973740 = XOVPfQPSzc82619714;     XOVPfQPSzc82619714 = XOVPfQPSzc33668929;     XOVPfQPSzc33668929 = XOVPfQPSzc14051073;     XOVPfQPSzc14051073 = XOVPfQPSzc8877248;     XOVPfQPSzc8877248 = XOVPfQPSzc93853366;     XOVPfQPSzc93853366 = XOVPfQPSzc90426176;     XOVPfQPSzc90426176 = XOVPfQPSzc15524210;     XOVPfQPSzc15524210 = XOVPfQPSzc96764937;     XOVPfQPSzc96764937 = XOVPfQPSzc87506202;     XOVPfQPSzc87506202 = XOVPfQPSzc14730862;     XOVPfQPSzc14730862 = XOVPfQPSzc24952429;     XOVPfQPSzc24952429 = XOVPfQPSzc97742361;     XOVPfQPSzc97742361 = XOVPfQPSzc38422301;     XOVPfQPSzc38422301 = XOVPfQPSzc18804021;     XOVPfQPSzc18804021 = XOVPfQPSzc68452848;     XOVPfQPSzc68452848 = XOVPfQPSzc36218303;     XOVPfQPSzc36218303 = XOVPfQPSzc22080561;     XOVPfQPSzc22080561 = XOVPfQPSzc69532661;     XOVPfQPSzc69532661 = XOVPfQPSzc39982055;     XOVPfQPSzc39982055 = XOVPfQPSzc98338681;     XOVPfQPSzc98338681 = XOVPfQPSzc85897809;     XOVPfQPSzc85897809 = XOVPfQPSzc68894531;     XOVPfQPSzc68894531 = XOVPfQPSzc90982758;     XOVPfQPSzc90982758 = XOVPfQPSzc52332292;     XOVPfQPSzc52332292 = XOVPfQPSzc27953806;     XOVPfQPSzc27953806 = XOVPfQPSzc95864718;     XOVPfQPSzc95864718 = XOVPfQPSzc3509598;     XOVPfQPSzc3509598 = XOVPfQPSzc58877040;     XOVPfQPSzc58877040 = XOVPfQPSzc44028595;     XOVPfQPSzc44028595 = XOVPfQPSzc81171857;     XOVPfQPSzc81171857 = XOVPfQPSzc32943973;     XOVPfQPSzc32943973 = XOVPfQPSzc30756037;     XOVPfQPSzc30756037 = XOVPfQPSzc77513568;     XOVPfQPSzc77513568 = XOVPfQPSzc99796861;     XOVPfQPSzc99796861 = XOVPfQPSzc85504290;     XOVPfQPSzc85504290 = XOVPfQPSzc32154990;     XOVPfQPSzc32154990 = XOVPfQPSzc24775039;     XOVPfQPSzc24775039 = XOVPfQPSzc69021180;     XOVPfQPSzc69021180 = XOVPfQPSzc94381233;     XOVPfQPSzc94381233 = XOVPfQPSzc88914572;     XOVPfQPSzc88914572 = XOVPfQPSzc88953309;     XOVPfQPSzc88953309 = XOVPfQPSzc90928022;     XOVPfQPSzc90928022 = XOVPfQPSzc53594690;     XOVPfQPSzc53594690 = XOVPfQPSzc22570434;     XOVPfQPSzc22570434 = XOVPfQPSzc48803593;     XOVPfQPSzc48803593 = XOVPfQPSzc46387825;     XOVPfQPSzc46387825 = XOVPfQPSzc63239087;     XOVPfQPSzc63239087 = XOVPfQPSzc15225581;     XOVPfQPSzc15225581 = XOVPfQPSzc19625008;     XOVPfQPSzc19625008 = XOVPfQPSzc78185755;     XOVPfQPSzc78185755 = XOVPfQPSzc37744482;     XOVPfQPSzc37744482 = XOVPfQPSzc6459020;     XOVPfQPSzc6459020 = XOVPfQPSzc55587549;     XOVPfQPSzc55587549 = XOVPfQPSzc51171756;     XOVPfQPSzc51171756 = XOVPfQPSzc97075930;     XOVPfQPSzc97075930 = XOVPfQPSzc13725184;     XOVPfQPSzc13725184 = XOVPfQPSzc42686170;     XOVPfQPSzc42686170 = XOVPfQPSzc61718781;     XOVPfQPSzc61718781 = XOVPfQPSzc80923442;     XOVPfQPSzc80923442 = XOVPfQPSzc97988648;     XOVPfQPSzc97988648 = XOVPfQPSzc86916579;     XOVPfQPSzc86916579 = XOVPfQPSzc99978988;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void XwPpYMHofG81042764() {     long JXlMoKPLAa9294255 = -279412162;    long JXlMoKPLAa13190184 = -436434735;    long JXlMoKPLAa39474885 = -643962818;    long JXlMoKPLAa60606222 = -292008217;    long JXlMoKPLAa13654009 = 59645147;    long JXlMoKPLAa3538787 = -236352133;    long JXlMoKPLAa1938126 = -374369047;    long JXlMoKPLAa15224470 = -830662110;    long JXlMoKPLAa64892650 = -980784228;    long JXlMoKPLAa66150564 = -694919761;    long JXlMoKPLAa74441183 = -607587888;    long JXlMoKPLAa58192645 = -810704107;    long JXlMoKPLAa54214368 = -23679829;    long JXlMoKPLAa73085541 = -875815006;    long JXlMoKPLAa24211692 = -906862757;    long JXlMoKPLAa31296166 = -859225224;    long JXlMoKPLAa70834774 = -275049519;    long JXlMoKPLAa94963243 = -691241625;    long JXlMoKPLAa50136937 = -56415046;    long JXlMoKPLAa29019267 = -719825899;    long JXlMoKPLAa27505463 = -354575741;    long JXlMoKPLAa2107340 = -391123978;    long JXlMoKPLAa24295867 = -990257761;    long JXlMoKPLAa38547379 = -39459980;    long JXlMoKPLAa67540061 = 72201441;    long JXlMoKPLAa53600778 = -272358271;    long JXlMoKPLAa74859159 = -419128906;    long JXlMoKPLAa8395772 = -377206493;    long JXlMoKPLAa40872928 = -324849264;    long JXlMoKPLAa56911135 = -921927548;    long JXlMoKPLAa71659767 = 82967013;    long JXlMoKPLAa11288965 = -462096176;    long JXlMoKPLAa37937186 = -850772575;    long JXlMoKPLAa3433006 = -461581960;    long JXlMoKPLAa46475318 = -821045060;    long JXlMoKPLAa7816575 = -996116965;    long JXlMoKPLAa33037212 = -81336067;    long JXlMoKPLAa5547236 = -303976364;    long JXlMoKPLAa49199441 = -450536573;    long JXlMoKPLAa58719612 = -419436161;    long JXlMoKPLAa57732063 = -866406290;    long JXlMoKPLAa1702729 = -20087745;    long JXlMoKPLAa30594254 = -305281745;    long JXlMoKPLAa52610267 = -756227493;    long JXlMoKPLAa26376252 = -960239676;    long JXlMoKPLAa85763444 = -981151057;    long JXlMoKPLAa2014569 = -523113413;    long JXlMoKPLAa58977240 = -986893174;    long JXlMoKPLAa80758005 = -112528933;    long JXlMoKPLAa94590330 = -810334940;    long JXlMoKPLAa34710071 = 81136307;    long JXlMoKPLAa2639343 = 92104688;    long JXlMoKPLAa6811679 = -594844829;    long JXlMoKPLAa73034092 = -640859933;    long JXlMoKPLAa53537301 = -949959901;    long JXlMoKPLAa7186915 = -888288184;    long JXlMoKPLAa88894317 = -446176975;    long JXlMoKPLAa927507 = -504502838;    long JXlMoKPLAa93066161 = -264209659;    long JXlMoKPLAa60053231 = -667996582;    long JXlMoKPLAa28679627 = -817223228;    long JXlMoKPLAa93542354 = -997162555;    long JXlMoKPLAa74351541 = -405812847;    long JXlMoKPLAa7981515 = 41143320;    long JXlMoKPLAa94490796 = -677886775;    long JXlMoKPLAa63152219 = -45491713;    long JXlMoKPLAa20255459 = -959931532;    long JXlMoKPLAa50781362 = -562097869;    long JXlMoKPLAa26610224 = 45230053;    long JXlMoKPLAa16395117 = -910745793;    long JXlMoKPLAa98258953 = -677889157;    long JXlMoKPLAa65287538 = -971073156;    long JXlMoKPLAa45763802 = -140705053;    long JXlMoKPLAa91417324 = -636978886;    long JXlMoKPLAa71287204 = -853419610;    long JXlMoKPLAa25802735 = -234487996;    long JXlMoKPLAa71513086 = 14157766;    long JXlMoKPLAa71685599 = -134030269;    long JXlMoKPLAa12171128 = -79220305;    long JXlMoKPLAa81776616 = 53352498;    long JXlMoKPLAa51586209 = -749244859;    long JXlMoKPLAa15881920 = -432235732;    long JXlMoKPLAa27637766 = -164677560;    long JXlMoKPLAa46282598 = -514514324;    long JXlMoKPLAa22201065 = -903063856;    long JXlMoKPLAa69020424 = 90862324;    long JXlMoKPLAa4477286 = -867251347;    long JXlMoKPLAa64903093 = -109912643;    long JXlMoKPLAa49895704 = -511622060;    long JXlMoKPLAa39288403 = -932756876;    long JXlMoKPLAa18922258 = -449939991;    long JXlMoKPLAa32109706 = -576833230;    long JXlMoKPLAa12481075 = 60233294;    long JXlMoKPLAa89146210 = -782539991;    long JXlMoKPLAa30039985 = -602212933;    long JXlMoKPLAa64189708 = -869243736;    long JXlMoKPLAa27351188 = -614274899;    long JXlMoKPLAa22612739 = -246425065;    long JXlMoKPLAa58119470 = 21659282;    long JXlMoKPLAa63224033 = -279412162;     JXlMoKPLAa9294255 = JXlMoKPLAa13190184;     JXlMoKPLAa13190184 = JXlMoKPLAa39474885;     JXlMoKPLAa39474885 = JXlMoKPLAa60606222;     JXlMoKPLAa60606222 = JXlMoKPLAa13654009;     JXlMoKPLAa13654009 = JXlMoKPLAa3538787;     JXlMoKPLAa3538787 = JXlMoKPLAa1938126;     JXlMoKPLAa1938126 = JXlMoKPLAa15224470;     JXlMoKPLAa15224470 = JXlMoKPLAa64892650;     JXlMoKPLAa64892650 = JXlMoKPLAa66150564;     JXlMoKPLAa66150564 = JXlMoKPLAa74441183;     JXlMoKPLAa74441183 = JXlMoKPLAa58192645;     JXlMoKPLAa58192645 = JXlMoKPLAa54214368;     JXlMoKPLAa54214368 = JXlMoKPLAa73085541;     JXlMoKPLAa73085541 = JXlMoKPLAa24211692;     JXlMoKPLAa24211692 = JXlMoKPLAa31296166;     JXlMoKPLAa31296166 = JXlMoKPLAa70834774;     JXlMoKPLAa70834774 = JXlMoKPLAa94963243;     JXlMoKPLAa94963243 = JXlMoKPLAa50136937;     JXlMoKPLAa50136937 = JXlMoKPLAa29019267;     JXlMoKPLAa29019267 = JXlMoKPLAa27505463;     JXlMoKPLAa27505463 = JXlMoKPLAa2107340;     JXlMoKPLAa2107340 = JXlMoKPLAa24295867;     JXlMoKPLAa24295867 = JXlMoKPLAa38547379;     JXlMoKPLAa38547379 = JXlMoKPLAa67540061;     JXlMoKPLAa67540061 = JXlMoKPLAa53600778;     JXlMoKPLAa53600778 = JXlMoKPLAa74859159;     JXlMoKPLAa74859159 = JXlMoKPLAa8395772;     JXlMoKPLAa8395772 = JXlMoKPLAa40872928;     JXlMoKPLAa40872928 = JXlMoKPLAa56911135;     JXlMoKPLAa56911135 = JXlMoKPLAa71659767;     JXlMoKPLAa71659767 = JXlMoKPLAa11288965;     JXlMoKPLAa11288965 = JXlMoKPLAa37937186;     JXlMoKPLAa37937186 = JXlMoKPLAa3433006;     JXlMoKPLAa3433006 = JXlMoKPLAa46475318;     JXlMoKPLAa46475318 = JXlMoKPLAa7816575;     JXlMoKPLAa7816575 = JXlMoKPLAa33037212;     JXlMoKPLAa33037212 = JXlMoKPLAa5547236;     JXlMoKPLAa5547236 = JXlMoKPLAa49199441;     JXlMoKPLAa49199441 = JXlMoKPLAa58719612;     JXlMoKPLAa58719612 = JXlMoKPLAa57732063;     JXlMoKPLAa57732063 = JXlMoKPLAa1702729;     JXlMoKPLAa1702729 = JXlMoKPLAa30594254;     JXlMoKPLAa30594254 = JXlMoKPLAa52610267;     JXlMoKPLAa52610267 = JXlMoKPLAa26376252;     JXlMoKPLAa26376252 = JXlMoKPLAa85763444;     JXlMoKPLAa85763444 = JXlMoKPLAa2014569;     JXlMoKPLAa2014569 = JXlMoKPLAa58977240;     JXlMoKPLAa58977240 = JXlMoKPLAa80758005;     JXlMoKPLAa80758005 = JXlMoKPLAa94590330;     JXlMoKPLAa94590330 = JXlMoKPLAa34710071;     JXlMoKPLAa34710071 = JXlMoKPLAa2639343;     JXlMoKPLAa2639343 = JXlMoKPLAa6811679;     JXlMoKPLAa6811679 = JXlMoKPLAa73034092;     JXlMoKPLAa73034092 = JXlMoKPLAa53537301;     JXlMoKPLAa53537301 = JXlMoKPLAa7186915;     JXlMoKPLAa7186915 = JXlMoKPLAa88894317;     JXlMoKPLAa88894317 = JXlMoKPLAa927507;     JXlMoKPLAa927507 = JXlMoKPLAa93066161;     JXlMoKPLAa93066161 = JXlMoKPLAa60053231;     JXlMoKPLAa60053231 = JXlMoKPLAa28679627;     JXlMoKPLAa28679627 = JXlMoKPLAa93542354;     JXlMoKPLAa93542354 = JXlMoKPLAa74351541;     JXlMoKPLAa74351541 = JXlMoKPLAa7981515;     JXlMoKPLAa7981515 = JXlMoKPLAa94490796;     JXlMoKPLAa94490796 = JXlMoKPLAa63152219;     JXlMoKPLAa63152219 = JXlMoKPLAa20255459;     JXlMoKPLAa20255459 = JXlMoKPLAa50781362;     JXlMoKPLAa50781362 = JXlMoKPLAa26610224;     JXlMoKPLAa26610224 = JXlMoKPLAa16395117;     JXlMoKPLAa16395117 = JXlMoKPLAa98258953;     JXlMoKPLAa98258953 = JXlMoKPLAa65287538;     JXlMoKPLAa65287538 = JXlMoKPLAa45763802;     JXlMoKPLAa45763802 = JXlMoKPLAa91417324;     JXlMoKPLAa91417324 = JXlMoKPLAa71287204;     JXlMoKPLAa71287204 = JXlMoKPLAa25802735;     JXlMoKPLAa25802735 = JXlMoKPLAa71513086;     JXlMoKPLAa71513086 = JXlMoKPLAa71685599;     JXlMoKPLAa71685599 = JXlMoKPLAa12171128;     JXlMoKPLAa12171128 = JXlMoKPLAa81776616;     JXlMoKPLAa81776616 = JXlMoKPLAa51586209;     JXlMoKPLAa51586209 = JXlMoKPLAa15881920;     JXlMoKPLAa15881920 = JXlMoKPLAa27637766;     JXlMoKPLAa27637766 = JXlMoKPLAa46282598;     JXlMoKPLAa46282598 = JXlMoKPLAa22201065;     JXlMoKPLAa22201065 = JXlMoKPLAa69020424;     JXlMoKPLAa69020424 = JXlMoKPLAa4477286;     JXlMoKPLAa4477286 = JXlMoKPLAa64903093;     JXlMoKPLAa64903093 = JXlMoKPLAa49895704;     JXlMoKPLAa49895704 = JXlMoKPLAa39288403;     JXlMoKPLAa39288403 = JXlMoKPLAa18922258;     JXlMoKPLAa18922258 = JXlMoKPLAa32109706;     JXlMoKPLAa32109706 = JXlMoKPLAa12481075;     JXlMoKPLAa12481075 = JXlMoKPLAa89146210;     JXlMoKPLAa89146210 = JXlMoKPLAa30039985;     JXlMoKPLAa30039985 = JXlMoKPLAa64189708;     JXlMoKPLAa64189708 = JXlMoKPLAa27351188;     JXlMoKPLAa27351188 = JXlMoKPLAa22612739;     JXlMoKPLAa22612739 = JXlMoKPLAa58119470;     JXlMoKPLAa58119470 = JXlMoKPLAa63224033;     JXlMoKPLAa63224033 = JXlMoKPLAa9294255;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void xDZMMfOpPT39283646() {     long gDYLTVVwRH47951383 = -536318199;    long gDYLTVVwRH38511905 = -623921064;    long gDYLTVVwRH80971259 = -972437585;    long gDYLTVVwRH63544818 = -114074650;    long gDYLTVVwRH41107029 = -601818984;    long gDYLTVVwRH41887818 = -793072654;    long gDYLTVVwRH25760638 = -979378682;    long gDYLTVVwRH16732296 = -384981485;    long gDYLTVVwRH94576417 = -981143641;    long gDYLTVVwRH8448074 = -951426950;    long gDYLTVVwRH89414612 = -723399103;    long gDYLTVVwRH38175500 = -867788491;    long gDYLTVVwRH2021611 = -821223379;    long gDYLTVVwRH75171369 = -298993952;    long gDYLTVVwRH1939819 = -571356017;    long gDYLTVVwRH78302596 = -549560766;    long gDYLTVVwRH76645474 = -213064046;    long gDYLTVVwRH9862923 = -829204364;    long gDYLTVVwRH70870356 = 79149800;    long gDYLTVVwRH5501669 = -868191887;    long gDYLTVVwRH40718484 = -482062902;    long gDYLTVVwRH43216753 = -392726888;    long gDYLTVVwRH95291720 = -495901458;    long gDYLTVVwRH7554580 = -945140707;    long gDYLTVVwRH94920904 = -997720775;    long gDYLTVVwRH5485092 = -181491396;    long gDYLTVVwRH70578222 = -33639688;    long gDYLTVVwRH7111547 = -988186446;    long gDYLTVVwRH88525794 = -790980798;    long gDYLTVVwRH6609858 = -945048414;    long gDYLTVVwRH67998828 = -166344353;    long gDYLTVVwRH28760725 = -956792069;    long gDYLTVVwRH90303578 = -916846195;    long gDYLTVVwRH27379816 = -258293895;    long gDYLTVVwRH37459395 = -309824799;    long gDYLTVVwRH22476400 = -312881610;    long gDYLTVVwRH38654702 = -693596101;    long gDYLTVVwRH78709225 = -267721298;    long gDYLTVVwRH72205450 = -158705139;    long gDYLTVVwRH11589104 = -218444997;    long gDYLTVVwRH9577581 = -707487728;    long gDYLTVVwRH46569054 = -365762849;    long gDYLTVVwRH63627659 = 461672;    long gDYLTVVwRH64090765 = -731611852;    long gDYLTVVwRH50390320 = -916628477;    long gDYLTVVwRH90024813 = -260393131;    long gDYLTVVwRH8830789 = -461158381;    long gDYLTVVwRH40217648 = -490067623;    long gDYLTVVwRH60002087 = 73774659;    long gDYLTVVwRH75846169 = 92635350;    long gDYLTVVwRH16747826 = -786722575;    long gDYLTVVwRH39532138 = -968807555;    long gDYLTVVwRH15192028 = -146877702;    long gDYLTVVwRH97638302 = -871751405;    long gDYLTVVwRH98565161 = -530836349;    long gDYLTVVwRH4734630 = -43591312;    long gDYLTVVwRH43220184 = -28019606;    long gDYLTVVwRH73416679 = 72703121;    long gDYLTVVwRH68623914 = -116353875;    long gDYLTVVwRH35621937 = -320327589;    long gDYLTVVwRH71309595 = -659432966;    long gDYLTVVwRH18649092 = -991192236;    long gDYLTVVwRH28206501 = -594000688;    long gDYLTVVwRH87966559 = 63904773;    long gDYLTVVwRH40449246 = -685082597;    long gDYLTVVwRH60653888 = -766607034;    long gDYLTVVwRH47871922 = -950942297;    long gDYLTVVwRH74641794 = -462929485;    long gDYLTVVwRH37711974 = -989169153;    long gDYLTVVwRH79463418 = -158474408;    long gDYLTVVwRH39647895 = -855964666;    long gDYLTVVwRH97936248 = -945342748;    long gDYLTVVwRH37657473 = -570499225;    long gDYLTVVwRH59281253 = -702405203;    long gDYLTVVwRH95924087 = -60704160;    long gDYLTVVwRH94149429 = -16300053;    long gDYLTVVwRH79589093 = -293188560;    long gDYLTVVwRH31200956 = -764289606;    long gDYLTVVwRH57164260 = 71487770;    long gDYLTVVwRH4896092 = -637327644;    long gDYLTVVwRH96654302 = -720333015;    long gDYLTVVwRH30360574 = -543572066;    long gDYLTVVwRH47109459 = -961961105;    long gDYLTVVwRH12679626 = -783616148;    long gDYLTVVwRH89862031 = -58325839;    long gDYLTVVwRH28466690 = -197536798;    long gDYLTVVwRH13568697 = -709914368;    long gDYLTVVwRH92665275 = 54905210;    long gDYLTVVwRH28814655 = -727457547;    long gDYLTVVwRH32724766 = -166233488;    long gDYLTVVwRH79256216 = -184862005;    long gDYLTVVwRH65238022 = -666299223;    long gDYLTVVwRH10085312 = -51367423;    long gDYLTVVwRH36583513 = -838377551;    long gDYLTVVwRH40279508 = -559012031;    long gDYLTVVwRH90928489 = -716295492;    long gDYLTVVwRH18362553 = -771762162;    long gDYLTVVwRH75661100 = 36556898;    long gDYLTVVwRH23641519 = 53470744;    long gDYLTVVwRH89736431 = -536318199;     gDYLTVVwRH47951383 = gDYLTVVwRH38511905;     gDYLTVVwRH38511905 = gDYLTVVwRH80971259;     gDYLTVVwRH80971259 = gDYLTVVwRH63544818;     gDYLTVVwRH63544818 = gDYLTVVwRH41107029;     gDYLTVVwRH41107029 = gDYLTVVwRH41887818;     gDYLTVVwRH41887818 = gDYLTVVwRH25760638;     gDYLTVVwRH25760638 = gDYLTVVwRH16732296;     gDYLTVVwRH16732296 = gDYLTVVwRH94576417;     gDYLTVVwRH94576417 = gDYLTVVwRH8448074;     gDYLTVVwRH8448074 = gDYLTVVwRH89414612;     gDYLTVVwRH89414612 = gDYLTVVwRH38175500;     gDYLTVVwRH38175500 = gDYLTVVwRH2021611;     gDYLTVVwRH2021611 = gDYLTVVwRH75171369;     gDYLTVVwRH75171369 = gDYLTVVwRH1939819;     gDYLTVVwRH1939819 = gDYLTVVwRH78302596;     gDYLTVVwRH78302596 = gDYLTVVwRH76645474;     gDYLTVVwRH76645474 = gDYLTVVwRH9862923;     gDYLTVVwRH9862923 = gDYLTVVwRH70870356;     gDYLTVVwRH70870356 = gDYLTVVwRH5501669;     gDYLTVVwRH5501669 = gDYLTVVwRH40718484;     gDYLTVVwRH40718484 = gDYLTVVwRH43216753;     gDYLTVVwRH43216753 = gDYLTVVwRH95291720;     gDYLTVVwRH95291720 = gDYLTVVwRH7554580;     gDYLTVVwRH7554580 = gDYLTVVwRH94920904;     gDYLTVVwRH94920904 = gDYLTVVwRH5485092;     gDYLTVVwRH5485092 = gDYLTVVwRH70578222;     gDYLTVVwRH70578222 = gDYLTVVwRH7111547;     gDYLTVVwRH7111547 = gDYLTVVwRH88525794;     gDYLTVVwRH88525794 = gDYLTVVwRH6609858;     gDYLTVVwRH6609858 = gDYLTVVwRH67998828;     gDYLTVVwRH67998828 = gDYLTVVwRH28760725;     gDYLTVVwRH28760725 = gDYLTVVwRH90303578;     gDYLTVVwRH90303578 = gDYLTVVwRH27379816;     gDYLTVVwRH27379816 = gDYLTVVwRH37459395;     gDYLTVVwRH37459395 = gDYLTVVwRH22476400;     gDYLTVVwRH22476400 = gDYLTVVwRH38654702;     gDYLTVVwRH38654702 = gDYLTVVwRH78709225;     gDYLTVVwRH78709225 = gDYLTVVwRH72205450;     gDYLTVVwRH72205450 = gDYLTVVwRH11589104;     gDYLTVVwRH11589104 = gDYLTVVwRH9577581;     gDYLTVVwRH9577581 = gDYLTVVwRH46569054;     gDYLTVVwRH46569054 = gDYLTVVwRH63627659;     gDYLTVVwRH63627659 = gDYLTVVwRH64090765;     gDYLTVVwRH64090765 = gDYLTVVwRH50390320;     gDYLTVVwRH50390320 = gDYLTVVwRH90024813;     gDYLTVVwRH90024813 = gDYLTVVwRH8830789;     gDYLTVVwRH8830789 = gDYLTVVwRH40217648;     gDYLTVVwRH40217648 = gDYLTVVwRH60002087;     gDYLTVVwRH60002087 = gDYLTVVwRH75846169;     gDYLTVVwRH75846169 = gDYLTVVwRH16747826;     gDYLTVVwRH16747826 = gDYLTVVwRH39532138;     gDYLTVVwRH39532138 = gDYLTVVwRH15192028;     gDYLTVVwRH15192028 = gDYLTVVwRH97638302;     gDYLTVVwRH97638302 = gDYLTVVwRH98565161;     gDYLTVVwRH98565161 = gDYLTVVwRH4734630;     gDYLTVVwRH4734630 = gDYLTVVwRH43220184;     gDYLTVVwRH43220184 = gDYLTVVwRH73416679;     gDYLTVVwRH73416679 = gDYLTVVwRH68623914;     gDYLTVVwRH68623914 = gDYLTVVwRH35621937;     gDYLTVVwRH35621937 = gDYLTVVwRH71309595;     gDYLTVVwRH71309595 = gDYLTVVwRH18649092;     gDYLTVVwRH18649092 = gDYLTVVwRH28206501;     gDYLTVVwRH28206501 = gDYLTVVwRH87966559;     gDYLTVVwRH87966559 = gDYLTVVwRH40449246;     gDYLTVVwRH40449246 = gDYLTVVwRH60653888;     gDYLTVVwRH60653888 = gDYLTVVwRH47871922;     gDYLTVVwRH47871922 = gDYLTVVwRH74641794;     gDYLTVVwRH74641794 = gDYLTVVwRH37711974;     gDYLTVVwRH37711974 = gDYLTVVwRH79463418;     gDYLTVVwRH79463418 = gDYLTVVwRH39647895;     gDYLTVVwRH39647895 = gDYLTVVwRH97936248;     gDYLTVVwRH97936248 = gDYLTVVwRH37657473;     gDYLTVVwRH37657473 = gDYLTVVwRH59281253;     gDYLTVVwRH59281253 = gDYLTVVwRH95924087;     gDYLTVVwRH95924087 = gDYLTVVwRH94149429;     gDYLTVVwRH94149429 = gDYLTVVwRH79589093;     gDYLTVVwRH79589093 = gDYLTVVwRH31200956;     gDYLTVVwRH31200956 = gDYLTVVwRH57164260;     gDYLTVVwRH57164260 = gDYLTVVwRH4896092;     gDYLTVVwRH4896092 = gDYLTVVwRH96654302;     gDYLTVVwRH96654302 = gDYLTVVwRH30360574;     gDYLTVVwRH30360574 = gDYLTVVwRH47109459;     gDYLTVVwRH47109459 = gDYLTVVwRH12679626;     gDYLTVVwRH12679626 = gDYLTVVwRH89862031;     gDYLTVVwRH89862031 = gDYLTVVwRH28466690;     gDYLTVVwRH28466690 = gDYLTVVwRH13568697;     gDYLTVVwRH13568697 = gDYLTVVwRH92665275;     gDYLTVVwRH92665275 = gDYLTVVwRH28814655;     gDYLTVVwRH28814655 = gDYLTVVwRH32724766;     gDYLTVVwRH32724766 = gDYLTVVwRH79256216;     gDYLTVVwRH79256216 = gDYLTVVwRH65238022;     gDYLTVVwRH65238022 = gDYLTVVwRH10085312;     gDYLTVVwRH10085312 = gDYLTVVwRH36583513;     gDYLTVVwRH36583513 = gDYLTVVwRH40279508;     gDYLTVVwRH40279508 = gDYLTVVwRH90928489;     gDYLTVVwRH90928489 = gDYLTVVwRH18362553;     gDYLTVVwRH18362553 = gDYLTVVwRH75661100;     gDYLTVVwRH75661100 = gDYLTVVwRH23641519;     gDYLTVVwRH23641519 = gDYLTVVwRH89736431;     gDYLTVVwRH89736431 = gDYLTVVwRH47951383;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void WcstYgCFwh52593307() {     long CfRoxBUExm20336883 = -413853198;    long CfRoxBUExm57502484 = -583112900;    long CfRoxBUExm85068840 = -926373583;    long CfRoxBUExm74821471 = -350617389;    long CfRoxBUExm41491309 = -759964816;    long CfRoxBUExm65472293 = -578539235;    long CfRoxBUExm60053210 = -452500105;    long CfRoxBUExm59669667 = -231622459;    long CfRoxBUExm62424589 = 31248803;    long CfRoxBUExm99371536 = -328370001;    long CfRoxBUExm32036623 = -159891522;    long CfRoxBUExm97651368 = -502403375;    long CfRoxBUExm26993020 = -70355318;    long CfRoxBUExm6247177 = -362990972;    long CfRoxBUExm99968122 = -189511943;    long CfRoxBUExm16199646 = -189427221;    long CfRoxBUExm40432016 = -995247508;    long CfRoxBUExm96589472 = -5214960;    long CfRoxBUExm92259055 = -760447891;    long CfRoxBUExm52036393 = -723832453;    long CfRoxBUExm61518947 = -951166804;    long CfRoxBUExm51606038 = -749991172;    long CfRoxBUExm7804643 = -511721511;    long CfRoxBUExm94966893 = -929363695;    long CfRoxBUExm12571824 = 99046637;    long CfRoxBUExm22931605 = -52437102;    long CfRoxBUExm10223038 = -966426109;    long CfRoxBUExm21873878 = -378882647;    long CfRoxBUExm48587967 = 78244505;    long CfRoxBUExm9789505 = -784202124;    long CfRoxBUExm20642803 = -894433814;    long CfRoxBUExm82482879 = -56818865;    long CfRoxBUExm42705354 = -121675744;    long CfRoxBUExm86839212 = -276591178;    long CfRoxBUExm84278269 = -438409703;    long CfRoxBUExm74876316 = -923218423;    long CfRoxBUExm10079756 = -176457397;    long CfRoxBUExm99814745 = -134162440;    long CfRoxBUExm79341980 = 87078475;    long CfRoxBUExm24743041 = 17973053;    long CfRoxBUExm35298869 = -957651368;    long CfRoxBUExm31501861 = -172119561;    long CfRoxBUExm59248691 = -576851476;    long CfRoxBUExm3342859 = -618216612;    long CfRoxBUExm89378099 = -569086805;    long CfRoxBUExm8232088 = -120633395;    long CfRoxBUExm78052634 = -609888789;    long CfRoxBUExm28662218 = -123792953;    long CfRoxBUExm84126589 = -41883005;    long CfRoxBUExm27941271 = 53858869;    long CfRoxBUExm78931885 = 63562396;    long CfRoxBUExm67020209 = -670163275;    long CfRoxBUExm37368518 = -66802434;    long CfRoxBUExm10487416 = 94587823;    long CfRoxBUExm91074359 = -610765284;    long CfRoxBUExm68730844 = -663862026;    long CfRoxBUExm49697841 = 28608611;    long CfRoxBUExm90101947 = -997009888;    long CfRoxBUExm62249648 = -349664027;    long CfRoxBUExm18559704 = -607527715;    long CfRoxBUExm55249255 = -612113126;    long CfRoxBUExm38179333 = 26382542;    long CfRoxBUExm11081700 = -209866964;    long CfRoxBUExm52635084 = -184549074;    long CfRoxBUExm78728734 = -433936187;    long CfRoxBUExm49553743 = -3072658;    long CfRoxBUExm54946014 = -280727632;    long CfRoxBUExm40153807 = -793764140;    long CfRoxBUExm21968908 = -924581269;    long CfRoxBUExm25091807 = -266293521;    long CfRoxBUExm6119891 = 87030175;    long CfRoxBUExm40617270 = -761085069;    long CfRoxBUExm17247492 = 7706564;    long CfRoxBUExm67516014 = -678420944;    long CfRoxBUExm16737524 = -766181085;    long CfRoxBUExm30017087 = -679047243;    long CfRoxBUExm92357346 = -73139697;    long CfRoxBUExm4461785 = -893504899;    long CfRoxBUExm5588795 = -260276890;    long CfRoxBUExm4339737 = -780319969;    long CfRoxBUExm44878971 = -442548313;    long CfRoxBUExm81560820 = -742633157;    long CfRoxBUExm37747288 = -236999642;    long CfRoxBUExm20646697 = -975614365;    long CfRoxBUExm30857620 = -747764520;    long CfRoxBUExm53622593 = -124270540;    long CfRoxBUExm45114362 = -990016431;    long CfRoxBUExm32217939 = -116263567;    long CfRoxBUExm95764853 = -665825895;    long CfRoxBUExm15547425 = -774547677;    long CfRoxBUExm25178476 = -851827034;    long CfRoxBUExm19977809 = -179447509;    long CfRoxBUExm37565098 = -784498414;    long CfRoxBUExm60782277 = -305393811;    long CfRoxBUExm69493786 = -369913822;    long CfRoxBUExm97119535 = -884033910;    long CfRoxBUExm20420161 = -962252598;    long CfRoxBUExm6613607 = -292302402;    long CfRoxBUExm24614125 = -84280425;    long CfRoxBUExm39824356 = -413853198;     CfRoxBUExm20336883 = CfRoxBUExm57502484;     CfRoxBUExm57502484 = CfRoxBUExm85068840;     CfRoxBUExm85068840 = CfRoxBUExm74821471;     CfRoxBUExm74821471 = CfRoxBUExm41491309;     CfRoxBUExm41491309 = CfRoxBUExm65472293;     CfRoxBUExm65472293 = CfRoxBUExm60053210;     CfRoxBUExm60053210 = CfRoxBUExm59669667;     CfRoxBUExm59669667 = CfRoxBUExm62424589;     CfRoxBUExm62424589 = CfRoxBUExm99371536;     CfRoxBUExm99371536 = CfRoxBUExm32036623;     CfRoxBUExm32036623 = CfRoxBUExm97651368;     CfRoxBUExm97651368 = CfRoxBUExm26993020;     CfRoxBUExm26993020 = CfRoxBUExm6247177;     CfRoxBUExm6247177 = CfRoxBUExm99968122;     CfRoxBUExm99968122 = CfRoxBUExm16199646;     CfRoxBUExm16199646 = CfRoxBUExm40432016;     CfRoxBUExm40432016 = CfRoxBUExm96589472;     CfRoxBUExm96589472 = CfRoxBUExm92259055;     CfRoxBUExm92259055 = CfRoxBUExm52036393;     CfRoxBUExm52036393 = CfRoxBUExm61518947;     CfRoxBUExm61518947 = CfRoxBUExm51606038;     CfRoxBUExm51606038 = CfRoxBUExm7804643;     CfRoxBUExm7804643 = CfRoxBUExm94966893;     CfRoxBUExm94966893 = CfRoxBUExm12571824;     CfRoxBUExm12571824 = CfRoxBUExm22931605;     CfRoxBUExm22931605 = CfRoxBUExm10223038;     CfRoxBUExm10223038 = CfRoxBUExm21873878;     CfRoxBUExm21873878 = CfRoxBUExm48587967;     CfRoxBUExm48587967 = CfRoxBUExm9789505;     CfRoxBUExm9789505 = CfRoxBUExm20642803;     CfRoxBUExm20642803 = CfRoxBUExm82482879;     CfRoxBUExm82482879 = CfRoxBUExm42705354;     CfRoxBUExm42705354 = CfRoxBUExm86839212;     CfRoxBUExm86839212 = CfRoxBUExm84278269;     CfRoxBUExm84278269 = CfRoxBUExm74876316;     CfRoxBUExm74876316 = CfRoxBUExm10079756;     CfRoxBUExm10079756 = CfRoxBUExm99814745;     CfRoxBUExm99814745 = CfRoxBUExm79341980;     CfRoxBUExm79341980 = CfRoxBUExm24743041;     CfRoxBUExm24743041 = CfRoxBUExm35298869;     CfRoxBUExm35298869 = CfRoxBUExm31501861;     CfRoxBUExm31501861 = CfRoxBUExm59248691;     CfRoxBUExm59248691 = CfRoxBUExm3342859;     CfRoxBUExm3342859 = CfRoxBUExm89378099;     CfRoxBUExm89378099 = CfRoxBUExm8232088;     CfRoxBUExm8232088 = CfRoxBUExm78052634;     CfRoxBUExm78052634 = CfRoxBUExm28662218;     CfRoxBUExm28662218 = CfRoxBUExm84126589;     CfRoxBUExm84126589 = CfRoxBUExm27941271;     CfRoxBUExm27941271 = CfRoxBUExm78931885;     CfRoxBUExm78931885 = CfRoxBUExm67020209;     CfRoxBUExm67020209 = CfRoxBUExm37368518;     CfRoxBUExm37368518 = CfRoxBUExm10487416;     CfRoxBUExm10487416 = CfRoxBUExm91074359;     CfRoxBUExm91074359 = CfRoxBUExm68730844;     CfRoxBUExm68730844 = CfRoxBUExm49697841;     CfRoxBUExm49697841 = CfRoxBUExm90101947;     CfRoxBUExm90101947 = CfRoxBUExm62249648;     CfRoxBUExm62249648 = CfRoxBUExm18559704;     CfRoxBUExm18559704 = CfRoxBUExm55249255;     CfRoxBUExm55249255 = CfRoxBUExm38179333;     CfRoxBUExm38179333 = CfRoxBUExm11081700;     CfRoxBUExm11081700 = CfRoxBUExm52635084;     CfRoxBUExm52635084 = CfRoxBUExm78728734;     CfRoxBUExm78728734 = CfRoxBUExm49553743;     CfRoxBUExm49553743 = CfRoxBUExm54946014;     CfRoxBUExm54946014 = CfRoxBUExm40153807;     CfRoxBUExm40153807 = CfRoxBUExm21968908;     CfRoxBUExm21968908 = CfRoxBUExm25091807;     CfRoxBUExm25091807 = CfRoxBUExm6119891;     CfRoxBUExm6119891 = CfRoxBUExm40617270;     CfRoxBUExm40617270 = CfRoxBUExm17247492;     CfRoxBUExm17247492 = CfRoxBUExm67516014;     CfRoxBUExm67516014 = CfRoxBUExm16737524;     CfRoxBUExm16737524 = CfRoxBUExm30017087;     CfRoxBUExm30017087 = CfRoxBUExm92357346;     CfRoxBUExm92357346 = CfRoxBUExm4461785;     CfRoxBUExm4461785 = CfRoxBUExm5588795;     CfRoxBUExm5588795 = CfRoxBUExm4339737;     CfRoxBUExm4339737 = CfRoxBUExm44878971;     CfRoxBUExm44878971 = CfRoxBUExm81560820;     CfRoxBUExm81560820 = CfRoxBUExm37747288;     CfRoxBUExm37747288 = CfRoxBUExm20646697;     CfRoxBUExm20646697 = CfRoxBUExm30857620;     CfRoxBUExm30857620 = CfRoxBUExm53622593;     CfRoxBUExm53622593 = CfRoxBUExm45114362;     CfRoxBUExm45114362 = CfRoxBUExm32217939;     CfRoxBUExm32217939 = CfRoxBUExm95764853;     CfRoxBUExm95764853 = CfRoxBUExm15547425;     CfRoxBUExm15547425 = CfRoxBUExm25178476;     CfRoxBUExm25178476 = CfRoxBUExm19977809;     CfRoxBUExm19977809 = CfRoxBUExm37565098;     CfRoxBUExm37565098 = CfRoxBUExm60782277;     CfRoxBUExm60782277 = CfRoxBUExm69493786;     CfRoxBUExm69493786 = CfRoxBUExm97119535;     CfRoxBUExm97119535 = CfRoxBUExm20420161;     CfRoxBUExm20420161 = CfRoxBUExm6613607;     CfRoxBUExm6613607 = CfRoxBUExm24614125;     CfRoxBUExm24614125 = CfRoxBUExm39824356;     CfRoxBUExm39824356 = CfRoxBUExm20336883;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void UdBVOuVWDZ33509107() {     long RybxKGPjyT18317366 = -457635085;    long RybxKGPjyT65031732 = -733889824;    long RybxKGPjyT70075780 = -197078196;    long RybxKGPjyT54200264 = -313671776;    long RybxKGPjyT97957439 = -262499953;    long RybxKGPjyT53857896 = -969837601;    long RybxKGPjyT46845624 = -401088034;    long RybxKGPjyT13796087 = -902679336;    long RybxKGPjyT31491808 = -432113693;    long RybxKGPjyT14494112 = -34664268;    long RybxKGPjyT58511280 = 32396280;    long RybxKGPjyT71007377 = -977509622;    long RybxKGPjyT80765836 = -701247910;    long RybxKGPjyT98116370 = -983782708;    long RybxKGPjyT86507151 = -852144549;    long RybxKGPjyT99970917 = -316291677;    long RybxKGPjyT20237535 = -636328910;    long RybxKGPjyT19931127 = -751895484;    long RybxKGPjyT94470069 = -356046041;    long RybxKGPjyT51621939 = -735875718;    long RybxKGPjyT63824539 = -116270926;    long RybxKGPjyT94054030 = -212000561;    long RybxKGPjyT68763768 = -433196905;    long RybxKGPjyT18618088 = -951054879;    long RybxKGPjyT92461847 = -481641313;    long RybxKGPjyT95006140 = -725524457;    long RybxKGPjyT76923390 = 86173904;    long RybxKGPjyT47429722 = -185790598;    long RybxKGPjyT97509681 = 30281429;    long RybxKGPjyT2156838 = 80881665;    long RybxKGPjyT45214394 = -820530453;    long RybxKGPjyT96309181 = 57091149;    long RybxKGPjyT88115547 = -21410106;    long RybxKGPjyT99461424 = 13989166;    long RybxKGPjyT8965075 = -879459631;    long RybxKGPjyT67825799 = -281501333;    long RybxKGPjyT63238143 = -540350570;    long RybxKGPjyT91616434 = -956424388;    long RybxKGPjyT74431695 = -706454109;    long RybxKGPjyT88871313 = -251431688;    long RybxKGPjyT26387936 = -959351;    long RybxKGPjyT81897765 = -196826653;    long RybxKGPjyT63644902 = -344075623;    long RybxKGPjyT74540384 = -537790306;    long RybxKGPjyT94319676 = -677148;    long RybxKGPjyT32197731 = -235354189;    long RybxKGPjyT5496246 = -69167125;    long RybxKGPjyT62144187 = -687559095;    long RybxKGPjyT81074827 = -361571103;    long RybxKGPjyT3778233 = -452403435;    long RybxKGPjyT31363137 = 33021276;    long RybxKGPjyT99945974 = -526379551;    long RybxKGPjyT14446090 = -166544703;    long RybxKGPjyT76860031 = -106256953;    long RybxKGPjyT69684568 = -430435719;    long RybxKGPjyT24263336 = -145634524;    long RybxKGPjyT96267963 = -200692920;    long RybxKGPjyT51457692 = -246023318;    long RybxKGPjyT61738416 = -832030464;    long RybxKGPjyT2951300 = -536975496;    long RybxKGPjyT76934505 = -956011506;    long RybxKGPjyT99415902 = -115297437;    long RybxKGPjyT16286405 = -832960765;    long RybxKGPjyT29334970 = -412995359;    long RybxKGPjyT69279718 = -214133815;    long RybxKGPjyT62202099 = 75305131;    long RybxKGPjyT82891829 = -856099517;    long RybxKGPjyT81304411 = -615237076;    long RybxKGPjyT89151296 = -4323077;    long RybxKGPjyT18681353 = -470643217;    long RybxKGPjyT36732774 = -775941108;    long RybxKGPjyT28621100 = -679904523;    long RybxKGPjyT45499432 = 54558624;    long RybxKGPjyT5598757 = -4614353;    long RybxKGPjyT25234003 = -634916368;    long RybxKGPjyT81926774 = -919444274;    long RybxKGPjyT30409128 = -867924939;    long RybxKGPjyT94223384 = -895406600;    long RybxKGPjyT24298411 = -850377731;    long RybxKGPjyT60264117 = -146287125;    long RybxKGPjyT89509894 = -556357333;    long RybxKGPjyT14779204 = -226267001;    long RybxKGPjyT66354894 = -824219495;    long RybxKGPjyT93731449 = -517315137;    long RybxKGPjyT70793701 = -952139611;    long RybxKGPjyT45268420 = -194150903;    long RybxKGPjyT81863092 = -776364149;    long RybxKGPjyT11255517 = -915153153;    long RybxKGPjyT29776857 = -555575116;    long RybxKGPjyT84701739 = -633825107;    long RybxKGPjyT71557835 = 19191586;    long RybxKGPjyT11780452 = -194327253;    long RybxKGPjyT29878018 = -24393925;    long RybxKGPjyT71480395 = -69478613;    long RybxKGPjyT11936808 = -295420183;    long RybxKGPjyT26972034 = -885661915;    long RybxKGPjyT65611360 = -363865889;    long RybxKGPjyT34309933 = -931080265;    long RybxKGPjyT5260667 = -223656491;    long RybxKGPjyT32117578 = -457635085;     RybxKGPjyT18317366 = RybxKGPjyT65031732;     RybxKGPjyT65031732 = RybxKGPjyT70075780;     RybxKGPjyT70075780 = RybxKGPjyT54200264;     RybxKGPjyT54200264 = RybxKGPjyT97957439;     RybxKGPjyT97957439 = RybxKGPjyT53857896;     RybxKGPjyT53857896 = RybxKGPjyT46845624;     RybxKGPjyT46845624 = RybxKGPjyT13796087;     RybxKGPjyT13796087 = RybxKGPjyT31491808;     RybxKGPjyT31491808 = RybxKGPjyT14494112;     RybxKGPjyT14494112 = RybxKGPjyT58511280;     RybxKGPjyT58511280 = RybxKGPjyT71007377;     RybxKGPjyT71007377 = RybxKGPjyT80765836;     RybxKGPjyT80765836 = RybxKGPjyT98116370;     RybxKGPjyT98116370 = RybxKGPjyT86507151;     RybxKGPjyT86507151 = RybxKGPjyT99970917;     RybxKGPjyT99970917 = RybxKGPjyT20237535;     RybxKGPjyT20237535 = RybxKGPjyT19931127;     RybxKGPjyT19931127 = RybxKGPjyT94470069;     RybxKGPjyT94470069 = RybxKGPjyT51621939;     RybxKGPjyT51621939 = RybxKGPjyT63824539;     RybxKGPjyT63824539 = RybxKGPjyT94054030;     RybxKGPjyT94054030 = RybxKGPjyT68763768;     RybxKGPjyT68763768 = RybxKGPjyT18618088;     RybxKGPjyT18618088 = RybxKGPjyT92461847;     RybxKGPjyT92461847 = RybxKGPjyT95006140;     RybxKGPjyT95006140 = RybxKGPjyT76923390;     RybxKGPjyT76923390 = RybxKGPjyT47429722;     RybxKGPjyT47429722 = RybxKGPjyT97509681;     RybxKGPjyT97509681 = RybxKGPjyT2156838;     RybxKGPjyT2156838 = RybxKGPjyT45214394;     RybxKGPjyT45214394 = RybxKGPjyT96309181;     RybxKGPjyT96309181 = RybxKGPjyT88115547;     RybxKGPjyT88115547 = RybxKGPjyT99461424;     RybxKGPjyT99461424 = RybxKGPjyT8965075;     RybxKGPjyT8965075 = RybxKGPjyT67825799;     RybxKGPjyT67825799 = RybxKGPjyT63238143;     RybxKGPjyT63238143 = RybxKGPjyT91616434;     RybxKGPjyT91616434 = RybxKGPjyT74431695;     RybxKGPjyT74431695 = RybxKGPjyT88871313;     RybxKGPjyT88871313 = RybxKGPjyT26387936;     RybxKGPjyT26387936 = RybxKGPjyT81897765;     RybxKGPjyT81897765 = RybxKGPjyT63644902;     RybxKGPjyT63644902 = RybxKGPjyT74540384;     RybxKGPjyT74540384 = RybxKGPjyT94319676;     RybxKGPjyT94319676 = RybxKGPjyT32197731;     RybxKGPjyT32197731 = RybxKGPjyT5496246;     RybxKGPjyT5496246 = RybxKGPjyT62144187;     RybxKGPjyT62144187 = RybxKGPjyT81074827;     RybxKGPjyT81074827 = RybxKGPjyT3778233;     RybxKGPjyT3778233 = RybxKGPjyT31363137;     RybxKGPjyT31363137 = RybxKGPjyT99945974;     RybxKGPjyT99945974 = RybxKGPjyT14446090;     RybxKGPjyT14446090 = RybxKGPjyT76860031;     RybxKGPjyT76860031 = RybxKGPjyT69684568;     RybxKGPjyT69684568 = RybxKGPjyT24263336;     RybxKGPjyT24263336 = RybxKGPjyT96267963;     RybxKGPjyT96267963 = RybxKGPjyT51457692;     RybxKGPjyT51457692 = RybxKGPjyT61738416;     RybxKGPjyT61738416 = RybxKGPjyT2951300;     RybxKGPjyT2951300 = RybxKGPjyT76934505;     RybxKGPjyT76934505 = RybxKGPjyT99415902;     RybxKGPjyT99415902 = RybxKGPjyT16286405;     RybxKGPjyT16286405 = RybxKGPjyT29334970;     RybxKGPjyT29334970 = RybxKGPjyT69279718;     RybxKGPjyT69279718 = RybxKGPjyT62202099;     RybxKGPjyT62202099 = RybxKGPjyT82891829;     RybxKGPjyT82891829 = RybxKGPjyT81304411;     RybxKGPjyT81304411 = RybxKGPjyT89151296;     RybxKGPjyT89151296 = RybxKGPjyT18681353;     RybxKGPjyT18681353 = RybxKGPjyT36732774;     RybxKGPjyT36732774 = RybxKGPjyT28621100;     RybxKGPjyT28621100 = RybxKGPjyT45499432;     RybxKGPjyT45499432 = RybxKGPjyT5598757;     RybxKGPjyT5598757 = RybxKGPjyT25234003;     RybxKGPjyT25234003 = RybxKGPjyT81926774;     RybxKGPjyT81926774 = RybxKGPjyT30409128;     RybxKGPjyT30409128 = RybxKGPjyT94223384;     RybxKGPjyT94223384 = RybxKGPjyT24298411;     RybxKGPjyT24298411 = RybxKGPjyT60264117;     RybxKGPjyT60264117 = RybxKGPjyT89509894;     RybxKGPjyT89509894 = RybxKGPjyT14779204;     RybxKGPjyT14779204 = RybxKGPjyT66354894;     RybxKGPjyT66354894 = RybxKGPjyT93731449;     RybxKGPjyT93731449 = RybxKGPjyT70793701;     RybxKGPjyT70793701 = RybxKGPjyT45268420;     RybxKGPjyT45268420 = RybxKGPjyT81863092;     RybxKGPjyT81863092 = RybxKGPjyT11255517;     RybxKGPjyT11255517 = RybxKGPjyT29776857;     RybxKGPjyT29776857 = RybxKGPjyT84701739;     RybxKGPjyT84701739 = RybxKGPjyT71557835;     RybxKGPjyT71557835 = RybxKGPjyT11780452;     RybxKGPjyT11780452 = RybxKGPjyT29878018;     RybxKGPjyT29878018 = RybxKGPjyT71480395;     RybxKGPjyT71480395 = RybxKGPjyT11936808;     RybxKGPjyT11936808 = RybxKGPjyT26972034;     RybxKGPjyT26972034 = RybxKGPjyT65611360;     RybxKGPjyT65611360 = RybxKGPjyT34309933;     RybxKGPjyT34309933 = RybxKGPjyT5260667;     RybxKGPjyT5260667 = RybxKGPjyT32117578;     RybxKGPjyT32117578 = RybxKGPjyT18317366;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void gmylOrWZeV51077711() {     float XJHtqgcEjq66286524 = -3334219;    float XJHtqgcEjq15062534 = -959208219;    float XJHtqgcEjq82380840 = -767757883;    float XJHtqgcEjq7468813 = -787672616;    float XJHtqgcEjq69893750 = -34656902;    float XJHtqgcEjq3153006 = -835143388;    float XJHtqgcEjq28595876 = -107614781;    float XJHtqgcEjq39047095 = -506046518;    float XJHtqgcEjq95354313 = -119356812;    float XJHtqgcEjq23735836 = -480370560;    float XJHtqgcEjq20546997 = -254392043;    float XJHtqgcEjq69373820 = -439703003;    float XJHtqgcEjq40281251 = -545042832;    float XJHtqgcEjq9212950 = -558592284;    float XJHtqgcEjq2097372 = -349524646;    float XJHtqgcEjq39230146 = -750548522;    float XJHtqgcEjq97785382 = -511735809;    float XJHtqgcEjq8523714 = -268451347;    float XJHtqgcEjq57430556 = -483113157;    float XJHtqgcEjq27336570 = -168961773;    float XJHtqgcEjq5551625 = -424691203;    float XJHtqgcEjq48520489 = -841495745;    float XJHtqgcEjq94260087 = -332848524;    float XJHtqgcEjq68656971 = -214356977;    float XJHtqgcEjq47126579 = -434043507;    float XJHtqgcEjq71085310 = -687972186;    float XJHtqgcEjq91511510 = -487625580;    float XJHtqgcEjq34941735 = -508390778;    float XJHtqgcEjq18618679 = -323304698;    float XJHtqgcEjq52681443 = -686039450;    float XJHtqgcEjq28258173 = -874040254;    float XJHtqgcEjq83402233 = 35313080;    float XJHtqgcEjq33067482 = -549546845;    float XJHtqgcEjq12578602 = -854623522;    float XJHtqgcEjq27778447 = -674308927;    float XJHtqgcEjq35724614 = -604374298;    float XJHtqgcEjq62293925 = -253637155;    float XJHtqgcEjq55774061 = -670396025;    float XJHtqgcEjq81320203 = -272179170;    float XJHtqgcEjq3777149 = -112341880;    float XJHtqgcEjq96224421 = -573169156;    float XJHtqgcEjq7264578 = -560669889;    float XJHtqgcEjq98536994 = -539777715;    float XJHtqgcEjq24393732 = -782051775;    float XJHtqgcEjq96292845 = -535471444;    float XJHtqgcEjq26912319 = -296153158;    float XJHtqgcEjq31410993 = -920686549;    float XJHtqgcEjq2547424 = -391657611;    float XJHtqgcEjq85675247 = -742845959;    float XJHtqgcEjq64694685 = -235959290;    float XJHtqgcEjq56998285 = -359991107;    float XJHtqgcEjq8329440 = -776350886;    float XJHtqgcEjq98558270 = -902381486;    float XJHtqgcEjq11170621 = -412961469;    float XJHtqgcEjq45526087 = -924679890;    float XJHtqgcEjq17766036 = -161838474;    float XJHtqgcEjq20802447 = -526359695;    float XJHtqgcEjq13723869 = -453400907;    float XJHtqgcEjq60342234 = -253629110;    float XJHtqgcEjq98808439 = -346684716;    float XJHtqgcEjq11641495 = -247517809;    float XJHtqgcEjq93654141 = -599224003;    float XJHtqgcEjq20428417 = -82741820;    float XJHtqgcEjq42672871 = -433317362;    float XJHtqgcEjq95477662 = -606330307;    float XJHtqgcEjq37144763 = -189705123;    float XJHtqgcEjq36306339 = -890156159;    float XJHtqgcEjq27702650 = -690419310;    float XJHtqgcEjq81434503 = -884283357;    float XJHtqgcEjq66372757 = -745150348;    float XJHtqgcEjq76936221 = -396911367;    float XJHtqgcEjq42011321 = -841339785;    float XJHtqgcEjq27203511 = -996272177;    float XJHtqgcEjq53653408 = -270771277;    float XJHtqgcEjq31112149 = -595792617;    float XJHtqgcEjq98287046 = -864021314;    float XJHtqgcEjq49983494 = -201718031;    float XJHtqgcEjq69866356 = -550796749;    float XJHtqgcEjq72364126 = -678885533;    float XJHtqgcEjq20214260 = -37890350;    float XJHtqgcEjq39674317 = -767285638;    float XJHtqgcEjq88964087 = 4032031;    float XJHtqgcEjq49266487 = -765544820;    float XJHtqgcEjq53923993 = 12654591;    float XJHtqgcEjq95683157 = -226048344;    float XJHtqgcEjq19928733 = 2310632;    float XJHtqgcEjq84843963 = -62305435;    float XJHtqgcEjq21896862 = -36585377;    float XJHtqgcEjq67052515 = -929943632;    float XJHtqgcEjq10012411 = -412470453;    float XJHtqgcEjq14922168 = 21985396;    float XJHtqgcEjq48570056 = -800236249;    float XJHtqgcEjq95431827 = -316766915;    float XJHtqgcEjq82511763 = -925494455;    float XJHtqgcEjq92135653 = -864824071;    float XJHtqgcEjq2570281 = -973945153;    float XJHtqgcEjq86836160 = -377928070;    float XJHtqgcEjq55864123 = -6460353;    float XJHtqgcEjq28916069 = -75721469;    float XJHtqgcEjq59148082 = -3334219;     XJHtqgcEjq66286524 = XJHtqgcEjq15062534;     XJHtqgcEjq15062534 = XJHtqgcEjq82380840;     XJHtqgcEjq82380840 = XJHtqgcEjq7468813;     XJHtqgcEjq7468813 = XJHtqgcEjq69893750;     XJHtqgcEjq69893750 = XJHtqgcEjq3153006;     XJHtqgcEjq3153006 = XJHtqgcEjq28595876;     XJHtqgcEjq28595876 = XJHtqgcEjq39047095;     XJHtqgcEjq39047095 = XJHtqgcEjq95354313;     XJHtqgcEjq95354313 = XJHtqgcEjq23735836;     XJHtqgcEjq23735836 = XJHtqgcEjq20546997;     XJHtqgcEjq20546997 = XJHtqgcEjq69373820;     XJHtqgcEjq69373820 = XJHtqgcEjq40281251;     XJHtqgcEjq40281251 = XJHtqgcEjq9212950;     XJHtqgcEjq9212950 = XJHtqgcEjq2097372;     XJHtqgcEjq2097372 = XJHtqgcEjq39230146;     XJHtqgcEjq39230146 = XJHtqgcEjq97785382;     XJHtqgcEjq97785382 = XJHtqgcEjq8523714;     XJHtqgcEjq8523714 = XJHtqgcEjq57430556;     XJHtqgcEjq57430556 = XJHtqgcEjq27336570;     XJHtqgcEjq27336570 = XJHtqgcEjq5551625;     XJHtqgcEjq5551625 = XJHtqgcEjq48520489;     XJHtqgcEjq48520489 = XJHtqgcEjq94260087;     XJHtqgcEjq94260087 = XJHtqgcEjq68656971;     XJHtqgcEjq68656971 = XJHtqgcEjq47126579;     XJHtqgcEjq47126579 = XJHtqgcEjq71085310;     XJHtqgcEjq71085310 = XJHtqgcEjq91511510;     XJHtqgcEjq91511510 = XJHtqgcEjq34941735;     XJHtqgcEjq34941735 = XJHtqgcEjq18618679;     XJHtqgcEjq18618679 = XJHtqgcEjq52681443;     XJHtqgcEjq52681443 = XJHtqgcEjq28258173;     XJHtqgcEjq28258173 = XJHtqgcEjq83402233;     XJHtqgcEjq83402233 = XJHtqgcEjq33067482;     XJHtqgcEjq33067482 = XJHtqgcEjq12578602;     XJHtqgcEjq12578602 = XJHtqgcEjq27778447;     XJHtqgcEjq27778447 = XJHtqgcEjq35724614;     XJHtqgcEjq35724614 = XJHtqgcEjq62293925;     XJHtqgcEjq62293925 = XJHtqgcEjq55774061;     XJHtqgcEjq55774061 = XJHtqgcEjq81320203;     XJHtqgcEjq81320203 = XJHtqgcEjq3777149;     XJHtqgcEjq3777149 = XJHtqgcEjq96224421;     XJHtqgcEjq96224421 = XJHtqgcEjq7264578;     XJHtqgcEjq7264578 = XJHtqgcEjq98536994;     XJHtqgcEjq98536994 = XJHtqgcEjq24393732;     XJHtqgcEjq24393732 = XJHtqgcEjq96292845;     XJHtqgcEjq96292845 = XJHtqgcEjq26912319;     XJHtqgcEjq26912319 = XJHtqgcEjq31410993;     XJHtqgcEjq31410993 = XJHtqgcEjq2547424;     XJHtqgcEjq2547424 = XJHtqgcEjq85675247;     XJHtqgcEjq85675247 = XJHtqgcEjq64694685;     XJHtqgcEjq64694685 = XJHtqgcEjq56998285;     XJHtqgcEjq56998285 = XJHtqgcEjq8329440;     XJHtqgcEjq8329440 = XJHtqgcEjq98558270;     XJHtqgcEjq98558270 = XJHtqgcEjq11170621;     XJHtqgcEjq11170621 = XJHtqgcEjq45526087;     XJHtqgcEjq45526087 = XJHtqgcEjq17766036;     XJHtqgcEjq17766036 = XJHtqgcEjq20802447;     XJHtqgcEjq20802447 = XJHtqgcEjq13723869;     XJHtqgcEjq13723869 = XJHtqgcEjq60342234;     XJHtqgcEjq60342234 = XJHtqgcEjq98808439;     XJHtqgcEjq98808439 = XJHtqgcEjq11641495;     XJHtqgcEjq11641495 = XJHtqgcEjq93654141;     XJHtqgcEjq93654141 = XJHtqgcEjq20428417;     XJHtqgcEjq20428417 = XJHtqgcEjq42672871;     XJHtqgcEjq42672871 = XJHtqgcEjq95477662;     XJHtqgcEjq95477662 = XJHtqgcEjq37144763;     XJHtqgcEjq37144763 = XJHtqgcEjq36306339;     XJHtqgcEjq36306339 = XJHtqgcEjq27702650;     XJHtqgcEjq27702650 = XJHtqgcEjq81434503;     XJHtqgcEjq81434503 = XJHtqgcEjq66372757;     XJHtqgcEjq66372757 = XJHtqgcEjq76936221;     XJHtqgcEjq76936221 = XJHtqgcEjq42011321;     XJHtqgcEjq42011321 = XJHtqgcEjq27203511;     XJHtqgcEjq27203511 = XJHtqgcEjq53653408;     XJHtqgcEjq53653408 = XJHtqgcEjq31112149;     XJHtqgcEjq31112149 = XJHtqgcEjq98287046;     XJHtqgcEjq98287046 = XJHtqgcEjq49983494;     XJHtqgcEjq49983494 = XJHtqgcEjq69866356;     XJHtqgcEjq69866356 = XJHtqgcEjq72364126;     XJHtqgcEjq72364126 = XJHtqgcEjq20214260;     XJHtqgcEjq20214260 = XJHtqgcEjq39674317;     XJHtqgcEjq39674317 = XJHtqgcEjq88964087;     XJHtqgcEjq88964087 = XJHtqgcEjq49266487;     XJHtqgcEjq49266487 = XJHtqgcEjq53923993;     XJHtqgcEjq53923993 = XJHtqgcEjq95683157;     XJHtqgcEjq95683157 = XJHtqgcEjq19928733;     XJHtqgcEjq19928733 = XJHtqgcEjq84843963;     XJHtqgcEjq84843963 = XJHtqgcEjq21896862;     XJHtqgcEjq21896862 = XJHtqgcEjq67052515;     XJHtqgcEjq67052515 = XJHtqgcEjq10012411;     XJHtqgcEjq10012411 = XJHtqgcEjq14922168;     XJHtqgcEjq14922168 = XJHtqgcEjq48570056;     XJHtqgcEjq48570056 = XJHtqgcEjq95431827;     XJHtqgcEjq95431827 = XJHtqgcEjq82511763;     XJHtqgcEjq82511763 = XJHtqgcEjq92135653;     XJHtqgcEjq92135653 = XJHtqgcEjq2570281;     XJHtqgcEjq2570281 = XJHtqgcEjq86836160;     XJHtqgcEjq86836160 = XJHtqgcEjq55864123;     XJHtqgcEjq55864123 = XJHtqgcEjq28916069;     XJHtqgcEjq28916069 = XJHtqgcEjq59148082;     XJHtqgcEjq59148082 = XJHtqgcEjq66286524;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void IxEgDVDJxL57076061() {     float lsOyOrRThW75601791 = 94134754;    float lsOyOrRThW70485456 = -975742721;    float lsOyOrRThW2532921 = -481398496;    float lsOyOrRThW65258834 = -979336608;    float lsOyOrRThW9956989 = -11172059;    float lsOyOrRThW96696367 = -294012249;    float lsOyOrRThW2249947 = 93136341;    float lsOyOrRThW28183252 = -703005666;    float lsOyOrRThW88162970 = -499417483;    float lsOyOrRThW92360292 = -301166228;    float lsOyOrRThW57448981 = -445633116;    float lsOyOrRThW80611570 = 22927181;    float lsOyOrRThW36062967 = -130464966;    float lsOyOrRThW41301591 = -537646389;    float lsOyOrRThW17373409 = -201055057;    float lsOyOrRThW44200671 = -778072000;    float lsOyOrRThW41596150 = -539320896;    float lsOyOrRThW20716355 = 1491340;    float lsOyOrRThW39443488 = -436753986;    float lsOyOrRThW90531918 = -247669615;    float lsOyOrRThW94230975 = -750391131;    float lsOyOrRThW72729400 = -664392078;    float lsOyOrRThW30321354 = 52622476;    float lsOyOrRThW27863601 = -831518087;    float lsOyOrRThW10189121 = -753176557;    float lsOyOrRThW36993127 = -787183827;    float lsOyOrRThW25269778 = -196514424;    float lsOyOrRThW6036210 = 84472489;    float lsOyOrRThW85735585 = -437724134;    float lsOyOrRThW65462391 = -492124013;    float lsOyOrRThW98256551 = -484692071;    float lsOyOrRThW60661597 = -395858215;    float lsOyOrRThW82926811 = -360637858;    float lsOyOrRThW1607550 = -338183409;    float lsOyOrRThW14428722 = -850721522;    float lsOyOrRThW67549508 = 79400255;    float lsOyOrRThW99761532 = -954837018;    float lsOyOrRThW11810860 = -422991387;    float lsOyOrRThW47545904 = -323394957;    float lsOyOrRThW79877045 = -919527392;    float lsOyOrRThW20287556 = -710325574;    float lsOyOrRThW94916233 = -481941980;    float lsOyOrRThW20254000 = -761658097;    float lsOyOrRThW83150632 = -172446663;    float lsOyOrRThW32242920 = -327247238;    float lsOyOrRThW97151554 = -621687716;    float lsOyOrRThW36660624 = -493520694;    float lsOyOrRThW74018460 = -838200999;    float lsOyOrRThW51702391 = -978542475;    float lsOyOrRThW34332587 = -28972992;    float lsOyOrRThW93965994 = -95867104;    float lsOyOrRThW72546482 = -324337104;    float lsOyOrRThW86565928 = -490158392;    float lsOyOrRThW15751865 = -772814587;    float lsOyOrRThW62845085 = -859597975;    float lsOyOrRThW2872391 = -241473168;    float lsOyOrRThW40164102 = -928365198;    float lsOyOrRThW74669320 = -649880409;    float lsOyOrRThW55069713 = -126160051;    float lsOyOrRThW72963861 = -223988232;    float lsOyOrRThW71426590 = 2502175;    float lsOyOrRThW96213737 = -991336149;    float lsOyOrRThW42447666 = -165281532;    float lsOyOrRThW22700580 = 92706529;    float lsOyOrRThW94103741 = -816474158;    float lsOyOrRThW96787384 = 50225099;    float lsOyOrRThW97684758 = -616434961;    float lsOyOrRThW34455417 = -792281558;    float lsOyOrRThW26872869 = -686924868;    float lsOyOrRThW49823901 = -180455312;    float lsOyOrRThW44439138 = -823234982;    float lsOyOrRThW29785290 = -16329509;    float lsOyOrRThW73170451 = -675113703;    float lsOyOrRThW59566443 = -517226594;    float lsOyOrRThW70244362 = -537344041;    float lsOyOrRThW99314742 = -168449151;    float lsOyOrRThW52475401 = -902733982;    float lsOyOrRThW47170722 = -774930861;    float lsOyOrRThW95620680 = -404270850;    float lsOyOrRThW13037567 = -31488842;    float lsOyOrRThW332504 = -193663134;    float lsOyOrRThW51251317 = -358313426;    float lsOyOrRThW54333819 = 63014964;    float lsOyOrRThW51402999 = -308751142;    float lsOyOrRThW71496396 = -296256910;    float lsOyOrRThW25710070 = -60354967;    float lsOyOrRThW74095668 = -905699824;    float lsOyOrRThW67174947 = -587823272;    float lsOyOrRThW38762465 = -478585434;    float lsOyOrRThW11556332 = -509248354;    float lsOyOrRThW27385406 = 7765452;    float lsOyOrRThW25092213 = -204956609;    float lsOyOrRThW56741147 = -196831336;    float lsOyOrRThW74582042 = 593274;    float lsOyOrRThW8450456 = -822029568;    float lsOyOrRThW24073819 = -718989426;    float lsOyOrRThW52468567 = -216660449;    float lsOyOrRThW97553420 = -754364626;    float lsOyOrRThW89046890 = -355972506;    float lsOyOrRThW35455536 = 94134754;     lsOyOrRThW75601791 = lsOyOrRThW70485456;     lsOyOrRThW70485456 = lsOyOrRThW2532921;     lsOyOrRThW2532921 = lsOyOrRThW65258834;     lsOyOrRThW65258834 = lsOyOrRThW9956989;     lsOyOrRThW9956989 = lsOyOrRThW96696367;     lsOyOrRThW96696367 = lsOyOrRThW2249947;     lsOyOrRThW2249947 = lsOyOrRThW28183252;     lsOyOrRThW28183252 = lsOyOrRThW88162970;     lsOyOrRThW88162970 = lsOyOrRThW92360292;     lsOyOrRThW92360292 = lsOyOrRThW57448981;     lsOyOrRThW57448981 = lsOyOrRThW80611570;     lsOyOrRThW80611570 = lsOyOrRThW36062967;     lsOyOrRThW36062967 = lsOyOrRThW41301591;     lsOyOrRThW41301591 = lsOyOrRThW17373409;     lsOyOrRThW17373409 = lsOyOrRThW44200671;     lsOyOrRThW44200671 = lsOyOrRThW41596150;     lsOyOrRThW41596150 = lsOyOrRThW20716355;     lsOyOrRThW20716355 = lsOyOrRThW39443488;     lsOyOrRThW39443488 = lsOyOrRThW90531918;     lsOyOrRThW90531918 = lsOyOrRThW94230975;     lsOyOrRThW94230975 = lsOyOrRThW72729400;     lsOyOrRThW72729400 = lsOyOrRThW30321354;     lsOyOrRThW30321354 = lsOyOrRThW27863601;     lsOyOrRThW27863601 = lsOyOrRThW10189121;     lsOyOrRThW10189121 = lsOyOrRThW36993127;     lsOyOrRThW36993127 = lsOyOrRThW25269778;     lsOyOrRThW25269778 = lsOyOrRThW6036210;     lsOyOrRThW6036210 = lsOyOrRThW85735585;     lsOyOrRThW85735585 = lsOyOrRThW65462391;     lsOyOrRThW65462391 = lsOyOrRThW98256551;     lsOyOrRThW98256551 = lsOyOrRThW60661597;     lsOyOrRThW60661597 = lsOyOrRThW82926811;     lsOyOrRThW82926811 = lsOyOrRThW1607550;     lsOyOrRThW1607550 = lsOyOrRThW14428722;     lsOyOrRThW14428722 = lsOyOrRThW67549508;     lsOyOrRThW67549508 = lsOyOrRThW99761532;     lsOyOrRThW99761532 = lsOyOrRThW11810860;     lsOyOrRThW11810860 = lsOyOrRThW47545904;     lsOyOrRThW47545904 = lsOyOrRThW79877045;     lsOyOrRThW79877045 = lsOyOrRThW20287556;     lsOyOrRThW20287556 = lsOyOrRThW94916233;     lsOyOrRThW94916233 = lsOyOrRThW20254000;     lsOyOrRThW20254000 = lsOyOrRThW83150632;     lsOyOrRThW83150632 = lsOyOrRThW32242920;     lsOyOrRThW32242920 = lsOyOrRThW97151554;     lsOyOrRThW97151554 = lsOyOrRThW36660624;     lsOyOrRThW36660624 = lsOyOrRThW74018460;     lsOyOrRThW74018460 = lsOyOrRThW51702391;     lsOyOrRThW51702391 = lsOyOrRThW34332587;     lsOyOrRThW34332587 = lsOyOrRThW93965994;     lsOyOrRThW93965994 = lsOyOrRThW72546482;     lsOyOrRThW72546482 = lsOyOrRThW86565928;     lsOyOrRThW86565928 = lsOyOrRThW15751865;     lsOyOrRThW15751865 = lsOyOrRThW62845085;     lsOyOrRThW62845085 = lsOyOrRThW2872391;     lsOyOrRThW2872391 = lsOyOrRThW40164102;     lsOyOrRThW40164102 = lsOyOrRThW74669320;     lsOyOrRThW74669320 = lsOyOrRThW55069713;     lsOyOrRThW55069713 = lsOyOrRThW72963861;     lsOyOrRThW72963861 = lsOyOrRThW71426590;     lsOyOrRThW71426590 = lsOyOrRThW96213737;     lsOyOrRThW96213737 = lsOyOrRThW42447666;     lsOyOrRThW42447666 = lsOyOrRThW22700580;     lsOyOrRThW22700580 = lsOyOrRThW94103741;     lsOyOrRThW94103741 = lsOyOrRThW96787384;     lsOyOrRThW96787384 = lsOyOrRThW97684758;     lsOyOrRThW97684758 = lsOyOrRThW34455417;     lsOyOrRThW34455417 = lsOyOrRThW26872869;     lsOyOrRThW26872869 = lsOyOrRThW49823901;     lsOyOrRThW49823901 = lsOyOrRThW44439138;     lsOyOrRThW44439138 = lsOyOrRThW29785290;     lsOyOrRThW29785290 = lsOyOrRThW73170451;     lsOyOrRThW73170451 = lsOyOrRThW59566443;     lsOyOrRThW59566443 = lsOyOrRThW70244362;     lsOyOrRThW70244362 = lsOyOrRThW99314742;     lsOyOrRThW99314742 = lsOyOrRThW52475401;     lsOyOrRThW52475401 = lsOyOrRThW47170722;     lsOyOrRThW47170722 = lsOyOrRThW95620680;     lsOyOrRThW95620680 = lsOyOrRThW13037567;     lsOyOrRThW13037567 = lsOyOrRThW332504;     lsOyOrRThW332504 = lsOyOrRThW51251317;     lsOyOrRThW51251317 = lsOyOrRThW54333819;     lsOyOrRThW54333819 = lsOyOrRThW51402999;     lsOyOrRThW51402999 = lsOyOrRThW71496396;     lsOyOrRThW71496396 = lsOyOrRThW25710070;     lsOyOrRThW25710070 = lsOyOrRThW74095668;     lsOyOrRThW74095668 = lsOyOrRThW67174947;     lsOyOrRThW67174947 = lsOyOrRThW38762465;     lsOyOrRThW38762465 = lsOyOrRThW11556332;     lsOyOrRThW11556332 = lsOyOrRThW27385406;     lsOyOrRThW27385406 = lsOyOrRThW25092213;     lsOyOrRThW25092213 = lsOyOrRThW56741147;     lsOyOrRThW56741147 = lsOyOrRThW74582042;     lsOyOrRThW74582042 = lsOyOrRThW8450456;     lsOyOrRThW8450456 = lsOyOrRThW24073819;     lsOyOrRThW24073819 = lsOyOrRThW52468567;     lsOyOrRThW52468567 = lsOyOrRThW97553420;     lsOyOrRThW97553420 = lsOyOrRThW89046890;     lsOyOrRThW89046890 = lsOyOrRThW35455536;     lsOyOrRThW35455536 = lsOyOrRThW75601791;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void rKOIHeNdhZ70715919() {     float qukdKCJDae93326421 = -318396802;    float qukdKCJDae81050733 = -397964868;    float qukdKCJDae47211068 = -7078494;    float qukdKCJDae90180407 = 69682233;    float qukdKCJDae44729700 = -965237973;    float qukdKCJDae40911878 = -749760805;    float qukdKCJDae83989152 = -982628876;    float qukdKCJDae11159570 = -853255724;    float qukdKCJDae50976896 = -253817739;    float qukdKCJDae97291284 = -136114880;    float qukdKCJDae41780302 = -316241034;    float qukdKCJDae91614402 = -260741976;    float qukdKCJDae29069975 = -697715075;    float qukdKCJDae33255188 = -234692882;    float qukdKCJDae30349749 = -974802204;    float qukdKCJDae66894731 = -20574541;    float qukdKCJDae61403687 = -667519100;    float qukdKCJDae62846850 = -608026410;    float qukdKCJDae80039855 = -508522396;    float qukdKCJDae91505849 = -369968486;    float qukdKCJDae60526475 = -361878255;    float qukdKCJDae8162363 = -265204137;    float qukdKCJDae43242842 = -935411997;    float qukdKCJDae57497654 = -997620781;    float qukdKCJDae60530114 = -810189546;    float qukdKCJDae29772771 = -562626677;    float qukdKCJDae33146251 = -875256279;    float qukdKCJDae2953066 = 92861161;    float qukdKCJDae18578528 = -934324272;    float qukdKCJDae50296499 = -815951129;    float qukdKCJDae32607677 = 48997760;    float qukdKCJDae68115999 = -576210244;    float qukdKCJDae53125134 = -310894008;    float qukdKCJDae66693891 = -553041615;    float qukdKCJDae9101478 = 79242910;    float qukdKCJDae75451072 = -362706708;    float qukdKCJDae8423467 = -686925072;    float qukdKCJDae89856821 = -410766187;    float qukdKCJDae39226383 = -408344158;    float qukdKCJDae40917481 = -634232430;    float qukdKCJDae77905110 = -935882955;    float qukdKCJDae28872042 = -974558688;    float qukdKCJDae85158209 = -957596180;    float qukdKCJDae72640225 = -142336201;    float qukdKCJDae95264688 = -320447370;    float qukdKCJDae453195 = -225183035;    float qukdKCJDae17106551 = 3525659;    float qukdKCJDae14419302 = -3035313;    float qukdKCJDae52142516 = -758233811;    float qukdKCJDae61631449 = -514755065;    float qukdKCJDae94295883 = -266921638;    float qukdKCJDae25199661 = -992772588;    float qukdKCJDae52462764 = -562221675;    float qukdKCJDae81435490 = -442508725;    float qukdKCJDae10189444 = -300517506;    float qukdKCJDae85164059 = 46807335;    float qukdKCJDae37807892 = -462552872;    float qukdKCJDae89713413 = -9457713;    float qukdKCJDae29650294 = -120128221;    float qukdKCJDae14956930 = -302611296;    float qukdKCJDae7765627 = -874504526;    float qukdKCJDae81036086 = -975490037;    float qukdKCJDae92581041 = -918931453;    float qukdKCJDae680398 = -437866611;    float qukdKCJDae64683608 = -85112641;    float qukdKCJDae73664303 = -740030791;    float qukdKCJDae38489268 = -949847968;    float qukdKCJDae62376084 = -44673460;    float qukdKCJDae24153711 = -213935792;    float qukdKCJDae54898677 = -512095496;    float qukdKCJDae58471264 = -333649470;    float qukdKCJDae71546866 = -156752913;    float qukdKCJDae23620468 = -99682253;    float qukdKCJDae39122375 = -874289967;    float qukdKCJDae13600740 = -434085531;    float qukdKCJDae31654433 = -387319568;    float qukdKCJDae23004154 = -307607957;    float qukdKCJDae70602616 = -693075797;    float qukdKCJDae62232966 = -577173412;    float qukdKCJDae60076919 = -485006512;    float qukdKCJDae12666221 = -466152336;    float qukdKCJDae18726949 = -772220967;    float qukdKCJDae50810550 = -148905028;    float qukdKCJDae56947079 = -319569207;    float qukdKCJDae56000615 = -449029491;    float qukdKCJDae7408016 = 41770348;    float qukdKCJDae15653236 = 86011431;    float qukdKCJDae71689644 = -868385284;    float qukdKCJDae56504447 = -152524109;    float qukdKCJDae23937418 = -967564426;    float qukdKCJDae37643181 = -900153837;    float qukdKCJDae18710054 = -577467359;    float qukdKCJDae60206528 = -190637967;    float qukdKCJDae24269454 = -5732862;    float qukdKCJDae33151854 = -759727904;    float qukdKCJDae96869023 = -960392918;    float qukdKCJDae36291000 = 44372764;    float qukdKCJDae84477811 = -419729570;    float qukdKCJDae7956618 = 42776439;    float qukdKCJDae21600386 = -318396802;     qukdKCJDae93326421 = qukdKCJDae81050733;     qukdKCJDae81050733 = qukdKCJDae47211068;     qukdKCJDae47211068 = qukdKCJDae90180407;     qukdKCJDae90180407 = qukdKCJDae44729700;     qukdKCJDae44729700 = qukdKCJDae40911878;     qukdKCJDae40911878 = qukdKCJDae83989152;     qukdKCJDae83989152 = qukdKCJDae11159570;     qukdKCJDae11159570 = qukdKCJDae50976896;     qukdKCJDae50976896 = qukdKCJDae97291284;     qukdKCJDae97291284 = qukdKCJDae41780302;     qukdKCJDae41780302 = qukdKCJDae91614402;     qukdKCJDae91614402 = qukdKCJDae29069975;     qukdKCJDae29069975 = qukdKCJDae33255188;     qukdKCJDae33255188 = qukdKCJDae30349749;     qukdKCJDae30349749 = qukdKCJDae66894731;     qukdKCJDae66894731 = qukdKCJDae61403687;     qukdKCJDae61403687 = qukdKCJDae62846850;     qukdKCJDae62846850 = qukdKCJDae80039855;     qukdKCJDae80039855 = qukdKCJDae91505849;     qukdKCJDae91505849 = qukdKCJDae60526475;     qukdKCJDae60526475 = qukdKCJDae8162363;     qukdKCJDae8162363 = qukdKCJDae43242842;     qukdKCJDae43242842 = qukdKCJDae57497654;     qukdKCJDae57497654 = qukdKCJDae60530114;     qukdKCJDae60530114 = qukdKCJDae29772771;     qukdKCJDae29772771 = qukdKCJDae33146251;     qukdKCJDae33146251 = qukdKCJDae2953066;     qukdKCJDae2953066 = qukdKCJDae18578528;     qukdKCJDae18578528 = qukdKCJDae50296499;     qukdKCJDae50296499 = qukdKCJDae32607677;     qukdKCJDae32607677 = qukdKCJDae68115999;     qukdKCJDae68115999 = qukdKCJDae53125134;     qukdKCJDae53125134 = qukdKCJDae66693891;     qukdKCJDae66693891 = qukdKCJDae9101478;     qukdKCJDae9101478 = qukdKCJDae75451072;     qukdKCJDae75451072 = qukdKCJDae8423467;     qukdKCJDae8423467 = qukdKCJDae89856821;     qukdKCJDae89856821 = qukdKCJDae39226383;     qukdKCJDae39226383 = qukdKCJDae40917481;     qukdKCJDae40917481 = qukdKCJDae77905110;     qukdKCJDae77905110 = qukdKCJDae28872042;     qukdKCJDae28872042 = qukdKCJDae85158209;     qukdKCJDae85158209 = qukdKCJDae72640225;     qukdKCJDae72640225 = qukdKCJDae95264688;     qukdKCJDae95264688 = qukdKCJDae453195;     qukdKCJDae453195 = qukdKCJDae17106551;     qukdKCJDae17106551 = qukdKCJDae14419302;     qukdKCJDae14419302 = qukdKCJDae52142516;     qukdKCJDae52142516 = qukdKCJDae61631449;     qukdKCJDae61631449 = qukdKCJDae94295883;     qukdKCJDae94295883 = qukdKCJDae25199661;     qukdKCJDae25199661 = qukdKCJDae52462764;     qukdKCJDae52462764 = qukdKCJDae81435490;     qukdKCJDae81435490 = qukdKCJDae10189444;     qukdKCJDae10189444 = qukdKCJDae85164059;     qukdKCJDae85164059 = qukdKCJDae37807892;     qukdKCJDae37807892 = qukdKCJDae89713413;     qukdKCJDae89713413 = qukdKCJDae29650294;     qukdKCJDae29650294 = qukdKCJDae14956930;     qukdKCJDae14956930 = qukdKCJDae7765627;     qukdKCJDae7765627 = qukdKCJDae81036086;     qukdKCJDae81036086 = qukdKCJDae92581041;     qukdKCJDae92581041 = qukdKCJDae680398;     qukdKCJDae680398 = qukdKCJDae64683608;     qukdKCJDae64683608 = qukdKCJDae73664303;     qukdKCJDae73664303 = qukdKCJDae38489268;     qukdKCJDae38489268 = qukdKCJDae62376084;     qukdKCJDae62376084 = qukdKCJDae24153711;     qukdKCJDae24153711 = qukdKCJDae54898677;     qukdKCJDae54898677 = qukdKCJDae58471264;     qukdKCJDae58471264 = qukdKCJDae71546866;     qukdKCJDae71546866 = qukdKCJDae23620468;     qukdKCJDae23620468 = qukdKCJDae39122375;     qukdKCJDae39122375 = qukdKCJDae13600740;     qukdKCJDae13600740 = qukdKCJDae31654433;     qukdKCJDae31654433 = qukdKCJDae23004154;     qukdKCJDae23004154 = qukdKCJDae70602616;     qukdKCJDae70602616 = qukdKCJDae62232966;     qukdKCJDae62232966 = qukdKCJDae60076919;     qukdKCJDae60076919 = qukdKCJDae12666221;     qukdKCJDae12666221 = qukdKCJDae18726949;     qukdKCJDae18726949 = qukdKCJDae50810550;     qukdKCJDae50810550 = qukdKCJDae56947079;     qukdKCJDae56947079 = qukdKCJDae56000615;     qukdKCJDae56000615 = qukdKCJDae7408016;     qukdKCJDae7408016 = qukdKCJDae15653236;     qukdKCJDae15653236 = qukdKCJDae71689644;     qukdKCJDae71689644 = qukdKCJDae56504447;     qukdKCJDae56504447 = qukdKCJDae23937418;     qukdKCJDae23937418 = qukdKCJDae37643181;     qukdKCJDae37643181 = qukdKCJDae18710054;     qukdKCJDae18710054 = qukdKCJDae60206528;     qukdKCJDae60206528 = qukdKCJDae24269454;     qukdKCJDae24269454 = qukdKCJDae33151854;     qukdKCJDae33151854 = qukdKCJDae96869023;     qukdKCJDae96869023 = qukdKCJDae36291000;     qukdKCJDae36291000 = qukdKCJDae84477811;     qukdKCJDae84477811 = qukdKCJDae7956618;     qukdKCJDae7956618 = qukdKCJDae21600386;     qukdKCJDae21600386 = qukdKCJDae93326421;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void DYMFfWkUez54803822() {     double fAZRIOawpS96235661 = -998455765;    double fAZRIOawpS20232845 = -964522615;    double fAZRIOawpS11113173 = 19203357;    double fAZRIOawpS64159738 = -400962280;    double fAZRIOawpS55717591 = -532555540;    double fAZRIOawpS82411069 = -269033632;    double fAZRIOawpS84134016 = -468114653;    double fAZRIOawpS25364271 = -786310378;    double fAZRIOawpS11563477 = -630291504;    double fAZRIOawpS6085417 = -978480474;    double fAZRIOawpS55579806 = -919083243;    double fAZRIOawpS65460253 = -115402109;    double fAZRIOawpS53485867 = -374562829;    double fAZRIOawpS98220230 = -737845490;    double fAZRIOawpS74316728 = -248265403;    double fAZRIOawpS6781816 = -988243626;    double fAZRIOawpS89044065 = -502446093;    double fAZRIOawpS29261315 = -254621816;    double fAZRIOawpS2329013 = -93393161;    double fAZRIOawpS97521475 = -823103160;    double fAZRIOawpS68457646 = -412852799;    double fAZRIOawpS24439897 = -473155062;    double fAZRIOawpS33680754 = -952354734;    double fAZRIOawpS31750936 = -646450430;    double fAZRIOawpS77530906 = -227489341;    double fAZRIOawpS88169419 = -83034575;    double fAZRIOawpS51639913 = -318891410;    double fAZRIOawpS11178129 = -445310496;    double fAZRIOawpS2743803 = -583775717;    double fAZRIOawpS38881629 = -157955361;    double fAZRIOawpS97213414 = -747632151;    double fAZRIOawpS63163665 = -151665223;    double fAZRIOawpS50866336 = -37946060;    double fAZRIOawpS67750327 = -937587857;    double fAZRIOawpS68937563 = -310781594;    double fAZRIOawpS45320289 = 55817905;    double fAZRIOawpS1391781 = -592477015;    double fAZRIOawpS6876968 = -194076862;    double fAZRIOawpS73191584 = -863997489;    double fAZRIOawpS97630798 = -738072892;    double fAZRIOawpS46963925 = -511190173;    double fAZRIOawpS84067791 = -784259276;    double fAZRIOawpS60000200 = -636020348;    double fAZRIOawpS27546653 = -126073854;    double fAZRIOawpS9246623 = -12356987;    double fAZRIOawpS14734302 = -138147716;    double fAZRIOawpS88960756 = -283112205;    double fAZRIOawpS21937518 = 60574976;    double fAZRIOawpS83454414 = -589657063;    double fAZRIOawpS47884987 = -276300308;    double fAZRIOawpS48618586 = -683465495;    double fAZRIOawpS4422945 = 30542367;    double fAZRIOawpS24124543 = -946104957;    double fAZRIOawpS84167413 = -971475674;    double fAZRIOawpS86346175 = -302011903;    double fAZRIOawpS71795765 = -425300703;    double fAZRIOawpS86552090 = 87832119;    double fAZRIOawpS79362237 = -334346214;    double fAZRIOawpS86628832 = -73472940;    double fAZRIOawpS67548171 = -349520966;    double fAZRIOawpS30771156 = -950142223;    double fAZRIOawpS72955888 = 77195843;    double fAZRIOawpS22620469 = -102534661;    double fAZRIOawpS72681847 = -372336143;    double fAZRIOawpS8872002 = -130848323;    double fAZRIOawpS92416140 = -667418021;    double fAZRIOawpS14593917 = 22543951;    double fAZRIOawpS85735540 = -436974972;    double fAZRIOawpS29282668 = -327063897;    double fAZRIOawpS28996440 = -204083308;    double fAZRIOawpS5390036 = -295766611;    double fAZRIOawpS82167098 = -208369231;    double fAZRIOawpS56069731 = -390624327;    double fAZRIOawpS4698214 = -355320270;    double fAZRIOawpS50557550 = -211912988;    double fAZRIOawpS84389854 = -628593523;    double fAZRIOawpS64439696 = -837134715;    double fAZRIOawpS6134101 = -726280880;    double fAZRIOawpS22504313 = -534093444;    double fAZRIOawpS62796604 = 10658374;    double fAZRIOawpS99208662 = -799922371;    double fAZRIOawpS29702395 = -279466386;    double fAZRIOawpS27723715 = -855653434;    double fAZRIOawpS54858816 = -207475409;    double fAZRIOawpS90263043 = -474489867;    double fAZRIOawpS92790470 = -678174518;    double fAZRIOawpS39039123 = -205560266;    double fAZRIOawpS66698922 = -66470387;    double fAZRIOawpS81404152 = -535575955;    double fAZRIOawpS97141797 = -885480891;    double fAZRIOawpS58768198 = 67985785;    double fAZRIOawpS22029543 = -158130802;    double fAZRIOawpS20248135 = -20603923;    double fAZRIOawpS5643413 = -414476523;    double fAZRIOawpS66859642 = -787930669;    double fAZRIOawpS74008037 = -488386017;    double fAZRIOawpS61447323 = -581724616;    double fAZRIOawpS87318353 = -163684205;    double fAZRIOawpS18674652 = -995225531;    double fAZRIOawpS16830482 = -998455765;     fAZRIOawpS96235661 = fAZRIOawpS20232845;     fAZRIOawpS20232845 = fAZRIOawpS11113173;     fAZRIOawpS11113173 = fAZRIOawpS64159738;     fAZRIOawpS64159738 = fAZRIOawpS55717591;     fAZRIOawpS55717591 = fAZRIOawpS82411069;     fAZRIOawpS82411069 = fAZRIOawpS84134016;     fAZRIOawpS84134016 = fAZRIOawpS25364271;     fAZRIOawpS25364271 = fAZRIOawpS11563477;     fAZRIOawpS11563477 = fAZRIOawpS6085417;     fAZRIOawpS6085417 = fAZRIOawpS55579806;     fAZRIOawpS55579806 = fAZRIOawpS65460253;     fAZRIOawpS65460253 = fAZRIOawpS53485867;     fAZRIOawpS53485867 = fAZRIOawpS98220230;     fAZRIOawpS98220230 = fAZRIOawpS74316728;     fAZRIOawpS74316728 = fAZRIOawpS6781816;     fAZRIOawpS6781816 = fAZRIOawpS89044065;     fAZRIOawpS89044065 = fAZRIOawpS29261315;     fAZRIOawpS29261315 = fAZRIOawpS2329013;     fAZRIOawpS2329013 = fAZRIOawpS97521475;     fAZRIOawpS97521475 = fAZRIOawpS68457646;     fAZRIOawpS68457646 = fAZRIOawpS24439897;     fAZRIOawpS24439897 = fAZRIOawpS33680754;     fAZRIOawpS33680754 = fAZRIOawpS31750936;     fAZRIOawpS31750936 = fAZRIOawpS77530906;     fAZRIOawpS77530906 = fAZRIOawpS88169419;     fAZRIOawpS88169419 = fAZRIOawpS51639913;     fAZRIOawpS51639913 = fAZRIOawpS11178129;     fAZRIOawpS11178129 = fAZRIOawpS2743803;     fAZRIOawpS2743803 = fAZRIOawpS38881629;     fAZRIOawpS38881629 = fAZRIOawpS97213414;     fAZRIOawpS97213414 = fAZRIOawpS63163665;     fAZRIOawpS63163665 = fAZRIOawpS50866336;     fAZRIOawpS50866336 = fAZRIOawpS67750327;     fAZRIOawpS67750327 = fAZRIOawpS68937563;     fAZRIOawpS68937563 = fAZRIOawpS45320289;     fAZRIOawpS45320289 = fAZRIOawpS1391781;     fAZRIOawpS1391781 = fAZRIOawpS6876968;     fAZRIOawpS6876968 = fAZRIOawpS73191584;     fAZRIOawpS73191584 = fAZRIOawpS97630798;     fAZRIOawpS97630798 = fAZRIOawpS46963925;     fAZRIOawpS46963925 = fAZRIOawpS84067791;     fAZRIOawpS84067791 = fAZRIOawpS60000200;     fAZRIOawpS60000200 = fAZRIOawpS27546653;     fAZRIOawpS27546653 = fAZRIOawpS9246623;     fAZRIOawpS9246623 = fAZRIOawpS14734302;     fAZRIOawpS14734302 = fAZRIOawpS88960756;     fAZRIOawpS88960756 = fAZRIOawpS21937518;     fAZRIOawpS21937518 = fAZRIOawpS83454414;     fAZRIOawpS83454414 = fAZRIOawpS47884987;     fAZRIOawpS47884987 = fAZRIOawpS48618586;     fAZRIOawpS48618586 = fAZRIOawpS4422945;     fAZRIOawpS4422945 = fAZRIOawpS24124543;     fAZRIOawpS24124543 = fAZRIOawpS84167413;     fAZRIOawpS84167413 = fAZRIOawpS86346175;     fAZRIOawpS86346175 = fAZRIOawpS71795765;     fAZRIOawpS71795765 = fAZRIOawpS86552090;     fAZRIOawpS86552090 = fAZRIOawpS79362237;     fAZRIOawpS79362237 = fAZRIOawpS86628832;     fAZRIOawpS86628832 = fAZRIOawpS67548171;     fAZRIOawpS67548171 = fAZRIOawpS30771156;     fAZRIOawpS30771156 = fAZRIOawpS72955888;     fAZRIOawpS72955888 = fAZRIOawpS22620469;     fAZRIOawpS22620469 = fAZRIOawpS72681847;     fAZRIOawpS72681847 = fAZRIOawpS8872002;     fAZRIOawpS8872002 = fAZRIOawpS92416140;     fAZRIOawpS92416140 = fAZRIOawpS14593917;     fAZRIOawpS14593917 = fAZRIOawpS85735540;     fAZRIOawpS85735540 = fAZRIOawpS29282668;     fAZRIOawpS29282668 = fAZRIOawpS28996440;     fAZRIOawpS28996440 = fAZRIOawpS5390036;     fAZRIOawpS5390036 = fAZRIOawpS82167098;     fAZRIOawpS82167098 = fAZRIOawpS56069731;     fAZRIOawpS56069731 = fAZRIOawpS4698214;     fAZRIOawpS4698214 = fAZRIOawpS50557550;     fAZRIOawpS50557550 = fAZRIOawpS84389854;     fAZRIOawpS84389854 = fAZRIOawpS64439696;     fAZRIOawpS64439696 = fAZRIOawpS6134101;     fAZRIOawpS6134101 = fAZRIOawpS22504313;     fAZRIOawpS22504313 = fAZRIOawpS62796604;     fAZRIOawpS62796604 = fAZRIOawpS99208662;     fAZRIOawpS99208662 = fAZRIOawpS29702395;     fAZRIOawpS29702395 = fAZRIOawpS27723715;     fAZRIOawpS27723715 = fAZRIOawpS54858816;     fAZRIOawpS54858816 = fAZRIOawpS90263043;     fAZRIOawpS90263043 = fAZRIOawpS92790470;     fAZRIOawpS92790470 = fAZRIOawpS39039123;     fAZRIOawpS39039123 = fAZRIOawpS66698922;     fAZRIOawpS66698922 = fAZRIOawpS81404152;     fAZRIOawpS81404152 = fAZRIOawpS97141797;     fAZRIOawpS97141797 = fAZRIOawpS58768198;     fAZRIOawpS58768198 = fAZRIOawpS22029543;     fAZRIOawpS22029543 = fAZRIOawpS20248135;     fAZRIOawpS20248135 = fAZRIOawpS5643413;     fAZRIOawpS5643413 = fAZRIOawpS66859642;     fAZRIOawpS66859642 = fAZRIOawpS74008037;     fAZRIOawpS74008037 = fAZRIOawpS61447323;     fAZRIOawpS61447323 = fAZRIOawpS87318353;     fAZRIOawpS87318353 = fAZRIOawpS18674652;     fAZRIOawpS18674652 = fAZRIOawpS16830482;     fAZRIOawpS16830482 = fAZRIOawpS96235661;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void UbzwTVqCRd60802172() {     double WjapEzrEzA5550929 = -900986791;    double WjapEzrEzA75655766 = -981057118;    double WjapEzrEzA31265254 = -794437256;    double WjapEzrEzA21949760 = -592626272;    double WjapEzrEzA95780830 = -509070697;    double WjapEzrEzA75954431 = -827902493;    double WjapEzrEzA57788088 = -267363531;    double WjapEzrEzA14500428 = -983269526;    double WjapEzrEzA4372134 = 89647825;    double WjapEzrEzA74709873 = -799276142;    double WjapEzrEzA92481790 = -10324315;    double WjapEzrEzA76698002 = -752771924;    double WjapEzrEzA49267583 = 40015036;    double WjapEzrEzA30308872 = -716899595;    double WjapEzrEzA89592766 = -99795814;    double WjapEzrEzA11752340 = 84232896;    double WjapEzrEzA32854833 = -530031180;    double WjapEzrEzA41453956 = 15320872;    double WjapEzrEzA84341944 = -47033990;    double WjapEzrEzA60716824 = -901811002;    double WjapEzrEzA57136998 = -738552727;    double WjapEzrEzA48648808 = -296051395;    double WjapEzrEzA69742020 = -566883734;    double WjapEzrEzA90957564 = -163611540;    double WjapEzrEzA40593448 = -546622392;    double WjapEzrEzA54077237 = -182246216;    double WjapEzrEzA85398179 = -27780255;    double WjapEzrEzA82272604 = -952447229;    double WjapEzrEzA69860709 = -698195153;    double WjapEzrEzA51662577 = 35960076;    double WjapEzrEzA67211793 = -358283968;    double WjapEzrEzA40423029 = -582836517;    double WjapEzrEzA725667 = -949037073;    double WjapEzrEzA56779275 = -421147744;    double WjapEzrEzA55587838 = -487194188;    double WjapEzrEzA77145183 = -360407542;    double WjapEzrEzA38859388 = -193676878;    double WjapEzrEzA62913765 = 53327775;    double WjapEzrEzA39417285 = -915213276;    double WjapEzrEzA73730695 = -445258404;    double WjapEzrEzA71027059 = -648346591;    double WjapEzrEzA71719447 = -705531367;    double WjapEzrEzA81717205 = -857900729;    double WjapEzrEzA86303553 = -616468742;    double WjapEzrEzA45196697 = -904132781;    double WjapEzrEzA84973537 = -463682274;    double WjapEzrEzA94210387 = -955946350;    double WjapEzrEzA93408555 = -385968412;    double WjapEzrEzA49481558 = -825353580;    double WjapEzrEzA17522889 = -69314011;    double WjapEzrEzA85586295 = -419341491;    double WjapEzrEzA68639986 = -617443851;    double WjapEzrEzA12132201 = -533881863;    double WjapEzrEzA88748657 = -231328792;    double WjapEzrEzA3665174 = -236929988;    double WjapEzrEzA56902120 = -504935397;    double WjapEzrEzA5913747 = -314173384;    double WjapEzrEzA40307689 = -530825717;    double WjapEzrEzA81356311 = 53996119;    double WjapEzrEzA41703593 = -226824482;    double WjapEzrEzA90556251 = -700122239;    double WjapEzrEzA75515484 = -314916302;    double WjapEzrEzA44639718 = -185074373;    double WjapEzrEzA52709556 = -946312252;    double WjapEzrEzA7498080 = -340992174;    double WjapEzrEzA52058762 = -427487798;    double WjapEzrEzA75972336 = -803734851;    double WjapEzrEzA92488307 = -538837220;    double WjapEzrEzA74721033 = -129705407;    double WjapEzrEzA12447583 = -739388272;    double WjapEzrEzA72892952 = -722090226;    double WjapEzrEzA69941067 = -483358956;    double WjapEzrEzA2036671 = -69465853;    double WjapEzrEzA10611249 = -601775586;    double WjapEzrEzA89689764 = -153464411;    double WjapEzrEzA85417550 = 66978640;    double WjapEzrEzA66931603 = -438150666;    double WjapEzrEzA83438466 = -950414992;    double WjapEzrEzA45760868 = -259478760;    double WjapEzrEzA55619911 = 17059882;    double WjapEzrEzA59866849 = -226299867;    double WjapEzrEzA91989624 = -641811843;    double WjapEzrEzA32791046 = -27093650;    double WjapEzrEzA52337821 = -528881143;    double WjapEzrEzA66076282 = -544698433;    double WjapEzrEzA98571807 = -740840117;    double WjapEzrEzA28290828 = 51045345;    double WjapEzrEzA11977009 = -617708281;    double WjapEzrEzA53114102 = -84217757;    double WjapEzrEzA98685718 = -982258792;    double WjapEzrEzA71231436 = 53765841;    double WjapEzrEzA98551698 = -662851162;    double WjapEzrEzA81557454 = 99331655;    double WjapEzrEzA97713691 = -588388794;    double WjapEzrEzA83174444 = -745136166;    double WjapEzrEzA95511575 = -233430289;    double WjapEzrEzA27079729 = -420456995;    double WjapEzrEzA29007650 = -911588478;    double WjapEzrEzA78805473 = -175476568;    double WjapEzrEzA93137935 = -900986791;     WjapEzrEzA5550929 = WjapEzrEzA75655766;     WjapEzrEzA75655766 = WjapEzrEzA31265254;     WjapEzrEzA31265254 = WjapEzrEzA21949760;     WjapEzrEzA21949760 = WjapEzrEzA95780830;     WjapEzrEzA95780830 = WjapEzrEzA75954431;     WjapEzrEzA75954431 = WjapEzrEzA57788088;     WjapEzrEzA57788088 = WjapEzrEzA14500428;     WjapEzrEzA14500428 = WjapEzrEzA4372134;     WjapEzrEzA4372134 = WjapEzrEzA74709873;     WjapEzrEzA74709873 = WjapEzrEzA92481790;     WjapEzrEzA92481790 = WjapEzrEzA76698002;     WjapEzrEzA76698002 = WjapEzrEzA49267583;     WjapEzrEzA49267583 = WjapEzrEzA30308872;     WjapEzrEzA30308872 = WjapEzrEzA89592766;     WjapEzrEzA89592766 = WjapEzrEzA11752340;     WjapEzrEzA11752340 = WjapEzrEzA32854833;     WjapEzrEzA32854833 = WjapEzrEzA41453956;     WjapEzrEzA41453956 = WjapEzrEzA84341944;     WjapEzrEzA84341944 = WjapEzrEzA60716824;     WjapEzrEzA60716824 = WjapEzrEzA57136998;     WjapEzrEzA57136998 = WjapEzrEzA48648808;     WjapEzrEzA48648808 = WjapEzrEzA69742020;     WjapEzrEzA69742020 = WjapEzrEzA90957564;     WjapEzrEzA90957564 = WjapEzrEzA40593448;     WjapEzrEzA40593448 = WjapEzrEzA54077237;     WjapEzrEzA54077237 = WjapEzrEzA85398179;     WjapEzrEzA85398179 = WjapEzrEzA82272604;     WjapEzrEzA82272604 = WjapEzrEzA69860709;     WjapEzrEzA69860709 = WjapEzrEzA51662577;     WjapEzrEzA51662577 = WjapEzrEzA67211793;     WjapEzrEzA67211793 = WjapEzrEzA40423029;     WjapEzrEzA40423029 = WjapEzrEzA725667;     WjapEzrEzA725667 = WjapEzrEzA56779275;     WjapEzrEzA56779275 = WjapEzrEzA55587838;     WjapEzrEzA55587838 = WjapEzrEzA77145183;     WjapEzrEzA77145183 = WjapEzrEzA38859388;     WjapEzrEzA38859388 = WjapEzrEzA62913765;     WjapEzrEzA62913765 = WjapEzrEzA39417285;     WjapEzrEzA39417285 = WjapEzrEzA73730695;     WjapEzrEzA73730695 = WjapEzrEzA71027059;     WjapEzrEzA71027059 = WjapEzrEzA71719447;     WjapEzrEzA71719447 = WjapEzrEzA81717205;     WjapEzrEzA81717205 = WjapEzrEzA86303553;     WjapEzrEzA86303553 = WjapEzrEzA45196697;     WjapEzrEzA45196697 = WjapEzrEzA84973537;     WjapEzrEzA84973537 = WjapEzrEzA94210387;     WjapEzrEzA94210387 = WjapEzrEzA93408555;     WjapEzrEzA93408555 = WjapEzrEzA49481558;     WjapEzrEzA49481558 = WjapEzrEzA17522889;     WjapEzrEzA17522889 = WjapEzrEzA85586295;     WjapEzrEzA85586295 = WjapEzrEzA68639986;     WjapEzrEzA68639986 = WjapEzrEzA12132201;     WjapEzrEzA12132201 = WjapEzrEzA88748657;     WjapEzrEzA88748657 = WjapEzrEzA3665174;     WjapEzrEzA3665174 = WjapEzrEzA56902120;     WjapEzrEzA56902120 = WjapEzrEzA5913747;     WjapEzrEzA5913747 = WjapEzrEzA40307689;     WjapEzrEzA40307689 = WjapEzrEzA81356311;     WjapEzrEzA81356311 = WjapEzrEzA41703593;     WjapEzrEzA41703593 = WjapEzrEzA90556251;     WjapEzrEzA90556251 = WjapEzrEzA75515484;     WjapEzrEzA75515484 = WjapEzrEzA44639718;     WjapEzrEzA44639718 = WjapEzrEzA52709556;     WjapEzrEzA52709556 = WjapEzrEzA7498080;     WjapEzrEzA7498080 = WjapEzrEzA52058762;     WjapEzrEzA52058762 = WjapEzrEzA75972336;     WjapEzrEzA75972336 = WjapEzrEzA92488307;     WjapEzrEzA92488307 = WjapEzrEzA74721033;     WjapEzrEzA74721033 = WjapEzrEzA12447583;     WjapEzrEzA12447583 = WjapEzrEzA72892952;     WjapEzrEzA72892952 = WjapEzrEzA69941067;     WjapEzrEzA69941067 = WjapEzrEzA2036671;     WjapEzrEzA2036671 = WjapEzrEzA10611249;     WjapEzrEzA10611249 = WjapEzrEzA89689764;     WjapEzrEzA89689764 = WjapEzrEzA85417550;     WjapEzrEzA85417550 = WjapEzrEzA66931603;     WjapEzrEzA66931603 = WjapEzrEzA83438466;     WjapEzrEzA83438466 = WjapEzrEzA45760868;     WjapEzrEzA45760868 = WjapEzrEzA55619911;     WjapEzrEzA55619911 = WjapEzrEzA59866849;     WjapEzrEzA59866849 = WjapEzrEzA91989624;     WjapEzrEzA91989624 = WjapEzrEzA32791046;     WjapEzrEzA32791046 = WjapEzrEzA52337821;     WjapEzrEzA52337821 = WjapEzrEzA66076282;     WjapEzrEzA66076282 = WjapEzrEzA98571807;     WjapEzrEzA98571807 = WjapEzrEzA28290828;     WjapEzrEzA28290828 = WjapEzrEzA11977009;     WjapEzrEzA11977009 = WjapEzrEzA53114102;     WjapEzrEzA53114102 = WjapEzrEzA98685718;     WjapEzrEzA98685718 = WjapEzrEzA71231436;     WjapEzrEzA71231436 = WjapEzrEzA98551698;     WjapEzrEzA98551698 = WjapEzrEzA81557454;     WjapEzrEzA81557454 = WjapEzrEzA97713691;     WjapEzrEzA97713691 = WjapEzrEzA83174444;     WjapEzrEzA83174444 = WjapEzrEzA95511575;     WjapEzrEzA95511575 = WjapEzrEzA27079729;     WjapEzrEzA27079729 = WjapEzrEzA29007650;     WjapEzrEzA29007650 = WjapEzrEzA78805473;     WjapEzrEzA78805473 = WjapEzrEzA93137935;     WjapEzrEzA93137935 = WjapEzrEzA5550929;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void XimEEwRimR19043053() {     double RRiZRiWfCR44208056 = -57892829;    double RRiZRiWfCR977488 = -68543446;    double RRiZRiWfCR72761627 = -22912023;    double RRiZRiWfCR24888356 = -414692705;    double RRiZRiWfCR23233850 = -70534829;    double RRiZRiWfCR14303463 = -284623014;    double RRiZRiWfCR81610599 = -872373166;    double RRiZRiWfCR16008254 = -537588901;    double RRiZRiWfCR34055900 = 89288412;    double RRiZRiWfCR17007383 = 44216670;    double RRiZRiWfCR7455220 = -126135530;    double RRiZRiWfCR56680858 = -809856309;    double RRiZRiWfCR97074825 = -757528514;    double RRiZRiWfCR32394699 = -140078541;    double RRiZRiWfCR67320893 = -864289074;    double RRiZRiWfCR58758771 = -706102645;    double RRiZRiWfCR38665533 = -468045707;    double RRiZRiWfCR56353635 = -122641867;    double RRiZRiWfCR5075365 = 88530856;    double RRiZRiWfCR37199226 = 49823011;    double RRiZRiWfCR70350019 = -866039888;    double RRiZRiWfCR89758221 = -297654305;    double RRiZRiWfCR40737875 = -72527431;    double RRiZRiWfCR59964765 = 30707733;    double RRiZRiWfCR67974292 = -516544608;    double RRiZRiWfCR5961551 = -91379341;    double RRiZRiWfCR81117242 = -742291037;    double RRiZRiWfCR80988379 = -463427182;    double RRiZRiWfCR17513576 = -64326687;    double RRiZRiWfCR1361300 = 12839210;    double RRiZRiWfCR63550854 = -607595334;    double RRiZRiWfCR57894789 = 22467589;    double RRiZRiWfCR53092058 = 84889308;    double RRiZRiWfCR80726086 = -217859679;    double RRiZRiWfCR46571915 = 24026072;    double RRiZRiWfCR91805008 = -777172188;    double RRiZRiWfCR44476878 = -805936912;    double RRiZRiWfCR36075755 = 89582841;    double RRiZRiWfCR62423294 = -623381842;    double RRiZRiWfCR26600187 = -244267240;    double RRiZRiWfCR22872578 = -489428029;    double RRiZRiWfCR16585773 = 48793529;    double RRiZRiWfCR14750612 = -552157313;    double RRiZRiWfCR97784051 = -591853102;    double RRiZRiWfCR69210765 = -860521582;    double RRiZRiWfCR89234906 = -842924349;    double RRiZRiWfCR1026609 = -893991318;    double RRiZRiWfCR74648963 = -989142861;    double RRiZRiWfCR28725639 = -639049988;    double RRiZRiWfCR98778727 = -266343720;    double RRiZRiWfCR67624050 = -187200374;    double RRiZRiWfCR5532781 = -578356095;    double RRiZRiWfCR20512550 = -85914736;    double RRiZRiWfCR13352868 = -462220265;    double RRiZRiWfCR48693034 = -917806436;    double RRiZRiWfCR54449835 = -760238524;    double RRiZRiWfCR60239613 = -996016016;    double RRiZRiWfCR12796863 = 46380243;    double RRiZRiWfCR56914064 = -898148098;    double RRiZRiWfCR17272300 = -979155489;    double RRiZRiWfCR33186220 = -542331977;    double RRiZRiWfCR622221 = -308945984;    double RRiZRiWfCR98494678 = -373262215;    double RRiZRiWfCR32694601 = -923550799;    double RRiZRiWfCR53456529 = -348187997;    double RRiZRiWfCR49560431 = -48603120;    double RRiZRiWfCR3588800 = -794745617;    double RRiZRiWfCR16348740 = -439668836;    double RRiZRiWfCR85822783 = -64104614;    double RRiZRiWfCR75515885 = 12883113;    double RRiZRiWfCR14281894 = -900165734;    double RRiZRiWfCR2589779 = -457628548;    double RRiZRiWfCR93930341 = -499260025;    double RRiZRiWfCR78475177 = -667201904;    double RRiZRiWfCR14326649 = -460748961;    double RRiZRiWfCR53764246 = -814833418;    double RRiZRiWfCR75007610 = -745496992;    double RRiZRiWfCR42953823 = -480674330;    double RRiZRiWfCR90753999 = -108770686;    double RRiZRiWfCR78739386 = -673620260;    double RRiZRiWfCR4934943 = -197388023;    double RRiZRiWfCR6468279 = -753148176;    double RRiZRiWfCR52262740 = -824377195;    double RRiZRiWfCR18734848 = -797982967;    double RRiZRiWfCR33737250 = -799960416;    double RRiZRiWfCR58018073 = 70760761;    double RRiZRiWfCR37382239 = -891617676;    double RRiZRiWfCR39739191 = -452890428;    double RRiZRiWfCR32033052 = -300053244;    double RRiZRiWfCR92122080 = -215735404;    double RRiZRiWfCR31565395 = -781156173;    double RRiZRiWfCR31680016 = -752317155;    double RRiZRiWfCR79161690 = -12269062;    double RRiZRiWfCR45150994 = -644226354;    double RRiZRiWfCR93413966 = -701935264;    double RRiZRiWfCR22250357 = -80482045;    double RRiZRiWfCR18091095 = -577944257;    double RRiZRiWfCR82056011 = -628606515;    double RRiZRiWfCR44327523 = -143665106;    double RRiZRiWfCR19650335 = -57892829;     RRiZRiWfCR44208056 = RRiZRiWfCR977488;     RRiZRiWfCR977488 = RRiZRiWfCR72761627;     RRiZRiWfCR72761627 = RRiZRiWfCR24888356;     RRiZRiWfCR24888356 = RRiZRiWfCR23233850;     RRiZRiWfCR23233850 = RRiZRiWfCR14303463;     RRiZRiWfCR14303463 = RRiZRiWfCR81610599;     RRiZRiWfCR81610599 = RRiZRiWfCR16008254;     RRiZRiWfCR16008254 = RRiZRiWfCR34055900;     RRiZRiWfCR34055900 = RRiZRiWfCR17007383;     RRiZRiWfCR17007383 = RRiZRiWfCR7455220;     RRiZRiWfCR7455220 = RRiZRiWfCR56680858;     RRiZRiWfCR56680858 = RRiZRiWfCR97074825;     RRiZRiWfCR97074825 = RRiZRiWfCR32394699;     RRiZRiWfCR32394699 = RRiZRiWfCR67320893;     RRiZRiWfCR67320893 = RRiZRiWfCR58758771;     RRiZRiWfCR58758771 = RRiZRiWfCR38665533;     RRiZRiWfCR38665533 = RRiZRiWfCR56353635;     RRiZRiWfCR56353635 = RRiZRiWfCR5075365;     RRiZRiWfCR5075365 = RRiZRiWfCR37199226;     RRiZRiWfCR37199226 = RRiZRiWfCR70350019;     RRiZRiWfCR70350019 = RRiZRiWfCR89758221;     RRiZRiWfCR89758221 = RRiZRiWfCR40737875;     RRiZRiWfCR40737875 = RRiZRiWfCR59964765;     RRiZRiWfCR59964765 = RRiZRiWfCR67974292;     RRiZRiWfCR67974292 = RRiZRiWfCR5961551;     RRiZRiWfCR5961551 = RRiZRiWfCR81117242;     RRiZRiWfCR81117242 = RRiZRiWfCR80988379;     RRiZRiWfCR80988379 = RRiZRiWfCR17513576;     RRiZRiWfCR17513576 = RRiZRiWfCR1361300;     RRiZRiWfCR1361300 = RRiZRiWfCR63550854;     RRiZRiWfCR63550854 = RRiZRiWfCR57894789;     RRiZRiWfCR57894789 = RRiZRiWfCR53092058;     RRiZRiWfCR53092058 = RRiZRiWfCR80726086;     RRiZRiWfCR80726086 = RRiZRiWfCR46571915;     RRiZRiWfCR46571915 = RRiZRiWfCR91805008;     RRiZRiWfCR91805008 = RRiZRiWfCR44476878;     RRiZRiWfCR44476878 = RRiZRiWfCR36075755;     RRiZRiWfCR36075755 = RRiZRiWfCR62423294;     RRiZRiWfCR62423294 = RRiZRiWfCR26600187;     RRiZRiWfCR26600187 = RRiZRiWfCR22872578;     RRiZRiWfCR22872578 = RRiZRiWfCR16585773;     RRiZRiWfCR16585773 = RRiZRiWfCR14750612;     RRiZRiWfCR14750612 = RRiZRiWfCR97784051;     RRiZRiWfCR97784051 = RRiZRiWfCR69210765;     RRiZRiWfCR69210765 = RRiZRiWfCR89234906;     RRiZRiWfCR89234906 = RRiZRiWfCR1026609;     RRiZRiWfCR1026609 = RRiZRiWfCR74648963;     RRiZRiWfCR74648963 = RRiZRiWfCR28725639;     RRiZRiWfCR28725639 = RRiZRiWfCR98778727;     RRiZRiWfCR98778727 = RRiZRiWfCR67624050;     RRiZRiWfCR67624050 = RRiZRiWfCR5532781;     RRiZRiWfCR5532781 = RRiZRiWfCR20512550;     RRiZRiWfCR20512550 = RRiZRiWfCR13352868;     RRiZRiWfCR13352868 = RRiZRiWfCR48693034;     RRiZRiWfCR48693034 = RRiZRiWfCR54449835;     RRiZRiWfCR54449835 = RRiZRiWfCR60239613;     RRiZRiWfCR60239613 = RRiZRiWfCR12796863;     RRiZRiWfCR12796863 = RRiZRiWfCR56914064;     RRiZRiWfCR56914064 = RRiZRiWfCR17272300;     RRiZRiWfCR17272300 = RRiZRiWfCR33186220;     RRiZRiWfCR33186220 = RRiZRiWfCR622221;     RRiZRiWfCR622221 = RRiZRiWfCR98494678;     RRiZRiWfCR98494678 = RRiZRiWfCR32694601;     RRiZRiWfCR32694601 = RRiZRiWfCR53456529;     RRiZRiWfCR53456529 = RRiZRiWfCR49560431;     RRiZRiWfCR49560431 = RRiZRiWfCR3588800;     RRiZRiWfCR3588800 = RRiZRiWfCR16348740;     RRiZRiWfCR16348740 = RRiZRiWfCR85822783;     RRiZRiWfCR85822783 = RRiZRiWfCR75515885;     RRiZRiWfCR75515885 = RRiZRiWfCR14281894;     RRiZRiWfCR14281894 = RRiZRiWfCR2589779;     RRiZRiWfCR2589779 = RRiZRiWfCR93930341;     RRiZRiWfCR93930341 = RRiZRiWfCR78475177;     RRiZRiWfCR78475177 = RRiZRiWfCR14326649;     RRiZRiWfCR14326649 = RRiZRiWfCR53764246;     RRiZRiWfCR53764246 = RRiZRiWfCR75007610;     RRiZRiWfCR75007610 = RRiZRiWfCR42953823;     RRiZRiWfCR42953823 = RRiZRiWfCR90753999;     RRiZRiWfCR90753999 = RRiZRiWfCR78739386;     RRiZRiWfCR78739386 = RRiZRiWfCR4934943;     RRiZRiWfCR4934943 = RRiZRiWfCR6468279;     RRiZRiWfCR6468279 = RRiZRiWfCR52262740;     RRiZRiWfCR52262740 = RRiZRiWfCR18734848;     RRiZRiWfCR18734848 = RRiZRiWfCR33737250;     RRiZRiWfCR33737250 = RRiZRiWfCR58018073;     RRiZRiWfCR58018073 = RRiZRiWfCR37382239;     RRiZRiWfCR37382239 = RRiZRiWfCR39739191;     RRiZRiWfCR39739191 = RRiZRiWfCR32033052;     RRiZRiWfCR32033052 = RRiZRiWfCR92122080;     RRiZRiWfCR92122080 = RRiZRiWfCR31565395;     RRiZRiWfCR31565395 = RRiZRiWfCR31680016;     RRiZRiWfCR31680016 = RRiZRiWfCR79161690;     RRiZRiWfCR79161690 = RRiZRiWfCR45150994;     RRiZRiWfCR45150994 = RRiZRiWfCR93413966;     RRiZRiWfCR93413966 = RRiZRiWfCR22250357;     RRiZRiWfCR22250357 = RRiZRiWfCR18091095;     RRiZRiWfCR18091095 = RRiZRiWfCR82056011;     RRiZRiWfCR82056011 = RRiZRiWfCR44327523;     RRiZRiWfCR44327523 = RRiZRiWfCR19650335;     RRiZRiWfCR19650335 = RRiZRiWfCR44208056;}
// Junk Finished
