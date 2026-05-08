#include "hash_functions.h"
#include <string.h>

/* ─────────────────────────────────────────────
   Основные хэш-функции (приложение А)
───────────────────────────────────────────── */

unsigned int SDBMHash(const char* str, unsigned int length) {
    unsigned int hash = 0;
    for (unsigned int i = 0; i < length; str++, i++) {
        hash = (unsigned char)(*str) + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

unsigned int djb2(const char* str, unsigned int length) {
    unsigned int hash = 5381;
    for (unsigned int i = 0; i < length; str++, i++) {
        hash = ((hash << 5) + hash) + (unsigned char)(*str);
    }
    return hash;
}

unsigned int adler32(const char* str, unsigned int length) {
    unsigned int s1 = 1;
    unsigned int s2 = 0;
    for (unsigned int i = 0; i < length; str++, i++) {
        s1 = (s1 + (unsigned char)(*str)) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) | s1;
}

/* ─────────────────────────────────────────────
   Дополнительные хэш-функции
───────────────────────────────────────────── */

/* FNV-1a (Fowler–Noll–Vo) */
unsigned int fnv1a(const char* str, unsigned int length) {
    unsigned int hash = 2166136261u;
    for (unsigned int i = 0; i < length; str++, i++) {
        hash ^= (unsigned char)(*str);
        hash *= 16777619u;
    }
    return hash;
}

/* MurmurHash3 (упрощённая 32-битная версия) */
unsigned int murmur3(const char* str, unsigned int length) {
    unsigned int h = 0xdeadbeef;
    unsigned int c1 = 0xcc9e2d51u;
    unsigned int c2 = 0x1b873593u;

    const unsigned char* data = (const unsigned char*)str;
    unsigned int nblocks = length / 4;

    for (unsigned int i = 0; i < nblocks; i++) {
        unsigned int k;
        /* Читаем 4 байта little-endian */
        k = (unsigned int)data[i * 4 + 0];
        k |= (unsigned int)data[i * 4 + 1] << 8;
        k |= (unsigned int)data[i * 4 + 2] << 16;
        k |= (unsigned int)data[i * 4 + 3] << 24;

        k *= c1;
        k = (k << 15) | (k >> 17);
        k *= c2;

        h ^= k;
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64u;
    }

    /* Хвост */
    const unsigned char* tail = data + nblocks * 4;
    unsigned int k = 0;
    switch (length & 3) {
    case 3: k ^= (unsigned int)tail[2] << 16; /* fall through */
    case 2: k ^= (unsigned int)tail[1] << 8;  /* fall through */
    case 1: k ^= (unsigned int)tail[0];
        k *= c1;
        k = (k << 15) | (k >> 17);
        k *= c2;
        h ^= k;
    }

    /* Финализация */
    h ^= length;
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;

    return h;
}

/* ─────────────────────────────────────────────
   Имена функций для вывода
───────────────────────────────────────────── */
const char* hash_func_name(HashFunc fn) {
    if (fn == SDBMHash) return "SDBMHash";
    if (fn == djb2)     return "djb2";
    if (fn == adler32)  return "adler32";
    if (fn == fnv1a)    return "FNV-1a";
    if (fn == murmur3)  return "MurmurHash3";
    return "unknown";
}