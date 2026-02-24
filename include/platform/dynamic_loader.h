#ifndef DYNAMIC_LOADER_H
#define DYNAMIC_LOADER_H

#include "platform_config.h"
#include <string>

namespace Platform {

// Platform-agnostic library handle
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    typedef HMODULE LibraryHandle;
#else
    #include <dlfcn.h>
    typedef void* LibraryHandle;
#endif

class DynamicLibrary {
public:
    DynamicLibrary();
    ~DynamicLibrary();

    // Load/unload library
    bool load(const std::string& libraryName);
    bool unload();

    // Get function pointer
    template<typename T>
    bool getFunction(const std::string& functionName, T& functionPtr);

    // Status
    bool isLoaded() const;
    std::string getLastError() const;

private:
    LibraryHandle m_handle;
    std::string m_lastError;
    std::string m_libraryPath;

    void* getFunctionAddress(const std::string& functionName);
};

// Template implementation
template<typename T>
bool DynamicLibrary::getFunction(const std::string& functionName, T& functionPtr) {
    void* address = getFunctionAddress(functionName);
    if (!address) {
        return false;
    }
    functionPtr = reinterpret_cast<T>(address);
    return true;
}

} // namespace Platform

#endif // DYNAMIC_LOADER_H
