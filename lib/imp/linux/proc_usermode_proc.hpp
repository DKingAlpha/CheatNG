#pragma once

#include "native.hpp"

class ThreadImp_LinuxUserMode : public INativeThread
{
public:
    ThreadImp_LinuxUserMode(int id);
    ThreadImp_LinuxUserMode(int id, int parent_id);

    virtual bool is_valid() const override;
};

class ProcessImp_LinuxUserMode : public INativeProcess
{
public:
    ProcessImp_LinuxUserMode(int id);
    ProcessImp_LinuxUserMode(int id, int parent_id);

    virtual bool is_valid() const override;

    // argv. For linux it is the content of "cmdline" splited by \0
    virtual const std::vector<std::string> cmdlines() const override;

    // get current threads
    virtual const std::vector<std::unique_ptr<IThread>> threads() const override;

    // get children processes
    virtual const std::vector<std::unique_ptr<const IProcess>> children() const override;
};

class ProcessesImp_LinuxUserMode : public INativeProcesses
{
public:
    ProcessesImp_LinuxUserMode() { update(); }

    virtual void update() override;
};