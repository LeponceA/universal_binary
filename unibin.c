#define _FILE_OFFSET_BITS 64 // Asking glibc to use off_t with 64-bits of size

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "platform-specific/platforms.h"
#include "platform-specific/filemanip.h"
#include "unibin.h"
#include "compatibility.h"
#include "files_adder.h"
#include "platform.h"
#include "compat_test.h"

////////////////// Universal Binary Creation //////////////////


/**
 * Write in `fout` the packages `packagesPaths` of the
 *`nbPlatforms` platforms using the packages table `pkgTab`.
 *
 * Return true if successful.
 */
static inline bool WritePlatformsPackages(FILE* fout, const utf8** packagesPaths[], uint16_t nbPlatforms, uint64_t pkgTabPosition, PackagesTable* pkgTab) {
   int32_t nextIndex = 0, index;

   for (uint16_t i = 0; i < nbPlatforms; ++i) {
      // Adding the packages
      for (uint32_t count = 0; packagesPaths[i][count] != NO_MORE_PACKAGE; ++count) {
         index = FindIndexPackage(pkgTab, packagesPaths[i][count]);

         if (index == nextIndex) {
            printf("Package index %u for '%s' added.\n", index, packagesPaths[i][count]);
            SetPackagesTableEntry(fout, pkgTabPosition, (uint16_t)index);
            ++nextIndex;
            WritePackage(fout, packagesPaths[i][count]);
         } else {
            printf("Package index %u for '%s' already present.\n", index, packagesPaths[i][count]);
         }
      }
   }

   return true;
}


/**
 * Write the header of the universal binary into the file `fout`
 * (i.e., the version of the standard and the offset to reach
 * the packages table).
 *
 * Some of those fields hold temporary values. Call the function
 * `PrecreatePackagesTable()` when it is time to write the
 * packages table.
 *
 * Return false in case of failure.
 */
static inline bool WriteHeader(FILE* fout) {
   uint32_t version = TO_LITTLE_ENDIAN32(UNIBIN_VERSION);
   VERIFY_FWRITE(fwrite(&version, sizeof(version), 1, fout), 1);

   uint64_t offsetPackagesTable = 0LL; // Will be assigned later
   VERIFY_FWRITE(fwrite(&offsetPackagesTable, sizeof(offsetPackagesTable), 1, fout), 1);

   return true;
}

bool TryCreating(FILE* fout, const utf8** packagesPaths[], Platform platforms[], uint16_t nbPlatforms) {
   PackagesTable pkgTab;
   uint64_t packagesTablePosition;

   // Header
   if (!WriteHeader(fout))
      return false;

   // Init the packages table
   if (!InitPackagesTable(&pkgTab, packagesPaths, nbPlatforms))
      return false;

   // Platforms
   if (!WritePlatforms(fout, packagesPaths, &pkgTab, platforms, nbPlatforms))
      return false;

   // Fill the packages table with 0 and write its position in the header
   packagesTablePosition = GetPositionInBytes(fout);
   if (packagesTablePosition == INVALID_FILE_POSITION)
      return false;

   if (!PrecreatePackagesTable(fout, packagesTablePosition, &pkgTab))
      return false;

   if (!WritePlatformsPackages(fout, packagesPaths, nbPlatforms, packagesTablePosition, &pkgTab))
      return false;

   FreePackagesTable(&pkgTab);
   return true;
}

/**
 * Create a universal binary in the file `out` where
 * the platforms are in the array `platforms` of size
 * `size`.
 *
 * The packages intial path of each platform is stored
 * in `packagesPaths` (`packagesPaths[i][j]` is the
 * initial path of the j th package of the platform i.).
 * The last platform path for each package is always
 * NO_MORE_PACKAGE.
 *
 * Return true if successful.
 */
bool CreateUniversalBinary(const utf8* out, const utf8** packagesPaths[], Platform platforms[], uint16_t size) {
   FILE* fout;
   bool result;

   fout = fopen(out, "wb");
   if (fout == NULL) {
      perror("CreateUniversalBinary() opening of the file");
      return false;
   }

   result = TryCreating(fout, packagesPaths, platforms, size);

   fclose(fout);

   return result;
}


////////////////// Universal Binary Extraction //////////////////


/**
 * Extract the packages in `fin` for the paltform
 * `match` into the directory `outpath`.
 *
 * The position `offsetPackagesTable` in `fin`
 * is the position of the packages table.
 *
 * Return true if successful.
 */
static inline bool ExtractPackages(FILE* fin, utf8** outpath, const Platform* match, uint64_t offsetPackagesTable) {
   uint64_t offset;
   const uint32_t size = TO_LOCAL_ENDIAN32(match->packagesUsed);

   for (uint32_t i = 0; i < size; ++i) {
      uint32_t idx = TO_LOCAL_ENDIAN32(match->indicesPackages[i]);
      if (!GetPackagesTableEntry(fin, offsetPackagesTable, idx, &offset))
         return false;
      
      if (!SetPositionInBytes(fin, offset))
         return false;

      if (!RecursivelyReproduceFiles(fin, outpath))
         return false;
   }

   return true;
}

/**
 * Try to extract into `outpath` a version of the
 * application, inside the universal binary in `fin`,
 * that is compatible with `platform`.
 *
 * Return true if successful.
 */
static inline bool TryExtractingFor(FILE* fin, utf8** outpath, const Platform* platform) {
   uint16_t nbPlatforms;
   uint32_t version;
   uint64_t offsetPackagesTable;
   Platform match;
   bool r;

   if (!IsValidUTF8(*outpath, strlen(*outpath))) {
      fprintf(stderr, "The output directoy path is not a valid UTF-8 sequence.\n");
      return false;
   }

   VERIFY_FREAD(fread(&version, sizeof(version), 1, fin), 1);
   version = TO_LOCAL_ENDIAN32(version);
   if (version > UNIBIN_VERSION) {
      fprintf(stderr, "Unviseral binary version too high (%u > %u).\n", version, UNIBIN_VERSION);
      return false;
   }

   VERIFY_FREAD(fread(&offsetPackagesTable, sizeof(offsetPackagesTable), 1, fin), 1);
   offsetPackagesTable = TO_LOCAL_ENDIAN64(offsetPackagesTable);

   VERIFY_FREAD(fread(&nbPlatforms, sizeof(nbPlatforms), 1, fin), 1);
   nbPlatforms = TO_LOCAL_ENDIAN16(nbPlatforms);

   // Locating a compatible binary version
   if (!LocateCompatiblePlatform(fin, platform, nbPlatforms, &match))
      return false;

   // Unfolding the compatible binary version
   r = ExtractPackages(fin, outpath, &match, offsetPackagesTable);

   FreeLoadedPlatform(&match);

   return r;
}

/**
 * Extract into `outpath` a version of the application,
 * inside the universal binary `unibinName`, that is
 * compatible with the current system.
 *
 * Return true if successful.
 */
bool ExtractUniversalBinary(const utf8* unibinName, const utf8* outpath) {
   Platform platform;
   FILE* fin;
   utf8* path;
   bool r;

   fin = fopen(unibinName, "rb");
   if (fin == NULL) {
      perror("Opening the universal binary");
      return false;
   }

   path = malloc((strlen(outpath) + 1) * sizeof(char));
   if (path == NULL) {
      perror("Impossible to malloc() the output path");
      fclose(fin);
      return false;
   }

   strcpy(path, outpath);

   r = false;
   if (SetWithLocalPlatform(&platform)) {
      r = TryExtractingFor(fin, &path, &platform);
   }

   free(path);
   fclose(fin);

   return r;
}
