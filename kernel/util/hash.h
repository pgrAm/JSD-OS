#ifndef HASH_H
#define HASH_H
#ifdef __cplusplus

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string>
#include <vector>

struct hash_node
{
	std::string key;
	uint32_t data;
	hash_node* next = nullptr;
};

class hash_map
{
public:
	hash_map(size_t num_buckets = 16);
	~hash_map();

	bool lookup(const std::string& key, uint32_t* value);
	void insert(const std::string& key, const uint32_t value);
	void remove(const std::string& key);

private:
	std::vector<hash_node*> buckets;
};

#endif
#endif