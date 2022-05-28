#if defined(_POSIX_VERSION) || defined(__linux__)

#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/utsname.h>

#include "posixcompatibility.h"
#include "../../compat_test.h"
#include "../../compatibility.h"
#include "../platforms.h"

static void GetProcessorName(utf8* name, char* machine);

/**
 * Extract the OS version from the string `release`
 * obtained by calling uname() and set it inside
 * `version`.
 *
 * Return true in case of success.
 */
static inline bool PosixLoadOsVersion(char* release, VersionMMP* version) {
   char additionalInfos[255];
   int n;

   version->majorVersion = 0;
   version->minorVersion = 0;
   version->patchVersion = 0;

   // Assume the following format : major.minor.patch(.[\d])?-additional infos

   if (sscanf(release, "%" PRIu32 ".%" PRIu32 ".%" PRIu32 "%n", &version->majorVersion
          , &version->minorVersion, &version->patchVersion, &n) != 3)
      return false;

   sscanf(&release[n], "%*[^-]-%254s", additionalInfos);
   // Remaining info are lost in this version (could be put in the extra info for instance)

   return true;
}

/**
 * Verify if the version in `hosted` is compatible
 * with the version of the current system.
 *
 * Return true if they are compatible.
 */
bool IsCurrentOsVersionCompatible(const VersionMMP* hosted) {
   static struct utsname infos;
   VersionMMP version;

   if (uname(&infos) != 0) {
      perror("uname()");
      return false;
   }

   PosixLoadOsVersion(infos.release, &version);
   return CompareVersions(&version, hosted);
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
   static Platform platform;
   static utf8 name[MAX_SIZE_INDIVIDUAL_FILE_NAME];

   static struct utsname infos;
   VersionMMP version;
   VersionMMP versionIsa = { 0, 0, 0};

   if (uname(&infos) != 0) {
      perror("uname()");
      return false;
   }


   PosixLoadOsVersion(infos.release, &version);
   GetProcessorName(name, infos.machine);
   InitPlatform(&platform, name, "", &versionIsa, infos.sysname, "", &version);

   *plat = platform;
   return true;
}

/**
 * Get the processor name obtained from uname() in
 * `machine` and store it in `name`.
 *
 * The size of `name` should be large enough to hold the
 * processor name.
 */
static void GetProcessorName(utf8* name, char* machine) {
   if (strcmp(machine, "x86_64") == 0) {
      Isa_x86_x64 isa;

      InitExtensionsX86_X64(&isa);

      if (isa.isAMD)
         strcpy(name, "AMD64");
      else if (isa.isIntel)
         strcpy(name, "INTEL 64");
      else {
         fputs("Unknown x86-64 processor type.\n", stderr);
         strcpy(name, "");
      }
   } else if (strcmp(machine, "i686") == 0) {
      strcpy(name, "x86");
   } else {
      fputs("Unknown ISA type.\n", stderr);
      strcpy(name, "");
   }
}

#else

#define EMPTY_

#endif
