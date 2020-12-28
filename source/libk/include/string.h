#ifndef _STRING_H
#define _STRING_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

size_t memcmp(const void*, const void*, size_t);
void *memcpy(void*, void*, size_t);
void *memset(void*, uint8_t, size_t);
void *memmove(void*, void*, size_t);

size_t strlen(const char*);

int32_t atoi(const char*);
int32_t atox(const char*);

char xbtoa(uint8_t);
void xtoa(uint32_t, char*);


#ifdef __cplusplus
}
#endif



#endif

