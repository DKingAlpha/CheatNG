#pragma once

#include "mem.hpp"

class MemoryImp_LinuxUserMode : public IMemory
{
public:
    MemoryImp_LinuxUserMode(int pid) : _pid(pid) {}

    virtual MemoryRegions regions() override;
    virtual ssize_t read(uint64_t addr, size_t size, std::vector<uint8_t>& data) override;
    virtual ssize_t write(uint64_t addr, std::vector<uint8_t>& data) override;

private:
    int _pid;
};