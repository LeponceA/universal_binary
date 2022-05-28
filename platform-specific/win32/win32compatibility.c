#ifdef _WIN32

#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <direct.h>

#include "win32compatibility.h"

#include "../platforms.h"
#include "../../compatibility.h"

/**
 * Return if the version `version` is compatible with
 * the current system version.
 */
bool IsCurrentOsVersionCompatible(const VersionMMP* version) {
   OSVERSIONINFOEX osvi;
   DWORDLONG dwlConditionMask = 0;
   int op = VER_GREATER_EQUAL;

   // Initialize the OSVERSIONINFOEX structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   osvi.dwMajorVersion = version->majorVersion;
   osvi.dwMinorVersion = version->minorVersion;;
   osvi.wServicePackMajor = 0;
   osvi.wServicePackMinor = 0;

   osvi.dwBuildNumber = version->patchVersion;

   // Initialize the condition mask.

   VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, op);

   VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
   VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);
   VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, op);
   VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMINOR, op);

   // Perform the test.

   return VerifyVersionInfo(
     &osvi,
     VER_MAJORVERSION | VER_MINORVERSION |
     VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
     dwlConditionMask);
}

/**
 * Get the processor name.
 */
static void GetProcessorName(utf8* name) {
   SYSTEM_INFO info;
   Isa_x86_x64 isa;

   GetSystemInfo(&info);

   switch(info.wProcessorArchitecture) {
      
      case PROCESSOR_ARCHITECTURE_AMD64:
         InitExtensionsX86_X64(&isa);
         strcpy(name, (isa.isIntel ? "INTEL 64" : "AMD64"));
         break;
      
      #ifdef PROCESSOR_ARCHITECTURE_ARM64
      case PROCESSOR_ARCHITECTURE_ARM64:
         strcpy(name, "ARM64");
         break;
      #endif
      case PROCESSOR_ARCHITECTURE_ARM:
         strcpy(name, "ARM");
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         strcpy(name, "IA-64");
         break;
      case PROCESSOR_ARCHITECTURE_INTEL:
         strcpy(name, "x86");
         break;
      default:
         strcpy(name, "");
   }
}

/**
 * Set the platform with the local platform specifics
 * and store them into `plat`.
 *
 * Strings do not need to be freed and they cannot be
 * modified. Processors extensions are not taken into
 * account.
 *
 * Return true on success.
 */
bool SetWithLocalPlatform(Platform plat[static 1]) {
   static char isaName[MAX_SIZE_INDIVIDUAL_FILE_NAME];

   // https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversion
   // @TODO: update for newer OS versions + Service Pack
   VersionMMP version;
   VersionMMP versionIsa = { 0, 0, 0};

   DWORD dwVersion = 0;

   dwVersion = GetVersion();

   // Get the Windows version.

   version.majorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
   version.minorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

   // Get the build number.

   if (dwVersion < 0x80000000)
      version.patchVersion = (DWORD)(HIWORD(dwVersion));
   else
      version.patchVersion = 0;

   GetProcessorName(isaName);

   InitPlatform(plat, isaName, "", &versionIsa, "Windows NT", "", &version);

   return true;
}

#else

#define EMPTY_

#endif
