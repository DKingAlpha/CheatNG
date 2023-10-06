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
    MemoryViewDisplayDataType_BASE_MAX,
    // special types. raw <-> str_expr does not handle these types
    MemoryViewDisplayDataType_cstr,     // c-string ending with '\0'
    MemoryViewDisplayDataType_aob,      // hex string with space-seperated wildcard '?' or "??" as a byte
    MemoryViewDisplayDataType_SPECIAL_MAX,
};

std::string data_type_name(MemoryViewDisplayDataType dt);
int data_type_size(MemoryViewDisplayDataType dt);
std::string raw_to_str_expr(const uint8_t* ptr, bool hex, MemoryViewDisplayDataType dt);
std::vector<uint8_t> str_expr_to_raw(const std::string& data, bool hex, MemoryViewDisplayDataType dt);

bool encode_aob_pattern(std::string pattern /* IN */, std::vector<uint8_t>& raw /* OUT */, std::vector<bool>& wildcard_mask /* OUT */);
bool decode_aob_pattern(std::string& pattern /* OUT */, const std::vector<uint8_t>& raw /* IN */, const std::vector<bool>& wildcard_mask /* IN */);
