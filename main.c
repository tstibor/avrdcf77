#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <avr/interrupt.h>
#include "hd44780.h"
#include "lcdfont.h"

#define EDGE_1_MS	800	/* 200ms edge non-inverted. */
#define EDGE_0_MS	900	/* 100ms edge non-inverted.  */
#define EDGE_SYNC_0_MS	1900
#define EDGE_SYNC_1_MS	1800
#define TOLERANCE_MS	40

#define DCF77_DDR	DDRD
#define DCF77_PORT	PORTD
#define DCF77_INPUT	PD2

#define HD77480_COLS	20
#define HD77480_ROWS	4

#define BIT_SYNC	254
#define BIT_NOISY	255

#define OV_BUTTON_DDR	DDRB
#define OV_BUTTON_PORT	PORTB
#define OV_BUTTON_INPUT PB1

struct time_date_t {
	uint8_t sec;		/* 0..59 */
	uint8_t min;		/* 0..59 */
	uint8_t hour;		/* 0..23 */
	uint8_t day;		/* 1..31 */
	uint8_t wday;		/* 1..7 */
	uint8_t month;		/* 1..12 */
	uint8_t year;		/* 0..99 */
	uint8_t cest;		/* 0 false, 1 true */
};

const char *WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

typedef enum {
	WAIT_FOR_SYNC = 0,
	DECODING      = 1,
	OPERATING     = 2
} dcf77_state_t;

typedef enum {
	DATE_TIME  = 0,
	EDGE_INFO  = 1,
	STATISTICS = 2,
	VERSION	   = 3,
	MENU_END   = 4
} menu_state_t;

volatile dcf77_state_t dcf77_state = WAIT_FOR_SYNC;
volatile uint16_t tim0_cnt = 0;
volatile time_t tim1_cnt = 0;
volatile uint8_t bits[60] = {0};
volatile uint8_t bit_cnt = 0;
volatile uint16_t measured_edge = 0;
volatile uint64_t num_invalid_edges = 0;
volatile uint64_t num_total_edges = 0;
uint64_t num_sec_dcf77 = 0;
uint64_t num_sec_timer = 0;
menu_state_t menu_state = EDGE_INFO;

ISR(INT0_vect)
{
	if (tim0_cnt > 0) {
		if (EDGE_1_MS - TOLERANCE_MS < tim0_cnt &&
		    tim0_cnt < EDGE_1_MS + TOLERANCE_MS)
			bits[bit_cnt] = 1;
		else if (EDGE_0_MS - TOLERANCE_MS < tim0_cnt &&
			 tim0_cnt < EDGE_0_MS + TOLERANCE_MS)
			bits[bit_cnt] = 0;
		else if (EDGE_SYNC_0_MS - TOLERANCE_MS < tim0_cnt &&
			 tim0_cnt < EDGE_SYNC_0_MS + TOLERANCE_MS) {
			bits[bit_cnt] = 0;
			bit_cnt = 59;
			bits[bit_cnt] = BIT_SYNC;
			dcf77_state = dcf77_state == WAIT_FOR_SYNC ? DECODING : OPERATING;
		} else if (EDGE_SYNC_1_MS - TOLERANCE_MS < tim0_cnt &&
			   tim0_cnt < EDGE_SYNC_1_MS + TOLERANCE_MS) {
			bits[bit_cnt] = 1;
			bit_cnt = 59;
			bits[bit_cnt] = BIT_SYNC;
			dcf77_state = dcf77_state == WAIT_FOR_SYNC ? DECODING : OPERATING;
		} else {
			/* Noisy undetermined signal. */
			bit_cnt = 59;
			bits[bit_cnt] = BIT_NOISY;
			dcf77_state = WAIT_FOR_SYNC;
			num_invalid_edges++;
		}
		bit_cnt	= (bit_cnt + 1) % 60;
		measured_edge = tim0_cnt;
		num_total_edges++;
		tim0_cnt = 0;
	}
}

ISR(TIMER0_COMPA_vect)
{
	/* This counter is increased every 1ms. */
	tim0_cnt++;
}

ISR(TIMER1_COMPA_vect)
{
	/* This counter is increased every 1sec. */
	tim1_cnt++;
}

void print_ms_bit(const uint8_t bit)
{
	char edge_ms[HD77480_COLS + 1] = {0};

	if (bit == BIT_NOISY)
		sprintf(edge_ms, "%04d     %c", measured_edge, 'E');
	else
		sprintf(edge_ms, "%04d     %d", measured_edge, bit);
	print_bigfont(edge_ms);

	lcd_print_cr("     ", 12, 0);
	lcd_print_cr(" ms  ", 12, 1);
	lcd_print_cr("     ", 12, 2);
}

