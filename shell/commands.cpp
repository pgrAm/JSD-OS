#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <terminal/terminal.h>
#include <keyboard.h>
#include <sys/syscalls.h>

#include <string>
#include <vector>
#include <charconv>
#include <memory>
#include <array>
#include <algorithm>

#include <shell/util.h>
#include <shell/commands.h>
#include <shell/shell.h>

extern file_h current_directory;
extern std::string current_path;

static void print_help();

static void list_directory(const file_handle* dir_handle)
{
	if(dir_handle == nullptr)
	{
		print_strings("Invalid Directory\n");
		return;
	}

	auto dir = directory_ptr{open_dir_handle(dir_handle, 0)};
	if(!dir)
	{
		print_strings("Something didn't work\n");
		return;
	}

	print_strings("\n Name     Type  Size   Created     Modified\n\n");

	size_t total_bytes = 0;
	size_t i		   = 0;
	file_h f_handle	   = nullptr;

	while((f_handle = file_h{get_file_in_dir(dir.get(), i++)}))
	{
		file_info f;
		if(get_file_info(&f, f_handle.get()) != 0)
		{
			continue;
		}

		struct tm created  = *localtime(&f.time_created);
		struct tm modified = *localtime(&f.time_modified);

		total_bytes += f.size;

		std::string name = {f.name, f.name_len};

		if(f.flags & IS_DIR)
		{
			printf(" %-9s (DIR)     -  %02d-%02d-%4d  %02d-%02d-%4d\n",
				   name.c_str(), created.tm_mon + 1, created.tm_mday,
				   created.tm_year + 1900, modified.tm_mon + 1,
				   modified.tm_mday, modified.tm_year + 1900);
		}
		else
		{
			auto dot = name.find_first_of('.');

			printf(" %-10s %-3s  %5d  %02d-%02d-%4d  %02d-%02d-%4d\n",
				   name.substr(0, dot).c_str(), name.substr(dot + 1).c_str(),
				   f.size, created.tm_mon + 1, created.tm_mday,
				   created.tm_year + 1900, modified.tm_mon + 1,
				   modified.tm_mday, modified.tm_year + 1900);
		}
	}
	printf("\n %5u Files   %5u Bytes\n\n", i - 1, total_bytes);
}

struct command
{
	std::string_view name;
	std::string_view usage;
	std::string_view description;
	size_t min_args;
	int (*func)(const std::vector<std::string_view>& args);
};

struct alias
{
	std::string_view name;
	std::string_view alias_of;
};

constexpr auto aliases = []()
{
	std::array arr{
		alias{"clear", "cls"}, alias{"ls", "dir"},	   alias{"cp", "copy"},
		alias{"cat", "type"},  alias{"del", "delete"}, alias{"rm", "delete"},
	};
	std::sort(arr.begin(), arr.end(),
			  [](auto&& a, auto&& b) { return a.name < b.name; });
	return arr;
}();

