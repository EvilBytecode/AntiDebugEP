#include <windows.h>
#include <stdio.h>
#include <vector>

bool DebuggerDetected = false;

uintptr_t GetModuleEntryPoint(
    void* ModuleBase
)
{
    IMAGE_DOS_HEADER* DosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(ModuleBase);
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    IMAGE_NT_HEADERS* NtHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint8_t*>(ModuleBase) + DosHeader->e_lfanew);

    if (NtHeaders->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    return reinterpret_cast<uintptr_t>(ModuleBase) + NtHeaders->OptionalHeader.AddressOfEntryPoint;
}

bool CheckBreakpointAtAddress(
    uintptr_t Address
)
{
    if (!Address)
        return false;

    __try
    {
        uint8_t FirstByte = *reinterpret_cast<uint8_t*>(Address);

        if (FirstByte == 0xCC)
            return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return false;
}

void NTAPI TlsCallback(
    PVOID DllHandle,
    DWORD Reason,
    PVOID Reserved
)
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        void* ModuleBase = GetModuleHandle(NULL);
        uintptr_t EntryPoint = GetModuleEntryPoint(ModuleBase);

        if (CheckBreakpointAtAddress(EntryPoint))
        {
            DebuggerDetected = true;
            MessageBoxA(NULL, "Debugger detected at entry point!", "Detection", MB_OK | MB_ICONWARNING);
        }
    }
}

#ifdef _WIN64
#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:TlsCallbackFunc")
#else
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_TlsCallbackFunc")
#endif

#ifdef _WIN64
#pragma const_seg(".CRT$XLB")
EXTERN_C const
#else
#pragma data_seg(".CRT$XLB")
EXTERN_C
#endif
PIMAGE_TLS_CALLBACK TlsCallbackFunc = TlsCallback;
#ifdef _WIN64
#pragma const_seg()
#else
#pragma data_seg()
#endif

int main()
{
    void* ModuleBase = GetModuleHandle(NULL);
    uintptr_t EntryPoint = GetModuleEntryPoint(ModuleBase);

    printf("Module Base: 0x%p\n", ModuleBase);
    printf("Entry Point: 0x%p\n", (void*)EntryPoint);

    if (EntryPoint)
    {
        uint8_t* EntryBytes = reinterpret_cast<uint8_t*>(EntryPoint);
        printf("Entry Point Bytes: ");
        for (int i = 0; i < 16; i++)
        {
            printf("%02X ", EntryBytes[i]);
        }
        printf("\n");
    }

    if (DebuggerDetected)
    {
        printf("\nDebugger was detected during TLS callback!\n");
    }

    if (CheckBreakpointAtAddress(EntryPoint))
    {
        printf("\nINT3 breakpoint detected at entry point!\n");
    }
    else
    {
        printf("\nNo breakpoints detected.\n");
    }

    getchar();
    return 0;
}
