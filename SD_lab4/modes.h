#ifndef MODES_H
#define MODES_H

#include "hash_table.h"


#define MODE_COUNT 5


HTableConfig mode_config(int mode);


const char* mode_name(int mode);


double mode_recommended_lf(int mode);

#endif