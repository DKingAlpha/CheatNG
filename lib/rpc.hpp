#pragma once

// C++ 17 compatible

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <thread>

#include "base.hpp"
#include "rpc_tunnel.hpp"

#include "cereal/types/memory.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/tuple.hpp"
#include "cereal/types/string.hpp"

#include "cereal/archives/binary.hpp"
using CerealOutputArchive = cereal::BinaryOutputArchive;
using CerealInputArchive = cereal::BinaryInputArchive;

// #include <cereal/archives/json.hpp>
// using CerealOutputArchive = cereal::JSONOutputArchive;
// using CerealInputArchive =  cereal::JSONInputArchive;

// both client and server need to call this function
void init_rpc();

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

// -----------------

using RemoteObjectId = uint64_t;

extern std::map<FunctionId, std::function<std::string(const std::string&)>> functions;
extern std::set<RemoteObjectId> available_remote_objects;

// -----------------


template <typename... ParamTs>
struct FuncParams
{
    std::tuple<ParamTs...> params;

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(params);
    }
};

template <typename ParamTuple>
struct MethodParams
{
    RemoteObjectId obj; // remote object ptrr
    ParamTuple params;

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(obj, params);
    }
};

template <typename RetT>
struct FuncReturnValue
{
    RetT value;

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(value);
    }
};

template <typename S>
struct FuncType;

template <typename RetT, typename... ParamTs>
struct FuncType<RetT(ParamTs...)>
{
    using Ret = RetT;
    using ParamsTuple = std::tuple<ParamTs...>;
};

template <typename RetT, typename... ParamTs>
struct FuncType<RetT (*)(ParamTs...)>
{
    using Ret = RetT;
    using ParamsTuple = std::tuple<ParamTs...>;
};

template <typename RetT, typename... ParamTs>
struct FuncType<RetT (&)(ParamTs...)>
{
    using Ret = RetT;
    using ParamsTuple = std::tuple<ParamTs...>;
};

template <typename RetT, typename... ParamTs>
struct FuncType<std::function<RetT(ParamTs...)>>
{
    using Ret = RetT;
    using ParamsTuple = std::tuple<ParamTs...>;
};

template <typename ClassT, typename RetT, typename... ParamTs>
struct FuncType<RetT (ClassT::*)(ParamTs...)>
{
    using Ret = RetT;
    using Class = ClassT;
    using ParamsTuple = std::tuple<ParamTs...>;
};

// class method can not be converted to an address
// so we use a template to create another related address
template <typename T>
struct LocalFunctionId
{
    static constexpr void* value = (void*)(&LocalFunctionId<T>::value);
};

template <typename T, typename... Args>
RemoteObjectId new_remote(Args... args)
{
    RemoteObjectId obj = reinterpret_cast<RemoteObjectId>(new T(std::forward<Args>(args)...));
    available_remote_objects.insert(obj);
    return obj;
}

template <typename T>
bool delete_remote(RemoteObjectId obj)
{
    if (available_remote_objects.find(obj) == available_remote_objects.end()) {
        return false;
    }
    T* p = reinterpret_cast<T*>(obj);
    delete p;
    available_remote_objects.erase(obj);
    return true;
}

struct NoOpDeleter
{
    void operator()(void*) const {}
};

template <typename T>
std::unique_ptr<T, NoOpDeleter> get_remote(RemoteObjectId obj)
{
    if (available_remote_objects.find(obj) == available_remote_objects.end()) {
        return {nullptr};
    }
    T* p = reinterpret_cast<T*>(obj);
    NoOpDeleter d;
    return std::move(std::unique_ptr<T, NoOpDeleter>(p, d));
}

template <typename RetT, typename... ParamsT>
bool register_func(std::function<RetT(ParamsT...)> func, FunctionId func_id)
{
    if (functions.find(func_id) != functions.end()) {
        return false;
    }
    functions[func_id] = [func](const std::string& params) -> std::string {
        std::stringstream ss(params);
        FuncParams<ParamsT...> args;
        {
            CerealInputArchive ar(ss);
            ar(args);
        }
        auto retS = std::apply(func, args.params);
        std::stringstream ss2;
        {
            CerealOutputArchive ar(ss2);
            ar(retS);
        }
        return ss2.str();
    };
    return true;
}

