#if (defined(_M_AMD64) || defined(_M_IX86) || defined(_M_X64)) || \
 (defined(__amd64__) || defined(__i386__) || defined(__x86_64__)) || \
 (defined(_X86_) || defined(__i386) || defined(__x86_64) || defined(__amd64))

#if defined(__GNUC__)

   #include <cpuid.h>

   #define cpuid(reg32, level) __cpuid(level, reg32[0], reg32[1], reg32[2], reg32[3])
   #define cpuidex(info, leaf, subleaf) __cpuid_count(leaf, subleaf, info[0], info[1], info[2], info[3])

#elif defined(_MSC_VER)

   #include <intrin.h>

   #define cpuid __cpuid
   #define cpuidex __cpuidex

#endif

#include <string.h>
#include <stdbool.h>

#include "x86_64.h"

/**
 * Based on: https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
 */

void InitExtensionsX86_X64(Isa_x86_x64* cpu) {
    // Calling cpuid with 0x0 as the function_id argument
    // gets the number of the highest valid function ID.
    enum { EAX, EBX, ECX, EDX };
    uint32_t reg32[4];

    cpu->isIntel = false;
    cpu->isAMD = false;
    cpu->f_1_ECX = 0;
    cpu->f_1_EDX = 0;
    cpu->f_7_EBX = 0;
    cpu->f_7_ECX = 0;
    cpu->f_81_ECX = 0;
    cpu->f_81_EDX = 0;

    cpuid(reg32, 0);
    uint32_t nIds = reg32[0];

    cpuidex(reg32, 0x00000000u + 0u, 0);
    memcpy(cpu->vendor, &reg32[EBX], sizeof(*reg32));
    memcpy(cpu->vendor + 4, &reg32[EDX], sizeof(*reg32));
    memcpy(cpu->vendor + 8, &reg32[ECX], sizeof(*reg32));

    cpu->vendor[12] = '\0';

    if (strcmp(cpu->vendor, "GenuineIntel") == 0) {
        cpu->isIntel = true;
    }
    else if (strcmp(cpu->vendor, "AuthenticAMD") == 0) {
        cpu->isAMD = true;
    }

    // load bitset with flags for function 0x00000001
    if (nIds >= 1) {
        cpuidex(reg32, 0x00000000u + 1u, 0);

        cpu->f_1_ECX = reg32[ECX];
        cpu->f_1_EDX = reg32[EDX];
    }

    // load bitset with flags for function 0x00000007
    if (nIds >= 7) {
        cpuidex(reg32, 0x00000000u + 7u, 0);

        cpu->f_7_EBX = reg32[EBX];
        cpu->f_7_ECX = reg32[ECX];
    }

    // Calling cpuid with 0x80000000 as the function_id argument
    // gets the number of the highest valid cpuended ID.
    cpuid(reg32, 0x80000000u);
    uint32_t nExIds = reg32[0];

    memset(cpu->brand, 0, sizeof(cpu->brand));

    // load bitset with flags for function 0x80000001
    if ((nExIds ^ 0x80000000u) >= 1) {
        cpuidex(reg32, 0x80000000u + 1u, 0);
        cpu->f_81_ECX = reg32[ECX];
        cpu->f_81_EDX = reg32[EDX];
    }

    // Interpret the CPU brand string if reported
    if ((nExIds ^ 0x80000000u) >= 4) {
        cpuidex(reg32, 0x80000000u + 2u, 0);
        memcpy(cpu->brand, reg32, sizeof(reg32));

        cpuidex(reg32, 0x80000000u + 3u, 0);
        memcpy(cpu->brand + 16, reg32, sizeof(reg32));

        cpuidex(reg32, 0x80000000u + 4u, 0);
        memcpy(cpu->brand + 32, reg32, sizeof(reg32));
    }
}

/**
 * Verify if the ISA is compatible with the one of `hosted`.
 *
 * Return COMPATIBLE if compatible or the incompatiblity
 * if they are not.
 */
