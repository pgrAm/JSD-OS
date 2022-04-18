#ifndef LOCKS_H
#define LOCKS_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define	__I386_ONLY

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

typedef struct
{
	int value;
#ifndef SYNC_HAS_CAS_FUNC
	uint8_t tas_lock;
#endif
} lockable_val;

typedef struct
{
	lockable_val ownerPID;
} kernel_mutex;

bool kernel_try_lock_mutex(kernel_mutex* m);
void kernel_lock_mutex(kernel_mutex* m);
void kernel_unlock_mutex(kernel_mutex* m);

typedef struct
{
	lockable_val waitingPID;
	uint8_t unavailable;
} kernel_cv;

void kernel_signal_cv(kernel_cv* m);
void kernel_wait_cv(kernel_cv* m);
bool kernel_try_wait_cv(kernel_cv* m);

#ifdef __cplusplus
}

namespace sync {

consteval lockable_val init_lockable()
{
#ifndef SYNC_HAS_CAS_FUNC
	return {-1, 0};
#else
	return {-1};
#endif
}

consteval kernel_mutex init_mutex()
{
	return {init_lockable()};
}

consteval kernel_cv init_cv()
{
	return {init_lockable(), 1};
}

class cv
{
public:
	constexpr cv() = default;
	~cv() = default;
	cv(const cv&) = delete;
	cv(cv&&) = delete;
	cv& operator=(const cv&) = delete;

	void notify()
	{
		kernel_signal_cv(&m_cv);
	}

	void wait()
	{
		return kernel_wait_cv(&m_cv);
	}

	bool try_wait()
	{
		return kernel_try_wait_cv(&m_cv);
	}
private:
	kernel_cv m_cv = init_cv();
};

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

template<typename Mutex> class shared_lock;

template<typename Mutex>
class lock_guard {
	friend class shared_lock<Mutex>;
public:
	lock_guard(Mutex& m) : m_mutex(&m)
	{
		m_mutex->lock();
	}
	~lock_guard()
	{
		if(m_mutex)
			m_mutex->unlock();
	}
	lock_guard(const lock_guard&) = delete;
	lock_guard(lock_guard&& o) : m_mutex(o.m_mutex)
	{
		o.m_mutex = nullptr;
	}
private:
	Mutex* m_mutex;
};

template<class T> lock_guard(T)->lock_guard<T>;

class shared_mutex
{
public:
	void lock()
	{
		exclusive_mtx.lock();
	}

	void unlock()
	{
		exclusive_mtx.unlock();
	}

	void lock_shared()
	{
		lock_guard l{shared_mtx};
		if(++num_shared == 1) {
			exclusive_mtx.lock();
		}
	}

	void unlock_shared()
	{
		lock_guard l{shared_mtx};
		if(--num_shared == 0) {
			exclusive_mtx.unlock();
		}
	}

private:
	mutex exclusive_mtx;
	mutex shared_mtx;
	size_t num_shared = 0;
};

class shared_wpref_mutex
{
public:
	void lock()
	{
		lock_guard l{shared_mtx};
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
		lock_guard l{shared_mtx};
		exclusive_locked = true;
		kernel_signal_cv(&cond);
	}

	void lock_shared()
	{
		lock_guard l{shared_mtx};
		while(num_exclusive_waiting > 0 || exclusive_locked)
		{
			kernel_wait_cv(&cond);
		}
		++num_shared;
	}

	void unlock_shared()
	{
		lock_guard l{shared_mtx};
		if(--num_shared == 0)
		{
			kernel_signal_cv(&cond);
		}
	}

private:
	kernel_cv cond = init_cv();
	mutex shared_mtx;
	size_t num_shared = 0;
	size_t num_exclusive_waiting = 0;
	bool exclusive_locked = false;
};

class upgradable_shared_mutex
{
public:
	void lock()
	{
		lock_guard l{meta_mutex};
		mtx.lock();
	}

	void unlock()
	{
		mtx.unlock();
	}

	void lock_shared()
	{
		lock_guard l{meta_mutex};
		mtx.lock_shared();
	}

	void unlock_shared()
	{
		mtx.unlock_shared();
	}

	void upgrade()
	{
		lock_guard l{meta_mutex};
		mtx.unlock_shared();
		mtx.lock();
	}

	void downgrade()
	{
		lock_guard l{meta_mutex};
		mtx.unlock();
		mtx.lock_shared();
	}

private:
	mutex meta_mutex;
	shared_mutex mtx;
};

template<typename Mutex>
class shared_lock {
public:
	shared_lock(Mutex& m) : m_mutex(&m)
	{
		m_mutex->lock_shared();
	}
	~shared_lock()
	{
		if(m_mutex)
		{
			m_mutex->unlock_shared();
		}
	}

	shared_lock(lock_guard<upgradable_shared_mutex>&& o)
		: m_mutex(o.m_mutex)
	{
		o.m_mutex = nullptr;
		m_mutex->downgrade();
	}

	shared_lock(const shared_lock&) = delete;
	shared_lock(shared_lock&& o) : m_mutex(o.m_mutex)
	{
		o.m_mutex = nullptr;
	}

private:
	Mutex* m_mutex;
};

template<class T> shared_lock(T)->shared_lock<T>;
}
#endif
#endif