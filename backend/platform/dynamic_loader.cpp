#include "platform/dynamic_loader.h"
#include <vector>

namespace Platform {

DynamicLibrary::DynamicLibrary() : m_handle(nullptr) {
}

DynamicLibrary::~DynamicLibrary() {
    unload();
}

bool DynamicLibrary::load(const std::string& libraryName) {
    if (m_handle) {
        unload();
    }

    // Try different library name variations
    std::vector<std::string> namesToTry;
    
#ifdef PLATFORM_WINDOWS
    // Windows: visa64.dll, visa32.dll, visa.dll
    namesToTry.push_back(libraryName + ".dll");
    namesToTry.push_back(libraryName);
    if (libraryName == "visa64" || libraryName == "visa") {
        namesToTry.push_back("visa32.dll");
    }
#elif defined(PLATFORM_MACOS)
    // macOS: libvisa.dylib, libvisa.so (for compatibility)
    namesToTry.push_back("lib" + libraryName + ".dylib");
    namesToTry.push_back(libraryName + ".dylib");
    namesToTry.push_back("lib" + libraryName + ".so");
    namesToTry.push_back(libraryName);
#else
    // Linux: libvisa.so, libvisa.so.0
    namesToTry.push_back("lib" + libraryName + ".so");
    namesToTry.push_back(libraryName + ".so");
    namesToTry.push_back(libraryName);
    // Try with version suffix
    namesToTry.push_back("lib" + libraryName + ".so.0");
#endif

    for (const auto& name : namesToTry) {
#ifdef PLATFORM_WINDOWS
        m_handle = LoadLibraryA(name.c_str());
        if (m_handle) {
            m_libraryPath = name;
            return true;
        }
#else
        m_handle = dlopen(name.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (m_handle) {
            m_libraryPath = name;
            return true;
        }
#endif
    }

    // All attempts failed
#ifdef PLATFORM_WINDOWS
    DWORD error = GetLastError();
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Failed to load library: %s (Error code: %lu)", 
             libraryName.c_str(), error);
    m_lastError = buffer;
#else
    const char* error = dlerror();
    m_lastError = std::string("Failed to load library: ") + 
                  libraryName + " (" + (error ? error : "Unknown error") + ")";
#endif

    return false;
}

bool DynamicLibrary::unload() {
    if (!m_handle) {
        return true;
    }

#ifdef PLATFORM_WINDOWS
    BOOL result = FreeLibrary(m_handle);
    m_handle = nullptr;
    return result != 0;
#else
    int result = dlclose(m_handle);
    m_handle = nullptr;
    return result == 0;
#endif
}

void* DynamicLibrary::getFunctionAddress(const std::string& functionName) {
    if (!m_handle) {
        m_lastError = "Library not loaded";
        return nullptr;
    }

#ifdef PLATFORM_WINDOWS
    void* address = (void*)GetProcAddress(m_handle, functionName.c_str());
    if (!address) {
        m_lastError = "Function not found: " + functionName;
    }
#else
    // Clear any previous error
    dlerror();
    
    void* address = dlsym(m_handle, functionName.c_str());
    const char* error = dlerror();
    if (error) {
        m_lastError = std::string("Function not found: ") + functionName + 
                      " (" + error + ")";
        return nullptr;
    }
#endif

    return address;
}

bool DynamicLibrary::isLoaded() const {
    return m_handle != nullptr;
}

std::string DynamicLibrary::getLastError() const {
    return m_lastError;
}

} // namespace Platform
