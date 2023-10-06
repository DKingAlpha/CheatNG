#pragma once

#include <vector>
#include <string>
#include <stdint.h>

class IValidBoolOp
{
public:
    virtual operator bool() const { return is_valid(); }
    virtual bool is_valid() const = 0;
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
    MemoryViewDisplayDataType_cstr,
    MemoryViewDisplayDataType_MAX,
};

std::string data_type_name(MemoryViewDisplayDataType dt);
std::string raw_to_str_expr(const uint8_t* ptr, bool hex, MemoryViewDisplayDataType dt);
std::vector<uint8_t> str_expr_to_raw(std::string& data, bool hex, MemoryViewDisplayDataType dt);
