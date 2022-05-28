#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "compatibility.h"
#include "packages_table.h"
#include "platform-specific/filemanip.h"

/**
 * Return the number of elements inside `pkgTab`.
 */
static inline uint32_t SizePackagesTables(const PackagesTable* pkgTab) {
   uint32_t count = 0;
   for (; pkgTab != NULL; ++count, pkgTab = pkgTab->next);
   return count;
}

/**
 * Find the index of the package `path` inside
 * the packages table `pkgTab`.
 *
 * Return the index if successful or -1 otherwise.
 */
int32_t FindIndexPackage(const PackagesTable* pkgTab, const utf8* path) {
   int32_t i = 0;

   while (pkgTab != NULL && pkgTab->path != NULL) {
      if (strcmp(pkgTab->path, path) == 0)
         return i;

      pkgTab = pkgTab->next;
      ++i;
   }

   return -1;
}

/**
 * Deallocate the packages table `pkgTab`.
 * Do not try to free the first element.
 */
void FreePackagesTable(PackagesTable* pkgTab) {
   PackagesTable* next = pkgTab->next, *current = next;

   while (current != NULL) {
      next = current->next;
      free(current);
      current = next;
   }

   pkgTab->next = NULL;
}

/**
 * Initialise the packages table `pkgTab` with
 * the packages `packagesPaths[i][j]` where i
 * is the packages of the platform i and j
 * is the j th package for that platform.
 *
 * There are size platforms (i.e., i < size)
 * and the end of packages from a platform
 * is signaled by the value NO_MORE_PACKAGE.
 *
 * Return true if `pkgTab` is successfully
 * initialised.
 */
bool InitPackagesTable(PackagesTable* pkgTab, const utf8** packagesPaths[], uint16_t size) {
   PackagesTable* last = pkgTab, *secondlast = NULL;
   uint32_t count = 0;

   last->next = NULL;
   for (uint16_t i = 0; i < size; ++i) {
      for (int j = 0; packagesPaths[i][j] != NO_MORE_PACKAGE; ++j) {
         // Add the package to the table if it does not already exist
         if (last == pkgTab || FindIndexPackage(pkgTab, packagesPaths[i][j]) == -1) {
            last->path = packagesPaths[i][j];

            last->next = malloc(sizeof(PackagesTable));
            if (last->next == NULL) {
               return false;
            }

            secondlast = last;
            last = last->next;
            last->next = NULL;
            last->path = NULL;

            ++count;
         }
      }
   }

   if (secondlast != NULL) {
      free(last);
      secondlast->next = NULL;
   }

   return true;
}

/**
 * Complete the header by writing the `offset` of the packages
 * table into `fout` and fill the packages table with 0.
 *
 * Return true in case of success.
 */
bool PrecreatePackagesTable(FILE* fout, uint64_t offset, const PackagesTable* pkgTab) {
   uint64_t previousPos = GetPositionInBytes(fout);
   uint8_t buffer[sizeof(offset) * 50];
   uint32_t sizeWritten = 0;

   if (previousPos == INVALID_FILE_POSITION)
      return false;

   memset(buffer, 0, sizeof(buffer));

   rewind(fout);
   if (!SetPositionInBytes(fout, 4LL))
      return false;

   // Write the offset to reach the packages table
   offset = TO_LITTLE_ENDIAN64(offset);
   VERIFY_FWRITE(fwrite(&offset, sizeof(offset), 1, fout), 1);

   // Go back to where the packages table should be
   if (!SetPositionInBytes(fout, previousPos))
      return false;
   #define min(a, b) ((a) < (b) ? (a) : (b))

   // Fill it with 0 temporarily
   const uint32_t nbPackages = SizePackagesTables(pkgTab);
   for (uint32_t i = 0; i < nbPackages; i += sizeof(buffer) / sizeof(offset)) {
      sizeWritten = min(nbPackages - i, sizeof(buffer)/sizeof(offset));
      VERIFY_FWRITE(fwrite(buffer, sizeWritten * sizeof(offset), 1, fout), 1);
   }

   #undef min

   return true;
}


/**
 * Set the current position as the entry `idx` of the packages
 * table present at the position `table` in `fout`.
 *
 * Return true when successful.
 */
bool SetPackagesTableEntry(FILE* fout, uint64_t table, uint32_t idx) {
   fpos_t position;
   uint64_t offset;

   if (fgetpos(fout, &position) != 0) {
      perror("fgetpos()");
      return false;
   }

   offset = GetPositionInBytes(fout);
   if (offset == INVALID_FILE_POSITION)
      return false;

   if (!SetPositionInBytes(fout, table + idx * 8ULL))
      return false;

   offset = TO_LITTLE_ENDIAN64(offset);
   VERIFY_FWRITE(fwrite(&offset, sizeof(offset), 1, fout), 1);

   if (fsetpos(fout, &position) != 0) {
      perror("fgetpos()");
      return false;
   }

   return true;
}

/**
 * Get the value of the entry `idx` of the packages table present
 * at the position `table` in `fin`.
 *
 * Return true when successful but change the position in `fin`.
 */
bool GetPackagesTableEntry(FILE* fin, uint64_t table, uint32_t idx, uint64_t* offset) {
   if (!SetPositionInBytes(fin, table + idx * 8ULL))
      return false;

   VERIFY_FREAD(fread(offset, sizeof(*offset), 1, fin), 1);
   *offset = TO_LOCAL_ENDIAN64(*offset);

   return true;
}
