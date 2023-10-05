#pragma once

#include <stdint.h>
#include <string>
#include <vector>

enum class MemoryProtectionFlags : uint32_t
{
    NONE = 0,
    READ = 1 << 0,
    WRITE = 1 << 1,
    EXECUTE = 1 << 2,
    SHARED = 1 << 3,
    PRIVATE = 1 << 4,
    STACK = 1 << 5,
    HEAP = 1 << 6,
    OTHER = 1 << 7,
};

inline bool operator&(MemoryProtectionFlags lhs, MemoryProtectionFlags rhs) { return (uint32_t)lhs & (uint32_t)rhs; }

inline MemoryProtectionFlags operator|(MemoryProtectionFlags lhs, MemoryProtectionFlags rhs)
{
    // mutual exclusive
    // overwrite existing private/shared flag
    if (rhs & MemoryProtectionFlags::PRIVATE) {
        // clear SHARED flag in lhs
        lhs = (MemoryProtectionFlags)((uint32_t)lhs & ~(uint32_t)MemoryProtectionFlags::SHARED);
    }
    if (rhs & MemoryProtectionFlags::SHARED) {
        // clear PRIVATE flag in lhs
        lhs = (MemoryProtectionFlags)((uint32_t)lhs & ~(uint32_t)MemoryProtectionFlags::PRIVATE);
    }
    // overwrite existing stack/heap flag
    if (rhs & MemoryProtectionFlags::STACK) {
        // clear HEAP flag in lhs
        lhs = (MemoryProtectionFlags)((uint32_t)lhs & ~(uint32_t)MemoryProtectionFlags::HEAP);
    }
    if (rhs & MemoryProtectionFlags::HEAP) {
        // clear STACK flag in lhs
        lhs = (MemoryProtectionFlags)((uint32_t)lhs & ~(uint32_t)MemoryProtectionFlags::STACK);
    }
    return (MemoryProtectionFlags)((uint32_t)lhs | (uint32_t)rhs);
}

inline MemoryProtectionFlags& operator|=(MemoryProtectionFlags& lhs, MemoryProtectionFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline std::string to_string(MemoryProtectionFlags prot)
{
    char buf[6];
    for (int i=0; i<5; i++) {
        buf[i] = '-';
    }
    buf[5] = '\0';
    if (prot & MemoryProtectionFlags::READ) {
        buf[0] = 'r';
    }
    if (prot & MemoryProtectionFlags::WRITE) {
        buf[1] = 'w';
    }
    if (prot & MemoryProtectionFlags::EXECUTE) {
        buf[2] = 'x';
    }
    if (prot & MemoryProtectionFlags::SHARED) {
        buf[3] = 's';
    }
    if (prot & MemoryProtectionFlags::PRIVATE) {
        buf[3] = 'p';
    }
    if (prot & MemoryProtectionFlags::STACK) {
        buf[4] = 'k';
    }
    if (prot & MemoryProtectionFlags::HEAP) {
        buf[4] = 'h';
    }
    return buf;
}

struct MemoryRegion
{
    uint64_t start;
    uint64_t size;
    uint64_t file_offset;
    MemoryProtectionFlags prot;
    std::string name;
};

class MemoryRegions : public std::vector<MemoryRegion>
{
public:
    iterator search(uint64_t addr)
    {
        auto all_results = search(addr, false);
        if (all_results.empty()) {
            return this->end();
        }
        return all_results[0];
    }
    iterator search(std::string name_keyword)
    {
        auto all_results = search(name_keyword, false);
        if (all_results.empty()) {
            return this->end();
        }
        return all_results[0];
    }
    std::vector<iterator> search_all(uint64_t addr) { return search(addr, true); }
    std::vector<iterator> search_all(std::string name_keyword) { return search(name_keyword, true); }

private:
    std::vector<iterator> search(uint64_t addr, bool all)
    {
        std::vector<iterator> ret;
        for (iterator it = this->begin(); it != this->end(); it++) {
            if (it->start <= addr && addr < it->start + it->size) {
                ret.push_back(it);
                if (!all) {
                    break;
                }
            }
        }
        return ret;
    }
    std::vector<iterator> search(std::string name_keyword, bool all)
    {
        std::vector<iterator> ret;
        for (iterator it = this->begin(); it != this->end(); it++) {
            if (it->name.find(name_keyword) != std::string::npos) {
                ret.push_back(it);
                if (!all) {
                    break;
                }
            }
        }
        return ret;
    }
};

class IProcess;

class Memory
{
public:
    Memory(int pid);
    Memory(const IProcess& proc);

    ssize_t read(uint64_t addr, size_t size, std::vector<uint8_t>& data) const;
    ssize_t write(uint64_t addr, std::vector<uint8_t>& data) const;

    MemoryRegions regions() const;

protected:
    int _pid;
};
