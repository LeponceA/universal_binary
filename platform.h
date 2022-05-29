#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "compatibility.h"
#include "packages_table.h"

typedef struct {
   uint32_t majorVersion;
   uint32_t minorVersion;
   uint32_t patchVersion;
} VersionMMP;

typedef struct {
   uint8_t sizeIsaName;
   uint8_t sizeOsName;
   uint16_t sizeIsaExt;
   uint16_t sizeOsExtra;

   utf8* globalIsa;
   utf8* isaExtensions;
   VersionMMP isaVersion;

   utf8* osFamily;
   utf8* osExtra;
   VersionMMP osVersion;

   uint32_t packagesUsed;
   uint32_t* indicesPackages;
} Platform;

bool InitPlatform(Platform* plat, utf8* globalIsa, utf8* isaExtensions, VersionMMP* isaVersion, utf8* osFamily, utf8* osExtra, VersionMMP* osVersion);

void PrintPlatform(const Platform plat[static 1]);

bool LoadPlatform(FILE* fin, Platform* plat);

void FreeLoadedPlatform(Platform* plat);

bool WritePlatforms(FILE* fout, const utf8** packagesPaths[], PackagesTable* pkgTab, Platform platforms[], uint16_t nbPlatforms);

bool LocateCompatiblePlatform(FILE* fin, const Platform* current, uint16_t nbPlatforms, Platform* match);

#endif
