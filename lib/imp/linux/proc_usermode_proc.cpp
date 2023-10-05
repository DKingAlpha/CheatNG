#include "proc_usermode_proc.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>


// ====================

static std::string get_task_comm(int pid)
{
    std::ifstream ifs("/proc/" + std::to_string(pid) + "/comm");
    if (ifs) {
        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string name = ss.str();
        while (name.size() && name.back() == '\n') {
            name.pop_back();
        }
        return name;
    }
    return "";
}

static std::pair<int, int> get_task_tgid_ppid(int pid)
{
    int tgid = -1;
    int ppid = -1;
    std::ifstream ifs("/proc/" + std::to_string(pid) + "/status");
    if (!ifs) {
        return {tgid, ppid};
    }
    std::string line;
    while (std::getline(ifs, line)) {
        if (tgid < 0 && line.starts_with("Tgid:")) {
            tgid = std::stoi(line.substr(5));
        }
        if (ppid < 0 && line.starts_with("PPid:")) {
            ppid = std::stoi(line.substr(5));
        }
        if (ppid >= 0 && tgid >= 0) {
            break;
        }
    }
    return {tgid, ppid};
}

// ====================

ThreadImp_LinuxUserMode::ThreadImp_LinuxUserMode(int id) : TaskPropertiesReadOnly(id, get_task_tgid_ppid(id).second, get_task_comm(id)) {}

ThreadImp_LinuxUserMode::ThreadImp_LinuxUserMode(int id, int parent_id) : TaskPropertiesReadOnly(id, parent_id, get_task_comm(id)) {}

bool ThreadImp_LinuxUserMode::is_valid() const { return std::filesystem::exists("/proc/" + std::to_string(id)); }

// ====================

ProcessImp_LinuxUserMode::ProcessImp_LinuxUserMode(int id) : TaskPropertiesReadOnly(id, get_task_tgid_ppid(id).second, get_task_comm(id)) {}

ProcessImp_LinuxUserMode::ProcessImp_LinuxUserMode(int id, int parent_id) : TaskPropertiesReadOnly(id, parent_id, get_task_comm(id)) {}

bool ProcessImp_LinuxUserMode::is_valid() const { return std::filesystem::exists("/proc/" + std::to_string(id)); }

const std::vector<std::string> ProcessImp_LinuxUserMode::cmdlines() const
{
    std::vector<std::string> cmdlines;
    std::ifstream ifs("/proc/" + std::to_string(id) + "/cmdline");
    if (ifs) {
        std::string cmdline;
        while (std::getline(ifs, cmdline, '\0')) {
            if (cmdline.empty()) {
                continue;
            }
            cmdlines.push_back(cmdline);
        }
    }
    return cmdlines;
}

const std::vector<std::unique_ptr<IThread>> ProcessImp_LinuxUserMode::threads() const
{
    std::vector<std::unique_ptr<IThread>> threads;
    std::filesystem::path task_folder = "/proc/" + std::to_string(id) + "/task";
    if (!std::filesystem::exists(task_folder)) {
        return threads;
    }
    for (const auto& entry : std::filesystem::directory_iterator(task_folder)) {
        const auto& path = entry.path();
        if (path.filename().string().find_first_not_of("0123456789") != std::string::npos)
            continue;
        auto tid = std::stoi(path.filename().string());
        threads.push_back(std::make_unique<ThreadImp_LinuxUserMode>(tid, id));
    }
    return threads;
}


const std::vector<std::unique_ptr<const IProcess>> ProcessImp_LinuxUserMode::children() const
{
    std::vector<std::unique_ptr<const IProcess>> children;
    for (const auto& entry : std::filesystem::directory_iterator("/proc/" + std::to_string(id) + "/task")) {
        const auto& path = entry.path();
        if (path.filename().string().find_first_not_of("0123456789") != std::string::npos)
            continue;
        const auto tid = std::stoi(path.filename().string());
        if (tid == id) {
            continue;
        }
        auto [tgid, _ppid] = get_task_tgid_ppid(tid);
        if (tgid != id) {
            // thread
            continue;
        }
        children.push_back(std::make_unique<ProcessImp_LinuxUserMode>(tid, id));
    }
    return children;
}

// ====================

void ProcessesImp_LinuxUserMode::update()
{
    this->clear();
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        const auto& path = entry.path();
        if (path.filename().string().find_first_not_of("0123456789") != std::string::npos)
            continue;
        const auto pid = std::stoi(path.filename().string());
        std::ifstream ifs("/proc/" + std::to_string(pid) + "/status");
        if (!ifs) {
            continue;
        }
        std::string line;
        int tgid = -1;
        int ppid = -1;
        while (std::getline(ifs, line)) {
            if (tgid < 0 && line.starts_with("Tgid:")) {
                tgid = std::stoi(line.substr(5));
            }
            if (ppid < 0 && line.starts_with("PPid:")) {
                ppid = std::stoi(line.substr(5));
            }
            if (ppid >= 0 && tgid >= 0) {
                break;
            }
        }
        if (tgid != pid) {
            // thread
            continue;
        }
        this->push_back(std::make_unique<ProcessImp_LinuxUserMode>(pid, ppid));
    }
    std::sort(this->begin(), this->end(), [](std::unique_ptr<IProcess>& a, std::unique_ptr<IProcess>& b) { return a->id < b->id; });
}
