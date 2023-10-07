#pragma once

#include <stdint.h>
#include <string_view>

using namespace std::literals::string_view_literals;

// inline static constexpr std::map<std::string_view, std::string_view>&
// current_translation_map_zh = translation_map_zh;

enum class Language : uint32_t
{
    EN = 0, // programming text
    CHS = 1,
    MAX,
};

inline Language current_language = Language::CHS;

struct TranslationLine
{
    const char* translations[(int)Language::MAX];
};

inline constexpr TranslationLine translations_table[] = {
    {"🔎 Search Process", "🔎 搜索进程"},
    {"🔎 Search Module", "🔎 搜索模块"},
    {"🔎 Search Memory", "🔎 搜索内存"},
    {"Choose Process", "选择进程"},
    {"Search Memory", "搜索内存"},
    {"Edit Memory", "编辑内存"},
    {"Settings", "设置"},
    {"Process List", "进程列表"},
    {"Show Kernel Threads", "显示内核线程"},
    {"Parent Process", "父进程"},
    {"Jump To Parent", "跳转到父进程"},
    {"🔴 Log", "🔴 日志"},
    {"⛔ Error", "⛔ 错误"},
    {"⚠️ Warning", "⚠️ 警告"},
    {"Invalid pid", "PID无效"},
    {"Failed to read memory regions of process", "读取进程内存区域失败"},
    {"Failed to read memory of process", "读取进程内存失败"},
    {"Failed to write memory of process", "写入进程内存失败"},
    {"Jump To Children", "跳转到子进程"},
    {"OK", "确定"},
    {"Delete", "删除"},
    {"Clear", "清除"},
    {"PID", "进程ID"},
    {"Total", "总数"},
    {"Command lines", "命令行"},
    {"Memory Address", "内存地址"},
    {"Memory Regions", "内存布局"},
    {"Show in Memory Viewer", "在内存视图中显示"},
    {"Start Address", "起始地址"},
    {"End Address", "结束地址"},
    {"Size", "大小"},
    {"Protection", "保护属性"},
    {"File Offset", "文件偏移"},
    {"Name", "名称"},
    {"Copy Start Address", "复制起始地址"},
    {"Copy Name", "复制名称"},
    {"Bytes Per Row", "每行宽度"},
    {"Row Count", "行数"},
    {"Clear Results", "清除结果"},
    {"Task", "任务"},
    {"New Search", "新建搜索"},
    {"New Search Name", "新建搜索名称"},
    {"Start Search", "开始搜索"},
    {"Continue Search", "继续搜索"},
    {"Stop Search", "停止搜索"},
    {"Search Result", "搜索结果"},
    {"Search Type", "搜索类型"},
    {"Search Value", "搜索值"},
    {"Search Pattern", "搜索模板"},
    {"Search Range", "搜索范围"},
    {"Search Hex", "搜索16进制"},
    {"Show Hex", "显示16进制"},
};

inline constexpr const char* const* get_translation_line(const char* str, std::size_t len)
{
    for (size_t i = 0; i < sizeof(translations_table) / sizeof(translations_table[0]); i++) {
        if (__builtin_strncmp(translations_table[i].translations[0], str, len) == 0) {
            return translations_table[i].translations;
        }
    }
    return nullptr;
}

inline const char* operator"" _x(const char* str, std::size_t len)
{
    const char* const* line = get_translation_line(str, len);
    if (line) {
        return line[(int)current_language];
    } else {
        return str;
    }
}