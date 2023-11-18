#include "rpc.hpp"

#include <map>
#include <string>
#include <functional>

#include "imp/linux/mem_usermode_proc.hpp"
#include "imp/linux/proc_usermode_proc.hpp"

std::map<std::string, std::function<std::string(const std::string&)>> functions;
std::map<uintptr_t, std::string> functions_id_to_name;
std::set<RemoteObjectId> available_remote_objects;

void init_rpc()
{
    register_func(new_remote<ThreadImp_LinuxUserMode,int>, "new_remote<ThreadImp_LinuxUserMode,int>");
    register_func(new_remote<ThreadImp_LinuxUserMode,int,int>, "new_remote<ThreadImp_LinuxUserMode,int,int>");
    register_func(new_remote<ProcessImp_LinuxUserMode,int>, "new_remote<ProcessImp_LinuxUserMode,int>");
    register_func(new_remote<ProcessImp_LinuxUserMode,int,int>, "new_remote<ProcessImp_LinuxUserMode,int,int>");
    register_func(new_remote<ProcessesImp_LinuxUserMode>, "new_remote<ProcessesImp_LinuxUserMode>");
    register_func(new_remote<MemoryImp_LinuxUserMode,int>, "new_remote<MemoryImp_LinuxUserMode,int>");

    register_func(delete_remote<IThread>, "delete_remote<IThread>");
    register_func(delete_remote<IProcess>, "delete_remote<IProcess>");
    register_func(delete_remote<IProcesses>, "delete_remote<IProcesses>");
    register_func(delete_remote<IMemory>, "delete_remote<IMemory>");

    register_func(get_remote<IThread>, "get_remote<IThread>");
    register_func(get_remote<IProcess>, "get_remote<IProcess>");
    register_func(get_remote<IProcesses>, "get_remote<IProcesses>");
    register_func(get_remote<IMemory>, "get_remote<IMemory>");

    register_func(&IThread::is_valid, "IThread::is_valid");
    register_func(&IProcess::is_valid, "IProcess::is_valid");
    register_func(&IProcess::threads, "IProcess::threads");
    register_func(&IProcess::children, "IProcess::children");
    register_func(&IProcesses::update, "IProcesses::update");
    register_func(&IMemory::read_noref, "IMemory::read");
    register_func(&IMemory::write_noref, "IMemory::write");
    register_func(&IMemory::regions, "IMemory::regions");
};

// CEREAL_REGISTER_DYNAMIC_INIT(ThreadImp_LinuxUserMode);
// CEREAL_REGISTER_DYNAMIC_INIT(ProcessImp_LinuxUserMode);
// CEREAL_REGISTER_DYNAMIC_INIT(ProcessesImp_LinuxUserMode);
// CEREAL_REGISTER_DYNAMIC_INIT(MemoryImp_LinuxUserMode);
