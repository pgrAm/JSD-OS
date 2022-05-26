#include <liballoc.h>

// JSD/OS heap allocator, based on liballoc from SpoonOS by Durand Miller

/**  Durand's Amazing Super Duper Memory functions.  */
//#define DEBUG 1
//#include <stdio.h>
//#include <stdlib.h>

#include <string.h>
#include <assert.h>

#define VERSION 	"1.1j"

#define USE_CASE1
#define USE_CASE2
#define USE_CASE3
#define USE_CASE4
#define USE_CASE5

using align_t = size_t;

// This will conveniently align our pointer upwards
[[nodiscard]] static inline uintptr_t ALIGN(uintptr_t ptr, size_t alignment)
{
	uintptr_t nptr = ptr + sizeof(align_t);
	align_t diff   = (uintptr_t)nptr & (alignment - 1);

	align_t offset = (diff != 0) ? (alignment - diff) : 0;

	nptr += offset;

	memcpy((void*)(nptr - sizeof(align_t)), &offset, sizeof(align_t));
	return nptr;
}

[[nodiscard]] static inline uintptr_t UNALIGN(uintptr_t ptr)
{
	align_t offset;
	memcpy(&offset, (void*)(ptr - sizeof(align_t)), sizeof(align_t));
	return (uintptr_t)ptr - (offset + sizeof(align_t));
}

#define LIBALLOC_MAGIC	0xc001c0de
#define LIBALLOC_DEAD	0xdeaddead

#if defined DEBUG || defined INFO
#include <stdio.h>
#include <stdlib.h>

#define FLUSH()		fflush( stdout )

#endif

// A structure found at the top of all system allocated
// memory blocks. It details the usage of the memory block.
struct heap_allocator::major_block
{
	major_block* prev;		///< Linked list information.
	major_block* next;		///< Linked list information.
	size_t pages;			///< The number of pages in the block.
	size_t size;			///< The number of pages in the block.
	size_t usage;			///< The number of bytes used in the block.
	minor_block* first;		///< A pointer to the first allocated memory in the block.	
};

// This is a structure found at the beginning of all
// sections in a major block which were allocated by a
// malloc, calloc, realloc call.
struct heap_allocator::minor_block
{
	minor_block* prev;		///< Linked list information.
	minor_block* next;		///< Linked list information.
	major_block* block;		///< The owning block. A pointer to the major structure.
	uint32_t magic;			///< A magic number to idenfity correctness.
	size_t size; 			///< The size of the memory allocated. Could be 1 byte or more.
	size_t req_size;		///< The size of memory requested.
};

// ***********   HELPER FUNCTIONS  *******************************

static inline void* liballoc_memset(void* s, int c, size_t n)
{
	// Jake Del Mastro 12/01/2020 - use standard optimized memset
	return memset(s, c, n);
}
static inline void* liballoc_memcpy(void* s1, const void* s2, size_t n)
{
	// Jake Del Mastro 12/01/2020 - use standard optimized memcpy
	return memcpy(s1, s2, n);
}


#if defined DEBUG || defined INFO
static void liballoc_dump()
{
#ifdef DEBUG
	major_block* maj = l_memRoot;
	minor_block* min = NULL;
#endif

	printf("liballoc: ------ Memory data ---------------\n");
	printf("liballoc: System memory allocated: %i bytes\n", l_allocated);
	printf("liballoc: Memory in used (malloc'ed): %i bytes\n", l_inuse);
	printf("liballoc: Warning count: %i\n", l_warningCount);
	printf("liballoc: Error count: %i\n", l_errorCount);
	printf("liballoc: Possible overruns: %i\n", l_possibleOverruns);

#ifdef DEBUG
	while(maj != NULL)
	{
		printf("liballoc: %x: total = %i, used = %i\n",
			   maj,
			   maj->size,
			   maj->usage);

		min = maj->first;
		while(min != NULL)
		{
			printf("liballoc:    %x: %i bytes\n",
				   min,
				   min->size);
			min = min->next;
		}

		maj = maj->next;
	}
#endif

	FLUSH();
}
#endif

