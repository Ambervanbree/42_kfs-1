

#include "string.h"


size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}


int strcmp(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}


char* strcpy(char* dest, const char* src)
{
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}


char* strcat(char* dest, const char* src)
{
    char* original_dest = dest;
    

    while (*dest) {
        dest++;
    }
    

    while ((*dest++ = *src++));
    
    return original_dest;
}


void* memset(void* ptr, int value, size_t num)
{
    unsigned char* p = (unsigned char*)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}


void* memcpy(void* dest, const void* src, size_t num)
{
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    while (num--) {
        *d++ = *s++;
    }
    
    return dest;
}