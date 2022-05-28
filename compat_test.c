#include <string.h>
#include <stdint.h>

#include "compat_test.h"

#include "platform-specific/platforms.h"

/**
 * Compare the host version (i.e., our system) `host`
 * with the one from the universal binary, `hosted`.
 *
 * Return true if `host` is compatible with `hosted`.
 */
bool CompareVersions(const VersionMMP* host, const VersionMMP* hosted) {
   int i;

   uint32_t hostVer[3] = { host->majorVersion,
                           host->minorVersion,
                           host->patchVersion };

   uint32_t hostedVer[3] = { hosted->majorVersion,
                           hosted->minorVersion,
                           hosted->patchVersion };

   for (i = 0; i < 3 && hostVer[i] == hostedVer[i]; ++i);

   return (i == 3 || hostVer[i] > hostedVer[i]);
}

static inline bool CompareISA(const Platform* host, const Platform* hosted) {

   if (strcmp(host->globalIsa, hosted->globalIsa) != 0)
      return false;

   // TODO: string cmp
}



// IA-64 = Itanium ; Intel 64/EM64T ; AMD64
// https://fr.wikipedia.org/wiki/Intel_64

/**
 * Verify if `host` is compatible with `hosted`
 * and return the compatibility status.
 */
CompatStatus VerifyCompatibility(const Platform* host, const Platform* hosted) {
   // @TODO: extensions de l'ISA + extra + isa version

   if (host == NULL) { // Compare with host
      if (strcmp(CURRENT_OS_NAME, hosted->osFamily) != 0)
         return OS_FAMILY_INCOMPAT;

      if (!IsCurrentOsVersionCompatible(&hosted->osVersion))
         return OS_VERSION_INCOMPAT;

      return CurrentIsaCompatiblity(hosted);
   } else {
      if (strcmp(host->osFamily, hosted->osFamily) != 0)
         return OS_FAMILY_INCOMPAT;

      if (!CompareVersions(&host->osVersion, &hosted->osVersion))
         return OS_VERSION_INCOMPAT;

      if (strcmp(host->globalIsa, hosted->globalIsa) != 0)
         return ISA_FAMILY_INCOMPAT;

      if (!CompareVersions(&host->isaVersion, &hosted->isaVersion))
         return ISA_VERSION_INCOMPAT;
   }



   return COMPATIBLE;
}
