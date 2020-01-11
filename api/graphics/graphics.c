#include <graphics/graphics.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

uint8_t* VIDEOMEM = NULL;
size_t NUMROWS = 25;
size_t NUMCOLS = 80;
size_t SCREEN_SIZE_BYTES = 80 * 25 * 2;

volatile size_t cursorpos = 0;

inline char* get_current_draw_pointer()
{
	char* vmem = (char*)(VIDEOMEM + cursorpos);
	return vmem;
}

static const uint8_t vt100_colors[8] =
{
	0x00, 0x04, 0x02, 0x06, 0x01, 0x05, 0x03, 0x07
};

uint16_t clearval = 0x0f00;
uint8_t color = 0x0f;
uint8_t is_bright = 0x1;
uint8_t fgr_color = 0x07;
uint8_t bgr_color = 0x0;

void set_cursor_visibility(bool on);

void set_color(uint8_t bgr, uint8_t fgr, uint8_t bright)
{
	fgr_color = fgr;
	bgr_color = bgr;
	
	color = ((bgr & 0x0f) << 4) | ((fgr + (bright & 0x01) * 8) & 0x0f);
	is_bright = bright;
	clearval = ((uint16_t)color) << 8 | 0;
}

void set_cursor_offset(size_t offset) 
{ 
	if (offset > SCREEN_SIZE_BYTES)
	{
		return;
	}

	cursorpos = offset;
	
	//offset >>= 1;
	// // cursor LOW port to vga INDEX register
    //outb(REG_SCREEN_CTRL, 0x0f);
    //outb(REG_SCREEN_DATA, (uint8_t)(offset & 0xFF));
	//
    //// cursor HIGH port to vga INDEX register
    //outb(REG_SCREEN_CTRL, 0x0E);
    //outb(REG_SCREEN_DATA, (uint8_t)((offset >> 8) & 0xFF));
}

void video_erase_chars(size_t num)
{
	uint16_t* vmem = (uint16_t*)(get_current_draw_pointer());
	uint16_t* end = vmem - num;
	
	while(vmem > end)
	{
		*(--vmem) = clearval;
	}
	
	set_cursor_offset((uint32_t)vmem - (uint32_t)VIDEOMEM);
}

int initialize_text_mode(int col, int row)
{
	if (set_video_mode(col, row, VIDEO_TEXT_MODE) == 0)
	{
		NUMROWS = row;
		NUMCOLS = col;
		SCREEN_SIZE_BYTES = col * row * 2;
		VIDEOMEM = map_video_memory();
		set_cursor_visibility(true);
		set_cursor_offset(0);

		return 0;
	}

	return -1;
}

//uint16_t get_cursor_offset()
//{
//	int_lock lock = lock_interrupts();
//	outb(REG_SCREEN_CTRL, 0x0f);
//	uint16_t offset = inb(REG_SCREEN_DATA);
//	outb(REG_SCREEN_CTRL, 0x0e);
//	offset += inb(REG_SCREEN_DATA) << 8;
//	return offset << 1;
//	unlock_interrupts(lock);
//}


void set_cursor_visibility(bool on)
{
	//int_lock lock = lock_interrupts();
	//outb(REG_SCREEN_CTRL, 0x0a);
	//if (on)
	//{
	//	outb(REG_SCREEN_DATA, inb(REG_SCREEN_DATA) & ~0x20);
	//}
	//else
	//{
	//	outb(REG_SCREEN_DATA, inb(REG_SCREEN_DATA) | 0x20);
	//}
	//unlock_interrupts(lock);
}

void set_cursor_position(uint16_t row, uint16_t col)
{
	uint16_t offset = (row*NUMCOLS) + col;
	set_cursor_offset(offset << 1);
}

void video_clear()
{
	uint16_t* vmem = (uint16_t*)VIDEOMEM;
	uint16_t* end = vmem + NUMROWS*NUMCOLS;
	for(; vmem < end; vmem++)
	{
		*vmem = clearval;
	}
	set_cursor_offset(0);
}

void clear_row(uint16_t row)
{
	uint16_t* vmem = (uint16_t*)VIDEOMEM + row*NUMCOLS;
	uint16_t* end = vmem + NUMCOLS;
	for(; vmem < end; vmem++)
	{
		*vmem = clearval;
	}
}

void scroll_up()
{
	memcpy((void*)VIDEOMEM, (void*)(VIDEOMEM + 2*NUMCOLS), 2*NUMCOLS*NUMROWS);
	set_cursor_position(NUMROWS - 1, 0);
	clear_row(NUMROWS - 1);
}

static const int tab_size = 5;

int handle_escape_sequence(const char* sequence)
{
	static uint16_t cursorstore = 0;

	char* seq = (char*)sequence;
	
	seq++;
	
	if(*seq == '[') //CSI control code
	{	
		size_t argnum = 0;
	
		unsigned int args[4] = {0, 0, 0, 0};
		do
		{
			seq++;
			while(isdigit(*seq))
			{
				args[argnum] = args[argnum] * 10 + *seq - '0';
				seq++;
			}
			argnum++;
		}
		while(*seq == ';');
		
		unsigned int n = args[0];
		
		switch(*seq)
		{
			case 'J': //clear screen
				if(n == 2)
				{
					video_clear();
				}
				break;
			case 'H': //move cursor
				set_cursor_position(args[0] - 1, args[1] - 1);
				break;
			case 'm': //SGR
				for(int i = 0; i < argnum; i++)
				{
					int comm = args[i];
					
					if(comm == 1)
					{
						set_color(bgr_color, fgr_color, 1);
					}
					else if(comm == 22)
					{
						set_color(bgr_color, fgr_color, 0);
					}
					else if(comm >= 30 && comm <= 37) //change text color
					{
						set_color(bgr_color, vt100_colors[comm - 30], is_bright);
					}
					else if(comm >= 40 && comm <= 47) //change background color
					{
						set_color(vt100_colors[comm - 40], fgr_color, is_bright);
					}
				}
				break;
			case 's':
			{
				cursorstore = cursorpos; //save the current cursor position
			}
				break;
			case 'u':
			{
				cursorpos = cursorstore; //restore the cursor
			}
				break;
			default:
				break;
		}
	}
	
	return seq - sequence;
}

int handle_char(const char source, char* dest)
{
	int offset = 0;

	switch(source)
	{
	case '\r':
		break;
	case '\n':
		offset = (2 * NUMCOLS) - (((uint8_t*)dest - VIDEOMEM) % (2 * NUMCOLS));
		break;
	case '\t':
		offset = 2 * tab_size - (((uint8_t*)dest - VIDEOMEM) % (2 * tab_size));
		break;
	case '\b':
		*dest = '\0';
		offset = -2;
		break;
	default:
		*dest++ = source;
		*dest = color;
		offset = 2;
		break;
	}
	
	return offset;
}

void print_string(const char* str, size_t length)
{
	char* vmem = get_current_draw_pointer();

	const char* currentchar = str;
	const char* lastchar = str + length;
	
	int output_position = cursorpos;

	for(; currentchar < lastchar; currentchar++)
	{
		if(*currentchar == '\x1b') //ANSI escape sequence
		{
			currentchar += handle_escape_sequence(currentchar);
		}
		else
		{
			output_position += handle_char(*currentchar, vmem);
		}
		
		if(output_position >= SCREEN_SIZE_BYTES)
		{
			scroll_up();
			output_position = (NUMROWS - 1) * 2 * NUMCOLS;
		}
		vmem = (char*)(VIDEOMEM + output_position);
	}
	
	set_cursor_offset(output_position);
}