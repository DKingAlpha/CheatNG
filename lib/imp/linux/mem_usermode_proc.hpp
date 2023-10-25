#pragma once

#include "native.hpp"

class MemoryImp_LinuxUserMode : public INativeMemory
{
public:
    MemoryImp_LinuxUserMode(int pid) : INativeMemory(pid) {}

    virtual MemoryRegions regions() const override;
    virtual ssize_t read(uint64_t addr, size_t size, std::vector<uint8_t>& data) const override;
    virtual ssize_t write(uint64_t addr, std::vector<uint8_t>& data) const override;
};