#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

/* ── Основные хэш-функции из приложения А ── */
unsigned int SDBMHash(const char* str, unsigned int length);
unsigned int djb2(const char* str, unsigned int length);
unsigned int adler32(const char* str, unsigned int length);

/* ── Дополнительные хэш-функции (доп. задание 1) ── */
unsigned int fnv1a(const char* str, unsigned int length);   /* FNV-1a */
unsigned int murmur3(const char* str, unsigned int length); /* MurmurHash3 (simplified) */

/* Тип указателя на хэш-функцию */
typedef unsigned int (*HashFunc)(const char*, unsigned int);

/* Таблица имён функций для вывода */
const char* hash_func_name(HashFunc fn);

#endif /* HASH_FUNCTIONS_H */