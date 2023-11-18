#pragma once

#include "proc.hpp"
#include "mem.hpp"
#include "rpc.hpp"

#include "imp/linux/proc_usermode_proc.hpp"
#include "imp/linux/mem_usermode_proc.hpp"

#include "cereal/types/polymorphic.hpp"
#include "cereal/archives/binary.hpp"

CEREAL_REGISTER_TYPE(ThreadImp_LinuxUserMode);
CEREAL_REGISTER_TYPE(ProcessImp_LinuxUserMode);
CEREAL_REGISTER_TYPE(ProcessesImp_LinuxUserMode);
CEREAL_REGISTER_TYPE(MemoryImp_LinuxUserMode);

CEREAL_REGISTER_POLYMORPHIC_RELATION(IThread, ThreadImp_LinuxUserMode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(IProcess, ProcessImp_LinuxUserMode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(IProcesses, ProcessesImp_LinuxUserMode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(IMemory, MemoryImp_LinuxUserMode);

// CEREAL_FORCE_DYNAMIC_INIT(ThreadImp_LinuxUserMode)
// CEREAL_FORCE_DYNAMIC_INIT(ProcessImp_LinuxUserMode)
// CEREAL_FORCE_DYNAMIC_INIT(ProcessesImp_LinuxUserMode)
// CEREAL_FORCE_DYNAMIC_INIT(MemoryImp_LinuxUserMode)


class RemoteThread : public IThread
{
private:
    RPCClient* rpc_client;
    RemoteObjectId obj;
    ThreadImpType imp;
public:
    bool ok;

    template <typename... Args>
    RemoteThread(RPCClient* rpc_client, ThreadImpType imp, Args... args) : rpc_client(rpc_client), imp(imp), obj(0), ok(true)
    {
        switch (imp)
        {
        case ThreadImpType::LINUX_USERMODE_PROC:
            ok = ok && rpc_client->call(new_remote<ThreadImp_LinuxUserMode, Args...>, obj, {args ...});
            break;
        default:
            ok = false;
        }
    }
    ~RemoteThread()
    {
        if (!obj) {
            return;
        }
        rpc_client->call(delete_remote<IThread>, ok, {obj});
    }

    virtual bool is_valid() override
    {
        bool ret;
        ok = ok && rpc_client->call(obj, &IThread::is_valid, ret, {});
        return ret;
    }
};

class RemoteProcess : public IProcess
{
private:
    RPCClient* rpc_client;
    RemoteObjectId obj;
    ProcessImpType imp;
public:
    bool ok;

    template <typename... Args>
    RemoteProcess(RPCClient* rpc_client, ProcessImpType imp, Args... args) : rpc_client(rpc_client), imp(imp), obj(0), ok(true)
    {
        switch (imp)
        {
        case ProcessImpType::LINUX_USERMODE_PROC:
            ok = ok && rpc_client->call(new_remote<ProcessImp_LinuxUserMode, Args...>, obj, {args ...});
            break;
        default:
            ok = false;
        }
    }
    ~RemoteProcess()
    {
        if (!obj) {
            return;
        }
        rpc_client->call(delete_remote<IProcess>, ok, {obj});
    }

    virtual bool is_valid() override
    {
        bool ret;
        ok = ok && rpc_client->call(obj, &IProcess::is_valid, ret, {});
        return ret;
    }
    virtual const std::vector<std::unique_ptr<IThread>> threads() override
    {
        std::vector<std::unique_ptr<IThread>> ret;
        ok = ok && rpc_client->call(obj, &IProcess::threads, ret, {});
        return ret;
    }
    virtual const std::vector<std::unique_ptr<const IProcess>> children() override
    {
        std::vector<std::unique_ptr<const IProcess>> ret;
        ok = ok && rpc_client->call(obj, &IProcess::children, ret, {});
        return ret;
    }
};

class RemoteProcesses : public IProcesses
{
private:
    RPCClient* rpc_client;
    RemoteObjectId obj;
    ProcessesImpType imp;
public:
    bool ok;

    template <typename... Args>
    RemoteProcesses(RPCClient* rpc_client, ProcessesImpType imp, Args... args) : rpc_client(rpc_client), imp(imp), obj(0), ok(true)
    {
        switch (imp)
        {
        case ProcessesImpType::LINUX_USERMODE_PROC:
            ok = ok && rpc_client->call(new_remote<ProcessesImp_LinuxUserMode, Args...>, obj, {args ...});
            break;
        default:
            ok = false;
        }
    }
    ~RemoteProcesses()
    {
        if (!obj) {
            return;
        }
        rpc_client->call(delete_remote<IProcesses>, ok, {obj});
    }

    virtual bool update() override
    {
        bool ret;
        ok = ok && rpc_client->call(obj, &IProcesses::update, ret, {});
        if (!ok) {
            return ret;
        }
        std::unique_ptr<IProcesses> remote_copy;
        ok = ok && rpc_client->call(get_remote<IProcesses>, remote_copy, {obj});
        if (!ok) {
            return ret;
        }
        this->clear();
        for (auto& ptr : *remote_copy) {
            this->push_back(std::move(ptr));
        }
        return ret;
    }
};


class RemoteMemory : public IMemory
{
private:
    RPCClient* rpc_client;
    RemoteObjectId obj;
    MemoryImpType imp;
public:
    bool ok;

    template <typename... Args>
    RemoteMemory(RPCClient* rpc_client, MemoryImpType imp, Args... args) : rpc_client(rpc_client), imp(imp), obj(0), ok(true)
    {
        switch (imp)
        {
        case MemoryImpType::LINUX_USERMODE_PROC:
            ok = ok && rpc_client->call(new_remote<MemoryImp_LinuxUserMode, Args...>, obj, {args ...});
            break;
        default:
            ok = false;
        }
    }
    ~RemoteMemory()
    {
        if (!obj) {
            return;
        }
        rpc_client->call(delete_remote<IMemory>, ok, {obj});
    }

    virtual MemoryRegions regions() override
    {
        MemoryRegions ret;
        ok = ok && rpc_client->call(obj, &IMemory::regions, ret, {});
        return ret;
    }
    virtual ssize_t read(uint64_t addr, size_t size, std::vector<uint8_t>& data) override
    {
        std::tuple<size_t, std::vector<uint8_t>> ret;
        ok = ok && rpc_client->call(obj, &IMemory::read_noref, ret, {addr, size});
        if (ok) {
            data = std::get<1>(ret);
        }
        return data.size();
    }
    virtual ssize_t write(uint64_t addr, std::vector<uint8_t>& data) override
    {
        ssize_t ret;
        ok = ok && rpc_client->call(obj, &IMemory::write, ret, {addr, data});
        return ret;
    }
};
