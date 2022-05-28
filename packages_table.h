#ifndef PACKAGES_TABLE_H_
#define PACKAGES_TABLE_H_

#include <stdio.h>
#include <stdint.h>

#define NO_MORE_PACKAGE NULL

typedef struct PackagesTable {
   const utf8* path;
   struct PackagesTable* next;
} PackagesTable;

int32_t FindIndexPackage(const PackagesTable* pkgTab, const utf8* path);

void FreePackagesTable(PackagesTable* pkgTab);

bool InitPackagesTable(PackagesTable* pkgTab, const utf8** packagesPaths[], uint16_t size);

bool PrecreatePackagesTable(FILE* fout, uint64_t offset, const PackagesTable* pkgTab);

bool SetPackagesTableEntry(FILE* fout, uint64_t table, uint32_t idx);

bool GetPackagesTableEntry(FILE* fin, uint64_t table, uint32_t idx, uint64_t* offset);

#endif
