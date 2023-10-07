#include "gui_imgui.hpp"

#include "factory.hpp"
#include "i18n/i18n.hpp"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "mem_utils.hpp"
#include "proc.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <memory>
#include <set>
#include <stdint.h>
#include <thread>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"

namespace ImGui {
    bool SelectableWrrapped(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0))
    {
        std::string label_wrapped;
        int max_char_count_per_line = GetCursorScreenPos().x / 8;
        if (max_char_count_per_line < 4) {
            max_char_count_per_line = 4;
        }
        const char* p = label;
        while (*p) {
            const unsigned char c = *p;
            label_wrapped += c;
            if (c < 0x80 && ((p - label) % max_char_count_per_line == max_char_count_per_line - 1)) {
                label_wrapped += '\n';
            }
            p++;
        }

        return ImGui::Selectable(label_wrapped.c_str(), selected, flags, size);
    }
} // namespace ImGui

static int MemoryViewDisplayDataTypeStrWidth[] = {
    4, 8, 16, 32, 20, 40, 4, 8, 16, 32,
};

static int MemoryViewDisplayDataTypeStrWidthHex[] = {
    2, 4, 8, 16, 20, 40, 3, 5, 9, 17,
};

int memory_edit_callback(ImGuiInputTextCallbackData* data)
{
    int cell_width = (int)((uintptr_t)data->UserData);
    if (data->BufTextLen > cell_width) {
        if (data->CursorPos >= data->BufTextLen) {
            data->DeleteChars(data->CursorPos - 1, data->BufTextLen - cell_width);
        } else {
            data->DeleteChars(data->CursorPos, data->BufTextLen - cell_width);
        }
    }
    // clear non-ascii chars
    for (int i = 0; i < data->BufTextLen; i++) {
        if ((unsigned char)data->Buf[i] >= 0x80) {
            data->DeleteChars(i, 1);
        }
    }
    return 0;
}

bool CheatNGGUI::tick_update_process()
{
    if (pid < 0) {
        return true;
    }

    static int last_pid = -1;

    static GuiResult open_process_err = {GuiResultAction_OK, ""};
    static GuiResult update_region_err = {GuiResultAction_OK, ""};
    static GuiResult read_mem_err = {GuiResultAction_OK, ""};
    static GuiResult write_mem_err = {GuiResultAction_OK, ""};
    if (last_pid != pid) {
        last_pid = pid;
        GuiResult result = update_process(true, true, true, true);
        if (result.action != GuiResultAction_OK) {
            results[__LINE__] = result;
        }
        if (result.action & GuiResultAction_CloseRemoteProcess) {
            return false;
        }
    }

    double current_time = ImGui::GetTime();
    static double last_update_time_region = 0.0;
    if (current_time - last_update_time_region > 0.5) {
        last_update_time_region = current_time;
        GuiResult result = update_process(false, true, false, false);
        if (result.action != GuiResultAction_OK) {
            results[__LINE__] = result;
        }
        if (result.action & GuiResultAction_CloseRemoteProcess) {
            return false;
        }
    }

    static double last_update_time_view = 0.0;
    if (current_time - last_update_time_view > 0.05) {
        last_update_time_view = current_time;
        GuiResult result = update_process(false, false, false, true);
        if (result.action != GuiResultAction_OK) {
            results[__LINE__] = result;
        }
        if (result.action & GuiResultAction_CloseRemoteProcess) {
            return false;
        }
    }

    return true;
}