// ***************************************************************

heap_allocator::major_block* heap_allocator::allocate_new_page(size_t size)
{
	// This is how much space is required.
	size_t st = size + sizeof(major_block) + sizeof(minor_block);

	// Perfect amount of space?
	if((st % l_pageSize) == 0)
		st = st / (l_pageSize);
	else
		st = st / (l_pageSize)+1;
	// No, add the buffer. 

	// Make sure it's >= the minimum size.
	if(st < l_pageCount) st = l_pageCount;

	major_block* maj = (major_block*)sys_alloc_pages(st);

	if(maj == NULL)
	{
		l_warningCount += 1;
#if defined DEBUG || defined INFO
		printf("liballoc: WARNING: sys_alloc_pages( %i ) return NULL\n", st);
		FLUSH();
#endif
		return nullptr;	// uh oh, we ran out of memory.
	}

	maj->prev = nullptr;
	maj->next = nullptr;
	maj->pages = st;
	maj->size = st * l_pageSize;
	maj->usage = sizeof(major_block);
	maj->first = nullptr;

	l_allocated += maj->size;

#ifdef DEBUG
	printf("liballoc: Resource allocated %x of %i pages (%i bytes) for %i size.\n", maj, st, maj->size, size);

	printf("liballoc: Total memory usage = %i KB\n", (int)((l_allocated / (1024))));
	FLUSH();
#endif

	return maj;
}

