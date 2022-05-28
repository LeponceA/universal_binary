#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "unibin.h"
#include "platform-specific/platforms.h"


int main(void) {
   enum { WINDOWS, LINUX, NB_PLATFORMS, MACOS };
   const utf8** paths[NB_PLATFORMS] = {[WINDOWS] = (const utf8*[]){"test/import/resources", "test/import/hello-world.exe", NO_MORE_PACKAGE}, [LINUX] = (const utf8*[]){"test/import/resources", "test/import/hello-world", NO_MORE_PACKAGE}};
   Platform platforms[NB_PLATFORMS];

   VersionMMP version = { .majorVersion = 6, .minorVersion = 0, .patchVersion = 0 };
   VersionMMP version0 = { .majorVersion = 0, .minorVersion = 0, .patchVersion = 0 };

   InitPlatform(&platforms[0], "INTEL 64", "", &version0, "Windows NT", "", &version);
   SetWithLocalPlatform(&platforms[1]);
   PrintPlatform(&platforms[0]);

   printf("\n\nCreateUniversalBinary: %d\n\n", CreateUniversalBinary("unibin.ubn", paths, platforms, NB_PLATFORMS));

   printf("\n\nExtractUniversalBinary: %d\n\n", ExtractUniversalBinary("unibin.ubn", "test/export"));

   return 0;
}
