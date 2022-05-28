#include "../filemanip.h"

#if defined(_WIN32)

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <direct.h>
#include <windows.h>

/**
 * Create the directory `outPath` + `name` and gives
 * it all the autorisation (read, write and execute).
 *
 * Return true when successful.
 */
bool ReproduceDir(const utf8* name, utf8* outputPath) {
   strcat(outputPath, name);

   if (IsExistingDir(outputPath, true))
      return true; // Already existing directory does not need to be create

   if (_mkdir(outputPath) != 0) {
      perror("Creating the directory");
      return false;
   }

   return true;
}

/**
 * Check if `path` is an existing directory.
 *
 * When `quiet` is set to false, a warning exists
 * when no file or directory exists at that path.
 *
 * Return true if `path` refers to an existing directory.
 */
bool IsExistingDir(const utf8* path, bool quiet) {
   if (path[0] == '\0')
      return false;

   DWORD attribute = GetFileAttributesA(path);

   if (attribute == INVALID_FILE_ATTRIBUTES) {
      if (!quiet)
         fprintf(stderr, "Impossible to determine the file attribute of <%s>: error %lu.\n", path, GetLastError());

      return false;
   }

   return (attribute & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

/**
 * Return true if `fname` refers to a regular file.
 */
bool IsRegularFile(const utf8* fname) {
   if (fname[0] == '\0')
      return false;

   DWORD attribute = GetFileAttributesA(fname);

   if (attribute == INVALID_FILE_ATTRIBUTES) {
      fprintf(stderr, "Impossible to determine the file attribute of <%s>: error %lu.\n", fname, GetLastError());
      return false;
   }

   return (attribute & FILE_ATTRIBUTE_REPARSE_POINT) == 0 && (attribute & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

/**
 * Return the file size of `path`.
 *
 * In case of erreor, return INVALID_FILE_POSITION.
 */
uint64_t FileSize(const utf8* path) {
   LARGE_INTEGER fileSize;
   HANDLE hFile = CreateFileA(path, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

   if (hFile == INVALID_HANDLE_VALUE) {
      fprintf(stderr, "Impossible to open the file <%s>: error %lu.\n", path, GetLastError());
      return UNKNOWN_FILE_SIZE;
   }

   if (GetFileSizeEx(hFile, &fileSize) == 0) {
      fprintf(stderr, "Impossible to get the file <%s> info: error %lu.\n", path, GetLastError());
      CloseHandle(hFile);
      return UNKNOWN_FILE_SIZE;
   }

   CloseHandle(hFile);

   return (uint64_t)fileSize.QuadPart;
}

/**
 * Return the current position in `file` in bytes.
 *
 * In case of erreor, return INVALID_FILE_POSITION.
 */
uint64_t GetPositionInBytes(FILE* file) {
   int64_t position = _ftelli64(file);
   if (position < 0) {
      perror("ftello()");
      return INVALID_FILE_POSITION;
   }

   return (uint64_t)position;
}

/**
 * Set the position in `file` to `position` bytes.
 *
 * Return true when successful.
 */
 bool SetPositionInBytes(FILE* file, uint64_t position) {
   if (_fseeki64(file, position, SEEK_SET) != 0) {
      perror("_fseeki64()");
      return false;
   }

   return true;
}

/**
 * Return if the file `fname` is runnable.
 *
 * There is no such notion in Windows so the
 * result is always false.
 */
bool IsRunnableFile(const utf8* fname) {
   (void)fname;
   return false;
}

/**
 * Set the execution mode for `fname` and
 * return true if successful (always true
 * in Windows).
 */
bool SetAsExecutable(const utf8* fname) {
   (void)fname;
   return true;
}

#else

#define EMPTY_

#endif
