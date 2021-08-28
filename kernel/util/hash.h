#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct hash_node;
typedef struct hash_node hash_node;

struct hash_node
{
	const char* key;
	uint32_t data;
	hash_node* next;
};

typedef struct hash_map
{
	hash_node** buckets;
	size_t num_buckets;
} hash_map;

uint32_t hash_string(const char* name);
hash_map* hashmap_create(size_t num_buckets);
hash_node* hashmap_create_node(const char* key, uint32_t value);
void hashmap_free_node(hash_node* node);
void free_hash_map(hash_map* map);
bool hashmap_lookup(const hash_map* map, const char* key, uint32_t* value);
void hashmap_insert(hash_map* map, const char* key, const uint32_t value);
void hashmap_remove(hash_map* map, const char* key);
void hashmap_print(const hash_map* map);
#endif