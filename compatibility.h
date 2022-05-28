#ifndef COMPATIBILITY_H__
#define COMPATIBILITY_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef char utf8;

bool IsValidUTF8(const utf8* string, size_t size);

#if (defined(__GNUC__) || defined(__CLANG__))
   #define SWAP_FCT_16(v) __builtin_bswap16(v)
   #define SWAP_FCT_32(v) __builtin_bswap32(v)
   #define SWAP_FCT_64(v) __builtin_bswap64(v)
#elif (defined(_MSC_VER))
   #define SWAP_FCT_16(v) _byteswap_ushort(v)
   #define SWAP_FCT_32(v) _byteswap_ulong(v)
   #define SWAP_FCT_64(v) _byteswap_uint64(v)
#else
   #define SWAP_FCT_16(v) ((((v) & 0x0FFu) << 8) | (((v) & 0xFF00u) >> 8))
   #define SWAP_FCT_32(v) ((((v) & 0xFF000000u) >> 24) | (((v) & 0x00FF0000u) >> 8) | (((v) & 0x0000FF00u) << 8) | (((v) & 0x000000FFu) << 24))
   #define SWAP_FCT_64(v) ((((v) & 0xFF00000000000000ull) >> 56) | ((((v) & 0x00FF000000000000ull) >> 40))  \
                          |((((v) & 0x0000FF0000000000ull) >> 24)) | ((((v) & 0x000000FF00000000ull) >> 8)) \
                          |(((v) & 0x00000000000000FFull) << 56) | ((((v) & 0x00000000000000FF00ull) << 40))\
                          |((((v) & 0x0000000000FF0000ull) << 24)) | ((((v) & 0x00000000FF000000ull) << 8)))
#endif

#define IS_LITTLEENDIAN ((*(uint8_t*)&((uint32_t){1})) == 1)

#define TO_LITTLE_ENDIAN8(v) (v)
#define TO_LITTLE_ENDIAN16(v) (IS_LITTLEENDIAN ? (v) : SWAP_FCT_16(v))
#define TO_LITTLE_ENDIAN32(v) (IS_LITTLEENDIAN ? (v) : SWAP_FCT_32(v))
#define TO_LITTLE_ENDIAN64(v) (IS_LITTLEENDIAN ? (v) : SWAP_FCT_64(v))

#define TO_LOCAL_ENDIAN8(v) (v)
#define TO_LOCAL_ENDIAN16(v) (IS_LITTLEENDIAN ? (v) : SWAP_FCT_16(v))
#define TO_LOCAL_ENDIAN32(v) (IS_LITTLEENDIAN ? (v) : SWAP_FCT_32(v))
#define TO_LOCAL_ENDIAN64(v) (IS_LITTLEENDIAN ? (v) : SWAP_FCT_64(v))

#define DBG_PRINT printf

#define VERIFY_FWRITE(call, val) if ((call) != (val)) { \
                                    perror(__func__); \
                                    return false; \
                                 }

#define VERIFY_FREAD(call, val) VERIFY_FWRITE((call), (val))

#define perror(x) do { fprintf(stderr, " > %s::%s::%d\n <",__FILE__, __func__, __LINE__);} while(0)

// Maximal length in bytes of the individual name of a
// file. Decrease this value down to 510 is probably
// sufficient but we ketp 65535 to stick more to the
// real limit in the universal binary format.
#define MAX_SIZE_INDIVIDUAL_FILE_NAME 65535

#endif
