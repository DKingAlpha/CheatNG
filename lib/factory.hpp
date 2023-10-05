#pragma once

#include "proc.hpp"
#include "imp/linux/proc_usermode_proc.hpp"
#include "imp/linux/mem_usermode_proc.hpp"

class Factory
{
public:
    template <typename... Args>
    static std::unique_ptr<IThread> create(ThreadImpType imp, Args... args)
    {
        switch (imp)
        {
        case ThreadImpType::LINUX_USERMODE_PROC:
            return std::make_unique<ThreadImp_LinuxUserMode>(args...);
        default:
            return {};
        }
    }

    template <typename... Args>
    static std::unique_ptr<IProcess> create(ProcessImpType imp, Args... args)
    {
        switch (imp)
        {
        case ProcessImpType::LINUX_USERMODE_PROC:
            return std::make_unique<ProcessImp_LinuxUserMode>(args...);
        default:
            return {};
        }
    }

    template <typename... Args>
    static std::unique_ptr<IProcesses> create(ProcessesImpType imp, Args... args)
    {
        switch (imp)
        {
        case ProcessesImpType::LINUX_USERMODE_PROC:
            return std::make_unique<ProcessesImp_LinuxUserMode>(args...);
        default:
            return {};
        }
    }

    template <typename... Args>
    static std::unique_ptr<IMemory> create(MemoryImpType imp, Args... args)
    {
        switch (imp)
        {
        case MemoryImpType::LINUX_USERMODE_PROC:
            return std::make_unique<MemoryImp_LinuxUserMode>(args...);
        default:
            return {};
        }
    }
};
