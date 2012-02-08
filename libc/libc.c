/******************************************************************************
 *	Broken libc implementation ;)
 *		Just the functions I need with some custom stuff included.
 *****************************************************************************/

#include "libc.h"

static int _cursor_loc = 0;
static char _color = 0;

static int cursor_move(int cnt);
static int cursor_move_line(int cnt);
static int puts_(const char *text);

#define CHAR_TO_MEMVAL(chr)	\
		((chr) | (_color << 8))


void clear_screen()
{
	int i;
	int max_offset = MAX_CRS_X * MAX_CRS_Y;

	for (i = 0; i < max_offset; i++)
	{
		short mem_val = CHAR_TO_MEMVAL(' ');
		((short *)VIDEO_MEMORY)[i] = mem_val;
	}
}

/*
 * Sets foreground and background colors according to table defined
 * in libc.h
 */
void set_color(unsigned char backgrnd, unsigned char forgrnd)
{
	_color = (forgrnd | (backgrnd << 4));
}

/*
 * Moves cursor position to provided `x` and `y`.
 */
void goto_xy(unsigned x, unsigned y)
{
	_cursor_loc = MAX_CRS_X * y + x;
}

/* TODO: move those out to HAL or something */
/* Reads the data from IN port */
unsigned char inportb(unsigned short _port)
{
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}
/* Writes the date to OUT port */
void outportb (unsigned short _port, unsigned char _data)
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

/*
 * Sets memory location at `dest` to `val` for `count` bytes.
 * Returns `dest`
 */
void *memset(void *dest, int val, size_t count)
{
	while (count-- > 0)
		((unsigned char *) dest)[count] = val;
	return dest;
}

/*
 * Copyes `num` bytes of values from `src` to `dest`.
 * Does not do any error checking - pure binary copy!
 * Returns `dest`
 */
void *memcpy(void *dest, const void *src, size_t num)
{
	size_t i;

	for (i = 0; i < num; i++, dest++, src++)
		*(char *)dest = *(const char *)src;
	return dest;
}

/*
 * Returns the size of provided null terminated char array
 */
size_t strlen(const char* str)
{
	int i = 0;

	while (str[++i]);
	return i;
}

/*
 * Prints a given character on the screen.
 */
int putchar(int c)
{
	bool mov_forw = true;
	bool put_char = true;

	switch (c) {
	case (0x8):		/* backspace */
		cursor_move(-1);	
		c = ' ';
		mov_forw = false;
		break;
	case ('\r'):	/* carriage return */
	case ('\n'):	/* new line */
		cursor_move_line(1);
		mov_forw = false;
		put_char = false;
	}

	if (put_char)
		((short *)VIDEO_MEMORY)[_cursor_loc] = CHAR_TO_MEMVAL(c);
	if (mov_forw)
		cursor_move(1);

	return c;
}

/*
 * Prints a null terminated char array on the screen
 * with added new line character at the end.
 */
int puts(const char *text)
{
	puts_(text);
	putchar('\n');
	return 0;
}

/*
 * Prints a null terminated char array on the screen without '\n'
 */
static int puts_(const char *text)
{
	while (*text)
		if (putchar(*text++) == EOF)
			return EOF;
	return 0;
}

/*
 * All known `printf` ;)
 */
int printf(const char *format, ...)
{
	int i;
	va_list list;
	va_start(list, format);
	
	for (i = 0; format[i]; i++)
	{
		if (format[i] != '%')
		{
			putchar(format[i]);
			continue;
		}

		switch (format[i+1]) {
		/* double-% */
		case ('%'):
			putchar('%');
			break;
		/* integral */
		case ('i'):
		case ('d'): {
			int val = va_arg(list, int);
			char str[64];
			itoa(val, str, 10);
			puts_(str);
			break;
		}
		/* character */
		case ('c'): {
			char val = va_arg(list, char);
			putchar(val);
			break;
		}
		/* string */
		case ('s'): {
			char *val = va_arg(list, char *);
			puts_(val);
			break;
		}
		/* hex */
		case ('x'):
		case ('X'): {
			/* TODO */
			return -2;		
		}
		default:
			va_end(list);
			return -1;
		}
		// double increment since "%d" are double-chars
		i++;
	}

	va_end(list);
	return i;
}

/*
 * Moves the cursor by `cnt`.
 * Returns 0 if moved by all `cnt`
 */
static int cursor_move(int cnt)
{
	int new_loc = _cursor_loc + cnt;
	if (new_loc < 0)
	{
		_cursor_loc = 0;
		return ABS(new_loc);
	}

	int max_loc = MAX_CRS_X * MAX_CRS_Y;
	if (new_loc > max_loc)
	{
		/* TODO: scrolling */
		_cursor_loc = max_loc;
		return max_loc - new_loc;
	}

	_cursor_loc = new_loc;
	return 0;
}

/*
 * Moves the cursor by `cnt` lines.
 * Returns 0 if moved by all
 */
static int cursor_move_line(int cnt)
{
	int move_by;

	if (cnt > 0)
		move_by = MAX_CRS_X - (_cursor_loc % MAX_CRS_X) + (cnt - 1) * MAX_CRS_X;
	else
		move_by = _cursor_loc % MAX_CRS_X + (cnt + 1) * MAX_CRS_X;
	return cursor_move(move_by);
}

/*
 * Compares 2 null terminated char arrays.
 * Returns 0 if they are equal, 1 if the first is great, -1 otherwise.
 */
int strcmp(const char* str1, const char* str2)
{
	while (*str1 == *str2 || *str1 != '\0' || *str2 != '\0')
	{
		str1++;
		str2++;
	}

	if (*str1 == '\0' && *str2 == '\0')
		return 0;
	if (*str1)
		return 1;
	return -1;
}

/*
 * Converts string to int.
 */
int atoi(const char *str)
{
	int digit = 0;
	int i, j;
	bool neg = false;

	if (*str == '-')
	{
		str++;
		neg = true;
	}

	for (i = 0, j = strlen(str) - 1; j >= 0; i++, j--)
		digit += (str[i] - 0x30) * pow(10, j);

	return (neg ? -digit : digit);
}

/*
 * Converts int to null terminated string.
 * Returns `str` or EOF on error
 */
char *itoa(int value, char *str, int base)
{
	static const char *tokens = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	bool neg = false;
	int i, n;

	if (base < 2 || base > 32)
		return str;

	if (value < 0)
	{
		if (base == 10)
			neg = true;
		neg = true;
		value = -value;
	}

	i = 0;
	do {
		str[i++] = tokens[value % base];
		value /= base;
	} while (value);
	if (neg)
		str[i++] = '-';
	str[i--] = '\0';

	for (n = 0; n < i; n++, i--)
		SWAP(str[n], str[i]);

	return str;
}

/*
 * Returns base raised to the power of exponent
 */
int pow(int base, int exp)
{
	int digit = base;

	if (exp == 0)
		return 1;

	while (exp-- > 1)
		digit *= base;
	return digit;
}