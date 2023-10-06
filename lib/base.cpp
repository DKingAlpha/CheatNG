#include "base.hpp"

#include <vector>
#include <string>
#include <stdint.h>
#include <string.h>
#include <format>

static const char* MemoryViewDisplayDataTypeNames[] = {
    "u8", "u16", "u32", "u64", "f32", "f64", "s8", "s16", "s32", "s64", "C String",
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


std::string data_type_name(MemoryViewDisplayDataType dt)
{
    return MemoryViewDisplayDataTypeNames[dt];
}

std::vector<uint8_t> str_expr_to_raw(std::string& data, bool hex, MemoryViewDisplayDataType dt)
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
    } else if (dt == MemoryViewDisplayDataType_cstr) {
        ret.resize(data.size() + 1);
        memcpy(ret.data(), data.c_str(), data.size() + 1);
    }
    return ret;
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
    } else if (dt == MemoryViewDisplayDataType_cstr) {
        return std::string((const char*)ptr);
    }
    return "";
}
