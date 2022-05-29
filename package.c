#if defined(_POSIX_VERSION) || defined(__linux__)
   #include <dirent.h>
#elif defined(_WIN32)
   #include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compatibility.h"
#include "file_header.h"
#include "platform-specific/filemanip.h"

#if defined(_POSIX_VERSION) || defined(__linux__)

#define PATH_SEPARATOR "/"
typedef DIR Directory;

/**
 * Get the name of the next file or directory in the
 * directory `dir` and append its name to `&path[nameOffset]`.
 *
 * If the `dir` is NULL, the function starts by
 * initialising it to be on `path`.
 *
 * Return true if successful.
 */
static bool GetNextFileName(utf8* path, Directory** dir, size_t nameOffset) {
   struct dirent* file;
   int r;

   if (*dir == NULL) {
      *dir = opendir(path);

      if (*dir == NULL) {
         perror("opendir()");
         return false;
      }

      readdir(*dir);
      readdir(*dir); // Skip "." and ".."
   }

   file = readdir(*dir);
   if (file == NULL) {
      closedir(*dir);
      *dir = NULL;
      return false;
   }

   r = snprintf(&path[nameOffset], MAX_SIZE_INDIVIDUAL_FILE_NAME, "/%s", file->d_name);
   if (r < 0 || r >= MAX_SIZE_INDIVIDUAL_FILE_NAME) {
      perror("Copying the file name");
      return false;
   }

   return true;
}

/**
 * Find the name of the file at the end of
 * `path`.
 */
static utf8* FindName(utf8* path) {
   utf8* name = path;
   for (int i = 0; path[i] != '\0'; ++i) {
      if (path[i] == '/')
         name = &path[i + 1];
   }

   return name;
}

#elif defined(_WIN32)

#define PATH_SEPARATOR "\\"

typedef HANDLE Directory;

/**
 * Get the name of the next file or directory in the
 * directory `dir` and append its name to `&path[nameOffset]`.
 *
 * If the `dir` is NULL, the function starts by
 * initialising it to be on `path`.
 *
 * Return true if successful.
 */
static bool GetNextFileName(utf8* path, Directory** dir, size_t nameOffset) {
   WIN32_FIND_DATA file;
   int r;

   if (*dir == NULL) {
      strcat(path, "\\*.*");
      *dir = FindFirstFileA(path, &file);
      if (*dir == INVALID_HANDLE_VALUE) {
         fprintf(stderr, "FindFirstFileA() error: %lu.\n", GetLastError());
         *dir = NULL;
         return false;
      }
   } else {
      if (!FindNextFile(*dir, &file)) {
         FindClose(*dir);
         *dir = NULL;
         return false;
      }
   }
   
   // Skip . and ..
   while (strcmp(file.cFileName, ".") == 0 || strcmp(file.cFileName, "..") == 0) {
      if (!FindNextFile(*dir, &file)) {
         FindClose(*dir);
         *dir = NULL;
         return false;
      }
   }
   
   printf("Next filename = '%s'\n", file.cFileName);

   r = snprintf(&path[nameOffset], MAX_SIZE_INDIVIDUAL_FILE_NAME, "\\%s", file.cFileName);
   if (r < 0 || r >= MAX_SIZE_INDIVIDUAL_FILE_NAME) {
      perror("Copying the file name");
      return false;
   }

   return true;
}

/**
 * Find the name of the file at the end of
 * `path`.
 */
static utf8* FindName(utf8* path) {
   utf8* name = path;
   for (int i = 0; path[i] != '\0'; ++i) {
      if (path[i] == '/' || path[i] == '\\')
         name = &path[i + 1];
   }

   return name;
}

#else
   #error GetNextFileName() and FindName() need to be defined for this system.
#endif

#define PATH_SEPARATOR_SIZE (sizeof(PATH_SEPARATOR) / sizeof(char) - 1)

/**
 * Write the file `fullpath` (content and header)
 * into the file `dest`. The name of the file written
 * is `name`.
 *
 * Return true if successful.
 */
