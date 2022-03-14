#ifndef DRIVER_H
#define DRIVER_H

struct directory_stream;
typedef struct directory_stream directory_stream;

typedef int (*driver_init_func)(directory_stream* cwd);

#endif