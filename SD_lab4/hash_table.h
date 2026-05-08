#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "hash_functions.h"
#include <stddef.h>

/* ── Метод разрешения коллизий ── */
typedef enum {
    COLLISION_CHAINING,           /* Цепочки (separate chaining)    */
    COLLISION_LINEAR_PROBE,       /* Линейный поиск                 */
    COLLISION_QUADRATIC_PROBE     /* Квадратичный поиск (доп. зад.) */
} CollisionMethod;

const char* collision_method_name(CollisionMethod m);

/* ── Тип значения ── */
typedef enum {
    VALUE_STRING,  /* string / string */
    VALUE_INT      /* string / int    */
} ValueType;

/* ─────────────────────────────────────────────
   Узел цепочки
───────────────────────────────────────────── */
typedef struct ChainNode {
    char* key;
    char* str_val;   /* used when VALUE_STRING */
    int               int_val;   /* used when VALUE_INT    */
    struct ChainNode* next;
} ChainNode;

/* ─────────────────────────────────────────────
   Слот открытой адресации
───────────────────────────────────────────── */
typedef enum { SLOT_EMPTY, SLOT_OCCUPIED, SLOT_DELETED } SlotState;

typedef struct {
    SlotState  state;
    char* key;
    char* str_val;
    int        int_val;
} Slot;

/* ─────────────────────────────────────────────
   Конфигурация хэш-таблицы
───────────────────────────────────────────── */
typedef struct {
    HashFunc        hash_fn;        /* хэш-функция                  */
    CollisionMethod collision;      /* метод разрешения коллизий    */
    ValueType       value_type;     /* тип значения                 */
    unsigned int    init_capacity;  /* начальный размер             */
    double          load_factor;    /* порог расширения             */
} HTableConfig;

/* ─────────────────────────────────────────────
   Сама хэш-таблица
───────────────────────────────────────────── */
typedef struct {
    HTableConfig  cfg;

    /* Данные */
    ChainNode** chains;   /* массив для CHAINING              */
    Slot* slots;    /* массив для OPEN ADDRESSING       */

    unsigned int  capacity; /* текущий размер массива           */
    unsigned int  size;     /* количество элементов             */

    /* Статистика */
    unsigned long long collisions;   /* всего коллизий           */
    unsigned long long resizes;      /* количество расширений    */
} HTable;

/* ─────────────────────────────────────────────
   API
───────────────────────────────────────────── */

/* Создать/уничтожить */
HTable* ht_create(HTableConfig cfg);
void    ht_destroy(HTable* ht);

/* Операции */
int  ht_put(HTable* ht, const char* key, const char* str_val, int int_val);
/* Возвращает 1 если нашёл, str_out/int_out заполняются */
int  ht_get(HTable* ht, const char* key, char** str_out, int* int_out);
/* Возвращает 1 если удалил */
int  ht_del(HTable* ht, const char* key, char** str_out, int* int_out);

/* Вывод состояния первых num индексов */
void ht_list(const HTable* ht, unsigned int num);

/* Коэффициент заполнения */
double ht_load(const HTable* ht);

/* Сброс статистики */
void ht_reset_stats(HTable* ht);

#endif /* HASH_TABLE_H */