// plain c/c++ function (*free* function)
template <typename RetT, typename... ParamsT>
bool register_func(RetT (*func)(ParamsT...), FunctionId func_id)
{
    if (functions.find(func_id) != functions.end()) {
        return false;
    }
    functions[func_id] = [func](const std::string& params) -> std::string {
        std::stringstream ss(params);
        FuncParams<ParamsT...> args;
        {
            CerealInputArchive ar(ss);
            ar(args);
        }
        auto retS = std::apply(func, args.params);
        std::stringstream ss2;
        {
            CerealOutputArchive ar(ss2);
            ar(retS);
        }
        return ss2.str();
    };
    return true;
}

template <typename ClsT, typename RetT, typename... ParamsT>
bool register_func(RetT (ClsT::*func)(ParamsT...), FunctionId func_id)
{
    if (functions.find(func_id) != functions.end()) {
        return false;
    }
    functions[func_id] = [func](const std::string& params) -> std::string {
        std::stringstream ss(params);
        MethodParams<std::tuple<ParamsT...>> obj_args;
        {
            CerealInputArchive ar(ss);
            ar(obj_args);
        }
        std::tuple<ClsT*> flatten_tuple(reinterpret_cast<ClsT*>(obj_args.obj));
        auto retS = std::apply(std::mem_fn(func), std::tuple_cat(flatten_tuple, obj_args.params));
        std::stringstream ss2;
        {
            CerealOutputArchive ar(ss2);
            ar(retS);
        }
        return ss2.str();
    };
    return true;
}
// -----------------

class RPCClient
{
public:
    RPCClient(TargetConfig cfg) : tunnel(TunnelFactory::create(false, cfg)), authed(false)
    {
        if (!tunnel) {
            return;
        }
        std::string auth_result = "fail";
        TunnelStatus status = tunnel->send(cfg.auth);
        if (status == TunnelStatus::OK) {
            status = tunnel->recv(auth_result);
        }
        if (status == TunnelStatus::OK) {
            authed = (auth_result == "ok");
        }
        if (!authed) {
            tunnel.reset();
            cheatng_log("auth failed. closed connection\n");
        }
    }

    operator bool() const
    {
        if (!tunnel) {
            return false;
        }
        if(!tunnel->is_connected()) {
            return false;
        }
        return authed;
    }

private:
    std::unique_ptr<ITunnel> tunnel;
    bool authed = false;
    std::mutex mutex;

    bool do_call(FunctionId func_id, std::string& ret, std::string params)
    {
        if (!authed) {
            return false;
        }
        std::string s_func_id = std::to_string((uint32_t)func_id);
        std::lock_guard<std::mutex> lock(mutex);
        TunnelStatus status = tunnel->send(s_func_id);
        if (status == TunnelStatus::OK) {
            status = tunnel->send(params);
        }
        if (status == TunnelStatus::OK) {
            status = tunnel->recv(ret);
        }
        if (status != TunnelStatus::OK) {
            if (status == TunnelStatus::ERROR) {
                cheatng_log("connection to server error\n");
            }
            if (status == TunnelStatus::CLOSED) {
                cheatng_log("server closed connection\n");
            }
            tunnel.reset();
            authed = false;
        }
        return status == TunnelStatus::OK;
    }

public:
    // explicitly specify RetT and ParamsTuple, to avoid inaccurate deduction of func param types
    template <typename FType, typename RetT = typename FuncType<FType>::Ret, typename ParamsTuple = typename FuncType<FType>::ParamsTuple>
    bool call(FunctionId func_id, FType func, RetT& ret, ParamsTuple&& args)
    {
        std::stringstream ss;
        {
            CerealOutputArchive ar(ss);
            ar(args);
        }
        std::string params = ss.str();
        std::string ret_data;
        bool ok = do_call(func_id, ret_data, params);
        if (!ok) {
            return false;
        }
        std::stringstream ss2(ret_data);
        FuncReturnValue<RetT> retS;
        {
            CerealInputArchive ar(ss2);
            ar(retS);
        }
        ret = std::move(retS.value);
        return true;
    }

