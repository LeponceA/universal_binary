#ifndef X86_64_H_
#define X86_64_H_

#if (defined(_M_AMD64) || defined(_M_IX86) || defined(_M_X64)) || \
 (defined(__amd64__) || defined(__i386__) || defined(__x86_64__)) || \
 (defined(_X86_) || defined(__i386) || defined(__x86_64) || defined(__amd64))

#include <stdbool.h>

#include "../../compatibility.h"
#include "../../compat_test.h"

typedef struct Isa_x86_x64 {
    char vendor[20];
    char brand[0x40];

    bool isIntel;
    bool isAMD;

    uint32_t f_1_ECX;
    uint32_t f_1_EDX;
    uint32_t f_7_EBX;
    uint32_t f_7_ECX;
    uint32_t f_81_ECX;
    uint32_t f_81_EDX;
} Isa_x86_x64;

void InitExtensionsX86_X64(Isa_x86_x64* cpu);

bool SupportExtensionX86_X64(const Isa_x86_x64* cpu, const utf8* extension);

CompatStatus CurrentIsaCompatiblityX86_x64(const Platform* hosted);
 
   #define IsExtensionOnCurrentIsa SupportExtensionX86_X64
   #define CurrentIsaCompatiblity CurrentIsaCompatiblityX86_x64
   
#endif

#endif
