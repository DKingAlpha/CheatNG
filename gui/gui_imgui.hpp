#pragma once

#include <map>
#include <memory>
#include <thread>

#include "imgui.h"
#include "mem.hpp"
#include "mem_utils.hpp"
#include "proc.hpp"
#include "factory.hpp"

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

    bool operator<(const GuiResult& rhs) const { return message < rhs.message; }
};

struct CheatNGConfig
{
    bool rpc_mode;
    TargetConfig target_config;
    ThreadImpType thread_imp_type;
    ProcessImpType process_imp_type;
    ProcessesImpType processes_imp_type;
    MemoryImpType memory_imp_type;
};

struct SearchTask
{
    std::string name;
    std::string value;
    MemoryViewDisplayDataType data_type;
    bool is_searching;
    uint64_t search_start;
    uint64_t search_end;
    std::vector<uint64_t> results;
    std::unique_ptr<std::thread> search_thread;
    MemoryViewCache cache;

    SearchTask(std::string name) :name(name) , data_type(MemoryViewDisplayDataType_s32), is_searching(false), search_start(0), search_end(0x7fffffffffffffff) {}

    void clear() { results.clear(); cache.clear();}
};

class CheatNGGUI
{
    ImVec4 clear_color;
    ImGuiIO* io;
    ImFont* hex_font;

    // window states
    bool is_process_list_open;
    bool is_memory_editor_open;
    bool is_memory_regions_open;
    bool is_memory_search_open;
    bool is_settings_open;

    // shared states used by imgui windows
    int pid;

    // memory editor states
    uint64_t view_addr;
    int view_width;
    int view_height;
    bool view_hex;
    MemoryViewDisplayDataType view_data_type;


    // memory search states
    int selected_task_index;
    std::vector<SearchTask> search_tasks;
    
    // config
    CheatNGConfig config;

    std::unique_ptr<Factory> factory;
    // local / remote objects, must be declared after factory
    std::unique_ptr<IProcesses> processes;
    std::unique_ptr<IProcess> proc;
    std::unique_ptr<IMemory> mem;
    std::unique_ptr<MemoryViewRange> mem_view;
    std::unique_ptr<MemoryRegions> mem_regions;

public:
    CheatNGGUI(ImVec4 clear_color, ImFont* hex_font)
        : clear_color(clear_color), hex_font(hex_font), io(&ImGui::GetIO()),
        selected_task_index(0), config()
    {
        config.rpc_mode = true;
        config.target_config.protocol = TargetConfig::Protocol::TCP;
        config.target_config.server_addr = "127.0.0.1";
        config.target_config.port = 22334;
        config.target_config.auth = "123123";

        init_rpc();
        factory = std::make_unique<Factory>(config.rpc_mode, config.target_config);
        reset_process();
        reset_sub_windows();
    }

    bool tick();

    ImVec4 get_clear_color() const { return clear_color; }

private:
    bool show_main_panel();
    bool show_process_list();
    bool show_memory_editor();
    bool show_memory_regions();
    bool show_memory_search();
    bool show_settings();
    bool show_results();

    // components
    bool show_select_datatype(std::string str_id, MemoryViewDisplayDataType& dt, bool allow_aob, bool limit_width);

    GuiResult update_process(bool update_proc, bool update_mem_regions, bool auto_set_range, bool update_mem_view);
    bool tick_update_process();
    bool open_process();

    void reset_process()
    {
        pid = -1;
        view_addr = 0;
        view_width = 16;
        view_height = 16;
        view_hex = true;
        view_data_type = MemoryViewDisplayDataType_u8;
        proc.reset();
        mem.reset();
        mem_regions.reset();
        mem_view.reset();
    }

    void reset_sub_windows()
    {
        is_process_list_open = false;
        is_memory_editor_open = false;
        is_memory_regions_open = false;
        is_memory_search_open = false;
        is_settings_open = false;
    }

private:
    std::map<int, GuiResult> results;
};