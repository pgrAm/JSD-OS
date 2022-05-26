#ifndef _LIBALLOC_H
#define _LIBALLOC_H

#include <stddef.h>

class __attribute__((visibility("hidden"))) heap_allocator
{
public:
	void* malloc_bytes(size_t req_size, size_t align);
	void free_bytes(void* ptr);
	void* calloc_bytes(size_t nobj, size_t size, size_t align);
	void* realloc_bytes(void* p, size_t size, size_t align);

	constexpr heap_allocator(int (*lock_func)(),
				   int (*unlock_func)(),
				   void* (*alloc_func)(size_t),
				   int (*free_func)(void*, size_t))
		: lock(lock_func),
		unlock(unlock_func),
		sys_alloc_pages(alloc_func),
		sys_free_pages(free_func)
	{}

private:
	struct minor_block;
	struct major_block;

	struct major_block* l_memRoot = nullptr;	///< The root memory block acquired from the system.
	struct major_block* l_bestBet = nullptr;	///< The major with the most free memory.

	const size_t l_pageSize = 4096;		///< The size of an individual page. Set up in liballoc_init.
	const size_t l_pageCount = 16;		///< The number of pages to request per chunk. Set up in liballoc_init.
	unsigned long long l_allocated = 0;	///< Running total of allocated memory.
	unsigned long long l_inuse = 0;		///< Running total of used memory.

	long long l_warningCount = 0;		///< Number of warnings encountered
	long long l_errorCount = 0;			///< Number of actual errors
	long long l_possibleOverruns = 0;	///< Number of possible overruns

	major_block* allocate_new_page(size_t size);

	// This function is supposed to lock the memory data structures. It
	// could be as simple as disabling interrupts or acquiring a spinlock.
	// It's up to you to decide.
	// 
	// return 0 if the lock was acquired successfully. Anything else is
	// failure.
	int (* const lock)();

	// This function unlocks what was previously locked by the liballoc_lock
	// function.  If it disabled interrupts, it enables interrupts. If it
	// had acquiried a spinlock, it releases the spinlock. etc.
	// 
	// return 0 if the lock was successfully released.
	int (* const unlock)();

	// This is the hook into the local system which allocates pages. It
	// accepts an integer parameter which is the number of pages
	// required.  The page size was set up in the liballoc_init function.
	// 
	// return NULL if the pages were not allocated.
	// return A pointer to the allocated memory.
	void* (* const sys_alloc_pages)(size_t);

	//  This frees previously allocated memory. The void* parameter passed
	// to the function is the exact same value returned from a previous
	// liballoc_alloc call.
	// 
	// The integer value is the number of pages to free.
	// 
	// return 0 if the memory was successfully freed.
	int (* const sys_free_pages)(void*, size_t);
};

#endif


