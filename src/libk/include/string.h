#ifndef _STRING_H
#define _STRING_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

int32_t memcmp(const void*, const void*, size_t);
void *memcpy(void*, void*, size_t);
void *memset(void*, uint8_t, size_t);
void *memmove(void*, void*, size_t);

size_t strlen(const char*);
size_t strcmp(const char*, const char*);

int64_t atol(const char *buf);
int32_t atox(const char*);

char xbtoa(uint8_t);
void xtoa(uint64_t, char*);
uint32_t oct2bin(char*, uint32_t);
int32_t bin2oct(uint32_t, char*, uint32_t);

#ifdef __cplusplus
}
#endif



#endif

