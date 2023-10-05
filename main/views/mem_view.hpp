#pragma once

#include "mem.hpp"
#include <memory>

class MemoryView
{
public:
    MemoryView() : _view_start(0), _view_size(0) {};

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
