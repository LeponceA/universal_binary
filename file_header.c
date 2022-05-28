#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "file_header.h"
#include "platform-specific/filemanip.h"
#include "compatibility.h"

/**
 * Return true if `path` is a directory.
 */
static inline bool IsDir(const utf8* path) {
   return IsExistingDir(path, false);
}

/**
 * Write the file header of `fullpath` into `fout`
 * with the flag `headerFlag`.
 *
 * The filename is `fname` and has a size of
 * `namesize`.
 *
 * Return true if successful.
 */
static inline bool WriteFileHeader(FILE* fout, const utf8* fname, const utf8* fullpath, uint16_t nameSize, uint8_t headerFlag) {
   uint64_t fileSize;

   fileSize = FileSize(fullpath);
   if (fileSize == UNKNOWN_FILE_SIZE) {
      printf("Unknown file size.\n");
      return false;
   }

   headerFlag = TO_LITTLE_ENDIAN8(headerFlag);
   nameSize = TO_LITTLE_ENDIAN16(nameSize);

   VERIFY_FWRITE(fwrite(&headerFlag, sizeof(headerFlag), 1, fout), 1);
   VERIFY_FWRITE(fwrite(&nameSize, sizeof(nameSize), 1, fout), 1);

   nameSize = TO_LOCAL_ENDIAN16(nameSize);
   VERIFY_FWRITE(fwrite(fname, sizeof(utf8), nameSize, fout), nameSize);

   fileSize = TO_LITTLE_ENDIAN64(fileSize);
   VERIFY_FWRITE(fwrite(&fileSize, sizeof(fileSize), 1, fout), 1);

   DBG_PRINT("Success header of %s [%" PRIu64 " bytes].\n", fullpath, TO_LOCAL_ENDIAN64(fileSize));
   return true;
}

/**
 * Write the header of the directory `fname` in `fout`.
 * with the flag `headerFlag`.
 *
 * The size of the directory is `namesize`.
 *
 * Return true if successful.
 */
static inline bool WriteDirHeader(FILE* fout, const utf8* fname, uint16_t nameSize, uint8_t headerFlag) {
   uint64_t fileSize = 0LLU;
   headerFlag |= FH_FLAG_DIRECTORY;

   headerFlag = TO_LITTLE_ENDIAN8(headerFlag);
   nameSize = TO_LITTLE_ENDIAN16(nameSize);

   VERIFY_FWRITE(fwrite(&headerFlag, sizeof(headerFlag), 1, fout), 1);
   VERIFY_FWRITE(fwrite(&nameSize, sizeof(nameSize), 1, fout), 1);

   nameSize = TO_LOCAL_ENDIAN16(nameSize);
   VERIFY_FWRITE(fwrite(fname, sizeof(utf8), nameSize, fout), nameSize);
   VERIFY_FWRITE(fwrite(&fileSize, sizeof(fileSize), 1, fout), 1);

   return true;
}

/**
 * Write the header of `fullpath` in `fout` depending
 * where it can be either a file or a directory.
 *
 * The file/directory name is `fname` and its flag
 * is `headerFlag` (if the flag FH_FLAG_DIRECTORY
 * is set, `fullpath` is interpreted as a directory
 * not matter what its real type is).
 *
 * Return 0 when an error occured, 1 when successful
 * and 2 when `fullpath` was neither a file nor a
 * directory.
 */
int WriteFileDirHeader(FILE* fout, const utf8* fname, const utf8* fullpath, uint8_t headerFlag) {
   // Write the file name
   size_t size;
   uint16_t nameSize;

   size = strlen(fname);
   if (size > MAX_SIZE_INDIVIDUAL_FILE_NAME) {
      fprintf(stderr, "The file name <%s> is too large (it can be at most %d).\n",
             fname, MAX_SIZE_INDIVIDUAL_FILE_NAME);
      return false;
   }

   nameSize = (uint16_t)size;
   if ((headerFlag & FH_FLAG_DIRECTORY) || IsDir(fullpath)) {
      return (WriteDirHeader(fout, fname, nameSize, headerFlag) ? 1 : 0);
   } else if (IsRegularFile(fullpath)) {
      return (WriteFileHeader(fout, fname, fullpath, nameSize, headerFlag) ? 1 : 0);
   } else {
      printf("File '%s' ignored (not a directory nor a regular file).\n", fullpath);
      return 2;
   }
}