CompatStatus CurrentIsaCompatiblityX86_x64(const Platform* hosted) {
   Isa_x86_x64 isa;
   utf8* start = hosted->isaExtensions;
   uint16_t i = 0, tot = 0;

   InitExtensionsX86_X64(&isa);

   if (((isa.isAMD && strcmp("AMD64", hosted->globalIsa) != 0)
      || (isa.isIntel && (strcmp("Intel 64", hosted->globalIsa) != 0)))
      && strcmp("x86", hosted->globalIsa) != 0) {

      return ISA_FAMILY_INCOMPAT;
   }

   for (; i < hosted->sizeIsaExt; ++i) {
      if (!IsExtensionOnCurrentIsa(&isa, start))
         return ISA_EXTENSIONS_INCOMPAT;

      tot += (uint16_t)strlen(start);
      start = &hosted->isaExtensions[tot + 1];
   }

   return COMPATIBLE;
}

#define SSE3(cpu) ((cpu).f_1_ECX == (0x1u << 0))

#define PCLMULQDQ(cpu) ((cpu).f_1_ECX == (0x1u << 1))

#define MONITOR(cpu) ((cpu).f_1_ECX == (0x1u << 3))

#define SSSE3(cpu) ((cpu).f_1_ECX == (0x1u << 9))

#define FMA(cpu) ((cpu).f_1_ECX == (0x1u << 12))

#define CMPXCHG16B(cpu) ((cpu).f_1_ECX == (0x1u << 13))

#define SSE41(cpu) ((cpu).f_1_ECX == (0x1u << 19))

#define SSE42(cpu) ((cpu).f_1_ECX == (0x1u << 20))

#define MOVBE(cpu) ((cpu).f_1_ECX == (0x1u << 22))

#define POPCNT(cpu) ((cpu).f_1_ECX == (0x1u << 23))

#define AES(cpu) ((cpu).f_1_ECX == (0x1u << 25))

#define XSAVE(cpu) ((cpu).f_1_ECX == (0x1u << 26))

#define OSXSAVE(cpu) ((cpu).f_1_ECX == (0x1u << 27))

#define AVX(cpu) ((cpu).f_1_ECX == (0x1u << 28))

#define F16C(cpu) ((cpu).f_1_ECX == (0x1u << 29))

#define RDRAND(cpu) ((cpu).f_1_ECX == (0x1u << 30))


#define MSR(cpu) ((cpu).f_1_EDX == (0x1u << 5))

#define CX8(cpu) ((cpu).f_1_EDX == (0x1u << 8))

#define SEP(cpu) ((cpu).f_1_EDX == (0x1u << 11))

#define CMOV(cpu) ((cpu).f_1_EDX == (0x1u << 15))

#define CLFSH(cpu) ((cpu).f_1_EDX == (0x1u << 19))

#define MMX(cpu) ((cpu).f_1_EDX == (0x1u << 23))

#define FXSR(cpu) ((cpu).f_1_EDX == (0x1u << 24))

#define SSE(cpu) ((cpu).f_1_EDX == (0x1u << 25))

#define SSE2(cpu) ((cpu).f_1_EDX == (0x1u << 26))


#define FSGSBASE(cpu) ((cpu).f_7_EBX == (0x1u << 0))

#define BMI1(cpu) ((cpu).f_7_EBX == (0x1u << 3))

#define HLE(cpu) ((cpu).isIntel && (cpu).f_7_EBX == (0x1u << 4))

#define AVX2(cpu) ((cpu).f_7_EBX == (0x1u << 5))

#define BMI2(cpu) ((cpu).f_7_EBX == (0x1u << 8))

#define ERMS(cpu) ((cpu).f_7_EBX == (0x1u << 9))

#define INVPCID(cpu) ((cpu).f_7_EBX == (0x1u << 10))

#define RTM(cpu) ((cpu).isIntel && (cpu).f_7_EBX == (0x1u << 11))

#define AVX512F(cpu) ((cpu).f_7_EBX == (0x1u << 16))