constexpr std::array builtin_commands = []()
{
	std::array arr{
		command{"echo", "arg", "Writes text to the terminal", 2,
				[](const auto& keywords)
				{
					print_strings(keywords[1], '\n');
					return 0;
				}},
		command{"time", "", "Gets the current time", 1,
				[](const auto& keywords)
				{
					printf("%u\n", time(nullptr));
					return 0;
				}},
		command{"cls", "", "Clears the terminal", 1,
				[](const auto& keywords)
				{
					clear_console();
					return 0;
				}},
		command{"cd", "directory", "Changes the current directory", 2,
				[](const auto& keywords)
				{
					const auto& path = keywords[1];

					auto dir =
						directory_ptr{open_dir_handle(get_current_dir(), 0)};

					file_info file;
					file_h f_handle{
						find_path(dir.get(), path.data(), path.size(), 0, 0)};

					if(f_handle == nullptr)
					{
						print_strings("Could not find path ", path, '\n');
						return -1;
					}

					if(get_file_info(&file, f_handle.get()) != 0 ||
					   !(file.flags & IS_DIR))
					{
						print_strings(path);
						printf(" is not a valid directory\n");
						return -1;
					}

					current_path.assign(file.name, file.name_len);
					current_directory = std::move(f_handle);
					return 1;
				}},
		command{"dir", "[directory]", "Lists a directory", 1,
				[](const auto& keywords)
				{
					if(keywords.size() > 1)
					{
						auto dir = directory_ptr{
							open_dir_handle(get_current_dir(), 0)};
						file_h list_dir{find_path(dir.get(), keywords[1].data(),
												  keywords[1].size(), 0, 0)};

						list_directory(list_dir.get());
					}
					else
					{
						list_directory(get_current_dir());
					}
					return 0;
				}},
		command{"copy", "source destination", "Copies a file", 3,
				[](const auto& keywords)
				{
					auto dir =
						directory_ptr{open_dir_handle(get_current_dir(), 0)};

					const auto& src = keywords[1];
					const auto& dst = keywords[2];
					file_info file;
					auto src_handle = find_file_in_dir(dir.get(), &file, src);
					if(!src_handle)
					{
						print_strings("Could not find source file ", src, '\n');
						return -1;
					}

					auto src_stream =
						file_ptr{open_file_handle(src_handle.get(), 0)};
					if(!src_stream)
					{
						print_strings("Could not find open source file ", src,
									  '\n');
						return -1;
					}

					auto dst_stream =
						file_ptr{open(dir.get(), dst.data(), dst.size(),
									  FILE_WRITE | FILE_CREATE)};
					if(!dst_stream)
					{
						print_strings("Could not create destination file ", dst,
									  '\n');
						return -1;
					}

					auto dataBuf = std::make_unique<char[]>(file.size);

					read(&dataBuf[0], file.size, src_stream.get());
					write(&dataBuf[0], file.size, dst_stream.get());

					return 0;
				}},
		command{"delete", "target", "Deletes a file", 2,
				[](const auto& keywords)
				{
					auto dir =
						directory_ptr{open_dir_handle(get_current_dir(), 0)};

					file_h f_handle{find_path(dir.get(), keywords[1].data(),
											  keywords[1].size(), 0, 0)};
					if(!f_handle)
					{
						print_strings("Could not find file ", keywords[1],
									  '\n');
						return -1;
					}

					if(delete_file(f_handle.get()) != 0)
					{
						print_strings("Could not delete file ", keywords[1],
									  '\n');
					}
					return 0;
				}},
		command{"type", "target", "Writes a file to the terminal", 2,
				[](const auto& keywords)
				{
					file_info file;
					if(auto f_handle = find_file_in_dir(&file, keywords[1]))
					{
						auto fs = file_ptr{open_file_handle(f_handle.get(), 0)};
						if(fs)
						{
							auto dataBuf = std::make_unique<char[]>(file.size);

							read(&dataBuf[0], file.size, fs.get());

							print_strings(&dataBuf[0], file.size);

							putchar('\n');

							return 0;
						}
					}
					print_strings("Could not open file ", keywords[1], '\n');
					return -1;
				}},
		command{"mkdir", "name", "Creates a new directory", 2,
				[](const auto& keywords)
				{
					auto dir =
						directory_ptr{open_dir_handle(get_current_dir(), 0)};
					return !!find_path(dir.get(), keywords[1].data(),
									   keywords[1].size(), FILE_CREATE, IS_DIR)
							   ? 0
							   : -1;
				}},
		command{"mem", "", "Shows the amount of free memory", 1,
				[](const auto& keywords)
				{
					auto mem = get_free_memory();

					printf("Free memory %u:\n", mem);

					if(auto GiBs = (mem / 0x40000000) % 0x400)
					{
						printf("\t%d GiB(s)\n", GiBs);
					}
					if(auto MiBs = (mem / 0x100000) % 0x400)
					{
						printf("\t%d MiB(s)\n", MiBs);
					}
					if(auto KiBs = (mem / 0x400) % 0x400)
					{
						printf("\t%d KiB(s)\n", KiBs);
					}
					if(auto Bs = mem % 0x400)
					{
						printf("\t%d B(s)\n", Bs);
					}
					return 0;
				}},
		command{"mode", "width height",
				"Changes the display mode of the terminal", 3,
				[](const auto& keywords)
				{
					unsigned int width = 0;
					std::from_chars(keywords[1].cbegin(), keywords[1].cend(),
									width);
					unsigned int height = 0;
					std::from_chars(keywords[2].cbegin(), keywords[2].cend(),
									height);

					if(get_terminal().set_mode(width, height) == 0)
					{
						clear_console();
					}
					else
					{
						printf("Unable to set mode\n");
						return -1;
					}
					return 0;
				}},
		command{"help", "", "Displays the help for the shell", 1,
				[](const auto& keywords)
				{
					print_help();
					return 0;
				}},
	};
	std::sort(arr.begin(), arr.end(),
			  [](auto&& a, auto&& b) { return a.name < b.name; });
	return arr;
}();

constinit auto& longest_name =
	*std::max_element(builtin_commands.begin(), builtin_commands.end(),
					  [](auto&& a, auto&& b)
					  { return a.name.size() < b.name.size(); });

static void print_help()
{
	for(auto&& cmd : builtin_commands)
	{
		print_strings(' ', cmd.name);
		for(size_t i = 0; i < longest_name.name.size() - cmd.name.size(); i++)
		{
			print_strings(' ');
		}
		print_strings(" - ", cmd.description, '\n');
	}

	for(auto&& al : aliases)
	{
		print_strings(' ', al.name);
		for(size_t i = 0; i < longest_name.name.size() - al.name.size(); i++)
		{
			print_strings(' ');
		}
		print_strings(" - Alias of ", al.alias_of, '\n');
	}
	print_strings(" Use the command with -help or /? for more info\n");
}

static void print_help(std::string_view cmd, const command& c)
{
	print_strings("Usage: ", cmd, ' ', c.usage, '\n');
}

command_result run_command(std::string_view keyword,
						   std::vector<std::string_view>& args)
{
	if(auto it =
		   std::lower_bound(aliases.cbegin(), aliases.cend(), keyword,
							[](auto& c, auto val) { return c.name < val; });
	   it != aliases.cend() && it->name == keyword)
	{
		keyword = it->alias_of;
	}

	if(auto it =
		   std::lower_bound(builtin_commands.cbegin(), builtin_commands.cend(),
							keyword,
							[](auto& c, auto val) { return c.name < val; });
	   it != builtin_commands.cend() && it->name == keyword)
	{
		if(args.size() == 2 && (args[1] == "-help" || args[1] == "/?"))
		{
			print_help(keyword, *it);
			return {0, true};
		}
		if(args.size() >= it->min_args)
		{
			return {it->func(args), true};
		}
		print_strings("Incorrect number of arguments\n");
		print_help(keyword, *it);
		return {-1, true};
	}
	return {-1, false};
}