bool CheatNGGUI::show_select_datatype(MemoryViewDisplayDataType& dt)
{
    ImGui::PushItemWidth(ImGui::CalcTextSize("[f32]").x + ImGui::GetStyle().FramePadding.x * 2.0f);
    if (ImGui::BeginCombo("##view_data_type", data_type_name(dt).c_str(), ImGuiComboFlags_NoArrowButton)) {
        for (int i = 0; i < MemoryViewDisplayDataType_BASE_MAX; i++) {
            if (ImGui::Selectable(data_type_name((MemoryViewDisplayDataType)i).c_str())) {
                dt = (MemoryViewDisplayDataType)i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    return true;
}

bool CheatNGGUI::show_memory_editor()
{
    if (!is_memory_editor_open) {
        return false;
    }

    static uint64_t edit_addr = 0;
    static int view_width_step = view_width / 2;

    ImGui::SetNextWindowSize(ImVec2(1280, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Edit Memory"_x, &is_memory_editor_open)) {
        ImGui::End();
        return true;
    }

    // combo to select data types
    static MemoryViewDisplayDataType display_data_type = MemoryViewDisplayDataType_u8;
    show_select_datatype(display_data_type);
    // display in hex
    static bool display_hex = true;
    ImGui::SameLine();
    ImGui::Checkbox("Hex"_x, &display_hex);
    // select width per row
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::CalcTextSize("[0xff]|+|-|Bytes Per Row").x + ImGui::GetStyle().FramePadding.x * 2.0f);
    if (ImGui::InputScalar("Bytes Per Row"_x, ImGuiDataType_U32, &view_width, &view_width_step, &view_width_step, "0x%x")) {
        if (view_width / view_width_step == 3 && view_width % view_width_step == 0) {
            // this is an add action
            view_width += view_width_step;
            view_width_step = view_width / 2;
        } else if (view_width == view_width_step) {
            // this is an sub action
            // no need to do anything.
            view_width_step = view_width / 2;
        } else {
            // user input value
            // ceil view_width to power of 2
            int user_view_width = view_width;
            view_width = 1;
            while (view_width < user_view_width) {
                view_width <<= 1;
            }
            view_width_step = view_width / 2;
        }

        if (view_width_step == 0) {
            view_width_step = 1;
        }
        if (view_width <= 0) {
            view_width = 1;
        }
    }
    ImGui::SameLine();
    constexpr int step_by_1_u32 = 1;
    constexpr int step_by_16_u32 = 16;
    ImGui::PushItemWidth(ImGui::CalcTextSize("[0xff]|+|-|Rows").x + ImGui::GetStyle().FramePadding.x * 2.0f);
    ImGui::InputScalar("Row Count"_x, ImGuiDataType_U32, &view_height, &step_by_1_u32, &step_by_16_u32, "0x%x");

    // input text to change view addr
    const uint64_t view_addr_step = view_width;
    const uint64_t view_addr_step_fast = view_width * 0x10;
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::CalcTextSize("0x01234567890abcdef|+-|Memory Addrress").x + ImGui::GetStyle().FramePadding.x * 2.0f);
    ImGui::InputScalar("Memory Address"_x, ImGuiDataType_U64, &view_addr, &view_addr_step, &view_addr_step_fast, "%016lX", ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsHexadecimal);

    // render data
    ImGui::PushFont(hex_font);
    const uint8_t* remote_data = mem_view->data().data();
    std::string module_name = std::format("{:#0X}", view_addr);
    if (auto region_it = mem_regions->search(view_addr); region_it != mem_regions->end()) {
        module_name = std::format("{}({:#0X})+{:#0X} {}", region_it->name, region_it->start, view_addr - region_it->start, to_string(region_it->prot));
    }
    ImGui::SeparatorText(module_name.c_str());

    ImGui::BeginChild("##mem_view", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::IsWindowHovered()) {
        uint64_t step = ImGui::IsKeyboardKey(ImGuiKey_ModShift) ? view_addr_step_fast : view_addr_step;
        if (io->MouseWheel > 0) {
            view_addr -= step;
        } else if (io->MouseWheel < 0) {
            view_addr += step;
        }
    }
    int cell_width = display_hex ? MemoryViewDisplayDataTypeStrWidthHex[display_data_type] : MemoryViewDisplayDataTypeStrWidth[display_data_type];
    ImGui::Text(std::format("{:>18s}", "|").c_str());
    ImGui::SameLine();
    for (int i = 0; i < view_width; i += data_type_size(display_data_type)) {
        std::string header_cell_text = std::format("{:>0{}X}", i + view_addr % view_width, cell_width);
        ImGui::TextUnformatted(header_cell_text.c_str());
        if (i != view_width - data_type_size(display_data_type)) {
            ImGui::SameLine();
        }
    }
    ImGui::Separator();
    for (int i = 0; i < view_height; i++) {
        ImGui::Text("%016lX |", view_addr + i * view_width);
        ImGui::SameLine();
        for (int j = 0; j < view_width; j += data_type_size(display_data_type)) {
            int idx = i * view_width + j;
            uint64_t addr = view_addr + idx;
            std::string data_str;
            if (idx >= mem_view->data().size()) {
                // invalid data
                data_str = std::format("{:>{}s}", "??", cell_width);
                ImGui::TextUnformatted(data_str.c_str());
                ImGui::SameLine();
                continue;
            }

            // branch to editable memory
            data_str = raw_to_str_expr(remote_data + idx, display_hex, display_data_type);

            static bool need_to_copy_data_to_edit = false;
            if (edit_addr == addr) {
                static const char padding_chars_48[] = "123456781234567812345678123456781234567812345678";
                ImGui::PushItemWidth(ImGui::CalcTextSize(padding_chars_48, padding_chars_48 + cell_width).x);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0, 0});
                static std::string edit_data_str;
                if (need_to_copy_data_to_edit) {
                    edit_data_str = data_str;
                    need_to_copy_data_to_edit = false;
                }
                int edit_flags = ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_EnterReturnsTrue;
                if (ImGui::InputText("##edit_addr", &edit_data_str, edit_flags, memory_edit_callback, (void*)cell_width)) {
                    std::vector<uint8_t> parsed_data = str_expr_to_raw(edit_data_str, display_hex, display_data_type);
                    if (parsed_data.size() > 0 && memcmp(parsed_data.data(), remote_data + idx, parsed_data.size()) != 0) {
                        if (mem->write(addr, parsed_data) != parsed_data.size()) {
                            results[__LINE__] = {GuiResultAction_Error, std::format("{}: {} {:#X}", "Failed to write memory of process"_x, std::strerror(errno), addr)};
                        }
                    } else {
                        results[__LINE__] = {GuiResultAction_Error, std::format("{}: {}. errno: {}", "Invalid input"_x, edit_data_str, std::strerror(errno))};
                    }
                    edit_addr = 0;
                    edit_data_str.clear();
                }
                ImGui::PopStyleVar(3);
                ImGui::PopItemWidth();
            } else {
                ImGui::TextUnformatted(data_str.c_str());
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
                edit_addr = addr;
                need_to_copy_data_to_edit = true;
            }

            ImGui::SameLine();
        }

        // render ascii
        ImGui::Text(" | ");
        ImGui::SameLine();

        for (int k = 0; k < view_width; k++) {
            int idx = i * view_width + k;
            uint64_t addr = view_addr + idx;
            if (idx >= mem_view->data().size()) {
                ImGui::Text("?");
            } else {
                ImGui::Text("%c", remote_data[idx] >= 0x20 && remote_data[idx] < 0x7f ? remote_data[idx] : '.');
            }
            if (k != view_width - 1) {
                ImGui::SameLine();
            }
        }
    }
    ImGui::EndChild();

    ImGui::PopFont();

    ImGui::End(); // window
    return true;
}

