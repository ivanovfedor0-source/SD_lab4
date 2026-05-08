#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "hash_table.h"

/*
 * Результат одного прогона эксперимента.
 */
typedef struct {
    unsigned int   n;              /* кол-во элементов               */
    double         insert_time_ms; /* суммарное время вставки (мс)   */
    double         avg_get_ms;     /* среднее время get (мс)         */
    unsigned long long collisions; /* кол-во коллизий                */
    unsigned long long resizes;    /* кол-во расширений              */
    double         final_load;     /* итоговый load_factor           */
} ExpResult;

/*
 * Провести серию экспериментов для заданного режима (mode=1..5).
 * Проверяет n = 100, 1000, 1000000, 10000000.
 * Выводит таблицу в stdout.
 */
void run_experiments(int mode);

/*
 * Подбор оптимального load_factor для заданного режима.
 * Перебирает load_factor от 0.1 до 0.95 с шагом step,
 * выводит таблицу и сообщает оптимальное значение.
 */
void find_optimal_load_factor(int mode, double step);

/*
 * Доп. задание 2: определить load_factor на основе
 * объёма оперативной памяти машины.
 */
double auto_load_factor(void);
void   print_hw_info(void);

#endif /* EXPERIMENTS_H */