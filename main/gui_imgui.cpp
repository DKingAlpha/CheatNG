#include "gui_imgui.hpp"

#include "i18n/i18n.hpp"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "proc.hpp"
#include "views/mem_view.hpp"
#include <cstring>
#include <format>
#include <memory>
#include <set>
#include <stdint.h>
#include <thread>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"

static void walk_process_tree(std::vector<const Process*> procs)
{
    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    for (const Process* proc : procs) {
        std::string display_name = proc->cmdlines_str();
        if (display_name.empty()) {
            display_name = proc->name();
        }
        std::string line = std::format("[{}] {}", proc->id(), display_name);
        static int selected_pid = 0;
        const bool is_selected = selected_pid == proc->id();
        ImGuiTreeNodeFlags node_flags = base_flags;
        if (is_selected) {
            node_flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (proc->children().empty()) {
            node_flags |= ImGuiTreeNodeFlags_Leaf;
        }
        bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)proc->id(), node_flags, "%s", display_name.c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selected_pid = proc->id();
        }
        if (node_open) {
            ImGui::Indent();
            walk_process_tree(proc->children());
            ImGui::Unindent();
            ImGui::TreePop();
        }
    }
}

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
};

enum MemoryViewDisplayDataType
{
    MemoryViewDisplayDataType_u8,
    MemoryViewDisplayDataType_u16,
    MemoryViewDisplayDataType_u32,
    MemoryViewDisplayDataType_u64,
    MemoryViewDisplayDataType_f32,
    MemoryViewDisplayDataType_f64,
    MemoryViewDisplayDataType_s8,
    MemoryViewDisplayDataType_s16,
    MemoryViewDisplayDataType_s32,
    MemoryViewDisplayDataType_s64,
    MemoryViewDisplayDataType_MAX,
};

static const char* MemoryViewDisplayDataTypeNames[] = {
    "u8", "u16", "u32", "u64", "f32", "f64", "s8", "s16", "s32", "s64",
};

static int MemoryViewDisplayDataTypeWidth[] = {
    1, 2, 4, 8, 4, 8, 1, 2, 4, 8,
};

static int MemoryViewDisplayDataTypeStrWidth[] = {
    4, 8, 16, 32, 20, 40, 4, 8, 16, 32,
};

static int MemoryViewDisplayDataTypeStrWidthHex[] = {
    2, 4, 8, 16, 20, 40, 3, 5, 9, 17,
};

static const char* MemoryViewDisplayDataTypeFormat[] = {
    "{:4d}", "{:8d}", "{:16d}", "{:32d}", "{:20.10}", "{:40.20}", "{:4d}", "{:8d}", "{:16d}", "{:32d}",
};
static const char* MemoryViewDisplayDataTypeFormatHex[] = {
    "{:02X}", "{:04X}", "{:08X}", "{:016X}", "{:20.10}", "{:40.20}", "{:+03X}", "{:+05X}", "{:+09X}", "{:+017X}",
};

static const char* MemoryViewDisplayDataTypeParseFormat[] = {
    "%hhd", "%hd", "%d", "%ld", "%f", "%lf", "%hhd", "%hd", "%d", "%ld",
};
static const char* MemoryViewDisplayDataTypeParseFormatHex[] = {
    "%hhx", "%hx", "%x", "%lx", "%f", "%lf", "%hhx", "%hx", "%x", "%lx",
};

