#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "platform.h"
#include "platform-specific/platforms.h"
#include "platform-specific/filemanip.h"
#include "packages_table.h"

static bool AppendPlatform(FILE* fout, const Platform* plat, const utf8* packagesPaths[], PackagesTable* pkgTab);

/**
 * Write the `nbPlatforms` platforms in `platforms` into
 * `fout` using the packages table `pkgTab` for the packages
 * listed in `packagesPaths`.
 *
 * Return true if succesful.
 */
bool WritePlatforms(FILE* fout, const utf8** packagesPaths[], PackagesTable* pkgTab, Platform platforms[], uint16_t nbPlatforms) {
   nbPlatforms = TO_LITTLE_ENDIAN16(nbPlatforms);
   VERIFY_FWRITE(fwrite(&nbPlatforms, sizeof(nbPlatforms), 1, fout), 1);

   nbPlatforms = TO_LOCAL_ENDIAN16(nbPlatforms);
   for (uint16_t i = 0; i < nbPlatforms; ++i) {
      if (!AppendPlatform(fout, &platforms[i], packagesPaths[i], pkgTab))
         return false;
   }

   return true;
}

/**
 * Locate a compatible platform with `current` in `fin` and
 * store it in `match`.
 *
 * Go through the `nbPlatforms` before giving its answer
 * (i.e., stop before the packages table).
 *
 * Return true if successful.
 */
bool LocateCompatiblePlatform(FILE* fin, const Platform* current, uint16_t nbPlatforms, Platform* match) {
   if (current != NULL)
      PrintPlatform(current);

   for (uint16_t i = nbPlatforms; i > 0; --i) {
      printf("Load %u/%u\n", nbPlatforms - i, nbPlatforms);fflush(stdout);
      if (!LoadPlatform(fin, match)) {
         FreeLoadedPlatform(match);
         return false;
      }

      printf("\n\nPlatform available found:\n");
      PrintPlatform(match);

      CompatStatus s;
      if ((s = VerifyCompatibility(current, match)) == COMPATIBLE) {
         return true;
      }
      
      printf("Compatibility = %d\n", s);

      FreeLoadedPlatform(match);

   }

   fputs("No compatible platforms found in the universal binary.\n", stderr);
   return false;
}


/**
 * Init the platform `plat` fields with the eponym
 * parameters.
 *
 * The UTF-8 strings are referenced directly by address
 * and not duplicated.
 *
 * The platform integer are set to be little endian
 * (call TO_LOCAL_ENDIAN*() to convert them back
 * to the local endian).
 *
 * Return true.
 */
bool InitPlatform(Platform* plat, utf8* globalIsa, utf8* isaExtensions, VersionMMP* isaVersion, utf8* osFamily, utf8* osExtra, VersionMMP* osVersion) {
   #define ASSIGN_SIZE_WITH_OVERFLOW_CHECK(var, name, type) { \
            size_t sz = strlen(name); \
            if (sz > ((type)~(type)0)) { \
               fprintf(stderr, "<%s> length is too long (the maximal length for this field is %u).\n", name, (type)~0); \
               return false; \
            } \
            \
            var = (type)sz; \
         }

   ASSIGN_SIZE_WITH_OVERFLOW_CHECK(plat->sizeIsaName, globalIsa, uint8_t);
   plat->sizeIsaName = TO_LITTLE_ENDIAN8(plat->sizeIsaName);

   ASSIGN_SIZE_WITH_OVERFLOW_CHECK(plat->sizeOsName, osFamily, uint8_t);
   plat->sizeOsName = TO_LITTLE_ENDIAN8(plat->sizeOsName);

   ASSIGN_SIZE_WITH_OVERFLOW_CHECK(plat->sizeIsaExt, isaExtensions, uint16_t);
   plat->sizeIsaExt = TO_LITTLE_ENDIAN16(plat->sizeIsaExt);

   ASSIGN_SIZE_WITH_OVERFLOW_CHECK(plat->sizeOsExtra, osExtra, uint16_t);
   plat->sizeOsExtra = TO_LITTLE_ENDIAN16(plat->sizeOsExtra);

   plat->globalIsa = globalIsa;
   plat->isaExtensions = isaExtensions;

   plat->isaVersion.majorVersion = TO_LITTLE_ENDIAN32(isaVersion->majorVersion);
   plat->isaVersion.minorVersion = TO_LITTLE_ENDIAN32(isaVersion->minorVersion);
   plat->isaVersion.patchVersion = TO_LITTLE_ENDIAN32(isaVersion->patchVersion);

   plat->osFamily = osFamily;
   plat->osExtra = osExtra;

   plat->osVersion.majorVersion = TO_LITTLE_ENDIAN32(osVersion->majorVersion);
   plat->osVersion.minorVersion = TO_LITTLE_ENDIAN32(osVersion->minorVersion);
   plat->osVersion.patchVersion = TO_LITTLE_ENDIAN32(osVersion->patchVersion);

   plat->packagesUsed = 0;
   plat->indicesPackages = NULL;

   #undef ASSIGN_SIZE_WITH_OVERFLOW_CHECK
   return true;
}

