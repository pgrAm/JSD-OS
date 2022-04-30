#ifndef SHELL_UTILS_H
#define SHELL_UTILS_H

#include <memory>
#include <sys/syscalls.h>

struct dir_deleter {
	void operator()(directory_stream* b) { close_dir(b); }
};

using directory_ptr = std::unique_ptr <directory_stream, dir_deleter>;

struct file_deleter {
	void operator()(file_stream* b) { close(b); }
};

using file_ptr = std::unique_ptr <file_stream, file_deleter>;

struct file_handle_deleter {
	void operator()(const file_handle* f) { dispose_file_handle(f); }
};

using file_h = std::unique_ptr <const file_handle, file_handle_deleter>;

#endif
