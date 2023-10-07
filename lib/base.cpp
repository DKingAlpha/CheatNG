#include "base.hpp"

#include <assert.h>
#include <format>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

static int MemoryViewDisplayDataTypeWidth[] = {
    1, 2, 4, 8, 4, 8, 1, 2, 4, 8,
};

static const char* MemoryViewDisplayDataTypeNames[] = {"u8", "u16", "u32", "u64", "f32", "f64", "s8", "s16", "s32", "s64", "C String", "AoB"};

static const char* MemoryViewDisplayDataTypeFormat[] = {
    "{:4d}", "{:8d}", "{:16d}", "{:32d}", "{:20.10}", "{:40.20}", "{:4d}", "{:8d}", "{:16d}", "{:32d}", "{:s}", "{:s}",
};
static const char* MemoryViewDisplayDataTypeFormatHex[] = {"{:02X}", "{:04X}", "{:08X}", "{:016X}", "{:20.10}", "{:40.20}", "{:+03X}", "{:+05X}", "{:+09X}", "{:+017X}", "{:s}", "{:s}"};

static const char* MemoryViewDisplayDataTypeParseFormat[] = {
    "%hhd", "%hd", "%d", "%ld", "%f", "%lf", "%hhd", "%hd", "%d", "%ld",
};
static const char* MemoryViewDisplayDataTypeParseFormatHex[] = {
    "%hhx", "%hx", "%x", "%lx", "%f", "%lf", "%hhx", "%hx", "%x", "%lx",
};

std::string data_type_name(MemoryViewDisplayDataType dt) { return MemoryViewDisplayDataTypeNames[dt]; }

int data_type_size(MemoryViewDisplayDataType dt) { return MemoryViewDisplayDataTypeWidth[dt]; }

std::vector<uint8_t> str_expr_to_raw(const std::string& data, bool hex, MemoryViewDisplayDataType dt)
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
    } else if (dt == MemoryViewDisplayDataType_aob) {
        std::string buf;
        for (size_t i = 0; i < data.size(); i++) {
            char c = data[i];
            if (c != ' ') {
                buf += c;
                continue;
            }
            // handle buf
            if (buf.size() == 1) {
                buf = "0" + buf;
            }
            if (buf.size() != 2) {
                // invalid
                ret.clear();
                return ret;
            }
            // XX / 0? / ??
            if (buf == "0?" || buf == "??") {
                ret.push_back('\0');
            } else {
                if (std::scanf(buf.c_str(), "%hhx", &c)) {
                    ret.push_back(c);
                } else {
                    // invalid
                    ret.clear();
                    return ret;
                }
            }
            buf.clear();
        }
    } else {
        assert(false);
    }
    return ret;
}

bool encode_aob_pattern(std::string pattern, std::vector<uint8_t>& raw, std::vector<bool>& wildcard_mask)
{
    std::string ret;
    std::string mask;

    std::string buf;
    for (size_t i = 0; i < pattern.size(); i++) {
        char c = pattern[i];
        if (c != ' ') {
            buf += c;
            continue;
        }
        // handle buf
        if (buf.size() == 1) {
            buf = "0" + buf;
        }
        if (buf.size() != 2) {
            // invalid
            return false;
        }
        // XX / 0? / ??
        if (buf == "0?" || buf == "??") {
            ret.push_back('\0');
            mask.push_back(true);
        } else {
            if (std::scanf(buf.c_str(), "%hhx", &c)) {
                ret.push_back(c);
                mask.push_back(false);
            } else {
                // invalid
                return false;
            }
        }
        buf.clear();
    }
    raw = std::vector<uint8_t>(ret.begin(), ret.end());
    wildcard_mask = std::vector<bool>(mask.begin(), mask.end());
    return true;
}

bool decode_aob_pattern(std::string& pattern, const std::vector<uint8_t>& raw, const std::vector<bool>& wildcard_mask)
{
    if (wildcard_mask.size() != 0 && wildcard_mask.size() != raw.size()) {
        return false;
    }
    pattern.clear();
    for (size_t i = 0; i < raw.size(); i++) {
        char buf[4] = {0};
        char c = raw[i];
        if (wildcard_mask.size() == 0 || wildcard_mask[i] == false) {
            std::sprintf(buf, "%02X ", c);
        } else {
            std::sprintf(buf, "?? ");
        }
        pattern += buf;
    }
    if (pattern.size() > 0) {
        pattern.pop_back(); // pop last space
    }
    return true;
}

std::string raw_to_str_expr(const uint8_t* ptr, bool hex, MemoryViewDisplayDataType dt)
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
    } else {
        assert(false);
    }
    return "";
}
