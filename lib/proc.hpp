#pragma once

#include <string>
#include <vector>

#include "base.hpp"

class Thread : public IValidBoolOp
{
public:
    Thread(int id);
    Thread(int id, int parent_id);

    virtual bool is_valid() const override;

    // display name only. For linux it is the content of "comm"
    inline const int id() const { return _id; }
    inline const int parent_id() const { return _parent_id; }
    inline const std::string& name() const { return _name; }

protected:
    int _id;
    int _parent_id;
    std::string _name;
};

class Process : public Thread
{
public:
    Process(int id);
    Process(int id, int parent_id);

    virtual bool is_valid() const override;

    // argv. For linux it is the content of "cmdline" splited by \0
    const std::vector<std::string> cmdlines() const;

    const std::string cmdlines_str() const
    {
        std::string s;
        for (const auto& cmdline : cmdlines()) {
            s += cmdline + " ";
        }
        return s;
    }

    // get current threads
    const std::vector<Thread> threads() const;

    // must call Processes::build_tree() before calling this method
    const std::vector<const Process*>& children() const { return _children; };

private:
    friend class Processes;
    std::vector<const Process*> _children;
};

/**
 * @brief RAII process list.
 * NOT Thread-Safe.
 *
 * Also provides additional methods to operate process list.
 */
class Processes : public std::vector<Process>
{
public:
    Processes();

    /**
     * @brief update process list stored in this object.
     */
    void update();

    /**
     * @brief get process by pid, binary search to improve performance.
     *
     * @param id pid
     * @return Process* nullptr if not found
     */
    const Process* get(int id) const;

    std::vector<const Process*> children_of(Process& proc) const;

    /**
     * @brief return root list
     *
     * If a process has no parent (0), it is a root process.
     * If a process's parent is not in the list, it is a root process.
     */
    std::vector<const Process*> build_tree();
};
