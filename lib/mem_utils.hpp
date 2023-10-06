#pragma once

#include "mem.hpp"
#include <memory>
#include <functional>

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

    class Pattern
    {
    public:

    };

    std::vector<uint64_t> search(const std::unique_ptr<IMemory>& mem, const std::string pattern, uint64_t start, uint64_t end, FoundCallback on_found) const
    {
        std::vector<uint64_t> ret;
        for(auto& region : mem->regions()) {
            if (region.start < start || region.start + region.size > end) {
                continue;
            }
            MemoryView view;
            view.set_range(region.start, region.size);
            if (!view.update(mem)) {
                continue;
            }
            auto& data = view.data();
            for (size_t i = 0; i < data.size() - pattern.size(); i++) {
                if (memcmp(&data[i], pattern.c_str(), pattern.size()) == 0) {
                    if (on_found(region.start + i)) {
                        ret.push_back(region.start + i);
                    }
                }
            }
        }
        return ret;
    }
};