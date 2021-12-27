#include <kernel/rt_device.h>
#include <kernel/util/hash.h>
#include <bit>
#include <stdio.h>
#include <kernel/kassert.h>
#include <kernel/input.h>
#include <kernel/sysclock.h>

enum mouse_type 
{
	MOUSE_STANDARD = 0,
	MOUSE_WHEEL = 3,
	MOUSE_5BUTTON = 4
};

struct ps2_mouse
{
	uint8_t buffer[4];
	size_t buf_index = 0;
	uint8_t state = 0;
};

ps2_mouse mouse;

static void handle_mouse_packet(ps2_mouse& mouse, rt_device* device)
{
	auto time = sysclock_get_ticks(nullptr);

	mouse.buf_index = 0;

	auto new_state = mouse.buffer[0];

	int rel_x = (int)mouse.buffer[1] - (((int)new_state << 4) & 0x100);
	int rel_y = (int)mouse.buffer[2] - (((int)new_state << 3) & 0x100);
	
	if(rel_x != 0)
	{
		handle_input_event(input_event{0, 0, rel_x, AXIS_MOTION, time});
	}

	if(rel_y != 0)
	{
		handle_input_event(input_event{0, 1, -rel_y, AXIS_MOTION, time});
	}

	uint8_t modified = mouse.state ^ new_state;

	if(modified & (1 << 0))
	{
		auto ev = new_state & (1 << 0) ? BUTTON_DOWN : BUTTON_UP;

		handle_input_event(input_event{0, 0, 0, ev, time});
	}

	if(modified & (1 << 1))
	{
		auto ev = new_state & (1 << 0) ? BUTTON_DOWN : BUTTON_UP;

		handle_input_event(input_event{0, 1, 0, ev, time});
	}


	if(modified & (1 << 2))
	{
		auto ev = new_state & (1 << 0) ? BUTTON_DOWN : BUTTON_UP;

		handle_input_event(input_event{0, 2, 0, ev, time});
	}


	if(device->class_ID == MOUSE_WHEEL)
	{

	}
	else if(device->class_ID == MOUSE_5BUTTON)
	{
		//if(modified & (1 << 1))
		//	new_state& (1 << 1);
		//
		//if(modified & (1 << 2))
		//	new_state& (1 << 2);
	}

	mouse.state = new_state;
}

static void mouse_handler(rt_device* device, const uint8_t* data, size_t length)
{
	for(size_t i = 0; i < length; i++)
	{
		uint8_t byte = data[i];

		mouse.buffer[mouse.buf_index] = byte;

		if(device->class_ID == MOUSE_WHEEL || device->class_ID == MOUSE_5BUTTON)
		{
			if(mouse.buf_index == 3)
			{
				handle_mouse_packet(mouse, device);
				continue;
			}
		}
		else if(mouse.buf_index == 2)
		{
			handle_mouse_packet(mouse, device);
			continue;
		}
		mouse.buf_index++;
	}
}

void ps2_send_command(rt_device* device, uint8_t byte)
{
	device->send_bytes(device, &byte, 1);
}

void ps2_read(rt_device* device)
{
	uint8_t buf;
	device->read_bytes(device, &buf, 1);
}

extern "C" void ps2mouse_init()
{
	rt_device* device = find_realtime_device(0);

	device->on_new_data = mouse_handler;

	ps2_send_command(device, 0xF5);
	ps2_read(device);
	ps2_send_command(device, 0xF6);
	ps2_read(device);
	ps2_send_command(device, 0xF4);
	ps2_read(device);

	mouse.buf_index = 0;
}