std::vector<uint8_t> parse_mem_data(std::string& data, bool hex, MemoryViewDisplayDataType dt)
{
    std::vector<uint8_t> ret;
    if (data.empty()) {
        return ret;
    }
    const char* fmt = hex ? MemoryViewDisplayDataTypeParseFormatHex[dt] : MemoryViewDisplayDataTypeParseFormat[dt];
    if (dt == MemoryViewDisplayDataType_f32) {
        float data_float = 0.0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_float)) {
            ret.resize(sizeof(float));
            memcpy(ret.data(), &data_float, sizeof(float));
        }
    } else if (dt == MemoryViewDisplayDataType_f64) {
        double data_double = 0.0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_double)) {
            ret.resize(sizeof(double));
            memcpy(ret.data(), &data_double, sizeof(double));
        }
    } else if (dt == MemoryViewDisplayDataType_u8) {
        uint8_t data_int = 0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_int)) {
            ret.resize(sizeof(uint8_t));
            memcpy(ret.data(), &data_int, sizeof(uint8_t));
        }
    } else if (dt == MemoryViewDisplayDataType_u16) {
        uint16_t data_int = 0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_int)) {
            ret.resize(sizeof(uint16_t));
            memcpy(ret.data(), &data_int, sizeof(uint16_t));
        }
    } else if (dt == MemoryViewDisplayDataType_u32) {
        uint32_t data_int = 0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_int)) {
            ret.resize(sizeof(uint32_t));
            memcpy(ret.data(), &data_int, sizeof(uint32_t));
        }
    } else if (dt == MemoryViewDisplayDataType_u64) {
        uint64_t data_int = 0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_int)) {
            ret.resize(sizeof(uint64_t));
            memcpy(ret.data(), &data_int, sizeof(uint64_t));
        }
    } else if (dt == MemoryViewDisplayDataType_s8) {
        int8_t data_int = 0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_int)) {
            ret.resize(sizeof(int8_t));
            memcpy(ret.data(), &data_int, sizeof(int8_t));
        }
    } else if (dt == MemoryViewDisplayDataType_s16) {
        int16_t data_int = 0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_int)) {
            ret.resize(sizeof(int16_t));
            memcpy(ret.data(), &data_int, sizeof(int16_t));
        }
    } else if (dt == MemoryViewDisplayDataType_s32) {
        int32_t data_int = 0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_int)) {
            ret.resize(sizeof(int32_t));
            memcpy(ret.data(), &data_int, sizeof(int32_t));
        }
    } else if (dt == MemoryViewDisplayDataType_s64) {
        int64_t data_int = 0;
        if (1 == std::sscanf(data.c_str(), fmt, &data_int)) {
            ret.resize(sizeof(int64_t));
            memcpy(ret.data(), &data_int, sizeof(int64_t));
        }
    }
    return ret;
}

std::string mem_view_data_to_str(const uint8_t* ptr, bool hex, MemoryViewDisplayDataType dt)
{
    const char* data_fmt = hex ? MemoryViewDisplayDataTypeFormatHex[dt] : MemoryViewDisplayDataTypeFormat[dt];
    if (dt == MemoryViewDisplayDataType_f32) {
        float data_float = 0.0;
        memcpy(&data_float, ptr, sizeof(float));
        return std::vformat(data_fmt, std::make_format_args(data_float));
    } else if (dt == MemoryViewDisplayDataType_f64) {
        double data_double = 0.0;
        memcpy(&data_double, ptr, sizeof(double));
        return std::vformat(data_fmt, std::make_format_args(data_double));
    } else if (dt == MemoryViewDisplayDataType_u8) {
        uint8_t data_int = 0;
        memcpy(&data_int, ptr, sizeof(uint8_t));
        return std::vformat(data_fmt, std::make_format_args(data_int));
    } else if (dt == MemoryViewDisplayDataType_u16) {
        uint16_t data_int = 0;
        memcpy(&data_int, ptr, sizeof(uint16_t));
        return std::vformat(data_fmt, std::make_format_args(data_int));
    } else if (dt == MemoryViewDisplayDataType_u32) {
        uint32_t data_int = 0;
        memcpy(&data_int, ptr, sizeof(uint32_t));
        return std::vformat(data_fmt, std::make_format_args(data_int));
    } else if (dt == MemoryViewDisplayDataType_u64) {
        uint64_t data_int = 0;
        memcpy(&data_int, ptr, sizeof(uint64_t));
        return std::vformat(data_fmt, std::make_format_args(data_int));
    } else if (dt == MemoryViewDisplayDataType_s8) {
        int8_t data_int = 0;
        memcpy(&data_int, ptr, sizeof(int8_t));
        return std::vformat(data_fmt, std::make_format_args(data_int));
    } else if (dt == MemoryViewDisplayDataType_s16) {
        int16_t data_int = 0;
        memcpy(&data_int, ptr, sizeof(int16_t));
        return std::vformat(data_fmt, std::make_format_args(data_int));
    } else if (dt == MemoryViewDisplayDataType_s32) {
        int32_t data_int = 0;
        memcpy(&data_int, ptr, sizeof(int32_t));
        return std::vformat(data_fmt, std::make_format_args(data_int));
    } else if (dt == MemoryViewDisplayDataType_s64) {
        int64_t data_int = 0;
        memcpy(&data_int, ptr, sizeof(int64_t));
        return std::vformat(data_fmt, std::make_format_args(data_int));
    }
    return "";
}

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

