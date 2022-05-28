#include <stdbool.h>
#include "compatibility.h"

/**
 * Verify that `string` is a valid UTF-8 sequence of length
 * `size`. Use the syntax stated in Section 4 of IETF's RFC3629.
 *
 * See https://datatracker.ietf.org/doc/html/rfc3629#section-4
 */
bool IsValidUTF8(const utf8* string, size_t size) {
   #define BETWEEN(x, a, b) ((a) <= ((uint8_t)x)  && ((uint8_t)x) <= (b))
   #define IS_TAIL(x) BETWEEN(x, 0x80u, 0xBFu)
   #define EQ(x, a) ((uint8_t)(x) == (a))

   for (size_t i = 0; i < size; ) {
      // 1 octet sequence
      // %x00-7F
      if (BETWEEN(string[i], 0x00u, 0x7Fu)) {
         ++i;
      }
      // 2 octets sequence
      // %xC2-DF UTF8-tail
      else if (i + 1 < size && BETWEEN(string[i], 0xC2u, 0xDFu) && IS_TAIL(string[i + 1])) {
         i += 2;
      }

      // 3 octets sequence
      // %xE0 %xA0-BF UTF8-tail
      else if (i + 2 < size && EQ(string[i], 0xE0u) && BETWEEN(string[i + 1], 0xA0u, 0xBFu)
               && IS_TAIL(string[i + 2])) {
         i += 3;
      }
      // %xE1-EC 2( UTF8-tail )
      else if (i + 2 < size && BETWEEN(string[i], 0xE1u, 0xECu) && IS_TAIL(string[i + 1])
               && IS_TAIL(string[i + 2])) {
         i += 3;
      }
      // %xED %x80-9F UTF8-tail
      else if (i + 2 < size && EQ(string[i], 0xEDu) && BETWEEN(string[i + 1], 0x80u, 0x9Fu)
               && IS_TAIL(string[i + 2])) {
         i += 3;
      }
      // %xEE-EF 2( UTF8-tail )
      else if (i + 2 < size && BETWEEN(string[i], 0xEEu, 0xEFu) && IS_TAIL(string[i + 1])
               && IS_TAIL(string[i + 2])) {
         i += 3;
      }

      // 4 octets sequence
      // %xF0 %x90-BF 2( UTF8-tail )
      else if (i + 3 < size && EQ(string[i], 0xF0u) && BETWEEN(string[i + 1], 0x90u, 0xBFu)
               && IS_TAIL(string[i + 2]) && IS_TAIL(string[i + 3])) {
         i += 4;
      }
      // %xF1-F3 3( UTF8-tail )
      else if (i + 3 < size && BETWEEN(string[i], 0xF1u, 0xF3u) && IS_TAIL(string[i + 1])
               && IS_TAIL(string[i + 2]) && IS_TAIL(string[i + 3])) {
         i += 4;
      }
      // %xF4 %x80-8F 2( UTF8-tail )
      else if (i + 3 < size && EQ(string[i], 0xF4u) && BETWEEN(string[i + 1], 0x80u, 0x8Fu)
               && IS_TAIL(string[i + 2]) && IS_TAIL(string[i + 3])) {
         i += 4;
      } else {
         return false;
      }
   }

   return true;
}
