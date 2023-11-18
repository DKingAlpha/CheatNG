#pragma once

#include "proc.hpp"
#include "mem.hpp"
#include "rpc.hpp"
#include "remote.hpp"
#include "imp/linux/proc_usermode_proc.hpp"
#include "imp/linux/mem_usermode_proc.hpp"


class Factory
{
private:
    bool rpc_mode;
    std::unique_ptr<RPCClient> rpc_client; 

public:
    Factory(bool rpc_mode, TargetConfig config) : rpc_mode(rpc_mode) {
        if (rpc_mode) {
            rpc_client = std::make_unique<RPCClient>(config);
        }
    }

    template <typename... Args>
    std::unique_ptr<IThread> create(ThreadImpType imp, Args... args)
    {
        if (rpc_mode) {
            return std::make_unique<RemoteThread>(rpc_client.get(), std::forward<ThreadImpType>(imp), std::forward<Args>(args) ...);
        }
        switch (imp)
        {
        case ThreadImpType::LINUX_USERMODE_PROC:
            return std::make_unique<ThreadImp_LinuxUserMode>(std::forward<Args>(args)...);
        default:
            return {};
        }
    }

    template <typename... Args>
    std::unique_ptr<IProcess> create(ProcessImpType imp, Args... args)
    {
        if (rpc_mode) {
            return std::make_unique<RemoteProcess>(rpc_client.get(), std::forward<ProcessImpType>(imp), std::forward<Args>(args) ...);
        }
        switch (imp)
        {
        case ProcessImpType::LINUX_USERMODE_PROC:
            return std::make_unique<ProcessImp_LinuxUserMode>(std::forward<Args>(args)...);
        default:
            return {};
        }
    }

    template <typename... Args>
    std::unique_ptr<IProcesses> create(ProcessesImpType imp, Args... args)
    {
        if (rpc_mode) {
            return std::make_unique<RemoteProcesses>(rpc_client.get(), std::forward<ProcessesImpType>(imp), std::forward<Args>(args) ...);
        }
        switch (imp)
        {
        case ProcessesImpType::LINUX_USERMODE_PROC:
            return std::make_unique<ProcessesImp_LinuxUserMode>(std::forward<Args>(args)...);
        default:
            return {};
        }
    }

    template <typename... Args>
    std::unique_ptr<IMemory> create(MemoryImpType imp, Args... args)
    {
        if (rpc_mode) {
            return std::make_unique<RemoteMemory>(rpc_client.get(), std::forward<MemoryImpType>(imp), std::forward<Args>(args) ...);
        }
        switch (imp)
        {
        case MemoryImpType::LINUX_USERMODE_PROC:
            return std::make_unique<MemoryImp_LinuxUserMode>(std::forward<Args>(args)...);
        default:
            return {};
        }
    }
};
