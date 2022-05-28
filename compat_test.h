#ifndef COMPAT_TEST_H__
#define COMPAT_TEST_H__

#include <stdbool.h>

#include "platform.h"

typedef enum { COMPATIBLE, OS_FAMILY_INCOMPAT, OS_VERSION_INCOMPAT,
               ISA_FAMILY_INCOMPAT, ISA_EXTENSIONS_INCOMPAT, ISA_VERSION_INCOMPAT
} CompatStatus;

CompatStatus VerifyCompatibility(const Platform* host, const Platform* hosted);

bool CompareVersions(const VersionMMP* host, const VersionMMP* hosted);

#endif
