#ifndef REALTIME_DEVICE
#define REALTIME_DEVICE
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rt_device;

typedef void (data_handler)(rt_device* device, const uint8_t* data, size_t length);
typedef void (read_handler)(rt_device* device, uint8_t* data, size_t length);

struct rt_device
{
	int class_ID;

	read_handler* read_bytes;
	data_handler* send_bytes;
	data_handler* on_new_data;
	void (*on_removed)(rt_device* device);

	void* impl_data;
};

rt_device* find_realtime_device(int class_ID);

void add_realtime_device(rt_device* d);
void remove_realtime_device(rt_device* d);


#ifdef __cplusplus
}
#endif

#endif