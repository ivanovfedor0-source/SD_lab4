#include "modes.h"
#include "hash_functions.h"
#include <stdio.h>

HTableConfig mode_config(int mode) {
    HTableConfig cfg;
    cfg.load_factor = mode_recommended_lf(mode);

    switch (mode) {
    case 1:
        cfg.hash_fn = SDBMHash;
        cfg.collision = COLLISION_CHAINING;
        cfg.value_type = VALUE_STRING;
        cfg.init_capacity = 8;
        break;
    case 2:
        cfg.hash_fn = djb2;
        cfg.collision = COLLISION_LINEAR_PROBE;
        cfg.value_type = VALUE_INT;
        cfg.init_capacity = 16;
        break;
    case 3:
        cfg.hash_fn = adler32;
        cfg.collision = COLLISION_LINEAR_PROBE;
        cfg.value_type = VALUE_STRING;
        cfg.init_capacity = 32;
        break;
    case 4:
        cfg.hash_fn = fnv1a;
        cfg.collision = COLLISION_QUADRATIC_PROBE;
        cfg.value_type = VALUE_STRING;
        cfg.init_capacity = 17;
        break;
    case 5:
        cfg.hash_fn = murmur3;
        cfg.collision = COLLISION_CHAINING;
        cfg.value_type = VALUE_INT;
        cfg.init_capacity = 8;
        break;
    default:
        cfg.hash_fn = SDBMHash;
        cfg.collision = COLLISION_CHAINING;
        cfg.value_type = VALUE_STRING;
        cfg.init_capacity = 8;
        break;
    }
    return cfg;
}

const char* mode_name(int mode) {
    switch (mode) {
    case 1: return "Mode 1: SDBMHash + Chaining + string/string (cap=8)";
    case 2: return "Mode 2: djb2 + LinearProbe + string/int (cap=16)";
    case 3: return "Mode 3: adler32 + LinearProbe + string/string (cap=32)";
    case 4: return "Mode 4 [extra]: FNV-1a + QuadraticProbe + string/string (cap=16)";
    case 5: return "Mode 5 [extra]: MurmurHash3 + Chaining + string/int (cap=8)";
    }
    return "Unknown mode";
}

double mode_recommended_lf(int mode) {
    switch (mode) {
    case 1: return 0.90;           
    case 2: return 0.35;        
    case 3: return 0.65;        
    case 4: return 0.65;     
    case 5: return 0.75;           
    }
    return 0.75;
}