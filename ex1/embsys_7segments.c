#include "embsys_7segments.h"

#define LED_A (1 << 0)
#define LED_B (1 << 1)
#define LED_C (1 << 2)
#define LED_D (1 << 3)
#define LED_E (1 << 4)
#define LED_F (1 << 5)
#define LED_G (1 << 6)

/* bit 7 defines digit position */
#define UNITS_DIGIT (0 << 7)
#define TENS_DIGIT (1 << 7)

static const int digits[16] =
{
	/* 0 */ LED_A | LED_B | LED_C | LED_D | LED_E | LED_F,
	/* 1 */         LED_B | LED_C,
	/* 2 */ LED_A | LED_B         | LED_D | LED_E         | LED_G,
	/* 3 */ LED_A | LED_B | LED_C | LED_D                 | LED_G,
	/* 4 */         LED_B | LED_C                 | LED_F | LED_G,
	/* 5 */ LED_A         | LED_C | LED_D         | LED_F | LED_G,
	/* 6 */ LED_A         | LED_C | LED_D | LED_E | LED_F | LED_G,
	/* 7 */ LED_A | LED_B | LED_C,
	/* 8 */ LED_A | LED_B | LED_C | LED_D | LED_E | LED_F | LED_G,
	/* 9 */ LED_A | LED_B | LED_C | LED_D         | LED_F | LED_G,
	/* a */ LED_A | LED_B | LED_C         | LED_E | LED_F | LED_G,
	/* b */                 LED_C | LED_D | LED_E | LED_F | LED_G,
	/* c */ LED_A                 | LED_D | LED_E | LED_F,
	/* d */         LED_B | LED_C | LED_D | LED_E         | LED_G,
	/* e */ LED_A                 | LED_D | LED_E | LED_F | LED_G,
	/* f */ LED_A                         | LED_E | LED_F | LED_G
};

void embsys_7segments_write(char value)
{
	/*set unit digit, lower nibble*/
	_sr(UNITS_DIGIT | digits[(value & 0x0F)], EMBSYS_7SEGMENTS_ADDR);
	/*set tens digit, upper nibble*/
	_sr(TENS_DIGIT | digits[(value >> 4) & 0x0F], EMBSYS_7SEGMENTS_ADDR);
}
