#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

unsigned int SDBMHash(const char* str, unsigned int length);
unsigned int djb2(const char* str, unsigned int length);
unsigned int adler32(const char* str, unsigned int length);

unsigned int fnv1a(const char* str, unsigned int length); 
unsigned int murmur3(const char* str, unsigned int length); 

typedef unsigned int (*HashFunc)(const char*, unsigned int);

const char* hash_func_name(HashFunc fn);

#endif