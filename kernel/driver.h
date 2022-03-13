#ifndef DRIVER_H
#define DRIVER_H

struct directory_handle;
typedef struct directory_handle directory_handle;

typedef int (*driver_init_func)(directory_handle* cwd);

#endif