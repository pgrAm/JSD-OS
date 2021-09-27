#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <hash.h>

uint32_t hash_string(const std::string& name)
{
	uint32_t h = 0;
	for(size_t i = 0; i < name.size(); i++)
	{
		h = (h << 4) + *(uint8_t*)&name[i];
		uint32_t g = h & 0xf0000000;
		if(g)
		{
			h ^= g >> 24;
		}
		h &= ~g;
	}
	return h;
}

hash_map::hash_map(size_t num_buckets) : buckets(num_buckets, nullptr)
{
}

hash_map::~hash_map()
{
	for(size_t i = 0; i < buckets.size(); i++)
	{
		hash_node* entry = buckets[i];

		while(entry != nullptr)
		{
			hash_node* prev = entry;
			entry = entry->next;
			delete prev;
		}

		buckets[i] = nullptr;
	}
}

bool hash_map::lookup(const std::string& key, uint32_t* value)
{
	uint32_t i = hash_string(key) % buckets.size();
	hash_node* entry = buckets[i];

	while(entry != nullptr)
	{
		if(entry->key == key)
		{
			*value = entry->data;
			return true;
		}

		entry = entry->next;
	}

	return false;
}

void hash_map::insert(const std::string& key, const uint32_t value)
{
	uint32_t i = hash_string(key) % buckets.size();
	hash_node* prev = nullptr;
	hash_node* entry = buckets[i];

	while(entry != nullptr && entry->key != key)
	{
		prev = entry;
		entry = entry->next;
	}

	if(entry == nullptr)
	{
		entry = new hash_node{key, value};

		if(prev == nullptr)
		{
			buckets[i] = entry;
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

void hash_map::remove(const std::string& key)
{
	uint32_t i = hash_string(key) % buckets.size();
	hash_node* prev = nullptr;
	hash_node* entry = buckets[i];

	while(entry != nullptr && entry->key != key)
	{
		prev = entry;
		entry = entry->next;
	}

	if(entry == nullptr)
	{
		return;
	}
	else
	{
		if(prev == nullptr)
		{
			buckets[i] = entry->next;
		}
		else
		{
			prev->next = entry->next;
		}
		delete entry;
	}
}

/*void hashmap_print(const hash_map* map)
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
}*/