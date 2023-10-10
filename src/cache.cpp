#include "lrucache.hpp"
#include "cache.h"

#define CACHE_MAX_KEYS 1024

cache::lru_cache<std::string> lru_cache(CACHE_MAX_KEYS);

void cache_put(const char *key, int val) {
    lru_cache.put(key, val);
}

int cache_get(const char *key) {
    return lru_cache.get(key);
}
