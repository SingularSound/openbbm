/* based on crypto++ */

#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stdlib.h>


#define CRC32_INDEX(c) (c & 0xff)
#define CRC32_SHIFTED(c) (c >> 8)


class Crc32
{
public:
   Crc32();

   void update(const uint8_t *input, uint32_t length);
   uint32_t getCRC(bool final = false);
   uint32_t peakCRC(bool final = false);
   void updateByte(uint8_t b);

private:

   void reset();
   static const uint32_t m_tab[256];
   uint32_t m_crc;

};

#endif // CRC32_H
