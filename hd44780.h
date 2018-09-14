#ifndef HD44780_H
#define HD44780_H

#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>

/* Commands. */
#define LCD_CLEARDISPLAY	0x01
#define LCD_RETURNHOME		0x02
#define LCD_ENTRYMODESET	0x04
#define LCD_DISPLAYCONTROL	0x08
#define LCD_CURSORSHIFT		0x10
#define LCD_FUNCTIONSET		0x20
#define LCD_SETCGRAMADDR	0x40
#define LCD_SETDDRAMADDR	0x80

/* Flags for display entry mode. */
#define LCD_ENTRYRIGHT		0x00
#define LCD_ENTRYLEFT		0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

/* Flags for display on/off control. */
#define LCD_DISPLAYON	0x04
#define LCD_DISPLAYOFF	0x00
#define LCD_CURSORON	0x02
#define LCD_CURSOROFF	0x00
#define LCD_BLINKON	0x01
#define LCD_BLINKOFF	0x00

/* Flags for display/cursor shift. */
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE	0x00
#define LCD_MOVERIGHT	0x04
#define LCD_MOVELEFT	0x00

/* Flags for function set. */
#define LCD_8BITMODE	0x10
#define LCD_4BITMODE	0x00
#define LCD_2LINE	0x08
#define LCD_1LINE	0x00
#define LCD_5x10DOTS	0x04
#define LCD_5x8DOTS	0x00

void lcd_off(void);
void lcd_on(void);
void lcd_blink_off(void);
void lcd_blink_on(void);
void lcd_cursor_off(void);
void lcd_cursor_on(void);
void lcd_clear(void);
void lcd_home(void);
void lcd_scroll_left(void);
void lcd_scroll_right(void);
void lcd_entry_left(void);
void lcd_entry_right(void);
void lcd_autoscroll_on(void);
void lcd_autoscroll_off(void);
void lcd_init(const uint8_t ncols, const uint8_t nrows);
void lcd_create_charmap(uint8_t location, const uint8_t charmap[]);
void lcd_set_cursor(const uint8_t col, uint8_t row);
void lcd_print(const char *text);
void lcd_print_cr(const char *text, const uint8_t col, const uint8_t row);
void lcd_print_raw(const uint8_t val);

#endif
