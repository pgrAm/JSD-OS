#include <sys/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv)
{
	auto root = open_dir_handle(get_root_directory(1), 0);

	std::string_view name = "time.txt";

	auto file = open(root, name.data(), name.size(), FILE_WRITE | FILE_CREATE);

	auto tm = time(nullptr);

	auto dt = ctime(&tm);

	write(dt, strlen(dt), file);

	close(file);

	close_dir(root);

	return 0;
}