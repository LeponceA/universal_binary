#ifndef FILE_MANIP_H__
#define FILE_MANIP_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "../compatibility.h"

#define INVALID_FILE_POSITION (~(uint64_t)0LLU)
#define UNKNOWN_FILE_SIZE (~(uint64_t)0llu)

/**
 * Return the current position in `file` in bytes.
 *
 * In case of erreor, return INVALID_FILE_POSITION.
 */
uint64_t GetPositionInBytes(FILE* file);

/**
 * Set the position in `file` to `position` bytes.
 *
 * Return true when successful.
 */
bool SetPositionInBytes(FILE* file, uint64_t position);

/**
 * Return the file size of `path`.
 *
 * In case of erreor, return INVALID_FILE_POSITION.
 */
uint64_t FileSize(const utf8* path);

/**
 * Check if `path` is an existing directory.
 *
 * When `quiet` is set to false, a warning exists
 * when no file or directory exists at that path.
 *
 * Return true if `path` refers to an existing directory.
 */
bool IsExistingDir(const utf8* path, bool quiet);

/**
 * Return true if `fname` refers to a regular file.
 */
bool IsRegularFile(const utf8* fname);

/**
 * Create the directory `outPath` + `name` and gives
 * it all the autorisation (read, write and execute).
 *
 * Return true when successful.
 */
bool ReproduceDir(const utf8* name, utf8* outputPath);

/**
 * Return if the file `fname` is runnable (X
 * access mode).
 */
bool IsRunnableFile(const utf8* fname);

/**
 * Set the execution mode for `fname` and
 * return true if successful.
 */
bool SetAsExecutable(const utf8* fname);

#endif
