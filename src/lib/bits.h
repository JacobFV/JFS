# ifndef BITS_HEADER
# define BITS_HEADER

#include "def.h"

typedef byte bit;
const bit BIT0 = 0b10000000;
const bit BIT1 = 0b01000000;
const bit BIT2 = 0b00100000;
const bit BIT3 = 0b00010000;
const bit BIT4 = 0b00001000;
const bit BIT5 = 0b00000100;
const bit BIT6 = 0b00000010;
const bit BIT7 = 0b00000001;

void read_bytes_into_bits(int64_t num_bytes, byte* bytes, 
                          int64_t* bits_written, bit* bits, bool stop_at_0);

void read_bits_into_bytes(int64_t num_bits, bit* bits, 
                          byte* bytes);

# endif