void print_date_time(const struct time_date_t *time_date, const bool is_dcf77)
{
	char time[HD77480_COLS + 1] = {0};
	char date[HD77480_COLS + 1] = {0};

	sprintf(time, "%02d:%02d:%02d", time_date->hour, time_date->min, time_date->sec);
	sprintf(date, "%s %02d.%02d.20%02d %s", WEEKDAYS[time_date->wday % 7],
		time_date->day, time_date->month, time_date->year,
		time_date->cest ? "CEST " : "CET  ");

	print_bigfont(time);
	lcd_print_cr(date, 0, 3);

	if (is_dcf77) {
		/* Print antenna symbol to visualize that clock is using DCF77 signal. */
		lcd_set_cursor(19, 3);
		lcd_print_raw(183);
	}
}

void print_dcf77_state(void)
{
	char sync_cnt_str[HD77480_COLS + 1] = {0};

	dcf77_state == WAIT_FOR_SYNC ?
		sprintf(sync_cnt_str, "waiting for sync %3d", bit_cnt) :
		sprintf(sync_cnt_str, "decoding bit     %3d", bit_cnt);
	lcd_print_cr(sync_cnt_str, 0, 3);
}

void print_stats(void)
{
	char edge_error_rate[20] = {0};
	char uptime_dcf77_rate[20] = {0};
	char mean_sd_800[20] = "mean:sd: 809:12";
	char mean_sd_900[20] = "mean:sd: 911:08";

	snprintf(edge_error_rate, 20, "error rate: %3.2f %%",
		 100.0 * num_invalid_edges / num_total_edges);
	snprintf(uptime_dcf77_rate, 20, "dcf uptime: %3.2f %%",
		 100.0 * num_sec_dcf77 / (num_sec_dcf77 + num_sec_timer));

	lcd_clear();
	lcd_print_cr(edge_error_rate, 0, 0);
	lcd_print_cr(uptime_dcf77_rate, 0, 1);
	lcd_print_cr(mean_sd_800, 0, 2);
	lcd_print_cr(mean_sd_900, 0, 3);
}

uint8_t parity(volatile const uint8_t bits[60],
	       const uint8_t f, const uint8_t t)
{
	uint8_t res = 0;

	for (uint8_t b = f; b <= t; b++)
		res ^= bits[b];

	return res;
}

uint8_t decode_bcd(volatile const uint8_t bits[60], const uint8_t len)
{
	uint8_t res = 0;

	for (uint8_t l = 0; l < len; l++)
		res += bits[l] * (1 << l);

	return res;
}

bool decode(volatile const uint8_t bits[60], struct time_date_t *time_date)
{
	time_date->cest	 = bits[17];
	time_date->sec	 = bit_cnt;
	time_date->min	 = 10 *	decode_bcd(&bits[25], 3) + decode_bcd(&bits[21], 4);
	time_date->hour	 = 10 *	decode_bcd(&bits[33], 2) + decode_bcd(&bits[29], 4);
	time_date->day	 = 10 *	decode_bcd(&bits[40], 2) + decode_bcd(&bits[36], 4);
	time_date->wday	 = decode_bcd(&bits[42], 3);
	time_date->month = 10 * decode_bcd(&bits[49], 1) + decode_bcd(&bits[45], 4);
	time_date->year	 = 10 *	decode_bcd(&bits[54], 4) + decode_bcd(&bits[50], 4);

	/* Bit sanity checks. */
	return (!(bits[0] || !bits[20] || parity(bits, 21, 28)
		  || parity(bits, 29, 35) || parity(bits, 36, 58)));
}

time_t as_time_t(const struct time_date_t *time_date)
{
	struct tm _tm;
	memset(&_tm, 0, sizeof(struct tm));

	_tm.tm_sec  = time_date->sec;
	_tm.tm_min  = time_date->min;
	_tm.tm_hour = time_date->hour;
	_tm.tm_mday = time_date->day;
	_tm.tm_mon  = time_date->month - 1;
	_tm.tm_wday = time_date->wday;
	_tm.tm_year = time_date->year + 100;
	_tm.tm_isdst = ~time_date->cest;

	return (mktime(&_tm));
}

void as_time_date_t(struct time_date_t *time_date)
{
	struct tm *_tm = localtime((time_t *)&tim1_cnt);

	time_date->sec = _tm->tm_sec;
	time_date->min = _tm->tm_min;
	time_date->hour = _tm->tm_hour;
	time_date->day = _tm->tm_mday;
	time_date->month = _tm->tm_mon + 1;
	time_date->wday = _tm->tm_wday;
	time_date->year = _tm->tm_year - 100;
	time_date->cest = ~_tm->tm_isdst;
}