/**
 * Write to `outputPath` the file whose header is
 * stored in `header` and its data available on
 * the current position of `contentSource`.
 *
 * Return true when successful.
 */
bool ReproduceFile(const FileHeader* header, FILE* contentSource, utf8* outputPath)  {
   uint64_t i;
   size_t n, m;
   char buffer[1000];
   FILE* file;

   strcat(outputPath, header->name);

   file = fopen(outputPath, "wb");

   if (file == NULL) {
      fputs(outputPath, stderr);
      perror(" -- ReporduceFile() file creation");
      return false;
   }

   for (i = 0; i < header->contentSize; ) {
      #define min(a, b) ((a) < (b) ? (a) : (b))
      n = fread(buffer, sizeof(buffer[0]), min(sizeof(buffer), header->contentSize - i), contentSource);
      m = fwrite(buffer, sizeof(buffer[0]), n, file);

      if (n != min(sizeof(buffer), header->contentSize - i) || m != n) {
         fprintf(stderr, "Error while reproducing the content of the file <%s>.\n", header->name);
         fclose(file);
         return false;
      }

      i += n;
      #undef min
   }

   fclose(file);

   if (header->flag & FH_FLAG_RUNNABLE) {
      if (!SetAsExecutable(outputPath))
         return false;
   }

   return true;
}

/**
 * Return true if `header` represent an end of directory
 * marker.
 */
static inline bool IsClosingDirectory(const DirHeader* header) {
   return header->nameSize == 0 && (header->flag & FH_FLAG_DIRECTORY);
}

/**
 * Read the next file information in `fin` and reproduce
 * it on the directory `outputPath`.
 *
 * `outputPath` should be large enough to be added
 * MAX_SIZE_INDIVIDUAL_FILE_NAME characters at the
 * end of the current string and is terminated by '/'.
 *
 * The loaded file type is stored into `fileType`.
 *
 * Return true when successful.
 */
bool ExtractNextFileDir(FILE* fin, utf8* outputPath, FileType* fileType) {
   static char name[MAX_SIZE_INDIVIDUAL_FILE_NAME];
   uint64_t contentSize;
   uint8_t flag;
   uint16_t nameSize;

   VERIFY_FREAD(fread(&flag, sizeof(flag), 1, fin), 1);
   flag = TO_LOCAL_ENDIAN8(flag);
   VERIFY_FREAD(fread(&nameSize, sizeof(nameSize), 1, fin), 1);
   nameSize = TO_LOCAL_ENDIAN16(nameSize);

   if (nameSize > sizeof(name)) {
      fprintf(stderr, "Filename too long (max. %d).\n", MAX_SIZE_INDIVIDUAL_FILE_NAME);
      return false;
   }
   VERIFY_FREAD(fread(name, sizeof(name[0]), nameSize, fin), nameSize);

   name[nameSize] = '\0';
   VERIFY_FREAD(fread(&contentSize, sizeof(contentSize), 1, fin), 1);

   contentSize = TO_LOCAL_ENDIAN64(contentSize);

   if (!IsValidUTF8(name, nameSize)) {
      fprintf(stderr, "The file name '%s' is not a valid UTF-8 sequence.\n", name);
      return false;
   }

   if (flag & FH_FLAG_DIRECTORY) {
      DirHeader header = { .flag = flag , .nameSize = nameSize, .name = name, .contentSize = contentSize };

      if (IsClosingDirectory(&header)) {
         *fileType = FT_CLOSING_DIRECTORY;
         printf(" *** Extract [END]\n", outputPath);
         return true;
      }
      
      printf(" *** Extract directory '%s'\n", name);

      *fileType = FT_DIRECTORY;

      return ReproduceDir(header.name, outputPath);
   } else {
      FileHeader header = { .flag = flag , .nameSize = nameSize, .name = name, .contentSize = contentSize };

      *fileType = FT_FILE;
      
      printf(" *** Extract file '%s'\n", name);

      return ReproduceFile(&header, fin, outputPath);
   }
}
