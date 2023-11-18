#pragma once

#include "proc.hpp"

class ThreadImp_LinuxUserMode : public IThread
{
public:
    ThreadImp_LinuxUserMode() = default;    // for cereal
    ThreadImp_LinuxUserMode(int id);
    ThreadImp_LinuxUserMode(int id, int parent_id);

    virtual bool is_valid() override;
};

class ProcessImp_LinuxUserMode : public IProcess
{
public:
    ProcessImp_LinuxUserMode() = default;    // for cereal
    ProcessImp_LinuxUserMode(int id);
    ProcessImp_LinuxUserMode(int id, int parent_id);

    virtual bool is_valid() override;

    // get current threads
    virtual const std::vector<std::unique_ptr<IThread>> threads() override;

    // get children processes
    virtual const std::vector<std::unique_ptr<const IProcess>> children() override;
};

class ProcessesImp_LinuxUserMode : public IProcesses
{
public:
    ProcessesImp_LinuxUserMode() { update(); }

    virtual bool update() override;
};