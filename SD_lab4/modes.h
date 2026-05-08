#ifndef MODES_H
#define MODES_H

#include "hash_table.h"

/*
 * Режимы из таблицы 1 + дополнительные.
 * Каждый режим — это просто HTableConfig.
 *
 * Режим 1: SDBMHash  + Chaining        + string/string + capacity=8
 * Режим 2: djb2      + LinearProbe     + string/int    + capacity=16
 * Режим 3: adler32   + LinearProbe     + string/string + capacity=32
 *
 * Доп. режим 4: FNV-1a    + QuadraticProbe + string/string + capacity=16
 * Доп. режим 5: MurmurHash3+ Chaining       + string/int    + capacity=8
 */

#define MODE_COUNT 5

 /* Возвращает конфиг для режима номер mode (1..MODE_COUNT) */
HTableConfig mode_config(int mode);

/* Имя режима */
const char* mode_name(int mode);

/* Рекомендуемый load_factor для режима (найден экспериментально) */
double mode_recommended_lf(int mode);

#endif /* MODES_H */