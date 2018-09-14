#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "hd44780.h"

/*
  |   1 |   2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 |  15 |  16 |
  |-----+-----+----+----+----+----+----+----+----+----+----+----+----+----+-----+-----|
  | GND | VCC | VO | RS | RW | EN | D0 | D1 | D2 | D3 | D4 | D5 | D6 | D7 | BLA | BLK |

  (taken from https://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller)
  1  Ground
  2  VCC (+3.3 to +5V)
  3  Contrast adjustment (VO)
  4  Register Select (RS). RS=0: Command, RS=1: Data
  5  Read/Write (R/W). R/W=0: Write,
     R/W=1: Read (This pin is optional due to the fact
     that most of the time you will only want to write
     to it and not read. Therefore, in general use, this
     pin will be permanently connected directly to ground.)
  6  Clock (Enable). Falling edge triggered
  7  Bit 0 (Not used in 4-bit operation)
  8  Bit 1 (Not used in 4-bit operation)
  9  Bit 2 (Not used in 4-bit operation)
  10 Bit 3 (Not used in 4-bit operation)
  11 Bit 4
  12 Bit 5
  13 Bit 6
  14 Bit 7
  15 Backlight Anode (+) (If applicable)
  16 Backlight Cathode (-) (If applicable)
 */

#define HD77480_CTRL_DDR	DDRD
#define HD77480_CTRL_PORT	PORTD
#define HD77480_RS		PD4 /* Arduino 4 */
#define HD77480_RW		PD5 /* Arduino 5 */
#define HD77480_EN		PD6 /* Arduino 6 */

#define HD77480_DATA_DDR	DDRC // DDRB
#define HD77480_DATA_PORT	PORTC // PORTB
#define HD77480_D4		PC0 //PB0 /* Arduino 8 */
#define HD77480_D5		PC1 //PB1 /* Arduino 9 */
#define HD77480_D6		PC2 //PB2 /* Arduino 10 */
#define HD77480_D7		PC3 //PB3 /* Arduino 11 */

#define RS_COMMAND(val) send(val, 0)
#define RS_DATA(val) send(val, 1)

static uint8_t row_offsets[4]	= {0};
static uint8_t display_function = 0;
static uint8_t display_control	= 0;
static uint8_t display_mode	= 0;
static uint8_t nrows		= 0;

static void pulse_enable(void)
{
	HD77480_CTRL_PORT &= ~(1 << HD77480_EN);
	_delay_us(1);
	HD77480_CTRL_PORT |= (1 << HD77480_EN);
	_delay_us(1);		/* Enable pulse must be > 450ns. */
	HD77480_CTRL_PORT &= ~(1 << HD77480_EN);
	_delay_us(100);		/* Commands need > 37us to settle.  */

}

static void write4bits(const uint8_t val)
{
	if ((val >> 0) & 0x01)
		HD77480_DATA_PORT |= (1 << HD77480_D4);
	else
		HD77480_DATA_PORT &= ~(1 << HD77480_D4);

	if ((val >> 1) & 0x01)
		HD77480_DATA_PORT |= (1 << HD77480_D5);
	else
		HD77480_DATA_PORT &= ~(1 << HD77480_D5);

	if ((val >> 2) & 0x01)
		HD77480_DATA_PORT |= (1 << HD77480_D6);
	else
		HD77480_DATA_PORT &= ~(1 << HD77480_D6);

	if ((val >> 3) & 0x01)
		HD77480_DATA_PORT |= (1 << HD77480_D7);
	else
		HD77480_DATA_PORT &= ~(1 << HD77480_D7);

	pulse_enable();
}

static void send(const uint8_t val, const uint8_t mode)
{
	if (mode)
		HD77480_CTRL_PORT |= (1 << HD77480_RS);
	else
		HD77480_CTRL_PORT &= ~(1 << HD77480_RS);

	HD77480_CTRL_PORT &= ~(1 << HD77480_RW);

	write4bits(val >> 4);
	write4bits(val);
}

static void set_row_offsets(const uint8_t row0, const uint8_t row1,
			    const uint8_t row2, const uint8_t row3)
{
	row_offsets[0] = row0;
	row_offsets[1] = row1;
	row_offsets[2] = row2;
	row_offsets[3] = row3;
}

