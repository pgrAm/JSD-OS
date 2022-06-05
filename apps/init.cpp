#include <stdio.h>
#include <sys/syscalls.h>
#include <assert.h>
#include <memory>
#include <string>

struct dir_deleter
{
	void operator()(directory_stream* b)
	{
		close_dir(b);
	}
};

using directory_ptr = std::unique_ptr<directory_stream, dir_deleter>;

struct file_deleter
{
	void operator()(file_stream* b)
	{
		close(b);
	}
};

using file_ptr = std::unique_ptr<file_stream, file_deleter>;

struct file_handle_deleter
{
	void operator()(const file_handle* f)
	{
		dispose_file_handle(f);
	}
};

using file_h = std::unique_ptr<const file_handle, file_handle_deleter>;

using namespace std::literals;

static inline void print(char c)
{
	diagnostic_message(&c, 1);
}

static inline void print(std::string_view text)
{
	diagnostic_message(text.data(), text.size());
}

template<typename... Args>
static inline void print_strings(Args&&... args)
{
	(print(std::forward<Args>(args)), ...);
}

static void process_init_file(directory_stream* cwd, file_stream* f);

static void execute_line(std::string_view line, directory_stream* cwd)
{
	auto space = line.find_first_of(' ');
	auto token = line.substr(0, space);

	if(line.size() >= 2 && line[0] == '/' && line[1] == '/')
	{
	}
	else if(token == "load_driver"sv)
	{
		auto filename = line.substr(space + 1);

		print_strings("Loading driver "sv, filename, '\n');

		file_h f{find_path(cwd, filename.data(), filename.size(), 0, 0)};
		if(!f)
		{
			print_strings("Can't find driver ", filename, '\n');
			return;
		}

		if(load_driver(cwd, f.get()) != 0)
		{
			print_strings("Error loading driver ", filename, '\n');
		}
	}
	else if(token == "load_file"sv)
	{
		auto filename = line.substr(space + 1);

		//TODO: fix this stupid hack!
		size_t num_drives = 10;
		for(size_t drive = 0; drive < num_drives; drive++)
		{
			file_h root{get_root_directory(drive)};

			if(!root) continue;

			directory_ptr dir{open_dir_handle(root.get(), 0)};

			if(!dir) continue;

			if(file_ptr stream{
				   open(dir.get(), filename.data(), filename.size(), 0)};
			   !!stream)
			{
				print_strings("Processing file ", filename, '\n');

				process_init_file(dir.get(), stream.get());
				return;
			}
		}
		print_strings("Can't find ", filename, '\n');
	}
	else if(token == "execute"sv)
	{
		auto filename = line.substr(space + 1);
		auto f		  = find_path(cwd, filename.data(), filename.size(), 0, 0);

		if(!f)
		{
			print_strings("Can't find ", filename, '\n');
		}
		else
		{
			print_strings("Executing ", filename, '\n');

			spawn_process(&(*f), cwd, WAIT_FOR_PROCESS);
		}
	}
}

static void process_init_file(directory_stream* cwd, file_stream* f)
{
	assert(cwd);

	file_size_t offset = 0;

	std::string buffer;
	bool eof = false;
	while(!eof)
	{
		char c;
		if(read(offset, &c, sizeof(char), f) != sizeof(char))
		{
			eof = true;
			c	= '\n';
		}

		offset += sizeof(char);

		switch(c)
		{
		case '\r':
			continue;
		case '\n':
			execute_line(buffer, cwd);
			buffer.clear();
			continue;
		default:
			buffer += c;
		}
	}
}

static constexpr std::string_view init_path = "init.sys";

int main(int argc, char** argv)
{
	print_strings("Loading drivers\n"sv);

	directory_ptr cwd{open_dir_handle(get_root_directory(0), 0)};

	if(!cwd)
	{
		print_strings("Corrupted boot drive!\n");
		while(true);
	}

	if(file_ptr f{open(cwd.get(), init_path.data(), init_path.size(), 0)}; !!f)
	{
		process_init_file(cwd.get(), f.get());
		return 0;
	}

	print_strings("Can't find init.sys!\n");
	return -1;
}