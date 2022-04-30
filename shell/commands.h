#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H

#include <string_view>
#include <vector>

struct command_result
{
	int result;
	bool found;
};

command_result run_command(std::string_view keyword,
						   std::vector<std::string_view>& args);

#endif