static void begin(const uint8_t ncols, const uint8_t _nrows,
		  const uint8_t _dotsize)
{
	nrows = _nrows;

	if (nrows > 1)
		display_function |= LCD_2LINE;

	set_row_offsets(0x00, 0x40, 0x00 + ncols, 0x40 + ncols);

	/* For some 1 lines displays you can select a 10 pixel high font. */
	if ((_dotsize != LCD_5x8DOTS) && (nrows == 1))
		display_function |= LCD_5x10DOTS;

	_delay_us(50000);
	HD77480_CTRL_PORT &= ~(1 << HD77480_RS);
	HD77480_CTRL_PORT &= ~(1 << HD77480_EN);
	HD77480_CTRL_PORT &= ~(1 << HD77480_RW);

	/* Start in 8-bit mode, try to set 4-bit mode. */
	write4bits(0x03);
	/* Wait at least 4.1ms */
	_delay_us(4500);
	/* Second try */
	write4bits(0x03);
	/* Wait min 4.1ms */
	_delay_us(4500);
	/* Third try and go. */
	write4bits(0x03);
	_delay_us(150);
	/* Finally set 4-bit interface. */
	write4bits(0x02);

	/* Set # lines, font size, etc. */
	RS_COMMAND(LCD_FUNCTIONSET | display_function);

	/* Turn display on with no cursor or blinking default. */
	display_control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	lcd_on();
	lcd_clear();

	/* Initialize to default text direction (for roman languages). */
	display_mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	/* Set the entry mode. */
	RS_COMMAND(LCD_ENTRYMODESET | display_mode);
}

void lcd_off(void)
{
	display_control &= ~LCD_DISPLAYON;
	RS_COMMAND(LCD_DISPLAYCONTROL | display_control);
}

void lcd_on(void)
{
	display_control |= LCD_DISPLAYON;
	RS_COMMAND(LCD_DISPLAYCONTROL | display_control);
}

void lcd_blink_off(void)
{
	display_control &= ~LCD_BLINKON;
	RS_COMMAND(LCD_DISPLAYCONTROL | display_control);
}

void lcd_blink_on(void)
{
	display_control |= LCD_BLINKON;
	RS_COMMAND(LCD_DISPLAYCONTROL | display_control);
}

void lcd_cursor_off(void)
{
	display_control &= ~LCD_CURSORON;
	RS_COMMAND(LCD_DISPLAYCONTROL | display_control);
}

void lcd_cursor_on(void)
{
	display_control |= LCD_CURSORON;
	RS_COMMAND(LCD_DISPLAYCONTROL | display_control);
}

void lcd_clear(void)
{
	/* Clear display, set cursor position to zero. */
	RS_COMMAND(LCD_CLEARDISPLAY);
	_delay_us(2000);
}

void lcd_home(void)
{
	/* Set cursor position to zero. */
	RS_COMMAND(LCD_RETURNHOME);
	_delay_us(2000);
}

void lcd_scroll_left(void)
{
	RS_COMMAND(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void lcd_scroll_right(void)
{
	RS_COMMAND(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void lcd_entry_left(void)
{
	display_mode |= LCD_ENTRYLEFT;
	RS_COMMAND(LCD_ENTRYMODESET | display_mode);
}

void lcd_entry_right(void)
{
	display_mode &= ~LCD_ENTRYLEFT;
	RS_COMMAND(LCD_ENTRYMODESET | display_mode);
}

void lcd_autoscroll_on(void)
{
	display_mode |= LCD_ENTRYSHIFTINCREMENT;
	RS_COMMAND(LCD_ENTRYMODESET | display_mode);
}

void lcd_autoscroll_off(void)
{
	display_mode &= ~LCD_ENTRYSHIFTINCREMENT;
	RS_COMMAND(LCD_ENTRYMODESET | display_mode);
}

void lcd_init(const uint8_t ncols, const uint8_t nrows)
{
	/* Make control pins HD77480_RS, HD77480_RW, HD77480_EN as ouput for writing. */
	HD77480_CTRL_DDR |= (1 << HD77480_RS) | (1 << HD77480_RW) | (1 << HD77480_EN);
	/* Make data pins HD77480_D4, HD77480_D5, HD77480_D6, HD77480_D7 as output for writing. */
	HD77480_DATA_DDR |= (1 << HD77480_D4) | (1 << HD77480_D5) |
			    (1 << HD77480_D6) | (1 << HD77480_D7);


	display_function = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin(ncols, nrows, LCD_5x8DOTS);
}

void lcd_create_charmap(uint8_t location, const uint8_t charmap[])
{
	/* We only have 8 locations, 0 to 7. */
	location &= 0x07;
	RS_COMMAND(LCD_SETCGRAMADDR | (location << 3));
	RS_DATA(charmap[0]); RS_DATA(charmap[1]);
	RS_DATA(charmap[2]); RS_DATA(charmap[3]);
	RS_DATA(charmap[4]); RS_DATA(charmap[5]);
	RS_DATA(charmap[6]); RS_DATA(charmap[7]);
}

void lcd_set_cursor(const uint8_t col, uint8_t row)
{
	const uint8_t max_lines = sizeof(row_offsets) / sizeof(*row_offsets);
	if (row >= max_lines)
		row = max_lines - 1;
	if (row >= nrows)
		row = nrows - 1;

	RS_COMMAND(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void lcd_print(const char *text)
{
	while (*text) {
		RS_DATA(*text);
		text++;
	}
}

void lcd_print_cr(const char *text, const uint8_t col, const uint8_t row)
{
	lcd_set_cursor(col, row);
	lcd_print(text);
}

void lcd_print_raw(const uint8_t val)
{
	RS_DATA(val);
}
