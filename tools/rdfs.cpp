#include <time.h>
#include <stdint.h>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>
#include <algorithm>

struct __attribute__((packed)) rdfs_dir_entry
{
	char 		name[8];
	char 		extension[3];
	uint8_t		attributes;
	uint32_t	offset;
	uint32_t	size;
	time_t		modified;
	time_t		created;
};

std::vector<char> readFile(std::string in)
{
	std::ifstream f(in, std::ios::binary);
	return std::vector<char>(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv)
{
	std::vector<std::pair<std::string, std::vector<char>>> files;

	std::string output = "init.rfs";

	for(int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];

		if(arg == "-o")
		{
			output = argv[++i];
		}
		else
		{
			files.emplace_back(arg, readFile(arg));
		}
	}

	std::ofstream f(output, std::ios::binary);

	f.write("RDSK", 4);

	uint32_t num_files = files.size();
	f.write((char*)&num_files, sizeof(uint32_t));

	uint32_t offset = 4 + sizeof(uint32_t) + sizeof(rdfs_dir_entry) * num_files;
	for(auto&& fd : files)
	{
		rdfs_dir_entry e{{}, {}, 0, offset, (uint32_t)fd.second.size(), time(NULL), time(NULL)};
		auto filename = std::filesystem::path(fd.first);
		auto name = filename.stem().string();
		auto extension = filename.extension().string();

		extension.erase(remove(extension.begin(), extension.end(), '.'), extension.end());

		memset(e.name, 0, 8);
		memset(e.extension, 0, 3);
		memcpy(e.name, name.data(), std::min((size_t)8, name.size()));
		memcpy(e.extension, extension.data(), std::min((size_t)3, extension.size()));
		
		f.write((char*)&e, sizeof(rdfs_dir_entry));
		offset += e.size;
	}

	
	for(auto&& fd : files)
	{
		f.write(fd.second.data(), fd.second.size());
	}
}