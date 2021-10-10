#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <hash.h>



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