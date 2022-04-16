#ifndef LOCKS_H
#define LOCKS_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef __I386_ONLY
#define SYNC_HAS_CAS_FUNC 1
#endif

typedef uint32_t int_lock;

static inline int_lock lock_interrupts()
{
	int_lock lock;
	__asm__ volatile("pushfl\n"
		"cli\n"
		"popl %0\n"
		: "=r"(lock));
	return lock;
}

static inline void unlock_interrupts(int_lock lock)
{
	__asm__ volatile("pushl %0\n"
		"popfl\n"
		:
	: "r" (lock));
}

static inline void wait_for_interrupt()
{
	__asm__ volatile("hlt");
}

typedef struct
{
	int ownerPID;
	uint8_t tas_lock;
} kernel_mutex;

bool kernel_try_lock_mutex(kernel_mutex* m);
void kernel_lock_mutex(kernel_mutex* m);
void kernel_unlock_mutex(kernel_mutex* m);

typedef struct
{
	int waitingPID;
	uint8_t unavailable;
	uint8_t tas_lock;
} kernel_cv;

void kernel_signal_cv(kernel_cv* m);
void kernel_wait_cv(kernel_cv* m);
bool kernel_try_wait_cv(kernel_cv* m);

#ifdef __cplusplus
}

namespace sync {

consteval kernel_mutex init_mutex()
{
	return {-1, 0};
}

consteval kernel_cv init_cv()
{
	return {-1, 1};
}

class mutex
{
public:
	constexpr mutex() = default;
	~mutex() = default;
	mutex(const mutex&) = delete;
	mutex(mutex&&) = delete;
	mutex& operator=(const mutex&) = delete;

	void lock()
	{
		kernel_lock_mutex(&m_mtx);
	}

	bool try_lock()
	{
		return kernel_try_lock_mutex(&m_mtx);
	}

	void unlock() 
	{
		kernel_unlock_mutex(&m_mtx);
	}
private:
	kernel_mutex m_mtx = init_mutex();
};

template<typename T>
void atomic_store(T* ptr, T newval)
{
	__atomic_store_n(ptr, newval, __ATOMIC_RELEASE);
}
}

template<typename Mutex>
class scoped_lock {
public:
	scoped_lock(Mutex& m) : m_mutex(&m)
	{
		m_mutex->lock();
	}
	~scoped_lock()
	{
		m_mutex->unlock();
	}
	scoped_lock(const scoped_lock&) = delete;
	scoped_lock(scoped_lock&& o) : m_mutex(o.m_mutex)
	{
		o.m_mutex = nullptr;
	}
private:
	Mutex* m_mutex;
};

template<class T> scoped_lock(T)->scoped_lock<T>;

class shared_mutex 
{
public:
	void lock()
	{
		exclusive_mtx.lock();
	}

	void unlock()
	{
		exclusive_mtx.lock();
	}

	void lock_shared() 
	{
		scoped_lock l{shared_mtx};
		if(++num_shared == 1) {
			exclusive_mtx.lock();
		}
	}

	void unlock_shared()
	{
		scoped_lock l{shared_mtx};
		if(--num_shared == 0) {
			exclusive_mtx.unlock();
		}
	}

private:
	sync::mutex exclusive_mtx;
	sync::mutex shared_mtx;
	size_t num_shared = 0;
};

class shared_wpref_mutex
{
public:
	void lock()
	{
		scoped_lock l{shared_mtx};
		++num_exclusive_waiting;
		while(num_shared > 0 || exclusive_locked)
		{
			kernel_wait_cv(&cond);
		}
		--num_exclusive_waiting;
		exclusive_locked = true;
	}

	void unlock()
	{
		scoped_lock l{shared_mtx};
		exclusive_locked = true;
		kernel_signal_cv(&cond);
	}

	void lock_shared()
	{
		scoped_lock l{shared_mtx};
		while(num_exclusive_waiting > 0 || exclusive_locked)
		{
			kernel_wait_cv(&cond);
		}
		++num_shared;
	}

	void unlock_shared()
	{
		scoped_lock l{shared_mtx};
		if(--num_shared == 0)
		{
			kernel_signal_cv(&cond);
		}
	}

private:
	kernel_cv cond = sync::init_cv();
	sync::mutex shared_mtx;
	size_t num_shared = 0;
	size_t num_exclusive_waiting = 0;
	bool exclusive_locked = false;
};


class shared_lock {
public:
	shared_lock(shared_mutex& m) : m_mutex(&m)
	{
		//printf("lock s\n");
		m_mutex->lock_shared();
	}
	~shared_lock()
	{
		if(m_mutex)
		{
			//printf("unlock s\n");
			m_mutex->unlock_shared();
		}
	}

	shared_lock(const shared_lock&) = delete;	
	shared_lock(shared_lock&& o) : m_mutex(o.m_mutex) 
	{
		o.m_mutex = nullptr;
	}

private:
	shared_mutex* m_mutex;
};

#endif
#endif