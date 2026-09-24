#include <Windows.h>
#include <format>

#include "NL\Imports.hpp"
#include "NL\resource.h"

/*
    https://yougame.biz/threads/391266/

    base address: 0x212C3300000
    image size: 0x5001000 (~80MB)
    DllMain @ 0x212C82F1690
*/

LPVOID LoadBinary(HMODULE Module, void* Address) 
{
    std::printf("[~] loading binary...\n");

    auto FindedResource = FindResourceA(Module, MAKEINTRESOURCE(IDR_BINARY2), "BINARY");
    if (!FindedResource)
    {
        std::printf("[-] failed to locate binary\n");
        return nullptr;
    }

	auto ResourceData = LoadResource(Module, FindedResource);
    if (!ResourceData)
    {
        std::printf("[-] failed to load binary\n");
        return nullptr;
    }

	auto LockedResource = LockResource(ResourceData);
    if (!LockedResource)
    {
        std::printf("[-] failed to lock binary resource\n");
        return nullptr;
    }

	auto ResourceSize = SizeofResource(Module, FindedResource);
    if (ResourceSize > 0)
        std::printf("[+] loaded binary size: %d\n", (int)ResourceSize);
    else 
    {
        std::printf("[+] binary size 0 :(\n");
        return nullptr;
    }

    auto AllocatedBase = VirtualAlloc(Address, ResourceSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!AllocatedBase)
    {
        std::printf("[-] failed to allocate binary at %p\n", Address);
        return nullptr;
    }

//    if (!WriteProcessMemory(reinterpret_cast<HANDLE>(-1), AllocatedBase, LockedResource, ResourceSize, 0))
//    {
//#if _DEBUG
//        std::printf("[-] failed to write binary at %p\n", AllocatedBase);
//#endif
//        return nullptr;
//    }

    std::memcpy(AllocatedBase, LockedResource, ResourceSize);

    std::printf("\n");
    std::printf("[+] binary allocated at %p\n", AllocatedBase);
    std::printf("\n");

    return AllocatedBase;
}

BOOL FixImports(LPVOID BinaryAddress, void* ImportsAddress)
{
    //auto DeltaAddress = static_cast<intptr_t>(reinterpret_cast<uintptr_t>(BinaryAddress) - reinterpret_cast<uintptr_t>(ImportsAddress));

    std::printf("[~] fixing imports at %d...\n", (int)ImportsList.size());

    for (auto& Current : ImportsList)
    {
        std::printf("\n");

        auto ModuleName = std::get<1>(Current).c_str();
        auto Module = LoadLibraryA(ModuleName);
        if (!Module) 
        {
            std::printf("[-] cannot fix import module: %s\n", ModuleName);
            continue;
        }

        auto FunctionName = std::get<2>(Current).c_str();
        auto Function = reinterpret_cast<LONG_PTR>(GetProcAddress(Module, FunctionName));
        if (!Function)
        {
            std::printf("[-] cannot fix import address: %s\n", FunctionName);
            continue;
        }

        //auto FunctionAddress = std::get<0>(Current);
        //auto FunctionAddressWithDelta = reinterpret_cast<PVOID>(FunctionAddress + DeltaAddress);
        auto RVA = std::get<0>(Current) - reinterpret_cast<uintptr_t>(ImportsAddress);
        auto FunctionAddress = reinterpret_cast<PVOID>(reinterpret_cast<uintptr_t>(BinaryAddress) + RVA);

        std::printf("[~] import rva %llX, import address %p\n", static_cast<unsigned long long>(RVA), FunctionAddress);

        auto OldProtect = (DWORD)0;
        if (VirtualProtect(FunctionAddress, sizeof(LONG_PTR), PAGE_EXECUTE_READWRITE, &OldProtect)) 
        {
            InterlockedExchangePointer(reinterpret_cast<PVOID*>(FunctionAddress), reinterpret_cast<PVOID>(Function));
            VirtualProtect(FunctionAddress, sizeof(LONG_PTR), OldProtect, &OldProtect);
            
            std::printf("[+] fixed import: %s in %s at %p\n", FunctionName, ModuleName, FunctionAddress);
        }
        else
            std::printf("[-] failed to fix import at %p :(\n", FunctionAddress);
    }

    std::printf("\n");
    std::printf("[+] imports fixed :)\n");

    return TRUE;
}

