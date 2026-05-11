#include "hash_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>


static char* dup_str(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* d = malloc(len);
    if (d) memcpy(d, s, len);
    return d;
}

static unsigned int bucket(const HTable* ht, const char* key) {
    unsigned int h = ht->cfg.hash_fn(key, (unsigned int)strlen(key));
    return h % ht->capacity;
}

const char* collision_method_name(CollisionMethod m) {
    switch (m) {
    case COLLISION_CHAINING:        return "Chaining";
    case COLLISION_LINEAR_PROBE:    return "Linear probe";
    case COLLISION_QUADRATIC_PROBE: return "Quadratic probe";
    }
    return "unknown";
}



HTable* ht_create(HTableConfig cfg) {
    HTable* ht = calloc(1, sizeof(HTable));
    if (!ht) return NULL;
    ht->cfg = cfg;
    ht->capacity = cfg.init_capacity;
    ht->size = 0;
    ht->collisions = 0;
    ht->resizes = 0;

    if (cfg.collision == COLLISION_CHAINING) {
        ht->chains = calloc(ht->capacity, sizeof(ChainNode*));
        if (!ht->chains) { free(ht); return NULL; }
    }
    else {
        ht->slots = calloc(ht->capacity, sizeof(Slot));
        if (!ht->slots) { free(ht); return NULL; }
    }
    return ht;
}

static void free_chain(ChainNode* node) {
    while (node) {
        ChainNode* next = node->next;
        free(node->key);
        free(node->str_val);
        free(node);
        node = next;
    }
}

static void free_slot(Slot* s) {
    if (s->state == SLOT_OCCUPIED) {
        free(s->key);
        free(s->str_val);
    }
}

void ht_destroy(HTable* ht) {
    if (!ht) return;
    if (ht->chains) {
        for (unsigned int i = 0; i < ht->capacity; i++)
            free_chain(ht->chains[i]);
        free(ht->chains);
    }
    if (ht->slots) {
        for (unsigned int i = 0; i < ht->capacity; i++)
            free_slot(&ht->slots[i]);
        free(ht->slots);
    }
    free(ht);
}

static void raw_put_chain(ChainNode** chains, unsigned int cap,
    HashFunc fn, const char* key,
    const char* str_val, int int_val) {
    unsigned int idx = fn(key, (unsigned int)strlen(key)) % cap;
    ChainNode* node = malloc(sizeof(ChainNode));
    node->key = dup_str(key);
    node->str_val = dup_str(str_val);
    node->int_val = int_val;
    node->next = chains[idx];
    chains[idx] = node;
}

static void raw_put_slot(Slot* slots, unsigned int cap, HashFunc fn, const char* key, const char* str_val, int int_val) {     
    unsigned int base = fn(key, (unsigned int)strlen(key)) % cap;
    for (unsigned int i = 0; i < cap; i++) {
        unsigned int idx = (base + i) % cap;
        if (slots[idx].state != SLOT_OCCUPIED) {
            slots[idx].state = SLOT_OCCUPIED;
            slots[idx].key = dup_str(key);
            slots[idx].str_val = dup_str(str_val);
            slots[idx].int_val = int_val;
            return;
        }
    }
}

static void ht_resize(HTable* ht) {
    if (ht->capacity > UINT_MAX / 2) return;
    unsigned int new_cap = ht->capacity * 2;
    ht->resizes++;

    if (ht->cfg.collision == COLLISION_CHAINING) {
        ChainNode** new_chains = calloc(new_cap, sizeof(ChainNode*));
        if (!new_chains) return;

        for (unsigned int i = 0; i < ht->capacity; i++) {
            ChainNode* node = ht->chains[i];
            while (node) {
                raw_put_chain(new_chains, new_cap, ht->cfg.hash_fn,
                    node->key, node->str_val, node->int_val);
                node = node->next;
            }
            free_chain(ht->chains[i]);
        }
        free(ht->chains);
        ht->chains = new_chains;
        ht->capacity = new_cap;
    }
    else {
        Slot* new_slots = calloc(new_cap, sizeof(Slot));
        if (!new_slots) return;

        for (unsigned int i = 0; i < ht->capacity; i++) {
            if (ht->slots[i].state == SLOT_OCCUPIED) {
                raw_put_slot(new_slots, new_cap, ht->cfg.hash_fn,
                    ht->slots[i].key,
                    ht->slots[i].str_val,
                    ht->slots[i].int_val);
                free(ht->slots[i].key);
                free(ht->slots[i].str_val);
            }
        }
        free(ht->slots);
        ht->slots = new_slots;
        ht->capacity = new_cap;
    }
}

