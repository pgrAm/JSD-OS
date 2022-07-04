rdfs = executable('rdfs', 'rdfs.cpp', cpp_args: ['-std=c++20', '-O2'], native : true)

limine_deploy = executable(
	'limine-deploy', 
	['limine/limine-deploy.c'], 
	c_args: ['-O2', '-D_CRT_SECURE_NO_WARNINGS'], 
	native : true
)