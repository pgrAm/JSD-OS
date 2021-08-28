#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <hash.h>

uint32_t hash_string(const char* name)
{
	uint32_t h = 0;
	while(*name)
	{
		h = (h << 4) + *(uint8_t*)name++;
		uint32_t g = h & 0xf0000000;
		if(g)
		{
			h ^= g >> 24;
		}
		h &= ~g;
	}
	return h;
}

hash_map* hashmap_create(size_t num_buckets)
{
	hash_map* map = (hash_map*)malloc(sizeof(hash_map));
	map->buckets = (hash_node**)malloc(num_buckets * sizeof(hash_node*));
	memset(map->buckets, 0, num_buckets * sizeof(hash_node*));
	map->num_buckets = num_buckets;
	return map;
}

hash_node* hashmap_create_node(const char* key, uint32_t value)
{
	hash_node* n = (hash_node*)malloc(sizeof(hash_node));
	char* new_key = (char*)malloc(strlen(key));
	strcpy(new_key, key);

	n->key = new_key;
	n->data = value;
	n->next = (hash_node*)NULL;

	return n;
}

void hashmap_free_node(hash_node* node)
{
	free((void*)node->key);
	free(node);
}

void free_hash_map(hash_map* map)
{
	for(size_t i = 0; i < map->num_buckets; i++)
	{
		hash_node* entry = map->buckets[i];

		while(entry != NULL)
		{
			hash_node* prev = entry;
			entry = entry->next;
			hashmap_free_node(prev);
		}

		map->buckets[i] = (hash_node*)NULL;
	}
}

bool hashmap_lookup(const hash_map* map, const char* key, uint32_t* value)
{
	uint32_t i = hash_string(key) % map->num_buckets;
	hash_node* entry = map->buckets[i];

	while(entry != NULL)
	{
		if(strcmp(entry->key, key) == 0)
		{
			*value = entry->data;
			return true;
		}

		entry = entry->next;
	}

	return false;
}

void hashmap_insert(hash_map* map, const char* key, const uint32_t value)
{
	uint32_t i = hash_string(key) % map->num_buckets;
	hash_node* prev = (hash_node*)NULL;
	hash_node* entry = map->buckets[i];

	while(entry != NULL && strcmp(entry->key, key) != 0)
	{
		prev = entry;
		entry = entry->next;
	}

	if(entry == NULL)
	{
		entry = hashmap_create_node(key, value);

		if(prev == NULL)
		{
			map->buckets[i] = entry;
		}
		else
		{
			prev->next = entry;
		}
	}
	else
	{
		entry->data = value;
	}
}

void hashmap_remove(hash_map* map, const char* key)
{
	uint32_t i = hash_string(key) % map->num_buckets;
	hash_node* prev = (hash_node*)NULL;
	hash_node* entry = map->buckets[i];

	while(entry != NULL && strcmp(entry->key, key) != 0)
	{
		prev = entry;
		entry = entry->next;
	}

	if(entry == NULL)
	{
		return;
	}
	else
	{
		if(prev == NULL)
		{
			map->buckets[i] = entry->next;
		}
		else
		{
			prev->next = entry->next;
		}
		hashmap_free_node(entry);
	}
}

void hashmap_print(const hash_map* map)
{
	for(uint32_t i = 0; i < map->num_buckets; i++)
	{
		hash_node* entry = map->buckets[i];

		while(entry != NULL)
		{
			printf("key = %s, value = %X\n", entry->key, entry->data);
			entry = entry->next;
		}
	}
}