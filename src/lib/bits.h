# ifndef BITS_HEADER
# define BITS_HEADER

#include "def.h"
#include "diskutils.h"

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

char* parse_string(int64_t* loc, RAID raid);
int64_t parse_int64(int64_t loc, RAID raid);
int32_t parse_int32(int64_t loc, RAID raid);
int16_t parse_int16(int64_t loc, RAID raid);
int8_t parse_int8(int64_t loc, RAID raid);

void save_string(int64_t* loc, char* val, RAID raid);
void save_int64(int64_t* loc, int64_t val, RAID raid);
void save_int32(int64_t* loc, int32_t val, RAID raid);
void save_int16(int64_t* loc, int16_t val, RAID raid);
void save_int8(int64_t* loc, int8_t val, RAID raid);

# endif