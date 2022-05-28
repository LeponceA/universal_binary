#if defined(_POSIX_VERSION) || defined(__linux__)

#ifndef LINUX_COMPATIBILITY_H_
#define LINUX_COMPATIBILITY_H_

#include <stdbool.h>

#include "../../compatibility.h"
#include "../../platform.h"

#define CURRENT_OS_NAME "LINUX"

bool SetWithLocalPlatform(Platform plat[static 1]);

bool IsCurrentOsVersionCompatible(const VersionMMP* version);

#endif

#endif