void* heap_allocator::malloc_bytes(size_t req_size, size_t align)
{
	unsigned long size = req_size;

	// For alignment, we adjust size so there's enough space to align.
	size += sizeof(align_t) + (align - 1);

	// So, ideally, we really want an alignment of 0 or 1 in order
	// to save space.

	lock();

	if(size == 0)
	{
		l_warningCount += 1;
#if defined DEBUG || defined INFO
		printf("liballoc: WARNING: alloc( 0 ) called from %x\n",
			   __builtin_return_address(0));
		FLUSH();
#endif
		unlock();
		return malloc_bytes(1, align);
	}

	if(l_memRoot == NULL)
	{
#if defined DEBUG || defined INFO
#ifdef DEBUG
		printf("liballoc: initialization of liballoc " VERSION "\n");
#endif
		//atexit( liballoc_dump );
		FLUSH();
#endif

		// This is the first time we are being used.
		l_memRoot = allocate_new_page(size);
		if(l_memRoot == NULL)
		{
			unlock();
#ifdef DEBUG
			printf("liballoc: initial l_memRoot initialization failed\n", p);
			FLUSH();
#endif
			return NULL;
		}

#ifdef DEBUG
		printf("liballoc: set up first memory major %x\n", l_memRoot);
		FLUSH();
#endif
	}

#ifdef DEBUG
	printf("liballoc: %x PREFIX(malloc)( %i ): ",
		   __builtin_return_address(0),
		   size);
	FLUSH();
#endif

	// Now we need to bounce through every major and find enough space....

	major_block* maj = l_memRoot;
	int startedBet = 0;
	unsigned long long bestSize = 0;

	// Start at the best bet....
	if(l_bestBet != NULL)
	{
		bestSize = l_bestBet->size - l_bestBet->usage;

		if(bestSize > (size + sizeof(minor_block)))
		{
			maj = l_bestBet;
			startedBet = 1;
		}
	}

	while(maj != NULL)
	{
		uintptr_t diff = maj->size - maj->usage;
		// free memory in the block

		if(bestSize < diff)
		{
			// Hmm.. this one has more memory then our bestBet. Remember!
			l_bestBet = maj;
			bestSize = diff;
		}


#ifdef USE_CASE1

		// CASE 1:  There is not enough space in this major block.
		if(diff < (size + sizeof(minor_block)))
		{
#ifdef DEBUG
			printf("CASE 1: Insufficient space in block %x\n", maj);
			FLUSH();
#endif

			// Another major block next to this one?
			if(maj->next != NULL)
			{
				maj = maj->next;		// Hop to that one.
				continue;
			}

			if(startedBet == 1)		// If we started at the best bet,
			{							// let's start all over again.
				maj = l_memRoot;
				startedBet = 0;
				continue;
			}

			// Create a new major block next to this one and...
			maj->next = allocate_new_page(size);	// next one will be okay.
			if(maj->next == NULL) break;			// no more memory.
			maj->next->prev = maj;
			maj = maj->next;

			// .. fall through to CASE 2 ..
		}

#endif

#ifdef USE_CASE2

		// CASE 2: It's a brand new block.
		if(maj->first == nullptr)
		{
			maj->first = (minor_block*)((uintptr_t)maj + sizeof(major_block));


			maj->first->magic = LIBALLOC_MAGIC;
			maj->first->prev = nullptr;
			maj->first->next = nullptr;
			maj->first->block = maj;
			maj->first->size = size;
			maj->first->req_size = req_size;
			maj->usage += size + sizeof(minor_block);


			l_inuse += size;

			uintptr_t p =
				ALIGN((uintptr_t)maj->first + sizeof(minor_block), align);

#ifdef DEBUG
			printf("CASE 2: returning %x\n", p);
			FLUSH();
#endif
			unlock();		// release the lock
			return (void*)p;
		}

#endif

#ifdef USE_CASE3

		// CASE 3: Block in use and enough space at the start of the block.
		diff = (uintptr_t)(maj->first);
		diff -= (uintptr_t)maj;
		diff -= sizeof(major_block);

		if(diff >= (size + sizeof(minor_block)))
		{
			// Yes, space in front. Squeeze in.
			maj->first->prev = (minor_block*)((uintptr_t)maj + sizeof(major_block));
			maj->first->prev->next = maj->first;
			maj->first = maj->first->prev;

			maj->first->magic = LIBALLOC_MAGIC;
			maj->first->prev = nullptr;
			maj->first->block = maj;
			maj->first->size = size;
			maj->first->req_size = req_size;
			maj->usage += size + sizeof(minor_block);

			l_inuse += size;

			uintptr_t p =
				ALIGN((uintptr_t)(maj->first) + sizeof(minor_block), align);

#ifdef DEBUG
			printf("CASE 3: returning %x\n", p);
			FLUSH();
#endif
			unlock();		// release the lock
			return (void*)p;
		}

#endif


#ifdef USE_CASE4

		// CASE 4: There is enough space in this block. But is it contiguous?
		minor_block* min = maj->first;

		// Looping within the block now...
		while(min != NULL)
		{
			// CASE 4.1: End of minors in a block. Space from last and end?
			if(min->next == NULL)
			{
				// the rest of this block is free...  is it big enough?
				diff = (uintptr_t)(maj)+maj->size;
				diff -= (uintptr_t)min;
				diff -= sizeof(minor_block);
				diff -= min->size;
				// minus already existing usage..

				if(diff >= (size + sizeof(minor_block)))
				{
					// yay....
					min->next = (minor_block*)((uintptr_t)min + sizeof(minor_block) + min->size);
					min->next->prev = min;
					min = min->next;
					min->next = nullptr;
					min->magic = LIBALLOC_MAGIC;
					min->block = maj;
					min->size = size;
					min->req_size = req_size;
					maj->usage += size + sizeof(minor_block);

					l_inuse += size;

					uintptr_t p =
						ALIGN((uintptr_t)min + sizeof(minor_block), align);
#ifdef DEBUG
					printf("CASE 4.1: returning %x\n", p);
					FLUSH();
#endif
					unlock();		// release the lock
					return (void*)p;
				}
			}



			// CASE 4.2: Is there space between two minors?
			if(min->next != NULL)
			{
				// is the difference between here and next big enough?
				diff = (uintptr_t)(min->next);
				diff -= (uintptr_t)min;
				diff -= sizeof(minor_block);
				diff -= min->size;
				// minus our existing usage.

				if(diff >= (size + sizeof(minor_block)))
				{
					// yay......
					minor_block* new_min = (minor_block*)((uintptr_t)min + sizeof(minor_block) + min->size);

					new_min->magic = LIBALLOC_MAGIC;
					new_min->next = min->next;
					new_min->prev = min;
					new_min->size = size;
					new_min->req_size = req_size;
					new_min->block = maj;
					min->next->prev = new_min;
					min->next = new_min;
					maj->usage += size + sizeof(minor_block);

					l_inuse += size;

					uintptr_t p =
						ALIGN((uintptr_t)new_min + sizeof(minor_block), align);
#ifdef DEBUG
					printf("CASE 4.2: returning %x\n", p);
					FLUSH();
#endif

					unlock();		// release the lock
					return (void*)p;
				}
			}	// min->next != NULL

			min = min->next;
		} // while min != NULL ...


#endif

#ifdef USE_CASE5

		// CASE 5: Block full! Ensure next block and loop.
		if(maj->next == NULL)
		{
#ifdef DEBUG
			printf("CASE 5: block full\n");
			FLUSH();
#endif

			if(startedBet == 1)
			{
				maj = l_memRoot;
				startedBet = 0;
				continue;
			}

			// we've run out. we need more...
			maj->next = allocate_new_page(size);		// next one guaranteed to be okay
			if(maj->next == NULL) break;			//  uh oh,  no more memory.....
			maj->next->prev = maj;

		}

#endif

		maj = maj->next;
	} // while (maj != NULL)



	unlock();		// release the lock

#ifdef DEBUG
	printf("All cases exhausted. No memory available.\n");
	FLUSH();
#endif
#if defined DEBUG || defined INFO
	printf("liballoc: WARNING: PREFIX(malloc)( %i ) returning NULL.\n", size);
	liballoc_dump();
	FLUSH();
#endif
	return nullptr;
}