#define RDSEED(cpu) ((cpu).f_7_EBX == (0x1u << 18))

#define ADX(cpu) ((cpu).f_7_EBX == (0x1u << 19))

#define AVX512PF(cpu) ((cpu).f_7_EBX == (0x1u << 26))

#define AVX512ER(cpu) ((cpu).f_7_EBX == (0x1u << 27))

#define AVX512CD(cpu) ((cpu).f_7_EBX == (0x1u << 28))

#define SHA(cpu) ((cpu).f_7_EBX == (0x1u << 29))


#define PREFETCHWT1(cpu) ((cpu).f_7_ECX == (0x1u << 0))


#define LAHF(cpu) ((cpu).f_81_ECX == (0x1u << 0))

#define LZCNT(cpu) ((cpu).isIntel && (cpu).f_81_ECX == (0x1u << 5))

#define ABM(cpu) ((cpu).isAMD && (cpu).f_81_ECX == (0x1u << 5))

#define SSE4a(cpu) ((cpu).isAMD && (cpu).f_81_ECX == (0x1u << 6))

#define XOP(cpu) ((cpu).isAMD && (cpu).f_81_ECX == (0x1u << 11))

#define TBM(cpu) ((cpu).isAMD && (cpu).f_81_ECX == (0x1u << 21))


#define SYSCALL(cpu) ((cpu).isIntel && (cpu).f_81_EDX == (0x1u << 11))

#define MMXEXT(cpu) ((cpu).isAMD && (cpu).f_81_EDX == (0x1u << 22))

#define RDTSCP(cpu) ((cpu).isIntel && (cpu).f_81_EDX == (0x1u << 27))

#define _3DNOWEXT(cpu) ((cpu).isAMD && (cpu).f_81_EDX == (0x1u << 30))

#define _3DNOW(cpu) ((cpu).isAMD && (cpu).f_81_EDX == (0x1u << 31))


/**
 * Return true if the CPU `cpu` supports the extension `extension`.
 */
