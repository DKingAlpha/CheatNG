#pragma once

#include "imgui.h"
#include <memory>
#include "proc.hpp"
#include "mem.hpp"
#include "views/mem_view.hpp"

struct CheatNGConfig
{
    ThreadImpType thread_imp_type;
    ProcessImpType process_imp_type;
    ProcessesImpType processes_imp_type;
};

struct ImguiRuntimeContext
{
    ImVec4 clear_color;
    ImGuiIO* io;;
    ImFont* hex_font;

    // shared states used by imgui windows
    std::unique_ptr<IProcess> proc;
    std::unique_ptr<MemoryView> mem_view;
    std::unique_ptr<MemoryRegions> mem_regions;
    uint64_t view_addr;

    CheatNGConfig config;
};

bool gui_imgui(ImguiRuntimeContext* ctx);
