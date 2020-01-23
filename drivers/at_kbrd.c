#include <stdio.h>
#include "../kernel/interrupt.h"
#include "portio.h"
#include "kbrd.h"

static const virtual_keycode key_translation_table[] = {
	/*0x00*/ VK_NONE,		/*0x01*/ VK_ESCAPE,		/*0x02*/ VK_1,				/*0x03*/ VK_2,
	/*0x04*/ VK_3,			/*0x05*/ VK_4,			/*0x06*/ VK_5,				/*0x07*/ VK_6,
	/*0x08*/ VK_7,			/*0x09*/ VK_8,			/*0x0A*/ VK_9,				/*0x0B*/ VK_0,
	/*0x0C*/ VK_MINUS,		/*0x0D*/ VK_EQUALS,		/*0x0E*/ VK_BACKSPACE,		/*0x0F*/ VK_TAB,
	/*0x10*/ VK_Q,			/*0x11*/ VK_W,			/*0x12*/ VK_E,				/*0x13*/ VK_R,
	/*0x14*/ VK_T,			/*0x15*/ VK_Y,			/*0x16*/ VK_U,				/*0x17*/ VK_I,
	/*0x18*/ VK_O,			/*0x19*/ VK_P,			/*0x1A*/ VK_LSQR_BRACKET,	/*0x1B*/ VK_RSQR_BRACKET,
	/*0x1C*/ VK_ENTER,		/*0x1D*/ VK_LCTRL,		/*0x1E*/ VK_A,				/*0x1F*/ VK_S,
	/*0x20*/ VK_D,			/*0x21*/ VK_F,			/*0x22*/ VK_G,				/*0x23*/ VK_H,
	/*0x24*/ VK_J,			/*0x25*/ VK_K,			/*0x26*/ VK_L,				/*0x27*/ VK_SEMICOLON,
	/*0x28*/ VK_APOSTROPHE,	/*0x29*/ VK_BACKTICK,	/*0x2A*/ VK_LSHIFT,			/*0x2B*/ VK_BACKSLASH,
	/*0x2C*/ VK_Z,			/*0x2D*/ VK_X,			/*0x2E*/ VK_C,				/*0x2F*/ VK_V,
	/*0x30*/ VK_B,			/*0x31*/ VK_N,			/*0x32*/ VK_M,				/*0x33*/ VK_COMMA,
	/*0x34*/ VK_PERIOD,		/*0x35*/ VK_SLASH,		/*0x36*/ VK_RSHIFT,			/*0x37*/ VK_NUMPAD_MULIPLY,
	/*0x38*/ VK_LALT,		/*0x39*/ VK_SPACE,		/*0x3A*/ VK_CAPS_LOCK,		/*0x3B*/ VK_F1,
	/*0x3C*/ VK_F2,			/*0x3D*/ VK_F3,			/*0x3E*/ VK_F4,				/*0x3F*/ VK_F5,
	/*0x40*/ VK_F6,			/*0x41*/ VK_F7,			/*0x42*/ VK_F8,				/*0x43*/ VK_F9,
	/*0x44*/ VK_F10,		/*0x45*/ VK_NUMLOCK,	/*0x46*/ VK_CAPS_LOCK,		/*0x47*/ VK_NUMPAD_7,
	/*0x48*/ VK_NUMPAD_8,	/*0x49*/ VK_NUMPAD_9,	/*0x4A*/ VK_NUMPAD_MINUS,	/*0x4B*/ VK_NUMPAD_4,
	/*0x4C*/ VK_NUMPAD_5,	/*0x4D*/ VK_NUMPAD_6,	/*0x4E*/ VK_NUMPAD_PLUS,	/*0x4F*/ VK_NUMPAD_1,
	/*0x50*/ VK_NUMPAD_2,	/*0x51*/ VK_NUMPAD_3,	/*0x52*/ VK_NUMPAD_0,		/*0x53*/ VK_NUMPAD_PERIOD,
	/*0x54*/ VK_NONE,		/*0x55*/ VK_NONE,		/*0x56*/ VK_NONE,			/*0x57*/ VK_F11,
	/*0x58*/ VK_F12,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 VK_NONE,				 VK_NONE,				 VK_NONE,					 VK_NONE,
			 //0xE0:
			 /*0x00*/ VK_NONE,			/*0x01*/ VK_NONE,			/*0x02*/ VK_NONE,		/*0x03*/ VK_NONE,
			 /*0x04*/ VK_NONE,			/*0x05*/ VK_NONE,			/*0x06*/ VK_NONE,		/*0x07*/ VK_NONE,
			 /*0x08*/ VK_NONE,			/*0x09*/ VK_NONE,			/*0x0A*/ VK_NONE,		/*0x0B*/ VK_NONE,
			 /*0x0C*/ VK_NONE,			/*0x0D*/ VK_NONE,			/*0x0E*/ VK_NONE,		/*0x0F*/ VK_NONE,
			 /*0x10*/ VK_MEDIA_PREVIOUS,	/*0x11*/ VK_NONE,			/*0x12*/ VK_NONE,		/*0x13*/ VK_NONE,
			 /*0x14*/ VK_NONE,			/*0x15*/ VK_NONE,			/*0x16*/ VK_NONE,		/*0x17*/ VK_NONE,
			 /*0x18*/ VK_NONE,			/*0x19*/ VK_MEDIA_NEXT,		/*0x1A*/ VK_NONE,		/*0x1B*/ VK_NONE,
			 /*0x1C*/ VK_NUMPAD_ENTER,	/*0x1D*/ VK_RCTRL,			/*0x1E*/ VK_NONE,		/*0x1F*/ VK_NONE,
			 /*0x20*/ VK_MUTE,			/*0x21*/ VK_CALC,			/*0x22*/ VK_MEDIA_PLAY,	/*0x23*/ VK_NONE,
			 /*0x24*/ VK_MEDIA_STOP,		/*0x25*/ VK_NONE,			/*0x26*/ VK_NONE,		/*0x27*/ VK_NONE,
			 /*0x28*/ VK_NONE,			/*0x29*/ VK_NONE,			/*0x2A*/ VK_NONE,		/*0x2B*/ VK_NONE,
			 /*0x2C*/ VK_NONE,			/*0x2D*/ VK_NONE,			/*0x2E*/ VK_VOLUME_UP,	/*0x30*/ VK_VOLUME_DOWN,
			 /*0x31*/ VK_NONE,			/*0x32*/ VK_WWW,			/*0x33*/ VK_NONE,		/*0x33*/ VK_NONE,
			 /*0x34*/ VK_NONE,			/*0x35*/ VK_NUMPAD_DIVIDE,	/*0x36*/ VK_NONE,		/*0x37*/ VK_NONE,
			 /*0x38*/ VK_RALT,			/*0x39*/ VK_NONE,			/*0x3A*/ VK_NONE,		/*0x3B*/ VK_NONE,
			 /*0x3C*/ VK_NONE,			/*0x3D*/ VK_NONE,			/*0x3E*/ VK_NONE,		/*0x3F*/ VK_NONE,
			 /*0x40*/ VK_NONE,			/*0x41*/ VK_NONE,			/*0x42*/ VK_NONE,		/*0x43*/ VK_NONE,
			 /*0x44*/ VK_NONE,			/*0x45*/ VK_NONE,			/*0x46*/ VK_NONE,		/*0x47*/ VK_HOME,
			 /*0x48*/ VK_UP,				/*0x49*/ VK_PAGE_UP,		/*0x4A*/ VK_NONE,		/*0x4B*/ VK_LEFT,
			 /*0x4C*/ VK_NONE,			/*0x4D*/ VK_RIGHT,			/*0x4E*/ VK_NONE,		/*0x4F*/ VK_END,
			 /*0x50*/ VK_DOWN,			/*0x51*/ VK_PAGE_DOWN,		/*0x52*/ VK_INSERT,		/*0x53*/ VK_DELETE,
			 /*0x54*/ VK_NONE,			/*0x55*/ VK_NONE,			/*0x56*/ VK_NONE,		/*0x57*/ VK_NONE,
			 /*0x58*/ VK_NONE,			/*0x59*/ VK_NONE,			/*0x5A*/ VK_NONE,		/*0x5B*/ VK_LMETA,
			 /*0x5C*/ VK_RMETA,			/*0x5D*/ VK_APPS,			/*0x5E*/ VK_POWER,		/*0x5F*/ VK_SLEEP,
			 /*0x60*/ VK_NONE,			/*0x61*/ VK_NONE,			/*0x62*/ VK_NONE,		/*0x63*/ VK_WAKE,
			 /*0x64*/ VK_NONE,			/*0x65*/ VK_WWW_SEARCH,		/*0x66*/ VK_WWW_FAV,	/*0x67*/ VK_WWW_REF,
			 /*0x68*/ VK_WWW_STOP,		/*0x69*/ VK_WWW_FWD,		/*0x6A*/ VK_WWW_BACK,	/*0x6B*/ VK_MYCOMP,
			 /*0x6C*/ VK_MAIL,			/*0x6D*/ VK_MEDIA_SELECT,			 VK_NONE,				 VK_NONE,
					  VK_NONE,					 VK_NONE,					 VK_NONE,				 VK_NONE,
					  VK_NONE,					 VK_NONE,					 VK_NONE,				 VK_NONE,
					  VK_NONE,					 VK_NONE,					 VK_NONE,				 VK_NONE,
					  VK_NONE,					 VK_NONE,					 VK_NONE,				 VK_NONE
};

static INTERRUPT_HANDLER void AT_keyboard_handler(interrupt_frame* r)
{
	send_eoi(1 + 32);

	uint8_t lookup = 0;
	bool pressed = false;
	while (inb(0x64) & 1)
	{
		const uint8_t key = inb(0x60);
		if (key < 0xE0)
		{
			lookup += (key & 0x7f);
			pressed = !(key & 0x80);
		}
		else
		{
			lookup += 128;
		}
	}

	handle_keyevent(key_translation_table[lookup], pressed);
}

void AT_keyboard_init()
{
	irq_install_handler(1, AT_keyboard_handler);
}