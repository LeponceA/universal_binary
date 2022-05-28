#define _FILE_OFFSET_BITS 64 

#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <assert.h>

#if defined(_POSIX_VERSION) || defined(__linux__)
   #include <aio.h>
#endif

int main(void) {
   printf("Checking char are 8 bits.\n");
   assert(sizeof(char) == 1 && CHAR_BIT == 8);
   printf("Checking uint8_t are 8 bits.\n");
   assert(sizeof(uint8_t) == 1);
   printf("Checking uint16_t are 16 bits.\n");
   assert(sizeof(uint16_t) == 2);
   printf("Checking uint32_t are 32 bits.\n");
   assert(sizeof(uint32_t) == 4);
   printf("Checking uint64_t are 64 bits.\n");
   assert(sizeof(uint64_t) == 8);

   /* File position assertions */
   
   printf("Checking file offsets are 64 bits.\n");

   #if defined(_POSIX_VERSION) || defined(__linux__)
      assert(sizeof(off_t) == 8);
   #elif defined(_WIN32)
      assert(sizeof(_ftelli64(NULL)) == 8);
   #else
      #error "No test written for this system"
   #endif
   
   printf("No incompatibility issues have been encountered.\n");
   
   return 0;
}
