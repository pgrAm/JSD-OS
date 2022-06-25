#ifndef FS_UTIL_H
#define FS_UTIL_H

#include<kernel/kassert.h>
#include<stdlib.h>
#include<bit>

struct fs_chunks
{
	size_t start_chunk;

	size_t start_offset;	//offset to start in the first chunk
	size_t start_size;		//size of the first chunk

	size_t num_full_chunks;	//number of complete chunks

	size_t end_size;		//size of the last chunk
};

inline fs_chunks filesystem_chunkify_npow(size_t offset, size_t length, size_t chunk_size)
{
	size_t first_chunk = offset / chunk_size;
	size_t start_offset = offset % chunk_size;

	size_t last_chunk = (offset + length) / chunk_size;
	size_t end_size = (offset + length) % chunk_size;

	if(first_chunk == last_chunk)
	{
		end_size = 0; //start and end are in the same chunk
	}

	size_t start_size = ((length - end_size) % chunk_size);
	size_t num_chunks = (length - (start_size + end_size)) / chunk_size;

	return {first_chunk, start_offset, start_size, num_chunks, end_size};
}

inline fs_chunks filesystem_chunkify(file_size_t offset, size_t length,
									 size_t chunk_size_mask,
									 size_t chunk_size_log2)
{
	size_t first_chunk	= static_cast<size_t>(offset >> chunk_size_log2);
	size_t start_offset = static_cast<size_t>(offset) & chunk_size_mask;

	size_t last_chunk = static_cast<size_t>((offset + length) >> chunk_size_log2);
	size_t end_size	  = static_cast<size_t>(offset + length) & chunk_size_mask;

	if(first_chunk == last_chunk)
	{
		end_size = 0; //start and end are in the same chunk
	}

	size_t start_size = (length - end_size) & chunk_size_mask;
	size_t num_chunks = (length - (start_size + end_size)) >> chunk_size_log2;

	return {first_chunk, start_offset, start_size, num_chunks, end_size};
}

inline fs_chunks filesystem_chunkify(size_t offset, size_t length, size_t chunk_size)
{
	k_assert(std::has_single_bit(chunk_size));
	return filesystem_chunkify(offset, length, chunk_size - 1, (size_t)std::countr_zero(chunk_size));
}

#ifdef __cplusplus
namespace fs
{
template<typename T>
T align_power_2(T x, T align)
{
	k_assert(std::has_single_bit(align));
	return x & ~(align - 1);
}

template<typename T>
T round_up(T x, T align)
{
	return ((x + (align - 1)) / align) * align;
}
};
#endif

#endif
