#ifndef CLIB_INTERNAL_FILE
#define CLIB_INTERNAL_FILE

#include <stddef.h>
#include <api/files.h>

struct FILE
{
	inline size_t write(const char* buf, size_t size)
	{
		return m_write(buf, size, this);
	}
	inline size_t read(char* buf, size_t size)
	{
		return m_read(buf, size, this);
	}
	inline file_size_t get_pos()
	{
		return m_get_pos(this);
	}
	inline int set_pos(file_size_t pos)
	{
		return m_set_pos(pos, this);
	}
	inline file_size_t end()
	{
		return m_end(this);
	}
	inline int close()
	{
		return m_close(this);
	}

	size_t (*m_write)(const char* buf, size_t size, FILE* impl) = nullptr;
	size_t (*m_read)(char* buf, size_t size, FILE* impl)		= nullptr;
	file_size_t (*m_get_pos)(FILE* impl)						= nullptr;
	int (*m_set_pos)(file_size_t pos, FILE* impl)				= nullptr;
	file_size_t (*m_end)(FILE* impl)							= nullptr;
	int (*m_close)(FILE* impl)									= nullptr;
};

#endif