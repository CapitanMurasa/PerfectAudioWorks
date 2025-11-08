#ifndef CROSS_PLATFORM_DEFS_H
#define CROSS_PLATFORM_DEFS_H

#include <stdint.h>

// --- Windows Specific Definitions (MSVC) ---

#ifdef _WIN32

    // Windows platforms require explicit includes for network/endian functions.
    // However, since some functions like ntohl/htonl are implemented in WS2_32.lib,
    // we only need the include if we want the inline/macro definitions.
    // For simplicity, we define the missing POSIX functions here:

#include <string.h>
#include <winsock2.h> // Provides ntohl (Network to Host Long)

// 1. strncasecmp alias
// POSIX strncasecmp is equivalent to MSVC's _strnicmp.
#define strncasecmp _strnicmp

// 2. Endianness definitions for le32toh
// Windows on x86/x64 is Little Endian (LE).
#ifndef __BYTE_ORDER
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif

// le32toh (Little-Endian 32-bit to Host 32-bit)
// On a Little-Endian host (Windows), this is an identity operation.
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le32toh(x) ((uint32_t)(x))
#elif __BYTE_ORDER == __BIG_ENDIAN
    // If the host was Big-Endian, we would use byte swapping here.
#define le32toh(x) (uint32_t)__builtin_bswap32(x) 
#else
#error "Unknown endianness. Cannot define le32toh."
#endif

#else 
    // --- POSIX/Other OS Definitions ---
    // These functions are usually provided by system headers like <endian.h> and <strings.h>
    // We expect the native toolchain to handle these symbols correctly.
    // Including <endian.h> explicitly often helps for le32toh, but we rely on system
    // headers being included implicitly or explicitly in the C files using them.

#endif

#endif // CROSS_PLATFORM_DEFS_H