void heap_allocator::free_bytes(void* to_free)
{
	if(to_free == nullptr)
	{
		l_warningCount += 1;
#if defined DEBUG || defined INFO
		printf("liballoc: WARNING: PREFIX(free)( NULL ) called from %x\n",
			   __builtin_return_address(0));
		FLUSH();
#endif
		return;
	}

	uintptr_t ptr = UNALIGN((uintptr_t)to_free);

	lock();		// lockit

	minor_block* min = (minor_block*)((uintptr_t)ptr - sizeof(minor_block));

	if(min->magic != LIBALLOC_MAGIC)
	{
		l_errorCount += 1;

		// Check for overrun errors. For all bytes of LIBALLOC_MAGIC 
		if(
			((min->magic & 0xFFFFFF) == (LIBALLOC_MAGIC & 0xFFFFFF)) ||
			((min->magic & 0xFFFF) == (LIBALLOC_MAGIC & 0xFFFF)) ||
			((min->magic & 0xFF) == (LIBALLOC_MAGIC & 0xFF))
			)
		{
			l_possibleOverruns += 1;
#if defined DEBUG || defined INFO
			printf("liballoc: ERROR: Possible 1-3 byte overrun for magic %x != %x\n",
				   min->magic,
				   LIBALLOC_MAGIC);
			FLUSH();
#endif
		}


		if(min->magic == LIBALLOC_DEAD)
		{
#if defined DEBUG || defined INFO
			printf("liballoc: ERROR: multiple PREFIX(free)() attempt on %x from %x.\n",
				   ptr,
				   __builtin_return_address(0));
			FLUSH();
#endif
		}
		else
		{
#if defined DEBUG || defined INFO
			printf("liballoc: ERROR: Bad PREFIX(free)( %x ) called from %x\n",
				   ptr,
				   __builtin_return_address(0));
			FLUSH();
#endif
		}

		// being lied to...
		unlock();		// release the lock
		return;
	}

#ifdef DEBUG
	printf("liballoc: %x PREFIX(free)( %x ): ",
		   __builtin_return_address(0),
		   ptr);
	FLUSH();
#endif


	major_block* maj = min->block;

	l_inuse -= min->size;

	maj->usage -= (min->size + sizeof(minor_block));
	min->magic = LIBALLOC_DEAD;		// No mojo.

	if(min->next != NULL) min->next->prev = min->prev;
	if(min->prev != NULL) min->prev->next = min->next;

	if(min->prev == NULL) maj->first = min->next;
	// Might empty the block. This was the first
	// minor.


// We need to clean up after the majors now....

	if(maj->first == NULL)	// Block completely unused.
	{
		if(l_memRoot == maj) l_memRoot = maj->next;
		if(l_bestBet == maj) l_bestBet = nullptr;
		if(maj->prev != NULL) maj->prev->next = maj->next;
		if(maj->next != NULL) maj->next->prev = maj->prev;
		l_allocated -= maj->size;

		sys_free_pages(maj, maj->pages);
	}
	else
	{
		if(l_bestBet != NULL)
		{
			size_t bestSize = l_bestBet->size - l_bestBet->usage;
			size_t majSize	= maj->size - maj->usage;

			if(majSize > bestSize) l_bestBet = maj;
		}

	}


#ifdef DEBUG
	printf("OK\n");
	FLUSH();
#endif

	unlock();		// release the lock
}