int ht_put(HTable* ht, const char* key, const char* str_val, int int_val) {
    double threshold = ht->capacity * ht->cfg.load_factor;
    if ((double)ht->size >= threshold)
        ht_resize(ht);

    unsigned int base = bucket(ht, key);         

    if (ht->cfg.collision == COLLISION_CHAINING) {
        ChainNode* node = ht->chains[base];
        int first = 1;
        while (node) {
            if (strcmp(node->key, key) == 0) {
                free(node->str_val);
                node->str_val = dup_str(str_val);
                node->int_val = int_val;
                return 1;
            }
            node = node->next;
            first = 0;
        }
        if (!first) ht->collisions++;

        ChainNode* new_node = (ChainNode*)malloc(sizeof(ChainNode));
        if (!new_node) return 0;
        new_node->key = dup_str(key);
        new_node->str_val = dup_str(str_val);
        new_node->int_val = int_val;
        new_node->next = ht->chains[base];
        ht->chains[base] = new_node;
        ht->size++;
        return 1;
    }

    int collision_counted = 0;
    int first_deleted = -1;

    for (unsigned int i = 0; i < ht->capacity; i++) {
        unsigned int idx;
        if (ht->cfg.collision == COLLISION_LINEAR_PROBE)
            idx = (base + i) % ht->capacity;           
        else
            idx = (base + i * i) % ht->capacity;       

        if (ht->slots[idx].state == SLOT_EMPTY) {
            if (!collision_counted && i > 0) ht->collisions++;
            int ins = (first_deleted >= 0) ? (unsigned int)first_deleted : idx;
            ht->slots[ins].state = SLOT_OCCUPIED;
            ht->slots[ins].key = dup_str(key);
            ht->slots[ins].str_val = dup_str(str_val);
            ht->slots[ins].int_val = int_val;
            ht->size++;
            return 1;
        }
        if (ht->slots[idx].state == SLOT_DELETED) {
            if (first_deleted < 0) first_deleted = (int)idx;
            if (!collision_counted && i > 0) { ht->collisions++; collision_counted = 1; }
            continue;
        }
        if (strcmp(ht->slots[idx].key, key) == 0) {
            free(ht->slots[idx].str_val);
            ht->slots[idx].str_val = dup_str(str_val);
            ht->slots[idx].int_val = int_val;
            return 1;
        }
        if (!collision_counted && i > 0) { ht->collisions++; collision_counted = 1; }
    }

    if (first_deleted >= 0) {
        ht->slots[first_deleted].state = SLOT_OCCUPIED;
        ht->slots[first_deleted].key = dup_str(key);
        ht->slots[first_deleted].str_val = dup_str(str_val);
        ht->slots[first_deleted].int_val = int_val;
        ht->size++;
        return 1;
    }
    return 0;
}

int ht_get(HTable* ht, const char* key, char** str_out, int* int_out) {
    unsigned int base = bucket(ht, key); 

    if (ht->cfg.collision == COLLISION_CHAINING) {
        ChainNode* node = ht->chains[base];
        while (node) {
            if (strcmp(node->key, key) == 0) {
                if (str_out) *str_out = node->str_val;
                if (int_out) *int_out = node->int_val;
                return 1;
            }
            node = node->next;
        }
        return 0;
    }

    for (unsigned int i = 0; i < ht->capacity; i++) {
        unsigned int idx;
        if (ht->cfg.collision == COLLISION_LINEAR_PROBE)
            idx = (base + i) % ht->capacity;
        else
            idx = (base + i * i) % ht->capacity;

        if (ht->slots[idx].state == SLOT_EMPTY)    return 0;
        if (ht->slots[idx].state == SLOT_DELETED)  continue;
        if (strcmp(ht->slots[idx].key, key) == 0) {
            if (str_out) *str_out = ht->slots[idx].str_val;
            if (int_out) *int_out = ht->slots[idx].int_val;
            return 1;
        }
    }
    return 0;
}


