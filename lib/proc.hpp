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

struct TaskPropertiesReadOnly
{
    int id;
    int parent_id;
    std::string name;
};

// DO NOT mark "const" to methods/members in these interfaces, because RPC proxy class may alter internal class state, 
// for example, to update connection status.

class IThread : public IValidBoolOp, public virtual TaskPropertiesReadOnly
{
public:
    IThread() = default;
    virtual ~IThread() = default;

    template <class Archive>
    void serialize(Archive& archive) { archive(id, parent_id, name); }
};

class IProcess : public IValidBoolOp, public virtual TaskPropertiesReadOnly
{
public:
    IProcess() = default;
    virtual ~IProcess() = default;

    std::vector<std::string> cmdlines;

    template <class Archive>
    void serialize(Archive& archive) { archive(id, parent_id, name, cmdlines); }

    virtual const std::vector<std::unique_ptr<IThread>> threads() = 0;
    virtual const std::vector<std::unique_ptr<const IProcess>> children() = 0;

    // helper function
    const std::string cmdlines_str() const
    {
        std::string s;
        for (const auto& cmdline : cmdlines) {
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
    virtual bool update() = 0;


    /**
     * @brief get process by pid, binary search to improve performance.
     *
     * @param id pid
     * @return Process* nullptr if not found
     */

    /* RPC in-compatible
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
    */
};