static inline std::string str_tolower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool CheatNGGUI::show_memory_regions()
{
    if (!is_memory_regions_open) {
        return false;
    }
    if (!mem_regions) {
        is_memory_regions_open = false;
        return false;
    }
    ImGui::SetNextWindowSize(ImVec2(1280, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Memory Regions"_x, &is_memory_regions_open)) {
        static std::string search_buf;
        ImGui::InputTextWithHint("##Search Module", "🔎 Search Module"_x, &search_buf);

        if (ImGui::BeginTable("##Memory Regions List", 6,
                              ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)) {
            ImGui::TableSetupColumn("Start Address"_x);
            ImGui::TableSetupColumn("End Address"_x);
            ImGui::TableSetupColumn("Size"_x);
            ImGui::TableSetupColumn("Protection"_x);
            ImGui::TableSetupColumn("File Offset"_x);
            ImGui::TableSetupColumn("Name"_x);
            ImGui::TableHeadersRow();

            for (const auto& region : *mem_regions) {
                if (search_buf.size() > 0 && str_tolower(region.name).find(str_tolower(search_buf)) == std::string::npos) {
                    continue;
                }
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%016lX", region.start);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%016lX", region.start + region.size);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%lX", region.size);
                ImGui::TableSetColumnIndex(3);
                ImGui::PushFont(hex_font);
                ImGui::Text("%s", to_string(region.prot).c_str());
                ImGui::PopFont();
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%016lX", region.file_offset);
                ImGui::TableSetColumnIndex(5);
                ImGui::Selectable(region.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                std::string text_id = std::format("##Memory Region {} {}", region.start, region.size);
                if (ImGui::BeginPopupContextItem(text_id.c_str())) {
                    if (ImGui::Selectable("Show in Memory Viewer"_x)) {
                        view_addr = region.start;
                    }
                    if (ImGui::Selectable("Copy Start Address"_x)) {
                        std::string addr_str = std::format("{:#0X}", region.start);
                        ImGui::SetClipboardText(addr_str.c_str());
                    }
                    if (ImGui::Selectable("Copy Name"_x)) {
                        ImGui::SetClipboardText(region.name.c_str());
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    view_addr = region.start;
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
    return true;
}

bool CheatNGGUI::show_memory_search()
{
    if (!is_memory_search_open) {
        return false;
    }
    if (!mem_regions || !mem) {
        is_memory_search_open = false;
        return false;
    }
    ImGui::SetNextWindowSize(ImVec2(1280, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Search Memory"_x, &is_memory_search_open)) {
        if (search_tasks.empty()) {
            search_tasks.push_back(SearchTask(std::format("{}_{}", "Task"_x, search_tasks.size() + 1)));
        }
        if (selected_task_index >= search_tasks.size()) {
            // fix bad index
            selected_task_index = search_tasks.size() - 1;
        }
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("New Search Name"_x).x + ImGui::GetStyle().FramePadding.x * 2.0f);
        std::string new_task_name = "";
        if (ImGui::InputTextWithHint("##New Search Name", "New Search Name"_x, &new_task_name, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (new_task_name.empty()) {
                new_task_name = std::format("{}_{}", "Task"_x, search_tasks.size() + 1);
            }
            search_tasks.push_back(SearchTask(new_task_name));
        }
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::BeginTable("##Search Tasks List", search_tasks.size(), ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit)) {
            for (size_t i = 0; i < search_tasks.size(); i++) {
                auto& task = search_tasks[i];
                ImGui::TableNextColumn();
                if (ImGui::Selectable(task.name.c_str(), selected_task_index == i)) {
                    selected_task_index = i;
                }
                static size_t hovered_id = selected_task_index;
                if (ImGui::IsItemHovered()) {
                    hovered_id = i;
                }
                std::string text_id = std::format("##Search Task {} {}", task.name, i);
                if (ImGui::BeginPopupContextWindow(text_id.c_str())) {
                    if (ImGui::MenuItem("Delete"_x) && !search_tasks[hovered_id].is_searching) {
                        search_tasks.erase(search_tasks.begin() + hovered_id);
                        if (selected_task_index >= search_tasks.size() && search_tasks.size() > 0) {
                            selected_task_index = search_tasks.size() - 1;
                        }
                        if (selected_task_index < 0) {
                            selected_task_index = 0;
                            search_tasks.push_back(SearchTask(std::format("{}_{}", "Task"_x, search_tasks.size() + 1)));
                        }
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::EndTable();
        }

        ImGui::Separator();

        SearchTask& task = search_tasks[selected_task_index];

        ImGui::Text("Search Range"_x);
        ImGui::SameLine();
        std::string search_addr_begin = std::format("{:016X}", task.search_start);
        std::string search_addr_end = std::format("{:016X}", task.search_end);
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("0123456789abcdef"_x).x + ImGui::GetStyle().FramePadding.x * 2.0f);
        if (ImGui::InputText("##Search Address Begin"_x, &search_addr_begin, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {
            uint64_t addr = 0;
            if (std::sscanf(search_addr_begin.c_str(), "%lX", &addr) == 1) {
                task.search_start = addr;
            }
        }
        ImGui::SameLine();
        ImGui::Text(" - ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("0123456789abcdef"_x).x + ImGui::GetStyle().FramePadding.x * 2.0f);
        if (ImGui::InputText("##Search Address End"_x, &search_addr_end, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {
            uint64_t addr = 0;
            if (std::sscanf(search_addr_end.c_str(), "%lX", &addr) == 1) {
                task.search_end = addr;
            }
        }

        show_select_datatype(task.data_type);
        static bool search_hex = false;
        ImGui::SameLine();
        ImGui::Checkbox("Search Hex"_x, &search_hex);
        ImGui::SameLine();

        bool search_pattern_flags = ImGuiInputTextFlags_EnterReturnsTrue;
        if (task.is_searching) {
            search_pattern_flags |= ImGuiInputTextFlags_ReadOnly;
        }
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("🔎 Search Memory"_x).x + ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::InputTextWithHint("##Search Memory", "🔎 Search Memory"_x, &task.value);
        ImGui::SameLine();

        static std::mutex mtx_add_result;

        if (task.is_searching) {
            if (ImGui::Button("Stop Search"_x)) {
                task.is_searching = false;
                if (task.search_thread && task.search_thread->joinable()) {
                    task.search_thread->join();
                }
                task.search_thread.reset();
            }
        }
        if (!task.is_searching) {
            size_t result_count = 0;
            mtx_add_result.lock();
            result_count = task.results.size();
            mtx_add_result.unlock();
            if (result_count == 0) {
                if (ImGui::Button("Start Search"_x)) {
                    // new search
                    task.is_searching = true;
                    task.clear();
                    if (task.search_thread && task.search_thread->joinable()) {
                        task.search_thread->join();
                    }
                    task.search_thread.reset(new std::thread([this, &task]() {
                        SearchMemory::begin_search(mem, task.is_searching, search_hex, task.value, task.data_type, task.search_start, task.search_end, [&task](uint64_t addr) -> bool {
                            mtx_add_result.lock();
                            task.results.push_back(addr);
                            mtx_add_result.unlock();
                            return true;
                        });
                        task.is_searching = false;
                    }));
                }
            } else {
                if (ImGui::Button("Continue Search"_x)) {
                    // continue search
                    task.is_searching = true;
                    if (task.search_thread && task.search_thread->joinable()) {
                        task.search_thread->join();
                    }
                    task.search_thread.reset(new std::thread([this, &task]() {
                        std::vector<uint64_t> prev_results = std::move(task.results);
                        task.clear();
                        SearchMemory::continue_search(mem, task.is_searching, search_hex, task.value, task.data_type, prev_results, [&task](uint64_t addr) -> bool {
                            mtx_add_result.lock();
                            task.results.push_back(addr);
                            mtx_add_result.unlock();
                            return true;
                        });
                        task.is_searching = false;
                    }));
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear Results"_x)) {
                    task.clear();
                }
            }
        }

        if (ImGui::BeginChild("##Search Results", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            std::string search_result_title = std::format("{}: {}", "Total"_x, task.results.size());
            ImGui::Text(search_result_title.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Clear"_x)) {
                task.clear();
            }
            static bool show_hex = false;
            ImGui::SameLine();
            ImGui::Checkbox("Show Hex"_x, &show_hex);
            ImGui::Separator();
            ImGui::BeginChild("##Search Results List", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize);
            mtx_add_result.lock();
            int data_size = data_type_size(task.data_type);
            task.cache.update_data_size(data_size);
            for (auto addr : task.results) {
                std::string value_str;
                auto [valid, buf] = task.cache.get(mem, addr, data_size);
                if (valid) {
                    value_str = raw_to_str_expr(buf.data(), show_hex, task.data_type);
                    if (task.data_type < MemoryViewDisplayDataType_BASE_MAX && value_str.size() && value_str[0] == '+') {
                        // strip the leading '+' sign
                        value_str = value_str.substr(1);
                    }
                } else {
                    value_str = "??";
                }
                std::string display_addr = std::format("{:016X} {}", addr, value_str);
                ImGui::Selectable(display_addr.c_str());
            }
            mtx_add_result.unlock();
            ImGui::EndChild();
        }
        ImGui::EndChild();
    }
    ImGui::End();
    return true;
}

bool CheatNGGUI::show_settings()
{
    if (!is_settings_open) {
        return false;
    }
    ImGui::SetNextWindowSize(ImVec2(1280, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings"_x, &is_settings_open)) {
    }
    ImGui::End();
    return true;
}

GuiResult CheatNGGUI::update_process(bool update_proc, bool update_mem_regions, bool auto_set_range, bool update_mem_view)
{
    if (update_proc && pid >= 0) {
        proc.reset();
        proc = Factory::create(config.process_imp_type, pid);
        if (!proc->is_valid()) {
            return {(GuiResultAction)(GuiResultAction_CloseRemoteProcess | GuiResultAction_Error), std::format("{}: {}", "Invalid pid"_x, std::strerror(errno))};
        }
        mem.reset();
        mem = Factory::create(config.memory_imp_type, pid);
        mem_view.reset();
        mem_view = std::make_unique<MemoryViewRange>();
    }

    if (update_mem_regions && mem) {
        mem_regions.reset(new MemoryRegions(mem->regions()));
        if (!mem_regions->size()) {
            return {(GuiResultAction)(GuiResultAction_CloseRemoteProcess | GuiResultAction_Error), std::format("{}: {}", "Failed to read memory regions of process"_x, std::strerror(errno))};
        }
    }

    if (auto_set_range && proc && mem_regions && mem_view && !view_addr) {
        std::string main_keyword;
        if (auto cmdlines = proc->cmdlines(); cmdlines.size() > 0) {
            main_keyword = cmdlines[0];
        }
        if (main_keyword.empty()) {
            main_keyword = proc->name;
        }

        bool found_main = false;
        for (auto region : mem_regions->search_all(main_keyword)) {
            if (region->prot & MemoryProtectionFlags::EXECUTE) {
                view_addr = region->start;
                mem_view->set(view_addr, view_width * view_height);
                found_main = true;
                break;
            }
        }
        if (!found_main) {
            mem_view->set(mem_regions->begin()->start, mem_regions->begin()->size);
        }

        if (!mem_view->update(mem)) {
            return {GuiResultAction_Error, std::format("{}: {} {:#X}", "Failed to read memory of process"_x, std::strerror(errno), mem_view->start())};
        }
    }

    if (update_mem_view && mem_view) {
        mem_view->set(view_addr, view_width * view_height);
        if (!mem_view->update(mem)) {
            return {GuiResultAction_Error, std::format("{}: {} {:#X}", "Failed to read memory of process"_x, std::strerror(errno), mem_view->start())};
        }
    }

    return {GuiResultAction_OK, ""};
}

bool CheatNGGUI::open_process()
{
    static GuiResult result = {GuiResultAction_OK, ""};
    static int last_pid = -1;
    if (last_pid != pid) {
        last_pid = pid;
        result = update_process(true, true, true, true);
    }
    bool popup_msg_open = true;
    if (result.action & GuiResultAction_CloseRemoteProcess) {
        const char* popup_title = (result.action & GuiResultAction_Warn) ? "⚠️ Warning"_x : "⛔ Error"_x;
        if (ImGui::BeginPopupModal(popup_title, &popup_msg_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
            ImGui::Text("%s", result.message.c_str());
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 10);
            if (ImGui::Button("OK"_x)) {
                popup_msg_open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::OpenPopup(popup_title);
        if (!popup_msg_open) {
            result = {GuiResultAction_OK, ""};
            last_pid = -1;
            ImGui::OpenPopup("Choose Process"_x);
            return false;
        }
        return true;
    }
    return true;
}

bool CheatNGGUI::show_process_list()
{
    if (!is_process_list_open) {
        return true;
    }
    static int selected_pid = -1; // for table selection. not the final pid we are opening
    static int selected_pid_next = -1;
    static int opened_pid = -1;
    static std::set<int> interested_pids;
    static bool show_kthread = false;
    bool modal_open = true;
    static std::string search_buf;
    ImGui::SetNextWindowSize(ImVec2(800, 440), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Choose Process"_x, &modal_open, ImGuiWindowFlags_None)) {
        ImGui::InputTextWithHint("##Search Process", "🔎 Search Process"_x, &search_buf);
        ImGui::SameLine();
        ImGui::Checkbox("Show Kernel Threads"_x, &show_kthread);
        if (ImGui::BeginTable("##Process List", 2,
                              ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)) {
            ImGui::TableSetupColumn("PID"_x);
            ImGui::TableSetupColumn("Command lines"_x);
            ImGui::TableHeadersRow();

            static auto processes = Factory::create(config.processes_imp_type);
            static double last_update_time = 0.0;
            double current_time = ImGui::GetTime();
            if (current_time - last_update_time > 1.0) {
                last_update_time = current_time;
                processes->update();
            }
            for (const auto& proc : *processes) {
                std::string display_name = proc->cmdlines_str();
                if (!show_kthread && display_name.empty()) {
                    continue;
                }
                if (display_name.empty()) {
                    display_name = std::format("[{}]", proc->name);
                }
                if (search_buf.size() > 0 && str_tolower(display_name).find(str_tolower(search_buf)) == std::string::npos) {
                    // don't filter out pid we are interersted in
                    if (!interested_pids.contains(proc->id)) {
                        continue;
                    }
                }
                std::string line = std::format("{}##[{}]", proc->id, display_name);
                ImGui::PushID(proc->id);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", proc->id);
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Selectable(display_name.c_str(), selected_pid == proc->id, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAllColumns)) {
                    selected_pid = proc->id;
                }
                ImGui::PopID();
                if (selected_pid_next == proc->id) {
                    selected_pid = proc->id;
                    selected_pid_next = -1;
                    ImGui::SetScrollHereY(0.5);
                }

                if (ImGui::BeginPopupContextItem()) {
                    selected_pid = proc->id;
                    std::string jump_to_parent = std::format("{} [{}] {}", "Jump To Parent"_x, proc->parent_id, Factory::create(config.process_imp_type, proc->parent_id)->cmdlines_str());
                    if (ImGui::SelectableWrrapped(jump_to_parent.c_str())) {
                        selected_pid_next = proc->parent_id; // scroll in next frame
                        interested_pids.insert(selected_pid_next);
                    }
                    if (proc->children().size() > 0) {
                        if (ImGui::BeginMenu("Jump To Children"_x)) {
                            for (const auto& child : proc->children()) {
                                std::string jump_to_child = std::format("[{}] {}", child->id, child->cmdlines_str());
                                if (ImGui::SelectableWrrapped(jump_to_child.c_str())) {
                                    selected_pid_next = child->id; // scroll in next frame
                                    interested_pids.insert(selected_pid_next);
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    opened_pid = proc->id;
                    is_process_list_open = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndPopup();
    } else {
        // any time close popup, clear interested pids
        interested_pids.clear();
    }

    if (!modal_open) {
        // only when click close button of popup
        selected_pid = -1;
    }

    if (opened_pid >= 0) {
        pid = opened_pid;
        auto retval = open_process();
        if (!retval) {
            opened_pid = -1;
            selected_pid = -1;
            reset_process();
        }
        return retval;
    }
    return false;
}

bool CheatNGGUI::show_results()
{
    /*
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(ImGui::GetWindowWidth(), 120));
    if(!ImGui::BeginChild("##log results")) {
        ImGui::EndChild();
        return true;
    }
    */

    if (results.size() && ImGui::Button("Clear All"_x)) {
        results.clear();
    }
    for (auto it = results.begin(); it != results.end();) {
        auto err = it->second;
        if (err.action == GuiResultAction_OK) {
            it = results.erase(it);
        } else {
            if (err.action & GuiResultAction_Warn) {
                if (ImGui::Button("✖️")) {
                    it = results.erase(it);
                    continue;
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                ImGui::Text("%s: %s", "⚠️ Warning"_x, err.message.c_str());
                ImGui::PopStyleColor();
            } else if (err.action & GuiResultAction_Error) {
                if (ImGui::Button("✖️")) {
                    it = results.erase(it);
                    continue;
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("%s: %s", "⛔ Error"_x, err.message.c_str());
                ImGui::PopStyleColor();
            } else {
                assert(false);
            }
            it++;
        }
    }
    // ImGui::EndChild();
    return true;
}

bool CheatNGGUI::show_main_panel()
{
    // ImGuiViewport* viewport = ImGui::GetMainViewport();
    // ImGui::SetNextWindowPos(viewport->Pos);
    // ImGui::SetNextWindowSize(viewport->Size);
    // ImGui::Begin("CheatNG", NULL, ImGuiWindowFlags_NoDecoration |
    // ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    static bool main_window_open = true;
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

    /*
    // "Main Area = entire viewport,\nWork Area = entire viewport minus sections used by the main menu bars, task bars etc.\n\nEnable the main-menu bar in Examples menu to see the difference."
    static bool use_work_area = true;
    static ImGuiWindowFlags main_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one or the other.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);
    */

    if (ImGui::Begin("CheatNG", &main_window_open)) {
        //// buttons
        if (ImGui::Button("🖥️")) {
            ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
            is_process_list_open = true;
            ImGui::OpenPopup("Choose Process"_x);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Choose Process"_x);
        }
        show_process_list();

        ImGui::SameLine();
        if (ImGui::Button("🔧")) {
            is_settings_open = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Settings"_x);
        }
        show_settings();

        if (pid >= 0) {
            ImGui::SameLine();
            ImGui::Text("[%d] %s", pid, proc ? proc->cmdlines_str().c_str() : "");

            if (ImGui::Button("🔍")) {
                is_memory_search_open = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Search Memory"_x);
            }
            ImGui::SameLine();
            if (ImGui::Button("🧮")) {
                is_memory_editor_open = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Edit Memory"_x);
            }
            ImGui::SameLine();
            if (ImGui::Button("Ⓜ️")) {
                is_memory_regions_open = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Memory Regions"_x);
            }
            show_memory_editor();
            show_memory_regions();
            show_memory_search();
        }

        // log
        if (!tick_update_process()) {
            reset_process();
            reset_sub_windows();
        }
        show_results();
    }
    ImGui::End();
    return main_window_open;
}

bool CheatNGGUI::tick()
{
    bool retval = show_main_panel();
    // Our state
    static bool show_demo_window = false;

    // 1. Show a simple window that we create ourselves. We use a Begin/End pair
    // to create a named window.
    {
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!"
                                       // and append into it.
        ImGui::Checkbox("Demo Window",
                        &show_demo_window); // Edit bools storing our window
                                            // open/close state

        ImGui::ColorEdit4("clear color",
                          (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets
                                     // return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
        ImGui::End();
    }

    // 2. Show the big demo window (Most of the sample code is in
    // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
    // ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    return retval;
}

#pragma clang diagnostic pop
