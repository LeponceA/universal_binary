#ifdef _WIN32

#ifndef WINDOWS_COMPAT_H_
#define WINDOWS_COMPAT_H_

#include <stdbool.h>

#include "../../compatibility.h"
#include "../../platform.h"

#define CURRENT_OS_NAME "WINDOWS NT"

bool IsCurrentOsVersionCompatible(const VersionMMP* version);

bool SetWithLocalPlatform(Platform plat[static 1]);

#endif

#endif
