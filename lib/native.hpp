#pragma once

#include "proc.hpp"
#include "mem.hpp"

class INativeMemory: public IMemory {
    using IMemory::IMemory;
};

class INativeThread : public IThread {};

class INativeProcess : public IProcess {};

class INativeProcesses : public IProcesses {};