/**
 * Print the information of `plat`.
 */
void PrintPlatform(const Platform plat[static 1]) {
   printf("OS Family [%u]: %s\n"
          "OS Version: %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n"
          "OS Extra [%u]: %s\n"
          "Global ISA [%u]: %s\n"
          "ISA Extensions [%u]: %s\n"
          "ISA Version: %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n"
          "Number of packages: %" PRIu32 "\n",
          plat->sizeOsName, plat->osFamily,

          TO_LOCAL_ENDIAN32(plat->osVersion.majorVersion), TO_LOCAL_ENDIAN32(plat->osVersion.minorVersion),
          TO_LOCAL_ENDIAN32(plat->osVersion.patchVersion),

          plat->sizeOsExtra, plat->osExtra,
          plat->sizeIsaName, plat->globalIsa,
          plat->sizeIsaExt, plat->isaExtensions,

          TO_LOCAL_ENDIAN32(plat->isaVersion.majorVersion), TO_LOCAL_ENDIAN32(plat->isaVersion.minorVersion),
          TO_LOCAL_ENDIAN32(plat->isaVersion.patchVersion),

          TO_LOCAL_ENDIAN32(plat->packagesUsed));
}


/**
 * Append the platform `plat` into `fout`.
 * `packagesPaths` contains the packages used by
 * `plat` and they are stored in `fout` using the
 * packages table `pkgTab`.
 *
 * Return true if successful.
 */