bool SupportExtensionX86_X64(const Isa_x86_x64* cpu, const utf8* extension) {
   /*static const char* ext[] = {
      // *
      "3DNOW!", "3DNOWEXT!",
      // A
      "ABM", "ADX", "AES", "AVX", "AVX2", "AVX512CD", "AVX512ER", "AVX512F", "AVX512PF",
      // B
      "BMI1", "BMI2",
      // C
      "CLFSH", "CMOV", "CMPXCHG16B", "CX8",
      // D
      // E
      "ERMS",
      // F
      "F16C", "FMA", "FSGSBASE", "FXSR",
      // G
      // H
      "HLE",
      // I
      "INVPCID",
      // J-K
      // L
      "LAHF", "LZCNT",
      // M
      "MMX", "MMXEXT", "MONITOR", "MOVBE", "MSR",
      // N
      // O
      "OSXSAVE",
      // P
      "PCLMULQDQ", "POPCNT", "PREFETCHWT1",
      // Q
      // R
      "RDRAND", "RDSEED", "RDTSCP", "RTM",
      // S
      "SEP", "SHA", "SSE", "SSE2", "SSE3", "SSE4.1", "SSE4.2", "SSE4a", "SSSE3", "SYSCALL",
      // T
      "TBM",
      // U - Y
      // X
      "XOP", "XSAVE"
   };*/

   if (strcmp("3DNOW!", extension) == 0)
      return _3DNOW(*cpu);
   if (strcmp("3DNOWEXT!", extension) == 0)
      return _3DNOWEXT(*cpu);
   if (strcmp("ABM", extension) == 0)
      return ABM(*cpu);
   if (strcmp("ADX", extension) == 0)
      return ADX(*cpu);
   if (strcmp("AES", extension) == 0)
      return AES(*cpu);
   if (strcmp("AVX", extension) == 0)
      return AVX(*cpu);
   if (strcmp("AVX2", extension) == 0)
      return AVX2(*cpu);
   if (strcmp("AVX512CD", extension) == 0)
      return AVX512CD(*cpu);
   if (strcmp("AVX512ER", extension) == 0)
      return AVX512ER(*cpu);
   if (strcmp("AVX512F", extension) == 0)
      return AVX512F(*cpu);
   if (strcmp("AVX512PF", extension) == 0)
      return AVX512PF(*cpu);
   if (strcmp("BMI1", extension) == 0)
      return BMI1(*cpu);
   if (strcmp("BMI2", extension) == 0)
      return BMI2(*cpu);
   if (strcmp("CLFSH", extension) == 0)
      return CLFSH(*cpu);
   if (strcmp("CMOV", extension) == 0)
      return CMOV(*cpu);
   if (strcmp("CMPXCHG16B", extension) == 0)
      return CMPXCHG16B(*cpu);
   if (strcmp("CX8", extension) == 0)
      return CX8(*cpu);
   if (strcmp("ERMS", extension) == 0)
      return ERMS(*cpu);
   if (strcmp("F16C", extension) == 0)
      return F16C(*cpu);
   if (strcmp("FMA", extension) == 0)
      return FMA(*cpu);
   if (strcmp("FSGSBASE", extension) == 0)
      return FSGSBASE(*cpu);
   if (strcmp("FXSR", extension) == 0)
      return FXSR(*cpu);
   if (strcmp("HLE", extension) == 0)
      return HLE(*cpu);
   if (strcmp("INVPCID", extension) == 0)
      return INVPCID(*cpu);
   if (strcmp("LAHF", extension) == 0)
      return LAHF(*cpu);
   if (strcmp("LZCNT", extension) == 0)
      return LZCNT(*cpu);
   if (strcmp("MMX", extension) == 0)
      return MMX(*cpu);
   if (strcmp("MMXEXT", extension) == 0)
      return MMXEXT(*cpu);
   if (strcmp("MONITOR", extension) == 0)
      return MONITOR(*cpu);
   if (strcmp("MOVBE", extension) == 0)
      return MOVBE(*cpu);
   if (strcmp("MSR", extension) == 0)
      return MSR(*cpu);
   if (strcmp("OSXSAVE", extension) == 0)
      return OSXSAVE(*cpu);
   if (strcmp("PCLMULQDQ", extension) == 0)
      return PCLMULQDQ(*cpu);
   if (strcmp("POPCNT", extension) == 0)
      return POPCNT(*cpu);
   if (strcmp("PREFETCHWT1", extension) == 0)
      return PREFETCHWT1(*cpu);
   if (strcmp("RDRAND", extension) == 0)
      return RDRAND(*cpu);
   if (strcmp("RDSEED", extension) == 0)
      return RDSEED(*cpu);
   if (strcmp("RDTSCP", extension) == 0)
      return RDTSCP(*cpu);
   if (strcmp("RTM", extension) == 0)
      return RTM(*cpu);
   if (strcmp("SEP", extension) == 0)
      return SEP(*cpu);
   if (strcmp("SHA", extension) == 0)
      return SHA(*cpu);
   if (strcmp("SSE", extension) == 0)
      return SSE(*cpu);
   if (strcmp("SSE2", extension) == 0)
      return SSE2(*cpu);
   if (strcmp("SSE3", extension) == 0)
      return SSE3(*cpu);
   if (strcmp("SSE4.1", extension) == 0)
      return SSE41(*cpu);
   if (strcmp("SSE4.2", extension) == 0)
      return SSE42(*cpu);
   if (strcmp("SSE4a", extension) == 0)
      return SSE4a(*cpu);
   if (strcmp("SSSE3", extension) == 0)
      return SSSE3(*cpu);
   if (strcmp("SYSCALL", extension) == 0)
      return SYSCALL(*cpu);
   if (strcmp("TBM", extension) == 0)
      return TBM(*cpu);
   if (strcmp("XOP", extension) == 0)
      return XOP(*cpu);
   if (strcmp("XSAVE", extension) == 0)
      return XSAVE(*cpu);
   return false;
}

#else

#define EMPTY_

#endif
