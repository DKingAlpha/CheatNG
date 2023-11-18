# CheatNG

CheatNG is a versatile framework that provides helpful tools for cheating in various OSes.

It is primarily designed to offer unified experience with powerful, modern, and user-friendly capabilities on all major OSes.

It is also designed to be extensible and customizable, so that it can be easily ported to other OSes and be used in different scenarios.

W.I.P

```
Architecture Portability

          +------v-------+
          |  CheatNG GUI |
          +--------------+
                 |        
          +------v-------+
          |     imgui    |
          +--------------+
          |  OpenGL/SDL  |
          +--------------+
                 |
          +------v-------+
          | CheatNG Core |
          +--------------+
                 |
 +---------------v------------------+
 |        Abstract OS Layer         |
 +----------------------------------+
 |  IProcess  |  IThread  | IMemory |
 +----------------------------------+
           /               \        
          /           CheatNG Server (RPC)
         /                   \      
 +------v-----+        +------v-----+
 |  Local OS  | - OR - |  Remote OS |
 +------------+        +------------+
```

## Supported OS

- [x] Linux
- [ ] Android
- [ ] Windows
- [ ] Nintendo Switch
- [ ] ~~iOS/macOS~~ (No. F^ck Apple)

## Supported Featuers

| Feature | | Note |
| ------- | -------- | -------- |
| **Remote Server** | ✔ | |
| Memory Editor | ✔ | |
| Module Explorer | ✔ | |
| Memory Searcher | ✔ | |
| Script Engine | 🛠️ | Script Engine (most likely to be python) that perform automation outside and inside target process |
| Module Injector | 🛠️ | Module injection in diverse bypass methods |
| Debugger | 🛠️ | Traditional / Non-traditional Debugger |

## Supported OS Interaction Implementation

Implementations are extensible under strategy pattern.

- [x] Linux - process_vm_rw
- [ ] Linux - /proc/\<pid\>/mem
- [ ] Linux - ptrace
- [ ] Linux - Kernel Module
- [ ] Linux - Mem Injection
- [ ] Windows - RWProcessMemory / NtRWVirtualMemory
- [ ] Windows - Kernel Driver
- [ ] Nintendo Switch - Debugging

## How to Build CheatNG

See [BUILD.md](BUILD.md)

## built-in RPC features

- Native C/C++ function/class/methods/types support
  - new/delete on remote machine
  - call function/member on remote machine with native paramters
  - get return value of native type from remote machine
- Auto type conversion even in RPC. (char* to std::string, etc)
- Non-intrusive to code base.

## Why RPC interfaces look tedious

In RPC, FunctionId and Function are required to be manually specified while calling RPC function. Why is that?

### Part I: why FunctionId is required?

We can't garantee that different template function type mapped to different address, in the fact icf=none (identical code folding) not working well with template functions. So we need to specify it manually, to avoid icf issue.

### Part II: Why FunctionId (int) not other types?

- Why not string
  - FunctionId is a strong type that can be checked at compile time. String can be spelled wrong.
- Why not use macro #FuncType to generate string
  - The same function type can be spelled differently, leading to different string which can not be checked in registry at compile time.
  - Macro as interface in C++ 17/20 is not cool.
- Why not typeid hashcode
  - identical template function code still generate same hashcode, leading to icf.
- Why not typeid name
  - typeid name is not guaranteed to generate different name for different types.

### Part III: Why both FunctionId and FunctionType are required to be specified in RPC call? Other RPC libraries only need a string.

By passing the function type, our RPC code can infer the function signature including parameters/return value types, and do type conversion automatically. This benefits a lot.
