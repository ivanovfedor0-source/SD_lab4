#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "hash_table.h"

typedef struct {
    unsigned int   n;   
    double         insert_time_ms;
    double         avg_get_ms; 
    unsigned long long collisions;
    unsigned long long resizes; 
    double         final_load;
} ExpResult;


void run_experiments(int mode);


void find_optimal_load_factor(int mode, double step);


double auto_load_factor(void);
void   print_hw_info(void);

#endif