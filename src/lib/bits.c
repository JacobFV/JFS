#include "bits.h"


void read_bytes_into_bits(int64_t num_bytes, byte* bytes, 
                          int64_t* bits_written, bit* bits, bool stop_at_0)
{
    for(int i = 0; i < num_bytes; i++) {
        if(bytes[i] == 0 && stop_at_0) break;
        bits[i] = bytes[i];
        bits_written++;
    }
}

void read_bits_into_bytes(int64_t num_bits, bit* bits, 
                          byte* bytes)
{
    for(int i = 0; i < num_bits; i++) {
        bytes[i/8] = bits[i] << (i%8);
    }
}

char* parse_string(int64_t* loc, RAID raid) {
    byte* b = calloc(1, sizeof(byte));
    int N = 0;
    do {
        read_raid_bytes_raw(*loc+N, 1, b, raid);
        N++;
    } while(*b != 0);
    free(b);


    byte* b = calloc(N, sizeof(byte));
    read_raid_bytes_raw(*loc, N, b, raid);
    char* new_string = calloc(N, sizeof(char));
    memcpy(new_string, b, N);
    free(b);

    *loc = *loc + N;
    return new_string;
}

int64_t parse_int64(int64_t loc, RAID raid) {
    byte* b = calloc(8, sizeof(byte));
    read_raid_bytes_raw(loc, 8, b, raid);
    
    int val =
        b[0] |
        b[1] << 8 |
        b[2] << 16 |
        b[3] << 24 |
        b[4] << 32 |
        b[5] << 40 |
        b[6] << 48 |
        b[7] << 56;
    
    free(b);
    return val;
}

int32_t parse_int32(int64_t loc, RAID raid) {
    byte* b = calloc(4, sizeof(byte));
    read_raid_bytes_raw(loc, 4, b, raid);
    
    int val =
        b[0] |
        b[1] << 8 |
        b[2] << 16 |
        b[3] << 24;
    
    free(b);
    return val;
}

int16_t parse_int16(int64_t loc, RAID raid) {
    byte* b = calloc(2, sizeof(byte));
    read_raid_bytes_raw(loc, 2, b, raid);
    
    int val =
        b[0] |
        b[1] << 8;
    
    free(b);
    return val;
}

int8_t parse_int8(int64_t loc, RAID raid) {
    byte* b = calloc(1, sizeof(byte));
    read_raid_bytes_raw(loc, 1, b, raid);
    
    int val = b[0];
    
    free(b);
    return val;
}

void save_string(int64_t* loc, char* val, RAID raid) {
    write_raid_bytes_raw(*loc, strlen(val)+1, val, raid);
    *loc += strlen(val)+1;
}

void save_int64(int64_t* loc, int64_t val, RAID raid) {
    byte* b[8];
    b[0] = 0b11111111 & val;
    b[1] = 0b11111111 & val >> 8;
    b[2] = 0b11111111 & val >> 16;
    b[3] = 0b11111111 & val >> 24;
    b[4] = 0b11111111 & val >> 32;
    b[5] = 0b11111111 & val >> 40;
    b[6] = 0b11111111 & val >> 48;
    b[7] = 0b11111111 & val >> 56;
    write_raid_bytes_raw(*loc, 8, b, raid);
    *loc += 8;
}

void save_int32(int64_t* loc, int32_t val, RAID raid) {
    byte* b[4];
    b[0] = 0b11111111 & val;
    b[1] = 0b11111111 & val >> 8;
    b[2] = 0b11111111 & val >> 16;
    b[3] = 0b11111111 & val >> 24;
    write_raid_bytes_raw(*loc, 4, b, raid);
    *loc += 4;
}
void save_int16(int64_t* loc, int16_t val, RAID raid) {
    byte* b[2];
    b[0] = 0b11111111 & val;
    b[1] = 0b11111111 & val >> 8;
    write_raid_bytes_raw(*loc, 2, b, raid);
    *loc += 2;
}
void save_int8(int64_t* loc, int8_t val, RAID raid) {
    byte* b[1];
    b[0] = val;
    write_raid_bytes_raw(*loc, 1, b, raid);
    *loc += 1;
}