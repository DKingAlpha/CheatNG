#include "rpc.hpp"

#include <map>
#include <string>
#include <functional>

#include "imp/linux/mem_usermode_proc.hpp"
#include "imp/linux/proc_usermode_proc.hpp"

std::map<FunctionId, std::function<std::string(const std::string&)>> functions;
std::set<RemoteObjectId> available_remote_objects;

void init_rpc()
{
    register_func(new_remote<ThreadImp_LinuxUserMode,int>, FunctionId::new_remote_ThreadImp_LinuxUserMode_int);
    register_func(new_remote<ThreadImp_LinuxUserMode,int,int>, FunctionId::new_remote_ThreadImp_LinuxUserMode_int_int);
    register_func(new_remote<ProcessImp_LinuxUserMode,int>, FunctionId::new_remote_ProcessImp_LinuxUserMode_int);
    register_func(new_remote<ProcessImp_LinuxUserMode,int,int>, FunctionId::new_remote_ProcessImp_LinuxUserMode_int_int);
    register_func(new_remote<ProcessesImp_LinuxUserMode>, FunctionId::new_remote_ProcessesImp_LinuxUserMode);
    register_func(new_remote<MemoryImp_LinuxUserMode,int>, FunctionId::new_remote_MemoryImp_LinuxUserMode_int);

    register_func(delete_remote<IThread>, FunctionId::delete_remote_IThread);
    register_func(delete_remote<IProcess>, FunctionId::delete_remote_IProcess);
    register_func(delete_remote<IProcesses>, FunctionId::delete_remote_IProcesses);
    register_func(delete_remote<IMemory>, FunctionId::delete_remote_IMemory);

    register_func(get_remote<IThread>, FunctionId::get_remote_IThread);
    register_func(get_remote<IProcess>, FunctionId::get_remote_IProcess);
    register_func(get_remote<IProcesses>, FunctionId::get_remote_IProcesses);
    register_func(get_remote<IMemory>, FunctionId::get_remote_IMemory);

    register_func(&IThread::is_valid, FunctionId::IThread_is_valid);
    register_func(&IProcess::is_valid, FunctionId::IProcess_is_valid);
    register_func(&IProcess::threads, FunctionId::IProcess_threads);
    register_func(&IProcess::children, FunctionId::IProcess_children);
    register_func(&IProcesses::update, FunctionId::IProcesses_update);
    register_func(&IMemory::read_noref, FunctionId::IMemory_read_noref);
    register_func(&IMemory::write_noref, FunctionId::IMemory_write_noref);
    register_func(&IMemory::regions, FunctionId::IMemory_regions);
};
