#ifndef FILE_HEADER_H__
#define FILE_HEADER_H__

#include <stdint.h>
#include <stdbool.h>

#include "compatibility.h"

#define FH_FLAG_RUNNABLE  1
#define FH_FLAG_DIRECTORY 2

typedef struct {
   uint8_t flag;

   uint16_t nameSize;
   utf8* name;

   uint64_t contentSize;
} FileHeader, DirHeader;

typedef enum { FT_INIT, FT_FILE, FT_DIRECTORY, FT_CLOSING_DIRECTORY, FT_EOF } FileType;

int WriteFileDirHeader(FILE* fout, const utf8* fname, const utf8* fullpath, uint8_t headerFlag);

bool ExtractNextFileDir(FILE* fin, utf8* outputPath, FileType* fileType);

#endif
