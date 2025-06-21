# Debugger
Project: Linux Debugger in C (using ptrace)
Implemented a simple Linux debugger in C/C++ using ptrace. Supports breakpoints, single-stepping, backtrace generation, and mapping instruction addresses to source lines and functions. Includes a CLI with commands like add breakpoint, nextstep, continue, and backtrace.

*********************************************************************
🐞 Custom Linux Debugger (C/C++, ptrace)
A simple Linux debugger written in C/C++, using the ptrace system call to control and inspect a debuggee process.

🔧 Features
Set and manage breakpoints (via INT 3 / 0xCC injection)

Step through instructions (PTRACE_SINGLESTEP)

View backtrace using rbp-based stack unwinding

Resolve source lines and function names with addr2line

CLI with commands like:

break <function|address>

next – step forward one instruction

continue – resume execution

backtrace – print stack trace

print – inspect register or variable values

🧠 Tech Stack
C/C++ with std::map and other STL features

Linux system programming (ptrace, waitpid)

ELF parsing and DWARF debug symbols (addr2line)

Compiled with: -g -O0 -no-pie

![image](https://github.com/user-attachments/assets/c74c7210-a8a5-4829-a622-8cd50bcda8e4)

![image](https://github.com/user-attachments/assets/a51afdf5-b047-42c0-9106-01428386b4d8)