static bool AppendPlatform(FILE* fout, const Platform* plat, const utf8* packagesPaths[], PackagesTable* pkgTab) {
   // Verify UTF-8 validity
   if (!IsValidUTF8(plat->globalIsa, TO_LOCAL_ENDIAN8(plat->sizeIsaName))
       || !IsValidUTF8(plat->osFamily, TO_LOCAL_ENDIAN8(plat->sizeOsName))
       || !IsValidUTF8(plat->isaExtensions, TO_LOCAL_ENDIAN8(plat->sizeIsaExt))
       || !IsValidUTF8(plat->osExtra, TO_LOCAL_ENDIAN8(plat->sizeOsExtra))) {
      fprintf(stderr, "The global ISA, OS family, ISA extensions or OS extra information is not a valid UTF-8 sequence.\n");
      return false;
   }

   // Write the platform information
   VERIFY_FWRITE(fwrite(&plat->sizeIsaName, sizeof(plat->sizeIsaName), 1, fout), 1);
   VERIFY_FWRITE(fwrite(&plat->sizeOsName, sizeof(plat->sizeOsName), 1, fout), 1);
   VERIFY_FWRITE(fwrite(&plat->sizeIsaExt, sizeof(plat->sizeIsaExt), 1, fout), 1);
   VERIFY_FWRITE(fwrite(&plat->sizeOsExtra, sizeof(plat->sizeOsExtra), 1, fout), 1);

   VERIFY_FWRITE(fwrite(plat->globalIsa, sizeof(*plat->globalIsa), TO_LOCAL_ENDIAN8(plat->sizeIsaName), fout), TO_LOCAL_ENDIAN8(plat->sizeIsaName));
   VERIFY_FWRITE(fwrite(plat->osFamily, sizeof(*plat->osFamily), TO_LOCAL_ENDIAN8(plat->sizeOsName), fout), TO_LOCAL_ENDIAN8(plat->sizeOsName));
   VERIFY_FWRITE(fwrite(plat->isaExtensions, sizeof(*plat->isaExtensions), TO_LOCAL_ENDIAN16(plat->sizeIsaExt), fout), TO_LOCAL_ENDIAN16(plat->sizeIsaExt));
   VERIFY_FWRITE(fwrite(plat->osExtra, sizeof(*plat->osExtra), TO_LOCAL_ENDIAN16(plat->sizeOsExtra), fout), TO_LOCAL_ENDIAN16(plat->sizeOsExtra));

   VERIFY_FWRITE(fwrite(&plat->osVersion, sizeof(plat->osVersion), 1, fout), 1);
   VERIFY_FWRITE(fwrite(&plat->isaVersion, sizeof(plat->isaVersion), 1, fout), 1);

   // Write the index of the packages used by the platform
   uint32_t count = 0, uidx;
   int32_t index;
   for (; packagesPaths[count] != NO_MORE_PACKAGE; ++count);

   count = TO_LITTLE_ENDIAN32(count);
   VERIFY_FWRITE(fwrite(&count, sizeof(count), 1, fout), 1);

   // Write the list of the packages indices used for
   // this platform.
   for (count = 0; packagesPaths[count] != NO_MORE_PACKAGE; ++count) {
      index = FindIndexPackage(pkgTab, packagesPaths[count]);
      if (index < 0) {
         fprintf(stderr, "Inconsistency with the packages table for '%s'.\n", packagesPaths[count]);
         return false;
      }

      uidx = TO_LITTLE_ENDIAN32((uint32_t)index);
      VERIFY_FWRITE(fwrite(&uidx, sizeof(uidx), 1, fout), 1);
   }

   printf("\n >>> Platform appended:\n");
   PrintPlatform(plat);
   printf("\n");

   return true;
}

/**
 * Load the platform `plat` from the current position in `fin`.
 *
 * Even on failure, FreeLoadedPlatform() should be called.
 *
 * Return true if successful.
 */
