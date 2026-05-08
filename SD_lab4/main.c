#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hash_table.h"
#include "modes.h"
#include "experiments.h"
#include <locale.h>

/* ─────────────────────────────────────────────
   Вспомогательные функции
───────────────────────────────────────────── */

static void trim_newline(char* s) {
    char* p = strchr(s, '\n');
    if (p) *p = '\0';
    p = strchr(s, '\r');
    if (p) *p = '\0';
}

static void print_help(void) {
    printf(
        "\nКоманды:\n"
        "  put <key> <val>   — вставить/обновить элемент\n"
        "  get <key>         — получить элемент\n"
        "  del <key>         — удалить элемент\n"
        "  list [num]        — показать состояние таблицы (первые num строк)\n"
        "  load              — показать текущий load factor\n"
        "  stats             — показать статистику (коллизии, resize)\n"
        "  mode <1..%d>       — сменить режим работы\n"
        "  lf <value>        — установить load_factor вручную (0..1)\n"
        "  auto_lf           — установить load_factor по железу (доп. задание 2)\n"
        "  hw                — показать информацию об оборудовании\n"
        "  exp [mode]        — запустить эксперименты для режима\n"
        "  optlf [mode]      — подобрать оптимальный load_factor для режима\n"
        "  help              — эта справка\n"
        "  quit / exit       — выход\n\n",
        MODE_COUNT);
}

static void print_value(const HTable* ht, const char* str_val, int int_val) {
    if (ht->cfg.value_type == VALUE_STRING)
        printf("%s\n", str_val ? str_val : "(null)");
    else
        printf("%d\n", int_val);
}

/* ─────────────────────────────────────────────
   Парсинг команды put: поддерживает значения
   со встроенными пробелами (берём всё после
   второго пробела).
───────────────────────────────────────────── */
static int parse_put(const char* line, char* key, char* val, int val_is_int, int* ival) {
    /* Пропускаем "put " */
    const char* p = line + 4;
    while (*p == ' ') p++;

    /* Читаем ключ до пробела */
    const char* kstart = p;
    while (*p && *p != ' ') p++;
    if (p == kstart) return 0;
    int klen = (int)(p - kstart);
    strncpy(key, kstart, klen);
    key[klen] = '\0';

    /* Пропускаем пробелы */
    while (*p == ' ') p++;
    if (!*p) return 0;

    if (val_is_int) {
        *ival = atoi(p);
        val[0] = '\0';
    }
    else {
        strncpy(val, p, 255);
        val[255] = '\0';
    }
    return 1;
}

/* ─────────────────────────────────────────────
   main
───────────────────────────────────────────── */

int main(void) {
    setlocale(LC_ALL, "Russian");
    srand((unsigned)time(NULL));

    int cur_mode = 1;
    HTableConfig cfg = mode_config(cur_mode);
    HTable* ht = ht_create(cfg);
    if (!ht) { fprintf(stderr, "Failed to create hash table\n"); return 1; }

    printf("=== Хэш-таблица — лабораторная работа ===\n");
    printf("Текущий режим: %s\n", mode_name(cur_mode));
    printf("Введите 'help' для списка команд.\n\n");

    char line[1024];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        trim_newline(line);
        if (line[0] == '\0') continue;

        /* ── quit / exit ── */
        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            printf("Выход.\n");
            break;
        }

        /* ── help ── */
        if (strcmp(line, "help") == 0) {
            print_help();
            continue;
        }

        /* ── mode <n> ── */
        if (strncmp(line, "mode ", 5) == 0) {
            int m = atoi(line + 5);
            if (m < 1 || m > MODE_COUNT) {
                printf("Режим должен быть от 1 до %d\n", MODE_COUNT);
            }
            else {
                ht_destroy(ht);
                cur_mode = m;
                cfg = mode_config(cur_mode);
                ht = ht_create(cfg);
                printf("Режим изменён: %s\n", mode_name(cur_mode));
            }
            continue;
        }

        /* ── lf <value> ── */
        if (strncmp(line, "lf ", 3) == 0) {
            double lf = atof(line + 3);
            if (lf <= 0.0 || lf >= 1.0) {
                printf("load_factor должен быть в диапазоне (0, 1)\n");
            }
            else {
                ht->cfg.load_factor = lf;
                printf("load_factor установлен: %.3f\n", lf);
            }
            continue;
        }

        /* ── auto_lf ── */
        if (strcmp(line, "auto_lf") == 0) {
            double lf = auto_load_factor();
            ht->cfg.load_factor = lf;
            printf("auto load_factor = %.3f (установлен)\n", lf);
            continue;
        }

        /* ── hw ── */
        if (strcmp(line, "hw") == 0) {
            print_hw_info();
            continue;
        }

        /* ── put key val ── */
        if (strncmp(line, "put ", 4) == 0) {
            char key[256] = { 0 };
            char val[256] = { 0 };
            int  ival = 0;
            int is_int = (ht->cfg.value_type == VALUE_INT);

            if (!parse_put(line, key, val, is_int, &ival)) {
                printf("Использование: put <key> <val>\n");
                continue;
            }
            if (ht_put(ht, key, is_int ? NULL : val, ival))
                printf("OK\n");
            else
                printf("ERROR: не удалось вставить\n");
            continue;
        }

        /* ── get key ── */
        if (strncmp(line, "get ", 4) == 0) {
            const char* key = line + 4;
            char* sv = NULL;
            int   iv = 0;
            if (ht_get(ht, key, &sv, &iv))
                print_value(ht, sv, iv);
            else
                printf("NULL\n");
            continue;
        }

        /* ── del key ── */
        if (strncmp(line, "del ", 4) == 0) {
            const char* key = line + 4;
            char* sv = NULL;
            int   iv = 0;
            if (ht_del(ht, key, &sv, &iv)) {
                print_value(ht, sv, iv);
                free(sv);
            }
            else {
                printf("NULL\n");
            }
            continue;
        }

        /* ── list [num] ── */
        if (strncmp(line, "list", 4) == 0) {
            unsigned int num = 50;
            if (strlen(line) > 5) num = (unsigned int)atoi(line + 5);
            ht_list(ht, num);
            continue;
        }

        /* ── load ── */
        if (strcmp(line, "load") == 0) {
            printf("load_factor = %.4f  (size=%u / capacity=%u)\n",
                ht_load(ht), ht->size, ht->capacity);
            continue;
        }

        /* ── stats ── */
        if (strcmp(line, "stats") == 0) {
            printf("size=%u  capacity=%u  load=%.4f\n",
                ht->size, ht->capacity, ht_load(ht));
            printf("collisions=%llu  resizes=%llu\n",
                ht->collisions, ht->resizes);
            continue;
        }

        /* ── exp [mode] ── */
        if (strncmp(line, "exp", 3) == 0) {
            int m = cur_mode;
            if (strlen(line) > 4) m = atoi(line + 4);
            if (m < 1 || m > MODE_COUNT) m = cur_mode;
            printf("Запуск экспериментов для режима %d...\n", m);
            run_experiments(m);
            continue;
        }

        /* ── optlf [mode] ── */
        if (strncmp(line, "optlf", 5) == 0) {
            int m = cur_mode;
            if (strlen(line) > 6) m = atoi(line + 6);
            if (m < 1 || m > MODE_COUNT) m = cur_mode;
            find_optimal_load_factor(m, 0.05);
            continue;
        }

        printf("Неизвестная команда: '%s'. Введите 'help'.\n", line);
    }

    ht_destroy(ht);
    return 0;
}