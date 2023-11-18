#include "rpc.hpp"
#include "remote.hpp"
#include "base.hpp"

#include <map>
#include <string>
#include <functional>

int main(int argc, char** argv)
{
    init_rpc();

    TargetConfig cfg;
    cfg.protocol = TargetConfig::Protocol::TCP;
    cfg.server_addr = "127.0.0.1";
    cfg.port = 22334;
    cfg.auth = "123123";

    RPCServer rpc(cfg);
    if (!rpc) {
        cheatng_log("server not up\n");
        return -1;
    }
    cheatng_log("server up\n");
    while (rpc) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};
