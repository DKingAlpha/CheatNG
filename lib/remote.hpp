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

enum class FunctionId : uint32_t
{
    // new_remote
    new_remote_ThreadImp_LinuxUserMode_int = 0x00000000,
    new_remote_ThreadImp_LinuxUserMode_int_int = 0x00000001,
    new_remote_ProcessImp_LinuxUserMode_int = 0x00000002,
    new_remote_ProcessImp_LinuxUserMode_int_int = 0x00000003,
    new_remote_ProcessesImp_LinuxUserMode = 0x00000004,
    new_remote_MemoryImp_LinuxUserMode_int = 0x00000005,

    // delete_remote
    delete_remote_IThread = 0x00000006,
    delete_remote_IProcess = 0x00000007,
    delete_remote_IProcesses = 0x00000008,
    delete_remote_IMemory = 0x00000009,

    // get_remote
    get_remote_IThread = 0x0000000a,
    get_remote_IProcess = 0x0000000b,
    get_remote_IProcesses = 0x0000000c,
    get_remote_IMemory = 0x0000000d,

    // IThread
    IThread_is_valid = 0x0000000e,

    // IProcess
    IProcess_is_valid = 0x0000000f,
    IProcess_threads = 0x00000010,
    IProcess_children = 0x00000011,

    // IProcesses
    IProcesses_update = 0x00000012,

    // IMemory
    IMemory_read_noref = 0x00000013,
    IMemory_write_noref = 0x00000014,
    IMemory_regions = 0x00000015,
};


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
        FunctionId func_id = sizeof...(Args) == 1 ? FunctionId::new_remote_ThreadImp_LinuxUserMode_int : FunctionId::new_remote_ThreadImp_LinuxUserMode_int_int;
        switch (imp)
        {
        case ThreadImpType::LINUX_USERMODE_PROC:
            ok = ok && rpc_client->call(func_id, new_remote<ThreadImp_LinuxUserMode, Args...>, obj, {args ...});
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
        rpc_client->call(FunctionId::delete_remote_IThread, delete_remote<IThread>, ok, {obj});
    }

    virtual bool is_valid() override
    {
        bool ret;
        ok = ok && rpc_client->call(FunctionId::IThread_is_valid, obj, &IThread::is_valid, ret, {});
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
        FunctionId func_id = sizeof...(Args) == 1 ? FunctionId::new_remote_ProcessImp_LinuxUserMode_int : FunctionId::new_remote_ProcessImp_LinuxUserMode_int_int;
        switch (imp)
        {
        case ProcessImpType::LINUX_USERMODE_PROC:
            ok = ok && rpc_client->call(func_id, new_remote<ProcessImp_LinuxUserMode, Args...>, obj, {args ...});
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
        rpc_client->call(FunctionId::delete_remote_IProcess, delete_remote<IProcess>, ok, {obj});
    }

    virtual bool is_valid() override
    {
        bool ret;
        ok = ok && rpc_client->call(FunctionId::IProcess_is_valid, obj, &IProcess::is_valid, ret, {});
        return ret;
    }
    virtual const std::vector<std::unique_ptr<IThread>> threads() override
    {
        std::vector<std::unique_ptr<IThread>> ret;
        ok = ok && rpc_client->call(FunctionId::IProcess_threads, obj, &IProcess::threads, ret, {});
        return ret;
    }
    virtual const std::vector<std::unique_ptr<const IProcess>> children() override
    {
        std::vector<std::unique_ptr<const IProcess>> ret;
        ok = ok && rpc_client->call(FunctionId::IProcess_children, obj, &IProcess::children, ret, {});
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
            ok = ok && rpc_client->call(FunctionId::new_remote_ProcessesImp_LinuxUserMode, new_remote<ProcessesImp_LinuxUserMode, Args...>, obj, {args ...});
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
        rpc_client->call(FunctionId::delete_remote_IProcesses, delete_remote<IProcesses>, ok, {obj});
    }

    virtual bool update() override
    {
        bool ret;
        ok = ok && rpc_client->call(FunctionId::IProcesses_update, obj, &IProcesses::update, ret, {});
        if (!ok) {
            return ret;
        }
        std::unique_ptr<IProcesses> remote_copy;
        ok = ok && rpc_client->call(FunctionId::get_remote_IProcesses, get_remote<IProcesses>, remote_copy, {obj});
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
            ok = ok && rpc_client->call(FunctionId::new_remote_MemoryImp_LinuxUserMode_int, new_remote<MemoryImp_LinuxUserMode, Args...>, obj, {args ...});
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
        rpc_client->call(FunctionId::delete_remote_IMemory, delete_remote<IMemory>, ok, {obj});
    }

    virtual MemoryRegions regions() override
    {
        MemoryRegions ret;
        ok = ok && rpc_client->call(FunctionId::IMemory_regions, obj, &IMemory::regions, ret, {});
        return ret;
    }
    virtual ssize_t read(uint64_t addr, size_t size, std::vector<uint8_t>& data) override
    {
        std::tuple<size_t, std::vector<uint8_t>> ret;
        ok = ok && rpc_client->call(FunctionId::IMemory_read_noref, obj, &IMemory::read_noref, ret, {addr, size});
        if (ok) {
            data = std::get<1>(ret);
        }
        return data.size();
    }
    virtual ssize_t write(uint64_t addr, std::vector<uint8_t>& data) override
    {
        ssize_t ret;
        ok = ok && rpc_client->call(FunctionId::IMemory_write_noref, obj, &IMemory::write_noref, ret, {addr, data});
        return ret;
    }
};