    template <typename FType, typename RetT = typename FuncType<FType>::Ret, typename ParamsTuple = typename FuncType<FType>::ParamsTuple>
    bool call(FunctionId func_id, RemoteObjectId obj, FType func, RetT& ret, ParamsTuple&& args)
    {
        MethodParams<ParamsTuple> obj_args{.obj = obj, .params = args};

        std::stringstream ss;
        {
            CerealOutputArchive ar(ss);
            ar(obj_args);
        }
        std::string params = ss.str();
        std::string ret_data;
        bool ok = do_call(func_id, ret_data, params);
        if (!ok) {
            return false;
        }
        std::stringstream ss2(ret_data);
        FuncReturnValue<RetT> retS;
        {
            CerealInputArchive ar(ss2);
            ar(retS);
        }
        ret = std::move(retS.value);
        return true;
    }
};

class RPCServer
{
public:
    RPCServer(TargetConfig cfg) : tunnel(TunnelFactory::create(true, cfg)), authed(false), server_up(false)
    {
        if (!tunnel) {
            return;
        }
        if (!tunnel->is_server_up()) {
            return;
        }
        server_up = true;
        server_thread = std::make_unique<std::thread>([&](){
            while (server_up) {
                if (!tunnel->is_connected()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                std::string first_data = "";
                TunnelStatus status = tunnel->recv(first_data);
                if(status != TunnelStatus::OK) {
                    if(status == TunnelStatus::ERROR) {
                        cheatng_log("recv client data error, closing connection to client\n");
                    }
                    if(status == TunnelStatus::CLOSED) {
                        cheatng_log("client closed connection\n");
                    }
                    tunnel->close();    // close client connection
                    authed = false;
                    continue;
                }
                if (!authed) {
                    std::string& auth = cfg.auth;
                    if (auth == cfg.auth) {
                        authed = true;
                        tunnel->send("ok");
                    } else {
                        cheatng_log("client auth failed, closing client connection\n");
                        tunnel->send("fail");
                        tunnel->close();    // close client connection
                    }
                    continue;
                }
                std::string& s_func_id = first_data;
                std::string params = "";
                FunctionId func_id = (FunctionId)std::stoul(s_func_id);

                status = tunnel->recv(params);
                std::string ret = "";
                if (status == TunnelStatus::OK) {
                    if (do_call(func_id, ret, params)) {
                        status = tunnel->send(ret);
                    }
                }
                if (status != TunnelStatus::OK) {
                    if(status == TunnelStatus::ERROR) {
                        cheatng_log("recv client data error, closing connection to client\n");
                    }
                    if(status == TunnelStatus::CLOSED) {
                        cheatng_log("client closed connection\n");
                    }
                    tunnel->close();    // close client connection
                    authed = false;
                }
            }
            cheatng_log("server thread exit\n");
        });
    }

    ~RPCServer()
    {
        server_up = false;
        if (server_thread && server_thread->joinable()) {
            server_thread->join();
        }
    }

    operator bool() const
    {
        if (!tunnel) {
            return false;
        }
        if(!tunnel->is_server_up()) {
            return false;
        }
        return server_thread && server_thread->joinable();
    }

private:
    bool do_call(FunctionId func_id, std::string& ret, std::string params) const
    {
        if (!tunnel) {
            return false;
        }
        if (!(*tunnel)) {
            return false;
        }
        auto it = functions.find(func_id);
        if (it == functions.end()) {
            return false;
        }
        ret = (it->second)(params);
        return true;
    }

private:
    std::unique_ptr<ITunnel> tunnel;
    std::unique_ptr<std::thread> server_thread;
    bool authed;
    bool server_up;
};
