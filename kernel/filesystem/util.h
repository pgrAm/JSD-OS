#ifndef FS_UTIL_H
#define FS_UTIL_H

struct fs_chunks
{
	size_t start_chunk;

	size_t start_offset;	//offset to start in the first chunk
	size_t start_size;		//size of the first chunk

	size_t num_full_chunks;	//number of complete chunks

	size_t end_size;		//size of the last chunk
};

inline fs_chunks filesystem_chunkify(size_t offset, size_t length, size_t chunk_size)
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
#endif