bool LoadPlatform(FILE* fin, Platform* plat) {
   uint8_t tmp8;
   uint16_t tmp16;

   plat->globalIsa = NULL;
   plat->isaExtensions = NULL;
   plat->osFamily = NULL;
   plat->osExtra = NULL;

   VERIFY_FREAD(fread(&plat->sizeIsaName, sizeof(plat->sizeIsaName), 1, fin), 1);
   VERIFY_FREAD(fread(&plat->sizeOsName, sizeof(plat->sizeOsName), 1, fin), 1);
   VERIFY_FREAD(fread(&plat->sizeIsaExt, sizeof(plat->sizeIsaExt), 1, fin), 1);
   VERIFY_FREAD(fread(&plat->sizeOsExtra, sizeof(plat->sizeOsExtra), 1, fin), 1);

   if ((plat->globalIsa = malloc(sizeof(utf8) * ((size_t)TO_LITTLE_ENDIAN8(plat->sizeIsaName) + 1))) == NULL) {
      perror("LoadPlatform malloc()");
      return false;
   }

   if ((plat->osFamily = malloc(sizeof(utf8) * ((size_t)TO_LOCAL_ENDIAN8(plat->sizeOsName) + 1))) == NULL) {
      perror("LoadPlatform malloc()");
      return false;
   }

   if ((plat->isaExtensions = malloc(sizeof(utf8) * ((size_t)TO_LOCAL_ENDIAN16(plat->sizeIsaExt) + 1))) == NULL) {
      perror("LoadPlatform malloc()");
      return false;
   }

   if ((plat->osExtra = malloc(sizeof(utf8) * ((size_t)TO_LOCAL_ENDIAN16(plat->sizeOsExtra) + 1))) == NULL) {
      perror("LoadPlatform malloc()");
      free(plat->osExtra);
      return false;
   }

   tmp8 = TO_LOCAL_ENDIAN8(plat->sizeIsaName);
   VERIFY_FREAD(fread(plat->globalIsa, sizeof(*plat->globalIsa), tmp8, fin), tmp8);
   plat->globalIsa[tmp8] = '\0';

   tmp8 = TO_LOCAL_ENDIAN8(plat->sizeOsName);
   VERIFY_FREAD(fread(plat->osFamily, sizeof(*plat->osFamily), tmp8, fin), tmp8);
   plat->osFamily[tmp8] = '\0';

   tmp16 = TO_LOCAL_ENDIAN16(plat->sizeIsaExt);
   VERIFY_FREAD(fread(plat->isaExtensions, sizeof(*plat->isaExtensions), tmp16, fin), tmp16);
   plat->isaExtensions[tmp16] = '\0';

   tmp16 = TO_LOCAL_ENDIAN16(plat->sizeOsExtra);
   VERIFY_FREAD(fread(plat->osExtra, sizeof(*plat->osExtra), tmp16, fin), tmp16);
   plat->osExtra[tmp16] = '\0';

   VERIFY_FREAD(fread(&plat->osVersion, sizeof(plat->osVersion), 1, fin), 1);
   VERIFY_FREAD(fread(&plat->isaVersion, sizeof(plat->isaVersion), 1, fin), 1);

   if (!IsValidUTF8(plat->globalIsa, TO_LOCAL_ENDIAN8(plat->sizeIsaName))
       || !IsValidUTF8(plat->osFamily, TO_LOCAL_ENDIAN8(plat->sizeOsName))
       || !IsValidUTF8(plat->isaExtensions, TO_LOCAL_ENDIAN8(plat->sizeIsaExt))
       || !IsValidUTF8(plat->osExtra, TO_LOCAL_ENDIAN8(plat->sizeOsExtra))) {
      fprintf(stderr, "The global ISA, OS family, ISA extensions or OS extra information is not a valid UTF-8 sequence.\n");
      return false;
   }


   // Assign the packages indices
   VERIFY_FREAD(fread(&plat->packagesUsed, sizeof(plat->packagesUsed), 1, fin), 1);
   const uint32_t size = TO_LOCAL_ENDIAN32(plat->packagesUsed);
   plat->indicesPackages = malloc(sizeof(plat->indicesPackages[0] * size));

   if (size != 0 && plat->indicesPackages == NULL) {
      perror("Allocation packages' indices.");
      return false;
   }

   for (uint32_t i = 0; i < size; ++i) {
      VERIFY_FREAD(fread(&plat->indicesPackages[i], sizeof(plat->indicesPackages[i]), 1, fin), 1);
      printf("Extracting package %u/%u == %u\n", i, size, plat->indicesPackages[i]);
   }

   return true;
}

/**
 * Free a platform loaded from a universal binary file.
 */
void FreeLoadedPlatform(Platform* plat) {
   if (plat->globalIsa != NULL)
      free(plat->globalIsa);
   if (plat->osFamily != NULL)
      free(plat->osFamily);
   if (plat->osExtra != NULL)
      free(plat->osExtra);
   if (plat->isaExtensions != NULL)
      free(plat->isaExtensions);
   if (plat->indicesPackages != NULL)
      free(plat->indicesPackages);
}