void* heap_allocator::calloc_bytes(size_t nobj, size_t size, size_t align)
{
	size_t real_size = nobj * size;

	void* p = malloc_bytes(real_size, align);

	liballoc_memset(p, 0, real_size);

	return p;
}

void* heap_allocator::realloc_bytes(void* p, size_t size, size_t align)
{
	// Honour the case of size == 0 => free old and return NULL
	if(size == 0)
	{
		free_bytes(p);
		return nullptr;
	}

	// In the case of a NULL pointer, return a simple malloc.
	if(p == nullptr) return malloc_bytes(size, align);

	// Unalign the pointer if required.
	uintptr_t ptr = UNALIGN((uintptr_t)p);

	lock();		// lockit

	minor_block* min = (minor_block*)((uintptr_t)ptr - sizeof(minor_block));

	// Ensure it is a valid structure.
	if(min->magic != LIBALLOC_MAGIC)
	{
		l_errorCount += 1;

		// Check for overrun errors. For all bytes of LIBALLOC_MAGIC 
		if(
			((min->magic & 0xFFFFFF) == (LIBALLOC_MAGIC & 0xFFFFFF)) ||
			((min->magic & 0xFFFF) == (LIBALLOC_MAGIC & 0xFFFF)) ||
			((min->magic & 0xFF) == (LIBALLOC_MAGIC & 0xFF))
			)
		{
			l_possibleOverruns += 1;
#if defined DEBUG || defined INFO
			printf("liballoc: ERROR: Possible 1-3 byte overrun for magic %x != %x\n",
				   min->magic,
				   LIBALLOC_MAGIC);
			FLUSH();
#endif
		}


		if(min->magic == LIBALLOC_DEAD)
		{
#if defined DEBUG || defined INFO
			printf("liballoc: ERROR: multiple PREFIX(free)() attempt on %x from %x.\n",
				   ptr,
				   __builtin_return_address(0));
			FLUSH();
#endif
		}
		else
		{
#if defined DEBUG || defined INFO
			printf("liballoc: ERROR: Bad PREFIX(free)( %x ) called from %x\n",
				   ptr,
				   __builtin_return_address(0));
			FLUSH();
#endif
		}

		// being lied to...
		unlock();		// release the lock
		return NULL;
	}

	// Definitely a memory block.

	unsigned int real_size = min->req_size;

	if(real_size >= size)
	{
		min->req_size = size;
		unlock();
		return p;
	}

	unlock();

	// If we got here then we're reallocating to a block bigger than us.
	void* new_block = malloc_bytes(size, align);					// We need to allocate new memory
	liballoc_memcpy(new_block, p, real_size);
	free_bytes(p);

	return (void*)new_block;
}