#pragma once

#include <memory>
#include <set>

#include "imgui.h"
#include "proc.hpp"
#include "mem.hpp"
#include "mem_utils.hpp"

enum GuiResultAction
{
    GuiResultAction_OK = 0,
    // dont close process, popup warn/error
    GuiResultAction_Warn = 1 << 0,
    GuiResultAction_Error = 1 << 1,
    // close process, and popup error
    GuiResultAction_CloseRemoteProcess = 0x1000,
};

struct GuiResult
{
    GuiResultAction action;
    std::string message;

    bool operator < (const GuiResult& rhs) const { return message < rhs.message; }
};

struct CheatNGConfig
{
    ThreadImpType thread_imp_type;
    ProcessImpType process_imp_type;
    ProcessesImpType processes_imp_type;
    MemoryImpType memory_imp_type;
};

class CheatNGGUI
{
    ImVec4 clear_color;
    ImGuiIO* io;;
    ImFont* hex_font;

    // shared states used by imgui windows
    int pid;
    std::unique_ptr<IProcess> proc;
    std::unique_ptr<IMemory> mem;
    std::unique_ptr<MemoryView> mem_view;
    std::unique_ptr<MemoryRegions> mem_regions;
    uint64_t view_addr;
    int view_width;
    int view_height;

    // window states
    bool is_process_list_open;
    bool is_memory_editor_open;
    bool is_memory_regions_open;
    bool is_memory_search_open;
    bool is_settings_open;

    // config
    CheatNGConfig config;

public:
    CheatNGGUI(ImVec4 clear_color, ImFont* hex_font) : clear_color(clear_color), hex_font(hex_font), io(&ImGui::GetIO()),
        pid(-1), view_addr(0), view_width(16), view_height(16),
        is_process_list_open(false), is_memory_editor_open(false), is_memory_regions_open(false), is_memory_search_open(false), is_settings_open(false)
        {}

    bool tick();

    ImVec4 get_clear_color() const { return clear_color; }

private:
    bool main_panel();
    bool show_process_list();
    bool show_memory_editor();
    bool show_memory_regions();
    bool show_memory_search();
    bool show_settings();

    bool show_results();

    bool open_process();

    bool tick_update_process();

    GuiResult update_process(bool update_proc, bool update_mem_regions, bool auto_set_range, bool update_mem_view);

private:
    std::set<GuiResult> results;
};
