#include "hd44780.h"
#include "lcdfont.h"

/* See github.com/dblachut/LCD-font-generator */
static const uint8_t BIG_FONT_20_4[8][8] = {
	{ 3,  7, 15, 31, 32, 32, 32, 32},
	{24, 28, 30, 31, 32, 32, 32, 32},
	{31, 31, 31, 31, 32, 32, 32, 32},
	{31, 30, 28, 24, 32, 32, 32, 32},
	{31, 15,  7,  3, 32, 32, 32, 32},
	{3,   7, 15, 31, 31, 31, 31, 31},
	{24, 28, 30, 31, 31, 31, 31, 31},
	{32, 32, 14, 17, 17, 17, 14, 32}
};

void lcd_init_charmaps(void)
{
	for (uint8_t f = 0; f < 8; f++)
		lcd_create_charmap(f, BIG_FONT_20_4[f]);
}

void lcd_print_col_block(const uint8_t col, const uint8_t row,
			 const uint8_t blk1, const uint8_t blk2,
			 const uint8_t blk3)
{
	lcd_set_cursor(col, row);
	lcd_print_raw(blk1);
	lcd_print_raw(blk2);
	lcd_print_raw(blk3);
}

void print_bigfont(const char *text)
{
	uint8_t col_block = 0;

	while (*text) {
		switch (*text) {
			/* Error. */
			case 'E': {
				lcd_print_col_block(col_block, 0,   5, 2,   3);
				lcd_print_col_block(col_block, 1, 255, 2, 254);
				lcd_print_col_block(col_block, 2,   4, 2,   3);
				break;
			}
			case '0': {
				lcd_print_col_block(col_block, 0,   5,   2,   6);
				lcd_print_col_block(col_block, 1, 255, 254, 255);
				lcd_print_col_block(col_block, 2,   4,   2,   3);
				break;
			}
			case '1': {
				lcd_print_col_block(col_block, 0,   0, 255, 254);
				lcd_print_col_block(col_block, 1, 254, 255, 254);
				lcd_print_col_block(col_block, 2,   2,   2,   2);
				break;
			}
			case '2': {
				lcd_print_col_block(col_block, 0, 0, 2, 6);
				lcd_print_col_block(col_block, 1, 5, 2, 3);
				lcd_print_col_block(col_block, 2, 4, 2, 3);
				break;
			}
			case '3': {
				lcd_print_col_block(col_block, 0,   0, 2,   6);
				lcd_print_col_block(col_block, 1, 254, 2, 255);
				lcd_print_col_block(col_block, 2,   4, 2,   3);
				break;
			}
			case '4': {
				lcd_print_col_block(col_block, 0, 255, 254, 255);
				lcd_print_col_block(col_block, 1,   4,   2, 255);
				lcd_print_col_block(col_block, 2, 254, 254,   2);
				break;
			}
			case '5': {
				lcd_print_col_block(col_block, 0, 255, 2, 3);
				lcd_print_col_block(col_block, 1,   2, 2, 6);
				lcd_print_col_block(col_block, 2,   4, 2, 3);
				break;
			}
			case '6': {
				lcd_print_col_block(col_block, 0, 5,   2, 1);
				lcd_print_col_block(col_block, 1, 255, 2, 6);
				lcd_print_col_block(col_block, 2, 4,   2, 3);
				break;
			}
			case '7': {
				lcd_print_col_block(col_block, 0, 4,   2, 255);
				lcd_print_col_block(col_block, 1, 254, 5,   3);
				lcd_print_col_block(col_block, 2, 254, 2, 254);
				break;
			}
			case '8': {
				lcd_print_col_block(col_block, 0,   5, 2,   6);
				lcd_print_col_block(col_block, 1, 255, 2, 255);
				lcd_print_col_block(col_block, 2,   4, 2,   3);
				break;
			}
			case '9': {
				lcd_print_col_block(col_block, 0, 5, 2,   6);
				lcd_print_col_block(col_block, 1, 4, 2, 255);
				lcd_print_col_block(col_block, 2, 4, 2,   3);
				break;
			}
			case ':': {
				lcd_set_cursor(col_block, 0);
				lcd_print_raw(7);
				lcd_set_cursor(col_block, 1);
				lcd_print_raw(7);
				lcd_set_cursor(col_block, 2);
				lcd_print_raw(254);
				if (col_block > 2)
					col_block -=2;
				break;
			}
			case ' ': {
				col_block -= 2;
				/* Fall through. */
			}
		}
		text++;
		col_block += 3;
	}
}
