#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #define PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #define PLATFORM_LINUX
    #define PLATFORM_NAME "Linux"
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
    #define PLATFORM_NAME "macOS"
#else
    #define PLATFORM_UNKNOWN
    #define PLATFORM_NAME "Unknown"
    #warning "Unsupported platform detected"
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define COMPILER_MSVC
#elif defined(__GNUC__)
    #define COMPILER_GCC
#elif defined(__clang__)
    #define COMPILER_CLANG
#endif

// Architecture detection
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
    #define ARCH_X64
    #define ARCH_NAME "x64"
#elif defined(_M_IX86) || defined(__i386__)
    #define ARCH_X86
    #define ARCH_NAME "x86"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ARCH_ARM64
    #define ARCH_NAME "ARM64"
#endif

// Export/Import macros for shared libraries (future use)
#ifdef PLATFORM_WINDOWS
    #ifdef BUILD_SHARED_LIBS
        #define INSTRUMENT_API __declspec(dllexport)
    #else
        #define INSTRUMENT_API
    #endif
#else
    #define INSTRUMENT_API __attribute__((visibility("default")))
#endif

// Path separator
#ifdef PLATFORM_WINDOWS
    #define PATH_SEPARATOR "\\"
#else
    #define PATH_SEPARATOR "/"
#endif

// Library extension
#ifdef PLATFORM_WINDOWS
    #define LIB_EXTENSION ".dll"
#elif defined(PLATFORM_MACOS)
    #define LIB_EXTENSION ".dylib"
#else
    #define LIB_EXTENSION ".so"
#endif

#endif // PLATFORM_CONFIG_H
