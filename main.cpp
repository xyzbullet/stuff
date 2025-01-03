#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <TlHelp32.h>
#include <filesystem>

#define LUA_VM_LOAD_ADDRESS 0xaf18a0
#define LUA_PUSHVFSTRING_ADDRESS 0x2efc312
#define TASK_SPAWN_ADDRESS 0xe42830
#define GET_LUA_STATE_ADDRESS 0xd35d80

// Function pointer signatures (These will be used to interact with the Lua VM)
typedef void(__fastcall* LuaVMLoad)(uintptr_t L, const char* source, const char* chunk_name, int env);
typedef uintptr_t(__fastcall* LuaPushVFString)(uintptr_t L, const char* fmt, va_list argp);
typedef uintptr_t(__fastcall* TaskSpawn)(uintptr_t L);

// Function to get the process ID by process name
DWORD GetProcessIdByName(const std::wstring& processName) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Take a snapshot of all processes in the system
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to take process snapshot!" << std::endl;
        return 0;
    }

    // Iterate through all processes
    if (Process32First(hSnapshot, &pe32)) {
        do {
            // Compare the process name (with the correct file extension)
            if (pe32.szExeFile == processName) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

// Helper function to read memory from the process
uintptr_t ReadMemory(HANDLE hProcess, uintptr_t address) {
    uintptr_t value;
    SIZE_T bytesRead;
    ReadProcessMemory(hProcess, reinterpret_cast<void*>(address), &value, sizeof(value), &bytesRead);
    return value;
}

// Helper function to write memory into the process
void WriteMemory(HANDLE hProcess, uintptr_t address, const void* buffer, SIZE_T size) {
    SIZE_T bytesWritten;
    WriteProcessMemory(hProcess, reinterpret_cast<void*>(address), buffer, size, &bytesWritten);
}

// Inject Lua script into the target process
void InjectLuaScript(HANDLE hProcess, uintptr_t luaStateAddress, const std::string& script) {
    LuaVMLoad luaVMLoad = reinterpret_cast<LuaVMLoad>(LUA_VM_LOAD_ADDRESS);
    luaVMLoad(luaStateAddress, script.c_str(), "InjectedChunk", 0);
    std::cout << "Injected Lua script: " << script << std::endl;
}

// Execute Lua bytecode in the target process
void ExecuteLuaBytecode(HANDLE hProcess, uintptr_t luaStateAddress, const std::string& bytecode) {
    LuaPushVFString luaPushVFString = reinterpret_cast<LuaPushVFString>(LUA_PUSHVFSTRING_ADDRESS);
    va_list args;
    uintptr_t result = luaPushVFString(luaStateAddress, bytecode.c_str(), args);
    std::cout << "Executed Lua bytecode, result: " << result << std::endl;
}

// Lua console for interactive Lua commands
void RunLuaConsole(HANDLE hProcess, uintptr_t luaStateAddress) {
    std::string input;
    std::cout << "> ";
    while (std::getline(std::cin, input)) {
        if (input == "exit") break;
        InjectLuaScript(hProcess, luaStateAddress, input);
        ExecuteLuaBytecode(hProcess, luaStateAddress, input);
        std::cout << "> ";
    }
}

// Function to initialize Lua environment by injecting init.lua script
void InitializeLuaEnvironment(HANDLE hProcess, uintptr_t luaStateAddress) {
    // Load init.lua from the executable's resource section
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_INIT_LUA), L"RCDATA");
    if (hRes == NULL) {
        std::cerr << "Failed to find init.lua resource!" << std::endl;
        return;
    }

    // Load the resource into memory
    HGLOBAL hResData = LoadResource(NULL, hRes);
    if (hResData == NULL) {
        std::cerr << "Failed to load init.lua resource!" << std::endl;
        return;
    }

    // Get a pointer to the resource data
    LPVOID pResData = LockResource(hResData);
    DWORD dwSize = SizeofResource(NULL, hRes);
    
    // Convert resource data to string
    std::string initLuaScript(static_cast<char*>(pResData), dwSize);

    // Inject the init.lua script into the target process
    InjectLuaScript(hProcess, luaStateAddress, initLuaScript);
}

// Main function to execute Lua code in the game process
int main() {
    // Get the process ID for Roblox
    DWORD processID = GetProcessIdByName(L"RobloxPlayerBeta.exe"); // Change this to the correct executable name
    if (processID == 0) {
        std::cerr << "Failed to find process!" << std::endl;
        return 1;
    }

    // Open the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process!" << std::endl;
        return 1;
    }

    // Read the Lua state address from the game process
    uintptr_t luaStateAddress = ReadMemory(hProcess, GET_LUA_STATE_ADDRESS);

    // Initialize Lua environment by injecting the init.lua script
    InitializeLuaEnvironment(hProcess, luaStateAddress);

    // Launch Lua console for interactive execution
    RunLuaConsole(hProcess, luaStateAddress);

    return 0;
}
