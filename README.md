# CheatNG

CheatNG is a versatile framework that provides helpful tools for cheating in various OSes.

It is primarily designed to offer unified experience with powerful, modern, and user-friendly capabilities on all major OSes.

It is also designed to be extensible and customizable, so that it can be easily ported to other OSes and be used in different scenarios.

W.I.P

```
Architecture Portability

              ┌─────────────────────┐
              │             CheatNG GUI                  │
              └──┬───────────────┬──┘
                    │                              │
                    │                              │
┌─────────▼───┐    ┌────────▼─────────┐
│           imgui          │    │      Abstract OS Layer             │
├─────────────┤    ├──────┬─────┬─────┤
│         OpenGL/SDL       │    │  IProcess  │ IThread  │ IMemory  │
└─────────────┘    └──────┴─────┴─────┘
```

## Supported OS

- [x] Linux
- [ ] Android
- [ ] Windows
- [ ] Nintendo Switch
- [ ] ~~iOS/macOS~~ (No. F^ck Apple)

## Supported Featuers

| Feature | External | Internal | Note |
| ------- | -------- | -------- | -------- |
| Memory Editor | ✔ | 🛠️ |  |
| Module Explorer | ✔ | 🛠️ | |
| Memory Searcher | 🛠️ | 🛠️ | |
| Script Engine | 🛠️ | 🛠️ | Script Engine (most likely to be python) that perform automation outside and inside target process |
| Module Injector | 🛠️ | 🛠️ | Module injection in diverse bypass methods |
| Debugger | 🛠️ | 🛠️ | Traditional / Anti-traditional Debugger |

## Supported OS Interaction Implementation

Implementations are extensible under strategy pattern.

- [x] POSIX - process_vm_rw
- [ ] POSIX - /proc/\<pid\>/mem
- [ ] POSIX - ptrace
- [ ] POSIX - Kernel Module
- [ ] POSIX - Mem Injection
- [ ] Windows - RWProcessMemory / NtRWVirtualMemory
- [ ] Windows - Kernel Driver
- [ ] Nintendo Switch - Debugging
