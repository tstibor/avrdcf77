#ifndef LCDFONT_H
#define LCDFONT_H

#include <stdint.h>

void lcd_init_charmaps(void);
void lcd_print_col_block(const uint8_t col, const uint8_t row,
			 const uint8_t blk1, const uint8_t blk2,
			 const uint8_t blk3);
void print_bigfont(const char *text);

#endif