void init(void)
{
	/* Init DCF77_INPUT as signal input. */
	DCF77_DDR &= ~(1 << DCF77_INPUT);
	/* Activate Pull-Up resistor on DCF77_INPUT. */
	DCF77_PORT |= (1 << DCF77_INPUT);

	/* Low level ¯¯¯|__|¯¯¯ on INT0 generates an interrupt request. */
	EICRA = 0;

	/* Activate INT0. */
	EIMSK |= (1 << INT0);

	/* Initialize 1msec interrupt timer. */
	/* Set the timer to clear timer on compare match mode (CTC). */
	TCCR0A |= (1 << WGM01);

	/* Set prescaler to 64. */
	TCCR0B |= (1 << CS01) | (1 << CS00);

	/* We want a delay of 1ms (1 * 10^(-3)),
	   run at 16Mhz (16 * 10^6) and use a prescaler of 64, thus:
	   Timer count = Delay / Clock time period - 1
	           249 = (1 * 10^(-3)) / (1 / (16 * 10^6 / 64)) - 1
	   If this value is reached, then 1ms is passed. */
	OCR0A = 249;

	/* Enable compare and match ISR(TIMER0_COMPA_vect). */
	TIMSK0 |= (1 << OCIE0A);

	/* Initialize 1sec interrupt timer. */
	/* Set prescaler to 1024 and CTC mode. */
	TCCR1B |= (1 << CS12) | (1 << CS10) | (1 << WGM12);

	/* We want a delay of 1s, run at 16 Mhz and use a prescaler of 1024,
	   thus: (1 * 1) / (1 / (16 * 10^6 / 1024)) - 1 = 15624. */
	OCR1A = 15624;

	/* Enable compare and match ISR(TIMER1_COMPA_vect). */
	TIMSK1 |= (1 << OCIE1A);

	/* Enable interrupts. */
	sei();

	/* Init hd77480 LCD. */
	lcd_init(HD77480_COLS, HD77480_ROWS);
	lcd_init_charmaps();

	/* Init overview button as input switch. */
	OV_BUTTON_DDR &= ~(1 << OV_BUTTON_INPUT);
	/* Activate Pull-Up resistor on OV_BUTTON_INPUT. */
	OV_BUTTON_PORT |= (1 << OV_BUTTON_INPUT);
}

int main(void)
{
	struct time_date_t time_date;
	memset(&time_date, 0, sizeof(struct time_date_t));
	bool valid = false;
	struct time_date_t timer_time_date = {.year = 0};

	init();

	while (1) {
		if (!(PINB & (1 << OV_BUTTON_INPUT))) {
			_delay_us(1000);
			if (!(PINB & (1 << OV_BUTTON_INPUT)))
				menu_state = (menu_state + 1) % MENU_END;
		}

		/* Run in 1sec interrupt timer mode when no valid dcf77
		   edges can be received. We use timer_time_date.year to make
		   sure we received at least once a valid decoded dcf77 time. */
		if (timer_time_date.year && dcf77_state != OPERATING) {
			num_sec_timer++; /* For statistics, count number of seconds
					    we run in timer mode. */
			as_time_date_t(&time_date);
			print_date_time(&time_date, false);
		}
		/* A valid dcf77 edge was detected. */
		else if (tim0_cnt == 0) {
			if (dcf77_state == OPERATING) {
				time_date.sec = bit_cnt;
				num_sec_dcf77++; /* For statistics, count number of seconds
						    we run in operating dcf77 mode. */
				if (bits[59] == BIT_SYNC) {
					bits[59] = 0;
					valid = decode(bits, &time_date);
					/* If we hit the sync signal and decoded the
					   time, then set the 1sec timer tim1_cnt to
					   seconds since 1970 decoded from dcf77. */
					if (valid)
						tim1_cnt = as_time_t(&time_date);
				}
				if (valid) {
					print_date_time(&time_date, true);
					memcpy(&timer_time_date, &time_date,
					       sizeof(struct time_date_t));
				} else
					dcf77_state = WAIT_FOR_SYNC;
			} else {
				if (bits[59] == BIT_NOISY) {
					bits[59] = 0;
					print_ms_bit(BIT_NOISY);
				} else
					print_ms_bit(bits[bit_cnt - 1]);
				print_dcf77_state();
			}
		}
	}

	return 0;
}
