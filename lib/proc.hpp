#pragma once

#include <memory>
#include <string>
#include <vector>

#include "base.hpp"


enum class ThreadImpType
{
    LINUX_USERMODE_PROC,
    LINUX_KERNEL_MODULE,
};

enum class ProcessImpType
{
    LINUX_USERMODE_PROC,
    LINUX_KERNEL_MODULE,
};

enum class ProcessesImpType
{
    LINUX_USERMODE_PROC,
    LINUX_KERNEL_MODULE,
};

class TaskPropertiesReadOnly
{
public:
    TaskPropertiesReadOnly(int id, int parent_id, const std::string& name) : id(id), parent_id(parent_id), name(name) {}

    const int id;
    const int parent_id;
    const std::string name;

    TaskPropertiesReadOnly& operator=(const TaskPropertiesReadOnly& other)
    {
        const_cast<int&>(id) = other.id;
        const_cast<int&>(parent_id) = other.parent_id;
        const_cast<std::string&>(name) = other.name;
        return *this;
    }
};

class IThread : public IValidBoolOp, public virtual TaskPropertiesReadOnly
{
public:
    IThread() = default;
    virtual ~IThread() = default;
};

class IProcess : public IValidBoolOp, public virtual TaskPropertiesReadOnly
{
public:
    IProcess() = default;
    virtual ~IProcess() = default;

    virtual const std::vector<std::string> cmdlines() const = 0;
    virtual const std::vector<std::unique_ptr<IThread>> threads() const = 0;
    virtual const std::vector<std::unique_ptr<const IProcess>> children() const = 0;

    // helper function
    const std::string cmdlines_str() const
    {
        std::string s;
        for (const auto& cmdline : cmdlines()) {
            s += cmdline + " ";
        }
        return s;
    }
};

/**
 * @brief RAII process list. Ascending order by pid.
 */
class IProcesses : public std::vector<std::unique_ptr<IProcess>>
{
public:
    virtual ~IProcesses() = default;

    /**
     * @brief update process list stored in this object.
     */
    virtual void update() = 0;

    /**
     * @brief get process by pid, binary search to improve performance.
     *
     * @param id pid
     * @return Process* nullptr if not found
     */
    const IProcess* get(int id) const
    {
        // binary search
        auto& self = *this;
        int l = 0, r = self.size() - 1;
        while (l <= r) {
            int m = (l + r) / 2;
            if (self[m]->id == id) {
                return self[m].get();
            } else if (self[m]->id < id) {
                l = m + 1;
            } else {
                r = m - 1;
            }
        }
        return nullptr;
    }
};