VOID CallEntryPoint(LPVOID BinaryAddress, void* EntryPointAddress) 
{
    //// musor ebaniy
//    auto EntryAddress = static_cast<intptr_t>(auto ActualBase = reinterpret_cast<uintptr_t>(BinaryAddress); - reinterpret_cast<uintptr_t>(EntryPointAddress));
//
//    using DllEntry_t = BOOL(WINAPI*)(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
//    auto DllMain = reinterpret_cast<DllEntry_t>(EntryAddress);
//
//#if _DEBUG
//    std::printf("[~] calling entrypoint at 0x%llX...\n", EntryAddress);
//#endif
//
//    DllMain(reinterpret_cast<HMODULE>(BinaryAddress), DLL_PROCESS_ATTACH, nullptr);

//    using DllEntry_t = BOOL(WINAPI*)(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
//    auto DllMain = reinterpret_cast<DllEntry_t>(EntryPointAddress);
//
//#if _DEBUG
//    std::printf("[~] calling entrypoint at %p...\n", EntryPointAddress);
//#endif
//
//    DllMain(reinterpret_cast<HMODULE>(BinaryAddress), DLL_PROCESS_ATTACH, nullptr);

    std::printf("\n");
    std::printf("[~] entry nl: %p\n", EntryPointAddress);

    auto ThreadId = (DWORD)0;
    auto Thread = CreateThread(0, 0x40000, (LPTHREAD_START_ROUTINE)EntryPointAddress, 0, CREATE_SUSPENDED, &ThreadId);
    if (Thread) 
        std::printf("[+] entry thread at %08X\n", ThreadId);
    else
        std::printf("[-] failed to create nl thread\n");

    if (ResumeThread(Thread) == (DWORD)-1)     
        std::printf("[-] failed to resume nl thread\n");

    if (WaitForSingleObject(Thread, INFINITE) == WAIT_OBJECT_0) 
    {
        auto ExitCode = (DWORD)0;
        if (GetExitCodeThread(Thread, &ExitCode))
            std::printf("[+] entry returned: %08X\n", ExitCode);
        else
            std::printf("[-] failed to get nl thread exit code\n");
    } 
    else 
        std::printf("[-] failed to get nl thread exit code\n");

    CloseHandle(Thread);
}

static std::string GetCompileDateTime() {

    auto DateStr = (std::string)__DATE__;
    auto TimeStr = (std::string)__TIME__;
    
    auto RemoveShit = DateStr.find(',');
    if (RemoveShit != std::string::npos)
        DateStr.erase(RemoveShit, 1);
    
    return DateStr + " " + TimeStr;
}

LONG WINAPI ExceptionHandler(_EXCEPTION_POINTERS* exceptionInfo) 
{
    auto* ExceptionRecord = exceptionInfo->ExceptionRecord;
    if (ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        static const char ErrorFormat[] =
            "Neverlose CS2 has been stopped due to a fatal error.\n\n"
            "If this is the first time you've seen this error message, restart\n"
            "Neverlose. If this message appears again, follow these steps:\n\n"
            "Disable any newly downloaded Lua scripts. If problems continue, try\n"
            "to use another config.\n\n"
            "If nothing helps, please contact Neverlose Support via tickets section.\n\n"
            "Technical information:\n"
            "Build type: 0 (built on %s)\n"
            "Error code: 0x%08X";

        char ErrorBuffer[1024];
        auto CompileInfo = GetCompileDateTime();
        std::sprintf(ErrorBuffer, ErrorFormat, CompileInfo.c_str(), ExceptionRecord->ExceptionCode);

        MessageBoxA(nullptr, ErrorBuffer, "Neverlose has crashed!", MB_ICONERROR | MB_TOPMOST);

        return EXCEPTION_EXECUTE_HANDLER;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

DWORD WINAPI DllThread(LPVOID Parameter)
{
    AddVectoredExceptionHandler(1, ExceptionHandler);

    auto TGLink = "https://t.me/RevolveTeam";

#if _DEBUG
    if (AllocConsole()) 
    {
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONIN$", "r", stdin);

        SetConsoleTitleA(TGLink);
    }
#endif
  
    auto BinaryAddress = LoadBinary(reinterpret_cast<HMODULE>(Parameter), reinterpret_cast<void*>(0x212C3300000));
    if (!BinaryAddress)
        return FALSE;
    
    while (!GetModuleHandleA("navsystem.dll"))
        Sleep(1000);

    if (!FixImports(BinaryAddress, reinterpret_cast<void*>(0xC4290000)))
        return FALSE;

    CallEntryPoint(BinaryAddress, reinterpret_cast<void*>(/*0xC82F1690*/0x212C82F1690));

#if _DEBUG
    for (auto i = 0; i < 5; i++) 
    {
        if (i == 0)
            std::printf("\n");

        std::printf("[!] %s\n", TGLink);
    }
#endif

    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        auto Thread = CreateThread(nullptr, 0, DllThread, hModule, 0, nullptr);
        if (Thread)
            CloseHandle(Thread);
    }

    return TRUE;
}