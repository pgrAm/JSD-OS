#include "video.h"
#include "portio.h"

#include <stdlib.h>
#include <ctype.h>

#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

#define VIDEOMEM 0xb8000
#define NUMROWS 25
#define NUMCOLS 80

volatile uint16_t cursorpos = 0;

static const uint8_t vt100_colors[8] =
{
	0x00, 0x04, 0x02, 0x06, 0x01, 0x05, 0x03, 0x07
};

uint16_t clearval = 0x0f00;
uint8_t color = 0x0f;
uint8_t is_bright = 0x1;
uint8_t fgr_color = 0x07;
uint8_t bgr_color = 0x0;

void set_color(uint8_t bgr, uint8_t fgr, uint8_t bright)
{
	fgr_color = fgr;
	bgr_color = bgr;
	
	color = ((bgr & 0x0f) << 4) | ((fgr + (bright & 0x01) * 8) & 0x0f);
	is_bright = bright;
	clearval = ((uint16_t)color) << 8 | 0;
}

void set_cursor_offset(uint16_t offset) 
{ 
	cursorpos = offset;
	
	offset >>= 1;
	 // cursor LOW port to vga INDEX register
    outb(REG_SCREEN_CTRL, 0x0f);
    outb(REG_SCREEN_DATA, (uint8_t)(offset & 0xFF));
    // cursor HIGH port to vga INDEX register
    outb(REG_SCREEN_CTRL, 0x0E);
    outb(REG_SCREEN_DATA, (uint8_t)((offset >> 8) & 0xFF));
}

void delete_chars(size_t num)
{
	uint16_t* vmem = (uint16_t*)(VIDEOMEM + cursorpos);
	uint16_t* end = vmem - num;
	
	while(vmem > end)
	{
		vmem--;
		*vmem = clearval;
	}
	
	set_cursor_offset((uint32_t)vmem - 0xb8000);
}

void initialize_video()
{
	cursorpos = get_cursor_offset();
}

uint16_t get_cursor_offset()
{
	outb(REG_SCREEN_CTRL, 0x0f);
	uint16_t offset = inb(REG_SCREEN_DATA);
	outb(REG_SCREEN_CTRL, 0x0e);
	offset += inb(REG_SCREEN_DATA) << 8;
	return offset << 1;
}


void set_cursor_visibility(uint8_t on)
{
	outb(REG_SCREEN_CTRL, 0x0a);
    outb(REG_SCREEN_DATA, on);
}

void set_cursor(uint16_t row, uint16_t col) 
{
	uint16_t offset = (row*80) + col;
	 // cursor LOW port to vga INDEX register
    outb(REG_SCREEN_CTRL, 0x0f);
    outb(REG_SCREEN_DATA, (uint8_t)(offset&0xFF));
    // cursor HIGH port to vga INDEX register
    outb(REG_SCREEN_CTRL, 0x0E);
    outb(REG_SCREEN_DATA, (uint8_t)((offset>>8)&0xFF));
	
	cursorpos = offset << 1;
}



void clear_screen()
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
	set_cursor(24, 0);
	clear_row(24);
}

static const int tab_size = 5;

int handle_escape_sequence(const char* sequence)
{
	static uint16_t cursorstore = 0;

	char* seq = (char*)sequence;
	
	seq++;
	
	if(*seq = '[') //CSI control code
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
					clear_screen();
				}
				break;
			case 'H': //move cursor
				set_cursor(args[0] - 1, args[1] - 1);
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
				cursorstore = cursorpos; //save the current cursor position
				break;
			case 'u':
				cursorpos = cursorstore; //restore the cursor
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
		case '\n':
			offset = 160 - (((int)dest - VIDEOMEM) % 160);
			break;
		case '\t':
			offset = 2*tab_size - (((int)dest - VIDEOMEM) % (2*tab_size));
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

void print_char(const char c)
{
	char* vmem = (char*)VIDEOMEM;
	const char* end = vmem + 0xfa0;
	vmem += cursorpos;
	
	vmem += handle_char(c, vmem);
	
	if(vmem >= end)
	{
		scroll_up();
		vmem = (char*)(VIDEOMEM + cursorpos);
	}
	
	set_cursor_offset((int)vmem - VIDEOMEM);
}

void print_string_len(const char* str, size_t length)
{
	char* vmem = (char*)VIDEOMEM;
	vmem += cursorpos;
	
	const char* currentchar = str;
	const char* lastchar = str + length;
	
	for(; currentchar < lastchar; currentchar++)
	{
		if(*currentchar == '\x1b') //ANSI escape sequence
		{
			currentchar += handle_escape_sequence(currentchar);
		}
		else
		{
			cursorpos += handle_char(*currentchar, vmem);
		}
		
		if(cursorpos >= 0xfa0)
		{
			scroll_up();
		}
		
		vmem = (char*)(VIDEOMEM + cursorpos);
	}
	
	set_cursor_offset((int)vmem - VIDEOMEM);
}

void print_string(const char* str) 
{
	char* vmem = (char*)VIDEOMEM;
	vmem += cursorpos;
	
	const char* currentchar = str;
	
	while(*currentchar != '\0')
	{
		if(*currentchar == '\x1b') //ANSI escape sequence
		{
			currentchar += handle_escape_sequence(currentchar);
		}
		else
		{
			cursorpos += handle_char(*currentchar, vmem);
		}
		
		currentchar++;
		
		if(cursorpos >= 0xfa0)
		{
			scroll_up();
		}
		
		vmem = (char*)(VIDEOMEM + cursorpos);
	}
	
	set_cursor_offset((int)vmem - VIDEOMEM);
}