#include "imp/linux/mem_usermode_proc.hpp"
#include "proc.hpp"
#include <fstream>
#include <sstream>
#include <sys/uio.h>

ssize_t MemoryImp_LinuxUserMode::read(uint64_t addr, size_t size, std::vector<uint8_t>& data) const
{
    data.resize(size);
    struct iovec local[1];
    struct iovec remote[1];
    local[0].iov_base = data.data();
    local[0].iov_len = size;
    remote[0].iov_base = (void*)addr;
    remote[0].iov_len = size;
    ssize_t count = process_vm_readv(_pid, local, 1, remote, 1, 0);
    if (count < 0) {
        data.clear();
    } else {
        data.resize(count);
    }
    return count;
}

ssize_t MemoryImp_LinuxUserMode::write(uint64_t addr, std::vector<uint8_t>& data) const
{
    struct iovec local[1];
    struct iovec remote[1];
    local[0].iov_base = data.data();
    local[0].iov_len = data.size();
    remote[0].iov_base = (void*)addr;
    remote[0].iov_len = data.size();
    ssize_t count = process_vm_writev(_pid, local, 1, remote, 1, 0);
    return count;
}

struct linux_maps_line
{
    uint64_t start;
    uint64_t end;
    std::string prot;
    uint64_t offset;
    uint32_t major;
    uint32_t minor;
    uint64_t inode;
    std::string name;

    linux_maps_line(std::string line)
    {
        // parse line
        std::stringstream ss(line);
        std::string buf;
        std::getline(ss, buf, '-');
        start = std::stoull(buf, nullptr, 16);
        std::getline(ss, buf, ' ');
        end = std::stoull(buf, nullptr, 16);
        std::getline(ss, prot, ' ');
        std::getline(ss, buf, ' ');
        offset = std::stoull(buf, nullptr, 16);
        std::getline(ss, buf, ':');
        major = std::stoul(buf);
        std::getline(ss, buf, ' ');
        minor = std::stoul(buf);
        std::getline(ss, buf, ' ');
        inode = std::stoull(buf);
        // skip spaces
        while (!ss.eof()) {
            if (ss.peek() == ' ') {
                ss.get();
            } else {
                break;
            }
        }
        std::getline(ss, name);
    }
};

MemoryRegions MemoryImp_LinuxUserMode::regions() const
{
    MemoryRegions regions;
    std::ifstream maps("/proc/" + std::to_string(_pid) + "/maps");
    while (maps) {
        std::string line;
        std::getline(maps, line);
        if (line.empty()) {
            break;
        }
        MemoryRegion region;
        linux_maps_line parsed_line(line);
        region.start = parsed_line.start;
        region.size = parsed_line.end - parsed_line.start;
        region.file_offset = parsed_line.offset;
        region.prot = MemoryProtectionFlags::NONE;
        if (parsed_line.prot.size() > 0 && parsed_line.prot[0] == 'r') {
            region.prot |= MemoryProtectionFlags::READ;
        }
        if (parsed_line.prot.size() > 1 && parsed_line.prot[1] == 'w') {
            region.prot |= MemoryProtectionFlags::WRITE;
        }
        if (parsed_line.prot.size() > 2 && parsed_line.prot[2] == 'x') {
            region.prot |= MemoryProtectionFlags::EXECUTE;
        }
        if (parsed_line.prot.size() > 3 && parsed_line.prot[3] == 'p') {
            region.prot |= MemoryProtectionFlags::PRIVATE;
        } else {
            region.prot |= MemoryProtectionFlags::SHARED;
        }
        region.name = parsed_line.name;
        regions.push_back(region);
    }
    return regions;
}