#ifndef UNIBIN_H__
#define UNIBIN_H__

#include <stdbool.h>

#include "compatibility.h"
#include "platform.h"

#define UNIBIN_VERSION 1

bool CreateUniversalBinary(const utf8* out, const utf8** packagesPaths[], Platform platforms[], uint16_t size);

bool ExtractUniversalBinary(const utf8* unibinName, const utf8* outpath);

bool TryCreating(FILE* fout, const utf8** packagesPaths[], Platform platforms[], uint16_t nbPlatforms);

#endif
