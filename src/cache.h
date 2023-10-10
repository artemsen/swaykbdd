#pragma once

#define CACHE_MAX_KEY_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif

void cache_put(const char* key, int val);
int cache_get(const char *key);

#ifdef __cplusplus
}
#endif