static bool WriteFileInto(const utf8* name, const utf8* fullPath, FILE* dest) {
   char buffer[1000];
   FILE* source;
   size_t n;

   // Verify that the path is a valid UTF-8 string
   if (!IsValidUTF8(name, strlen(name))) {
      fprintf(stderr, "The file name '%s' is not a valid UTF-8 sequence.\n", name);
      return false;
   }

   // Get the flag
   uint8_t flag = 0;
   if (IsRunnableFile(fullPath))
      flag |= FH_FLAG_RUNNABLE;

   // Write the header
   int r = WriteFileDirHeader(dest, name, fullPath, 0);
   if (r == 0) { // Failure
      return false;
   } else if (r == 2) { // Not a regular file or directory
      return true; // The file is ignored
   }

   // Write the file content
   source = fopen(fullPath, "rb");
   if (source == NULL) {
      perror("WriteFileInto() file opening");
      return false;
   }

   do {
      n = fread(buffer, sizeof(char), sizeof(buffer) / sizeof(char), source);
      if (fwrite(buffer, sizeof(char), n, dest) < n) {
         fclose(source);
         perror("WriteFileInto() writing result");

         return false;
      }
   } while(!ferror(source) && !feof(source));

   if (ferror(source)) {
      fputs("WriteFileInto(): an error occured when reading the source file.", stderr);
      fclose(source);

      return false;
   }

   fclose(source);

   return true;
}

/**
 * Write the directory header marking the start
 * of the directory `fullPath`, named `name`,
 * into `dest`.
 *
 * Return true if successful.
 */
static bool WriteDirectoryStart(const utf8* name, const utf8* fullPath, FILE* dest) {
   if (!WriteFileDirHeader(dest, name, fullPath, FH_FLAG_DIRECTORY)) {
      return false;
   }

   return true;
}

/**
 * Write the directory header marking the end
 * of the last directory not closed yet.
 *
 * Return true if successful.
 */
static bool WriteDirectoryEnd(FILE* dest) {
   if (!WriteFileDirHeader(dest, "", "", FH_FLAG_DIRECTORY)) {
      return false;
   }

   return true;
}

/**
 * Recursively add the files and directories contained
 * in `*currentDirName` and its subfolders into `fout`.
 *
 * Ignore links when adding the files and directories.
 *
 * Return true if successful.
 */
static bool RecursivelyAddFiles(utf8** currentDirName, FILE* fout) {
   Directory* dir = NULL;
   size_t endDirName = strlen(*currentDirName);
   utf8* fullPath = realloc(*currentDirName, endDirName + MAX_SIZE_INDIVIDUAL_FILE_NAME + PATH_SEPARATOR_SIZE);

   if (fullPath == NULL) {
      perror("Some files could not be added");
      return false;
   }

   *currentDirName = fullPath;

   while (GetNextFileName(*currentDirName, &dir, endDirName)) {
      if (IsExistingDir(*currentDirName, true)) {
         if (!WriteDirectoryStart(*currentDirName + endDirName + 1, *currentDirName, fout))
            return false;
         if (!RecursivelyAddFiles(currentDirName, fout))
            return false;
         if (!WriteDirectoryEnd(fout))
            return false;
      } else {
         if (!WriteFileInto(*currentDirName + endDirName + 1, *currentDirName, fout))
            return false;
      }
   }

   return true;
}

/**
 * Write as a package all the files, directories,
 * subfiles and subfolders inside `parentPath`.
 *
 * Ignore links when adding the files and directories.
 *
 * Return true if successful.
 */
bool WritePackage(FILE* fout, const utf8 parentPath[]) {
   utf8* path;

   path = malloc(sizeof(utf8) * (strlen(parentPath) + 1));

   if (path == NULL) {
      perror("WritePackage() malloc");
      return false;
   }

   strcpy(path, parentPath);
   if (IsExistingDir(path, true)) {
      if (!RecursivelyAddFiles(&path, fout)) {
         return false;
      }
   } else {
      utf8* name = FindName(path);
      if (!WriteFileInto(name, path, fout))
         return false;
   }
   free(path);

   WriteDirectoryEnd(fout);

   return true;
}

/**
 * Recursively reproduce the files of the package stored
 * at the current position in `fin` into the directory
 * `*currentDirName`.
 *
 * Return true if succesful.
 */
bool ReproducePackageFiles(FILE* fin, utf8** currentDirName) {
   FileType fileType = FT_INIT;
   size_t endDirName = strlen(*currentDirName);
   utf8* fullPath = realloc(*currentDirName, endDirName + 65536 + PATH_SEPARATOR_SIZE);


   if (fullPath == NULL) {
      perror("Some files could not be reproduced");
      return false;
   }

   *currentDirName = fullPath;
   strcpy(*currentDirName + endDirName, PATH_SEPARATOR);
   endDirName += PATH_SEPARATOR_SIZE;


   while (ExtractNextFileDir(fin, *currentDirName, &fileType)) {
      if (fileType == FT_CLOSING_DIRECTORY) {
         // The directory has been fully explored
         return true;
      } else if (fileType == FT_DIRECTORY) {
         // A new file/directory has been encountered
         if (!ReproducePackageFiles(fin, currentDirName))
            return false;
      }

      (*currentDirName)[endDirName] = '\0';
   }

   return true;
}
