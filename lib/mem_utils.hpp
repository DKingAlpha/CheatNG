#pragma once

#include "base.hpp"
#include "mem.hpp"
#include <memory>
#include <functional>
#include <assert.h>

class MemoryView
{
public:
    MemoryView() : _view_start(0), _view_size(0) {}

    void set_range(uint64_t start, uint64_t size)
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

class SearchMemory
{
public:
    using FoundCallback = std::function<bool(uint64_t)>;

    class Pattern : public IValidBoolOp
    {
    public:
        Pattern(const std::string& str_expr, MemoryViewDisplayDataType data_type) : data_type_(data_type) {
            if (data_type < MemoryViewDisplayDataType_BASE_MAX) {
                raw = str_expr_to_raw(str_expr, true, data_type);
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

    std::vector<uint64_t> search(const std::unique_ptr<IMemory>& mem, const std::string pattern_str, MemoryViewDisplayDataType data_type,
        uint64_t start, uint64_t end, FoundCallback on_found) const
    {
        std::vector<uint64_t> ret;
        Pattern pattern(pattern_str, data_type);
        if (!pattern) {
            return ret;
        }
        size_t pattern_size = pattern.data_size();
        size_t search_alignment = pattern.data_size();
        for(auto& region : mem->regions()) {
            if (region.start < start || region.start + region.size > end) {
                continue;
            }
            MemoryView view;
            view.set_range(region.start, region.size);
            if (!view.update(mem)) {
                continue;
            }
            
            auto start_addr = view.start();
            auto offset = start_addr % search_alignment;
            if (offset != 0) {
                offset = search_alignment - offset;
            }

            auto& data = view.data();
            for (size_t i=offset; i <= data.size() - pattern_size; i+= search_alignment) {
                if (pattern.is_match(data.data() + i)) {
                    uint64_t addr = region.start + i;
                    if (on_found(addr)) {
                        ret.push_back(addr);
                    }
                }
            }
        }
        return ret;
    }
};