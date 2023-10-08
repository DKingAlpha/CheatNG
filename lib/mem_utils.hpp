#pragma once

#include "base.hpp"
#include "mem.hpp"
#include <assert.h>
#include <functional>
#include <map>
#include <memory>

class MemoryViewRange
{
public:
    MemoryViewRange() : _view_start(0), _view_size(0) {}

    void set(uint64_t start, uint64_t size)
    {
        _view_start = start;
        _view_size = size;
    }
    bool update(const std::unique_ptr<IMemory>& mem)
    {
        if (_view_size == 0) {
            _view_data.clear();
            return false;
        }
        return mem->read(_view_start, _view_size, _view_data) >= 0;
    }

    const std::vector<uint8_t>& data() const { return _view_data; }
    uint64_t start() const { return _view_start; }
    uint64_t size() const { return _view_size; }

private:
    uint64_t _view_start;
    uint64_t _view_size;
    std::vector<uint8_t> _view_data;
};

class MemoryViewCache
{
    struct CacheData
    {
        bool valid;
        double last_update_time;
        std::vector<uint8_t> data;
        // user input
        std::string name;
        MemoryViewDisplayDataType data_type;
    };

public:
    MemoryViewCache() : default_data_type(MemoryViewDisplayDataType_s32), refresh_time(0.2) {}
    MemoryViewCache(int data_size, double refresh_time = 1.0) : default_data_type(MemoryViewDisplayDataType_s32), refresh_time(refresh_time) {}

    void clear() { data.clear(); }

    void update_data_type(MemoryViewDisplayDataType data_type)
    {
        if (default_data_type != data_type) {
            data.clear();
            default_data_type = data_type;
        }
    }

    void add(std::vector<uint64_t>& addrs)
    {
        for (size_t i = 0; i < addrs.size(); i++) {
            data[addrs[i]] = {false, 0.0, {}};
        }
    }

    CacheData& get(const std::unique_ptr<IMemory>& mem, uint64_t addr, double current_time)
    {
        auto it = data.find(addr);
        if (it == data.end() || current_time - it->second.last_update_time > refresh_time) {
            std::string name = it == data.end() ? "" : it->second.name;
            MemoryViewDisplayDataType data_type = it == data.end() ? default_data_type : it->second.data_type;
            int data_size = data_type_size(data_type);
            std::vector<uint8_t> buf;
            bool valid = mem->read(addr, data_size, buf) == data_size;
            data[addr] = {valid, current_time, buf, name, data_type};
            return data[addr];
        } else {
            CacheData& retval = it->second;
            return retval;
        }
    }

    double refresh_time;
    MemoryViewDisplayDataType default_data_type;
    std::map<uint64_t, CacheData> data;
};

class SearchMemory
{
public:
    using FoundCallback = std::function<bool(uint64_t)>;

    class Pattern : public IValidBoolOp
    {
    public:
        Pattern(const std::string& str_expr, MemoryViewDisplayDataType data_type, bool hex) : data_type_(data_type)
        {
            if (data_type < MemoryViewDisplayDataType_BASE_MAX) {
                raw = str_expr_to_raw(str_expr, hex, data_type);
            } else if (data_type == MemoryViewDisplayDataType_cstr) {
                raw.resize(str_expr.size());
                memcpy(raw.data(), str_expr.data(), str_expr.size());
            } else if (data_type == MemoryViewDisplayDataType_aob) {
                encode_aob_pattern(str_expr, raw, wildcard);
            } else {
                assert(false);
            }
        }
        bool is_match(const uint8_t* data) const
        {
            if (data_type_ < MemoryViewDisplayDataType_BASE_MAX || data_type_ == MemoryViewDisplayDataType_cstr) {
                return memcmp(data, raw.data(), raw.size()) == 0;
            } else if (data_type_ == MemoryViewDisplayDataType_aob) {
                for (size_t i = 0; i < raw.size(); i++) {
                    if (wildcard[i]) {
                        continue;
                    }
                    if (data[i] != raw[i]) {
                        return false;
                    }
                }
                return true;
            } else {
                assert(false);
                return false;
            }
        }

        size_t data_size() const { return raw.size(); }
        MemoryViewDisplayDataType data_type() const { return data_type_; }
        bool is_valid() const override { return data_size() > 0; }

    private:
        MemoryViewDisplayDataType data_type_;
        std::vector<uint8_t> raw;
        std::vector<bool> wildcard;
    };

    static bool begin_search(const std::unique_ptr<IMemory>& mem, bool& stay_running, bool hex, const std::string pattern_str, MemoryViewDisplayDataType data_type, uint64_t start, uint64_t end, FoundCallback on_found)
    {
        Pattern pattern(pattern_str, data_type, hex);
        if (!pattern) {
            return false;
        }
        size_t pattern_size = pattern.data_size();
        size_t search_alignment = data_type_size(pattern.data_type());
        for (auto& region : mem->regions()) {
            if (!stay_running) {
                break;
            }
            if (region.start < start || region.start + region.size > end) {
                continue;
            }
            MemoryViewRange view;
            view.set(region.start, region.size);
            if (!view.update(mem)) {
                continue;
            }

            auto start_addr = view.start();
            auto offset = start_addr % search_alignment;
            if (offset != 0) {
                offset = search_alignment - offset;
            }

            auto& data = view.data();
            for (size_t i = offset; i <= data.size() - pattern_size; i += search_alignment) {
                if (!stay_running) {
                    break;
                }
                if (pattern.is_match(data.data() + i)) {
                    uint64_t addr = region.start + i;
                    if (!on_found(addr)) {
                        return true;
                    }
                }
            }
        }
        return true;
    }

    static bool continue_search(const std::unique_ptr<IMemory>& mem, bool& stay_running, bool hex, const std::string pattern_str, MemoryViewDisplayDataType data_type, const std::vector<uint64_t>& search_in_addresses, FoundCallback on_found)
    {
        Pattern pattern(pattern_str, data_type, hex);
        if (!pattern) {
            return false;
        }
        size_t pattern_size = pattern.data_size();
        size_t search_alignment = data_type_size(pattern.data_type());
        for (uint64_t addr : search_in_addresses) {
            if (!stay_running) {
                break;
            }
            std::vector<uint8_t> buf;
            ssize_t read_bytes = mem->read(addr, pattern_size, buf);
            if (read_bytes != pattern_size) {
                continue;
            }
            if (pattern.is_match(buf.data())) {
                if (!on_found(addr)) {
                    return true;
                }
            }
        }
        return true;
    }
};