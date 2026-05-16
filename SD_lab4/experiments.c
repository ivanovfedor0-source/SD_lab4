#include "experiments.h"
#include "modes.h"
#include "hash_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <sysinfoapi.h>

static char* rand_key(char* buf, int len) {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < len - 1; i++)
        buf[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
    buf[len - 1] = '\0';
    return buf;
}

static double clock_ms(void) {
    static LARGE_INTEGER frequency;
    static int init = 0;
    if (!init) {
        QueryPerformanceFrequency(&frequency);
        init = 1;
    }
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (count.QuadPart * 1000.0) / frequency.QuadPart;
}

static ExpResult run_one(int mode, unsigned int n, double lf) {
    ExpResult res;
    memset(&res, 0, sizeof(res));
    res.n = n;

    HTableConfig cfg = mode_config(mode);
    cfg.load_factor = lf;
    HTable* ht = ht_create(cfg);
    if (!ht) {
        fprintf(stderr, "Failed to create hash table\n");
        return res;
    }

    char** keys = (char**)malloc(n * sizeof(char*));
    if (!keys) return res;

    char buf[32];
    for (unsigned int i = 0; i < n; i++) {
        rand_key(buf, 12);
        keys[i] = _strdup(buf);
    }

    ht_reset_stats(ht);
    double t0 = clock_ms();
    for (unsigned int i = 0; i < n; i++) {
        if (cfg.value_type == VALUE_STRING)
            ht_put(ht, keys[i], "val", 0);
        else
            ht_put(ht, keys[i], NULL, (int)i);
    }
    double t1 = clock_ms();
    res.insert_time_ms = t1 - t0;
    res.collisions = ht->collisions;
    res.resizes = ht->resizes;
    res.final_load = ht_load(ht);

    unsigned int sample = (n < 100) ? n : 100;
    double get_total = 0.0;
    for (unsigned int i = 0; i < sample; i++) {
        unsigned int idx = (unsigned int)(rand() % n);
        double g0 = clock_ms();
        ht_get(ht, keys[idx], NULL, NULL);
        double g1 = clock_ms();
        get_total += g1 - g0;
    }
    res.avg_get_ms = get_total / sample;

    for (unsigned int i = 0; i < n; i++) free(keys[i]);
    free(keys);
    ht_destroy(ht);
    return res;
}

void run_experiments(int mode) {
    printf("\n========================================\n");
    printf("Experiments for %s\n", mode_name(mode));
    printf("load_factor = %.2f\n", mode_recommended_lf(mode));
    printf("========================================\n");

    unsigned int sizes[] = { 100, 1000, 1000000, 10000000 };
    int cnt = (int)(sizeof(sizes) / sizeof(sizes[0]));

    printf("%-12s | %-14s | %-14s | %-12s | %-8s | %-8s\n",
        "N", "Insert(ms)", "AvgGet(ms)", "Collisions", "Resizes", "Load");
    printf("------------------------------------------------------------------------\n");

    for (int i = 0; i < cnt; i++) {
        ExpResult r = run_one(mode, sizes[i], mode_recommended_lf(mode));
        printf("%-12u | %-14.4f | %-14.6f | %-12llu | %-8llu | %-8.3f\n",
            r.n, r.insert_time_ms, r.avg_get_ms,
            (unsigned long long)r.collisions,
            (unsigned long long)r.resizes,
            r.final_load);
    }
}

void find_optimal_load_factor(int mode, double step) {
    unsigned int n = 10000;
    printf("\n== Optimal load_factor search for mode %d (n=%u, step=%.2f) ==\n",
        mode, n, step);
    printf("%-6s | %-10s | %-12s | %-8s | %-12s | %-10s | %-10s\n",
        "LF", "Insert(ms)", "Collisions", "Resizes", "AvgGet(ms)", "MemPenalty", "TotalScore");
    printf("----------------------------------------------------------------------------------------------------------\n");

    double best_score = 1e18;
    double best_lf = 0.75;
    ExpResult best_r;
    memset(&best_r, 0, sizeof(best_r));

    for (double lf = 0.1; lf <= 0.95; lf += step) {
        ExpResult r = run_one(mode, n, lf);

        double score;
        double memory_cost;

        if (mode == 1) {
            memory_cost = (1.0 / lf) * 25.0;
            score = r.insert_time_ms +
                r.resizes * 10.0 +
                memory_cost;
        }
        else if (mode == 2 || mode == 3) {
            memory_cost = (1.0 / lf) * 15.0;
            score = r.insert_time_ms +
                r.collisions * 0.02 +
                r.resizes * 8.0 +
                r.avg_get_ms * 500 +
                memory_cost;
        }
        else {
            memory_cost = (1.0 / lf) * 20.0;
            score = r.insert_time_ms +
                r.collisions * 0.01 +
                r.resizes * 5.0 +
                r.avg_get_ms * 200 +
                memory_cost;
        }

        if (score < best_score) {
            best_score = score;
            best_lf = lf;
            best_r = r;
        }

        printf("%-6.2f | %-10.4f | %-12llu | %-8llu | %-12.6f | %-12.2f | %-10.2f\n",
            lf, r.insert_time_ms,
            (unsigned long long)r.collisions,
            (unsigned long long)r.resizes,
            r.avg_get_ms,
            memory_cost, score);
    }

    printf("\n");
    printf("==========================================================================================================\n");
    printf("=> Recommended load_factor = %.2f\n", best_lf);
    printf("   Insert time: %.4f ms, Collisions: %llu, Resizes: %llu, AvgGet: %.6f ms\n",
        best_r.insert_time_ms,
        (unsigned long long)best_r.collisions,
        (unsigned long long)best_r.resizes,
        best_r.avg_get_ms);
    printf("   Score = %.2f\n", best_score);
    printf("==========================================================================================================\n");
}

double auto_load_factor(void) {
    unsigned long total_mb = 1024;
    long nprocs = 4;

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        total_mb = (unsigned long)(memInfo.ullTotalPhys / (1024 * 1024));
    }

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    nprocs = (long)sysInfo.dwNumberOfProcessors;

    double lf = 0.75;

    if (total_mb < 512) {
        lf = 0.50;
    }
    else if (total_mb < 2048) {
        lf = 0.65;
    }
    else if (total_mb > 8192) {
        lf = 0.85;
    }
    else {
        lf = 0.75;
    }

    if (nprocs >= 16) {
        lf += 0.05;
    }
    else if (nprocs >= 8) {
        lf += 0.03;
    }
    else if (nprocs <= 2) {
        lf -= 0.05;
    }
    else if (nprocs <= 4) {
        lf -= 0.02;
    }

    if (total_mb < 2048 && lf > 0.75) {
        lf = 0.75;
    }
    if (total_mb > 8192 && lf < 0.75) {
        lf = 0.75;
    }
    if (lf > 0.90) lf = 0.90;

    printf("[Auto LF] RAM: %lu MB, CPU cores: %ld -> load_factor = %.2f\n",
        total_mb, nprocs, lf);

    return lf;
}

void print_hw_info(void) {
    printf("=== Hardware info ===\n");

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        unsigned long total_mb = (unsigned long)(memInfo.ullTotalPhys / (1024 * 1024));
        unsigned long free_mb = (unsigned long)(memInfo.ullAvailPhys / (1024 * 1024));
        printf("  Total RAM : %lu MB\n", total_mb);
        printf("  Free  RAM : %lu MB\n", free_mb);
    }

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    printf("  Processors: %u\n", sysInfo.dwNumberOfProcessors);

    printf("  Auto load_factor => %.2f\n", auto_load_factor());
    printf("===============================\n");
}