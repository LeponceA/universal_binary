#include "../filemanip.h"

#if defined(_POSIX_VERSION) || defined(__linux__)

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <aio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

   if (mkdir(outputPath, 0777) != 0) {
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

   struct stat info;
   if (lstat(path, &info) != 0) {
      if (!quiet)
         perror(path);

      return false;
   }

   return S_ISDIR(info.st_mode);
}

/**
 * Return true if `fname` refers to a regular file.
 */
bool IsRegularFile(const utf8* fname) {
   if (fname[0] == '\0')
      return false;

   struct stat info;
   if (lstat(fname, &info) != 0) {
      perror(fname);
      return false;
   }

   return S_ISREG(info.st_mode);
}

/**
 * Return the file size of `path`.
 *
 * In case of erreor, return INVALID_FILE_POSITION.
 */
uint64_t FileSize(const utf8* path) {
   struct stat info;
   if (lstat(path, &info) != 0) {
      perror(path);
      return UNKNOWN_FILE_SIZE;
   }

   return (uint64_t)info.st_size;
}

/**
 * Return the current position in `file` in bytes.
 *
 * In case of erreor, return INVALID_FILE_POSITION.
 */
uint64_t GetPositionInBytes(FILE* file) {
   off_t position = ftello(file);
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
   if (fseeko(file, position, SEEK_SET) != 0) {
      perror("fseeko()");
      return false;
   }

   return true;
}

/**
 * Return if the file `fname` is runnable (X
 * access mode).
 */
bool IsRunnableFile(const utf8* fname) {
   struct stat info;
   return lstat(fname, &info) == 0 && (info.st_mode & S_IXUSR);
}

/**
 * Set the execution mode for `fname` and
 * return true if successful.
 */
bool SetAsExecutable(const utf8* fname) {
   return chmod(fname, S_IRWXU | S_IRWXG) != -1;
}

#else

#define EMPTY_

#endif
