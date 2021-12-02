#ifndef HASH_H
#define HASH_H
#ifdef __cplusplus

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string>
#include <vector>

template <class K, class D>
class hash_map
{
public:
	constexpr hash_map(size_t num_buckets = 16) noexcept : buckets(num_buckets, nullptr)
	{}

	~hash_map()
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

	template<typename _Ky> bool lookup(const _Ky& key, D* value)
	{
		const size_t i = hash(key) % buckets.size();
		hash_node* entry = buckets[i];

		while(entry != nullptr)
		{
			if(key == entry->key)
			{
				*value = entry->data;
				return true;
			}

			entry = entry->next;
		}

		return false;
	}

	template<typename _Ky> void insert(const _Ky& key, const D& value)
	{
		size_t i = hash(key) % buckets.size();
		hash_node* prev = nullptr;
		hash_node* entry = buckets[i];

		while(entry != nullptr && entry->key != key)
		{
			prev = entry;
			entry = entry->next;
		}

		if(entry == nullptr)
		{
			entry = new hash_node{K(key), value};

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

	template<typename _Ky> void remove(const _Ky& key)
	{
		D i = hash_string(key) % buckets.size();
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

private:
	static uint32_t hash(uint32_t x)
	{
		x = ((x >> 16) ^ x) * 0x45d9f3b;
		x = ((x >> 16) ^ x) * 0x45d9f3b;
		x = (x >> 16) ^ x;
		return x;
	}

	static uint32_t hash(const std::string_view name)
	{
		uint32_t h = 0;
		for(size_t i = 0; i < name.size(); i++)
		{
			h = (h << 4) + *(uint8_t*)&name[i];
			const uint32_t g = h & 0xf0000000;
			if(g)
			{
				h ^= g >> 24;
			}
			h &= ~g;
		}
		return h;
	}

	struct hash_node
	{
		K key;
		D data;
		hash_node* next = nullptr;
	};

	std::vector<hash_node*> buckets;
};

#endif
#endif