# INT3 Breakpoint Detection at Entry Point

Detects software breakpoints at the program entry point by checking for INT3 opcode (0xCC) before debugger restoration occurs.

## Technical Background

x64dbg and other debuggers implement software breakpoints by patching the target address with the INT3 instruction (0xCC). When setting a breakpoint at the entry point, the debugger:

1. Reads and stores the original byte at the entry point address
2. Writes 0xCC to that address via `WriteProcessMemory` or direct memory modification
3. When 0xCC executes, the CPU generates exception 0x80000003 (STATUS_BREAKPOINT)
4. The debugger catches the exception through `WaitForDebugEvent`, decrements RIP/EIP by 1, restores the original byte, and optionally single-steps before re-applying the breakpoint

This implementation reads the entry point byte before the debugger can restore it, exposing the 0xCC patch.

## Why TLS Callbacks

TLS callbacks execute during process initialization, before the entry point. When you set a breakpoint at main/entry in x64dbg and run the process, the debugger patches 0xCC at that address immediately. The TLS callback runs before execution reaches the patched entry point, so the INT3 byte is already written but hasn't been hit yet - catching the debugger with its breakpoint exposed.

Without TLS, checking in main() would be too late since the debugger already hit the breakpoint, handled the exception, and potentially restored the original byte.

### TLS Callback Registration
TLS callback registered in `.CRT$XLB` section executes during `DLL_PROCESS_ATTACH` before the entry point. The linker places the callback pointer in the PE TLS directory through:
- x64: `/INCLUDE:_tls_used` and `/INCLUDE:TlsCallbackFunc` with `.CRT$XLB` const segment
- x86: `/INCLUDE:__tls_used` and `/INCLUDE:_TlsCallbackFunc` with `.CRT$XLB` data segment

# PoC:
<img width="752" height="18" alt="image" src="https://github.com/user-attachments/assets/064f4801-6e73-476f-92ca-7df3d361928e" />
<img width="264" height="144" alt="image" src="https://github.com/user-attachments/assets/2528f520-b9ea-4aa9-b33c-be7d9f59a107" />


## License
MIT