int ht_del(HTable* ht, const char* key, char** str_out, int* int_out) {
    unsigned int base = bucket(ht, key);

    if (ht->cfg.collision == COLLISION_CHAINING) {
        ChainNode** pp = &ht->chains[base];
        while (*pp) {
            ChainNode* node = *pp;
            if (strcmp(node->key, key) == 0) {
                if (str_out) *str_out = dup_str(node->str_val);
                if (int_out) *int_out = node->int_val;
                *pp = node->next;
                free(node->key);
                free(node->str_val);
                free(node);
                ht->size--;
                return 1;
            }
            pp = &node->next;
        }
        return 0;
    }

    for (unsigned int i = 0; i < ht->capacity; i++) {
        unsigned int idx;
        if (ht->cfg.collision == COLLISION_LINEAR_PROBE)
            idx = (base + i) % ht->capacity;
        else
            idx = (base + i * i) % ht->capacity;

        if (ht->slots[idx].state == SLOT_EMPTY)    return 0;
        if (ht->slots[idx].state == SLOT_DELETED)  continue;
        if (strcmp(ht->slots[idx].key, key) == 0) {
            if (str_out) *str_out = dup_str(ht->slots[idx].str_val);
            if (int_out) *int_out = ht->slots[idx].int_val;
            free(ht->slots[idx].key);
            free(ht->slots[idx].str_val);
            ht->slots[idx].key = NULL; 
            ht->slots[idx].str_val = NULL;
            ht->slots[idx].state = SLOT_DELETED;
            ht->size--;
            return 1;
        }
    }
    return 0;
}



void ht_list(const HTable* ht, unsigned int num) {
    if (num == 0 || num > ht->capacity) num = ht->capacity;

    printf("=== Hash Table State ===\n");
    printf("capacity=%u  size=%u  load=%.3f  collisions=%llu  resizes=%llu\n",
        ht->capacity, ht->size, ht_load(ht),
        ht->collisions, ht->resizes);
    printf("hash=%s  collision=%s\n\n",
        hash_func_name(ht->cfg.hash_fn),
        collision_method_name(ht->cfg.collision));

    if (ht->cfg.collision == COLLISION_CHAINING) {
        for (unsigned int i = 0; i < num; i++) {
            printf("[%4u] ", i);
            ChainNode* node = ht->chains[i];
            if (!node) {
                printf("(empty)\n");
            }
            else {
                int first = 1;
                while (node) {
                    if (!first) printf(" -> ");
                    if (ht->cfg.value_type == VALUE_STRING)
                        printf("{%s: \"%s\"}", node->key, node->str_val ? node->str_val : "");
                    else
                        printf("{%s: %d}", node->key, node->int_val);
                    node = node->next;
                    first = 0;
                }
                printf("\n");
            }
        }
    }
    else {
        for (unsigned int i = 0; i < num; i++) {
            Slot* s = &ht->slots[i];
            printf("[%4u] ", i);
            switch (s->state) {
            case SLOT_EMPTY:
                printf("(empty)\n");
                break;
            case SLOT_DELETED:
                printf("(deleted)\n");
                break;
            case SLOT_OCCUPIED:
                if (ht->cfg.value_type == VALUE_STRING)
                    printf("{%s: \"%s\"}\n", s->key, s->str_val ? s->str_val : "");
                else
                    printf("{%s: %d}\n", s->key, s->int_val);
                break;
            }
        }
    }
    printf("========================\n");
}


double ht_load(const HTable* ht) {
    return (double)ht->size / (double)ht->capacity;
}

void ht_reset_stats(HTable* ht) {
    ht->collisions = 0;
    ht->resizes = 0;
}