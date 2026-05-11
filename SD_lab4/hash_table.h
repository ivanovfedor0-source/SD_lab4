#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "hash_functions.h"
#include <stddef.h>


typedef enum {
    COLLISION_CHAINING, 
    COLLISION_LINEAR_PROBE,
    COLLISION_QUADRATIC_PROBE
} CollisionMethod;

const char* collision_method_name(CollisionMethod m);


typedef enum {
    VALUE_STRING,
    VALUE_INT
} ValueType;


typedef struct ChainNode {
    char* key;
    char* str_val;
    int               int_val;
    struct ChainNode* next;
} ChainNode;


typedef enum { SLOT_EMPTY, SLOT_OCCUPIED, SLOT_DELETED } SlotState;

typedef struct {
    SlotState  state;
    char* key;
    char* str_val;
    int        int_val;
} Slot;


typedef struct {
    HashFunc        hash_fn;
    CollisionMethod collision;  
    ValueType       value_type;  
    unsigned int    init_capacity; 
    double          load_factor; 
} HTableConfig;


typedef struct {
    HTableConfig  cfg;


    ChainNode** chains;  
    Slot* slots; 

    unsigned int  capacity;
    unsigned int  size; 


    unsigned long long collisions; 
    unsigned long long resizes; 
} HTable;



HTable* ht_create(HTableConfig cfg);
void    ht_destroy(HTable* ht);


int  ht_put(HTable* ht, const char* key, const char* str_val, int int_val);

int  ht_get(HTable* ht, const char* key, char** str_out, int* int_out);

int  ht_del(HTable* ht, const char* key, char** str_out, int* int_out);


void ht_list(const HTable* ht, unsigned int num);


double ht_load(const HTable* ht);


void ht_reset_stats(HTable* ht);

#endif