static GuiResult cross_edit_select_process(ImguiRuntimeContext* ctx, int pid)
{
    static int current_pid = -1;

    static uint64_t edit_addr = 0;
    static int view_width = 16;
    static int view_height = 16;

    auto& proc = ctx->proc;
    auto& mem_view = ctx->mem_view;
    auto& mem_regions = ctx->mem_regions;
    auto& view_addr = ctx->view_addr;

    auto update_process = [&](bool update_proc, bool update_mem_regions, bool auto_set_range, bool update_mem_view) -> GuiResult {
        if (update_proc) {
            proc.reset(new Process(pid));
            if (!proc->is_valid()) {
                return {(GuiResultAction)(GuiResultAction_CloseRemoteProcess | GuiResultAction_Error), std::format("{}: {}", "Invalid pid"_x, std::strerror(errno))};
            }
            mem_view.reset(new MemoryView(pid));
        }
        if (update_mem_regions) {
            mem_regions.reset(new MemoryRegions(mem_view->regions()));
            if (!mem_regions->size()) {
                return {(GuiResultAction)(GuiResultAction_CloseRemoteProcess | GuiResultAction_Error), std::format("{}: {}", "Failed to read memory regions of process"_x, std::strerror(errno))};
            }
        }

        if (auto_set_range) {
            std::string main_keyword;
            if (auto cmdlines = proc->cmdlines(); cmdlines.size() > 0) {
                main_keyword = cmdlines[0];
            }
            if (main_keyword.empty()) {
                main_keyword = proc->name();
            }

            bool found_main = false;
            for (auto region : mem_regions->search_all(main_keyword)) {
                if (region->prot & MemoryProtectionFlags::EXECUTE) {
                    view_addr = region->start;
                    mem_view->set_range(view_addr, view_width * view_height);
                    found_main = true;
                    break;
                }
            }
            if (!found_main) {
                mem_view->set_range(mem_regions->begin()->start, mem_regions->begin()->size);
            }

            if (!mem_view->update()) {
                return {GuiResultAction_Error, std::format("{}: {} {:#X}", "Failed to read memory of process"_x, std::strerror(errno), mem_view->start())};
            }
        }

        if (update_mem_view) {
            mem_view->set_range(view_addr, view_width * view_height);
            if (!mem_view->update()) {
                return {GuiResultAction_Error, std::format("{}: {} {:#X}", "Failed to read memory of process"_x, std::strerror(errno), mem_view->start())};
            }
        }

        return {GuiResultAction_OK, ""};
    };

    static GuiResult open_process_err = {GuiResultAction_OK, ""};
    static GuiResult update_region_err = {GuiResultAction_OK, ""};
    static GuiResult read_mem_err = {GuiResultAction_OK, ""};
    static GuiResult write_mem_err = {GuiResultAction_OK, ""};
    if (current_pid != pid) {
        current_pid = pid;
        open_process_err = update_process(true, true, true, true);
        if (open_process_err.action & GuiResultAction_CloseRemoteProcess) {
            return open_process_err;
        }
    }

    double current_time = ImGui::GetTime();
    static double last_update_time_region = 0.0;
    if (current_time - last_update_time_region > 0.5) {
        last_update_time_region = current_time;
        update_region_err = update_process(false, true, false, false);
        if (update_region_err.action & GuiResultAction_CloseRemoteProcess) {
            return update_region_err;
        }
    }

    static double last_update_time_view = 0.0;
    if (current_time - last_update_time_view > 0.05) {
        last_update_time_view = current_time;
        read_mem_err = update_process(false, false, false, true);
        if (read_mem_err.action & GuiResultAction_CloseRemoteProcess) {
            return read_mem_err;
        }
    }

    // combo to select data types
    ImGui::PushItemWidth(ImGui::CalcTextSize("f64|").x + ImGui::GetStyle().FramePadding.x * 2.0f);
    static MemoryViewDisplayDataType display_data_type = MemoryViewDisplayDataType_u8;
    if (ImGui::BeginCombo("##view_data_type", MemoryViewDisplayDataTypeNames[display_data_type], ImGuiComboFlags_NoArrowButton)) {
        for (int i = 0; i < MemoryViewDisplayDataType_MAX; i++) {
            if (ImGui::Selectable(MemoryViewDisplayDataTypeNames[i])) {
                display_data_type = (MemoryViewDisplayDataType)i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    // display in hex
    static bool display_hex = true;
    ImGui::SameLine();
    ImGui::Checkbox("Hex"_x, &display_hex);
    // input text to change view addr
    static uint64_t view_addr_step = 0x10;
    static uint64_t view_addr_step_fast = 0x1000;
    ImGui::SameLine();
    ImGui::InputScalar("Memory Address"_x, ImGuiDataType_U64, &view_addr, &view_addr_step, &view_addr_step_fast, "%016lX", ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsHexadecimal);

    for (auto* err : {&open_process_err, &update_region_err, &read_mem_err, &write_mem_err}) {
        if (err->action != GuiResultAction_OK) {
            if (err->action & GuiResultAction_Warn) {
                if (ImGui::Button("✖️")) {
                    err->action = GuiResultAction_OK;
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                ImGui::Text("%s: %s", "⚠️ Warning"_x, err->message.c_str());
                ImGui::PopStyleColor();
            } else if (err->action & GuiResultAction_Error) {
                if (ImGui::Button("✖️")) {
                    err->action = GuiResultAction_OK;
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("%s: %s", "⛔ Error"_x, err->message.c_str());
                ImGui::PopStyleColor();
            } else {
                assert(false);
            }
        }
    }

    // render data
    ImGui::PushFont(ctx->hex_font);
    const uint8_t* remote_data = mem_view->data().data();
    std::string module_name = std::format("{:#0X}", view_addr);
    if (auto region_it = mem_regions->search(view_addr); region_it != mem_regions->end()) {
        module_name = std::format("{}({:#0X})+{:#0X}", region_it->name, region_it->start, view_addr - region_it->start);
    }
    ImGui::SeparatorText(module_name.c_str());

    ImGui::BeginChild("##mem_view", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::IsWindowHovered()) {
        if (ctx->io->MouseWheel > 0) {
            view_addr -= 0x10;
        } else if (ctx->io->MouseWheel < 0) {
            view_addr += 0x10;
        }
    }
    int cell_width = display_hex ? MemoryViewDisplayDataTypeStrWidthHex[display_data_type] : MemoryViewDisplayDataTypeStrWidth[display_data_type];
    ImGui::Text(std::format("{:>18s}", "|").c_str());
    ImGui::SameLine();
    for (int i = 0; i < view_width; i += MemoryViewDisplayDataTypeWidth[display_data_type]) {
        std::string header_cell_text = std::format("{:>0{}X}", i + view_addr % view_width, cell_width);
        ImGui::TextUnformatted(header_cell_text.c_str());
        if (i != view_width - MemoryViewDisplayDataTypeWidth[display_data_type]) {
            ImGui::SameLine();
        }
    }
    ImGui::Separator();
    for (int i = 0; i < view_height; i++) {
        ImGui::Text("%016lX |", view_addr + i * view_width);
        ImGui::SameLine();
        for (int j = 0; j < view_width; j += MemoryViewDisplayDataTypeWidth[display_data_type]) {
            int idx = i * view_width + j;
            uint64_t addr = view_addr + idx;
            std::string data_str;
            if (idx >= mem_view->data().size()) {
                // invalid data
                data_str = std::format("{:>{}s}", "??", cell_width);
                ImGui::TextUnformatted(data_str.c_str());
                if (j != view_width - MemoryViewDisplayDataTypeWidth[display_data_type]) {
                    ImGui::SameLine();
                }
                continue;
            }

            // branch to editable memory
            data_str = mem_view_data_to_str(remote_data + idx, display_hex, display_data_type);

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
                    std::vector<uint8_t> parsed_data = parse_mem_data(edit_data_str, display_hex, display_data_type);
                    if (parsed_data.size() > 0 && memcmp(parsed_data.data(), remote_data + idx, parsed_data.size()) != 0) {
                        if (mem_view->write(addr, parsed_data) != parsed_data.size()) {
                            write_mem_err = {GuiResultAction_Error, std::format("{}: {} {:#X}", "Failed to write memory of process"_x, std::strerror(errno), addr)};
                        } else {
                            write_mem_err = {GuiResultAction_OK, ""};
                        }
                    } else {
                        write_mem_err = {GuiResultAction_Error, std::format("{}: {}. errno: {}", "Invalid input"_x, edit_data_str, std::strerror(errno))};
                    }
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

            if (j != view_width - MemoryViewDisplayDataTypeWidth[display_data_type]) {
                ImGui::SameLine();
            }
        }
    }
    ImGui::EndChild();

    ImGui::PopFont();
    return {GuiResultAction_OK, ""};
}

static void cross_edit_show_memory_regions_imgui(ImguiRuntimeContext* ctx, bool& enabled)
{
    if (!enabled) {
        return;
    }
    if (!ctx->mem_regions ) {
        enabled = false;
        return;
    }
    if (ImGui::Begin("Memory Regions"_x, &enabled), ImGuiWindowFlags_AlwaysAutoResize) {
        if (ImGui::BeginTable("##Memory Regions List", 6,
                                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)) {
            ImGui::TableSetupColumn("Start Address"_x);
            ImGui::TableSetupColumn("End Address"_x);
            ImGui::TableSetupColumn("Size"_x);
            ImGui::TableSetupColumn("Protection"_x);
            ImGui::TableSetupColumn("File Offset"_x);
            ImGui::TableSetupColumn("Name"_x);
            ImGui::TableHeadersRow();

            for (const auto& region : *ctx->mem_regions) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%#0lX", region.start);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%#0lX", region.start + region.size);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%lX", region.size);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", to_string(region.prot).c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%#0lX", region.file_offset);
                ImGui::TableSetColumnIndex(5);
                ImGui::Selectable(region.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                std::string text_id = std::format("##Memory Region {} {}", region.start, region.size);
                if (ImGui::BeginPopupContextItem(text_id.c_str())) {
                    if (ImGui::Selectable("Show in Memory Viewer"_x)) {
                        ctx->view_addr = region.start;
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
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

static void cross_edit_imgui(ImguiRuntimeContext* ctx)
{
    // ImGuiViewport* viewport = ImGui::GetMainViewport();
    // ImGui::SetNextWindowPos(viewport->Pos);
    // ImGui::SetNextWindowSize(viewport->Size);
    // ImGui::Begin("CheatNG", NULL, ImGuiWindowFlags_NoDecoration |
    // ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("CheatNG")) {
        //// process status
        static int selected_pid = -1; // for table selection. not the final pid we are opening
        static int selected_pid_next = -1;
        static int opened_pid = -1;

        //// buttons
        if (ImGui::Button("🖥️")) {
            ImGui::OpenPopup("Choose Process"_x);
        }
        ImGui::SameLine();

        static bool show_memory_regions = false;
        if (opened_pid >= 0) {
            if (ImGui::Button("Ⓜ️")) {
                show_memory_regions = true;
            }
            cross_edit_show_memory_regions_imgui(ctx, show_memory_regions);
        }
        //// choose process model
        static std::set<int> interested_pids;
        static bool show_kthread = false;
        static std::string search_buf;
        bool model_open = true;
        // ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal("Choose Process"_x, &model_open, ImGuiWindowFlags_None)) {
            ImGui::InputTextWithHint("##Search Process", "🔎 Search Process"_x, &search_buf);
            ImGui::SameLine();
            ImGui::Checkbox("Show Kernel Threads"_x, &show_kthread);
            if (ImGui::BeginTable("##Process List", 2,
                                  ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("PID"_x);
                ImGui::TableSetupColumn("Command lines"_x);
                ImGui::TableHeadersRow();

                static auto processes = Processes();
                processes.build_tree();
                static double last_update_time = 0.0;
                double current_time = ImGui::GetTime();
                if (current_time - last_update_time > 1.0) {
                    last_update_time = current_time;
                    processes.update();
                    processes.build_tree();
                }
                for (const auto& proc : processes) {
                    std::string display_name = proc.cmdlines_str();
                    if (!show_kthread && display_name.empty()) {
                        continue;
                    }
                    if (display_name.empty()) {
                        display_name = std::format("[{}]", proc.name());
                    }
                    if (search_buf.size() > 0 && display_name.find(search_buf) == std::string::npos) {
                        // don't filter out pid we are interersted in
                        if (!interested_pids.contains(proc.id())) {
                            continue;
                        }
                    }
                    std::string line = std::format("{}##[{}]", proc.id(), display_name);
                    ImGui::PushID(proc.id());
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", proc.id());
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Selectable(display_name.c_str(), selected_pid == proc.id(), ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAllColumns)) {
                        selected_pid = proc.id();
                    }
                    ImGui::PopID();
                    if (selected_pid_next == proc.id()) {
                        selected_pid = proc.id();
                        selected_pid_next = -1;
                        ImGui::SetScrollHereY(0.5);
                    }

                    if (ImGui::BeginPopupContextItem()) {
                        selected_pid = proc.id();
                        std::string jump_to_parent = std::format("{} [{}] {}", "Jump To Parent"_x, proc.parent_id(), Process(proc.parent_id()).cmdlines_str());
                        if (ImGui::SelectableWrrapped(jump_to_parent.c_str())) {
                            selected_pid_next = proc.parent_id(); // scroll in next frame
                            interested_pids.insert(selected_pid_next);
                        }
                        if (proc.children().size() > 0) {
                            if (ImGui::BeginMenu("Jump To Children"_x)) {
                                for (const auto& child : proc.children()) {
                                    std::string jump_to_child = std::format("[{}] {}", child->id(), child->cmdlines_str());
                                    if (ImGui::SelectableWrrapped(jump_to_child.c_str())) {
                                        selected_pid_next = child->id(); // scroll in next frame
                                        interested_pids.insert(selected_pid_next);
                                    }
                                }
                                ImGui::EndMenu();
                            }
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered()) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            opened_pid = proc.id();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndPopup();
        } else {
            // any time close popup, clear interested pids
            interested_pids.clear();
        }

        if (!model_open) {
            // only when click close button of popup
            selected_pid = -1;
        }

        //// memory view
        bool popup_msg_open = true;
        static GuiResult gui_result{GuiResultAction_OK, ""};
        if (opened_pid && opened_pid >= 0) {
            // open memory view
            if (!(gui_result.action & GuiResultAction_CloseRemoteProcess)) {
                gui_result = cross_edit_select_process(ctx, opened_pid);
            }

            if (gui_result.action & GuiResultAction_CloseRemoteProcess) {
                const char* popup_title = (gui_result.action & GuiResultAction_Warn) ? "⚠️ Warning"_x : "⛔ Error"_x;
                if (ImGui::BeginPopupModal(popup_title, &popup_msg_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
                    ImGui::Text("%s", gui_result.message.c_str());
                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 10);
                    if (ImGui::Button("OK"_x)) {
                        popup_msg_open = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                ImGui::OpenPopup(popup_title);
            }

            if (!popup_msg_open && (gui_result.action & GuiResultAction_CloseRemoteProcess)) {
                opened_pid = -1;
                selected_pid = -1;
                gui_result.action = GuiResultAction_OK;
                ImGui::OpenPopup("Choose Process"_x);
            }
        }
    }
    ImGui::End();
}

int gui_imgui(ImguiRuntimeContext* ctx)
{
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
                          (float*)&ctx->clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets
                                     // return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ctx->io->Framerate, ctx->io->Framerate);
        ImGui::End();
    }

    // 2. Show the big demo window (Most of the sample code is in
    // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
    // ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    cross_edit_imgui(ctx);
    return 0;
}

#pragma clang diagnostic pop
