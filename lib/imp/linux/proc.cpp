#include "proc.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

// ====================

Thread::Thread(int id) : _id(id), _parent_id(-1)
{
    // name
    std::ifstream ifs("/proc/" + std::to_string(_id) + "/comm");
    if (ifs) {
        std::stringstream ss;
        ss << ifs.rdbuf();
        _name = ss.str();
        while (_name.size() && _name.back() == '\n') {
            _name.pop_back();
        }
    }
    // find parent id
    std::ifstream ifs2("/proc/" + std::to_string(_id) + "/status");
    if (ifs2) {
        std::string line;
        while (std::getline(ifs2, line)) {
            if (line.find("PPid:") == 0) {
                _parent_id = std::stoi(line.substr(5));
                break;
            }
        }
    }
}

Thread::Thread(int id, int parent_id) : _id(id), _parent_id(parent_id)
{
    // name
    std::ifstream ifs("/proc/" + std::to_string(id) + "/comm");
    if (ifs) {
        std::stringstream ss;
        ss << ifs.rdbuf();
        _name = ss.str();
        while (_name.size() && _name.back() == '\n') {
            _name.pop_back();
        }
    }
}

bool Thread::is_valid() const { return std::filesystem::exists("/proc/" + std::to_string(_id)); }

// ====================

Process::Process(int id) : Thread(id) {}
Process::Process(int id, int parent_id) : Thread(id, parent_id) {}

bool Process::is_valid() const { return std::filesystem::exists("/proc/" + std::to_string(_id)); }

const std::vector<std::string> Process::cmdlines() const
{
    std::vector<std::string> _cmdlines;
    std::ifstream ifs("/proc/" + std::to_string(_id) + "/cmdline");
    if (ifs) {
        std::string cmdline;
        while (std::getline(ifs, cmdline, '\0')) {
            if (cmdline.empty()) {
                continue;
            }
            _cmdlines.push_back(cmdline);
        }
    }
    return _cmdlines;
}

const std::vector<Thread> Process::threads() const
{
    std::vector<Thread> _threads;
    std::filesystem::path task_folder = "/proc/" + std::to_string(_id) + "/task";
    if (!std::filesystem::exists(task_folder)) {
        return _threads;
    }
    for (const auto& entry : std::filesystem::directory_iterator(task_folder)) {
        const auto& path = entry.path();
        if (path.filename().string().find_first_not_of("0123456789") != std::string::npos)
            continue;
        auto tid = std::stoi(path.filename().string());
        _threads.push_back({tid, _id});
    }
    return _threads;
}

// ====================

Processes::Processes() { update(); }

void Processes::update()
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
        this->push_back({pid, ppid});
    }
    std::sort(this->begin(), this->end(), [](Process& a, Process& b) { return a.id() < b.id(); });
}

const Process* Processes::get(int id) const
{
    // binary search
    int l = 0, r = this->size() - 1;
    while (l <= r) {
        int m = (l + r) / 2;
        if ((*this)[m].id() == id) {
            return &((*this)[m]);
        } else if ((*this)[m].id() < id) {
            l = m + 1;
        } else {
            r = m - 1;
        }
    }
    return nullptr;
}

std::vector<const Process*> Processes::children_of(Process& proc) const
{
    std::vector<const Process*> children;
    for (const Process& p : *this) {
        if (p.parent_id() == proc.id()) {
            children.push_back(&p);
        }
    }
    return children;
}

std::vector<const Process*> Processes::build_tree()
{
    std::vector<const Process*> roots;

    for (Process& p : *this) {
        p._children.clear();
    }

    for (const Process& p : *this) {
        if (p.parent_id() == 0) {
            roots.push_back(&p);
        } else {
            const Process* parent = get(p.parent_id());
            if (parent == nullptr) {
                roots.push_back(&p);
            } else {
                const_cast<Process*>(parent)->_children.push_back(&p);
            }
        }
    }
    return roots;
}
