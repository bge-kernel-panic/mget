/*
 * Copyright(c) 2012 Tim Ruehsen
 *
 * This file is part of libmget.
 *
 * Libmget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Libmget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libmget.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Memory buffer printf routines
 *
 * Changelog
 * 24.09.2012  Tim Ruehsen  created
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libmget.h>
#include "private.h"

#define FLAG_ZERO_PADDED   1
#define FLAG_LEFT_ADJUST   2
#define FLAG_ALTERNATE     4
#define FLAG_SIGNED        8
#define FLAG_DECIMAL      16
#define FLAG_OCTAL        32
#define FLAG_HEXLO        64
#define FLAG_HEXUP       128

static void _copy_string(mget_buffer_t *buf, unsigned int flags, int field_width, int precision, const char *arg)
{
	size_t length;

	if (!arg) {
		mget_buffer_strcat(buf, "(null)");
		return;
	}

	length = strlen(arg);

	// info_printf("flags=0x%02x field_width=%d precision=%d length=%zd arg='%s'\n",
	//	flags,field_width,precision,length,arg);

	if (precision >= 0 && length > (size_t)precision)
		length = precision;

	if (field_width) {
		if ((unsigned)field_width > length) {
			if (flags & FLAG_LEFT_ADJUST) {
				mget_buffer_memcat(buf, arg, length);
				mget_buffer_memset_append(buf, ' ', field_width - length);
			} else {
				mget_buffer_memset_append(buf, ' ', field_width - length);
				mget_buffer_memcat(buf, arg, length);
			}
		} else {
			mget_buffer_memcat(buf, arg, length);
		}
	} else {
		mget_buffer_memcat(buf, arg, length);
	}
}

static void _convert_dec_fast(mget_buffer_t *buf, int arg)
{
	char str[32]; // long enough to hold decimal long long
	char *dst = str + sizeof(str) - 1;
	int minus;

	if (arg < 0) {
		minus = 1;
		arg = -arg;
	} else
		minus = 0;

	while (arg >= 10) {
		*dst-- = (arg % 10) + '0';
		arg /= 10;
	}
	*dst-- = (arg % 10) + '0';

	if (minus)
		*dst-- = '-';

	mget_buffer_memcat(buf, dst + 1, sizeof(str) - (dst - str) - 1);
}

static void _convert_dec(mget_buffer_t *buf, unsigned int flags, int field_width, int precision, long long arg)
{
	unsigned long long argu = arg;
	char str[32], minus = 0; // long enough to hold decimal long long
	char *dst = str + sizeof(str) - 1;
	unsigned char c;
	size_t length;

	// info_printf("arg1 = %lld %lld\n",arg,-arg);

	if (flags & FLAG_DECIMAL) {
		if (flags & FLAG_SIGNED && arg < 0) {
			minus = 1;
			argu = -arg;
		}

		while (argu) {
			*dst-- = argu % 10 + '0';
			argu /= 10;
		}
	} else if (flags & FLAG_HEXLO) {
		while (argu) {
			// slightly faster than having a HEX[] lookup table
			*dst-- = (c = (argu & 0xf)) >= 10 ? c + 'a' - 10 : c + '0';
			argu >>= 4;
		}
	} else if (flags & FLAG_HEXUP) {
		while (argu) {
			// slightly faster than having a HEX[] lookup table
			*dst-- = (c = (argu & 0xf)) >= 10 ? c + 'A' - 10 : c + '0';
			argu >>= 4;
		}
	} else if (flags & FLAG_OCTAL) {
		while (argu) {
			*dst-- = (argu & 0x07) + '0';
			argu >>= 3;
		}
	}

	// info_printf("arg2 = %lld\n",arg);


	dst++;

	length =  sizeof(str) - (dst - str);

	if (precision < 0) {
		precision = 1;
	} else {
		flags &= ~FLAG_ZERO_PADDED;
	}

	// info_printf("flags=0x%02x field_width=%d precision=%d length=%zd dst='%.*s'\n",
	//	flags,field_width,precision,length,length,dst);

	if (field_width) {
		if ((unsigned)field_width > length + minus) {
			if (flags & FLAG_LEFT_ADJUST) {
				if (minus)
					mget_buffer_memset_append(buf, '-', 1);

				if (length < (unsigned)precision) {
					mget_buffer_memset_append(buf, '0', precision - length);
					mget_buffer_memcat(buf, dst, length);
					if (field_width > precision + minus)
						mget_buffer_memset_append(buf, ' ', field_width - precision - minus);
				} else {
						mget_buffer_memcat(buf, dst, length);
						mget_buffer_memset_append(buf, ' ', field_width - length - minus);
				}
			} else {
				if (length < (unsigned)precision) {
					if (field_width > precision + minus) {
						if (flags & FLAG_ZERO_PADDED) {
							if (minus)
								mget_buffer_memset_append(buf, '-', 1);
							mget_buffer_memset_append(buf, '0', field_width - precision - minus);
						} else {
							mget_buffer_memset_append(buf, ' ', field_width - precision - minus);
							if (minus)
								mget_buffer_memset_append(buf, '-', 1);
						}
					} else {
						if (minus)
							mget_buffer_memset_append(buf, '-', 1);
					}
					mget_buffer_memset_append(buf, '0', precision - length);
				} else {
					if (flags & FLAG_ZERO_PADDED) {
						if (minus)
							mget_buffer_memset_append(buf, '-', 1);
						mget_buffer_memset_append(buf, '0', field_width - length - minus);
					} else {
						mget_buffer_memset_append(buf, ' ', field_width - length - minus);
						if (minus)
							mget_buffer_memset_append(buf, '-', 1);
					}
				}
				mget_buffer_memcat(buf, dst, length);
			}
		} else {
			if (minus)
				mget_buffer_memset_append(buf, '-', 1);
			if (length < (unsigned)precision)
				mget_buffer_memset_append(buf, '0', precision - length);
			mget_buffer_memcat(buf, dst, length);
		}
	} else {
		if (minus)
			mget_buffer_memset_append(buf, '-', 1);

		if (length < (unsigned)precision)
			mget_buffer_memset_append(buf, '0', precision - length);

		mget_buffer_memcat(buf, dst, length);
	}
}

static void _convert_pointer(mget_buffer_t *buf, void *pointer)
{
	static const char HEX[16] = "0123456789abcdef";
	char str[32]; // long enough to hold hexadecimal pointer
	char *dst;
	int length;
	size_t arg;

	if (!pointer) {
		mget_buffer_memcat(buf, "0x0", 3);
		return;
	} else {
		mget_buffer_memcat(buf, "0x", 2);
	}

	// convert to a size_t (covers full address room) tp allow integer arithmetic
	arg = (size_t)pointer;

	length = 0;
	dst = str + sizeof(str);
	*--dst = 0;
	do {
		*--dst = HEX[arg&0xF];
		arg >>= 4;
		length++;
	} while (arg);

	mget_buffer_memcat(buf, dst, length);
}

size_t mget_buffer_vprintf_append2(mget_buffer_t *buf, const char *fmt, va_list args)
{
	const char *p = fmt, *begin;
	int field_width, precision;
	unsigned int flags;
	long long arg;
	unsigned long long argu;

	for (;*p;) {

		// collect plain char sequence

		for (begin = p; *p && *p != '%'; p++);
		if (p != begin)
			mget_buffer_memcat(buf, begin, p - begin);

		if (!*p) break;

		// shortcut to %s and %p, handle %%

		if (*++p == 's') {
			mget_buffer_strcat(buf, va_arg(args, const char *));
			p++;
			continue;
		}
		else if (*p == 'd') {
			_convert_dec_fast(buf, va_arg(args, int));
			p++;
			continue;
		}
		else if (*p == 'p') {
			_convert_pointer(buf, va_arg(args, void *));
			p++;
			continue;
		}
		else if (*p == '%') {
			mget_buffer_memset_append(buf, '%', 1);
			p++;
			continue;
		}

		// read the flag chars (optional, simplified)

		for (flags = 0; *p; p++) {
			if (*p == '0')
				flags |= FLAG_ZERO_PADDED;
			else if (*p == '-')
				flags |= FLAG_LEFT_ADJUST;
			else if (*p == '#')
				flags |= FLAG_ALTERNATE;
			else
				break;
		}

		// read field width (optional)

		if (*p == '*') {
			field_width = va_arg(args, int);
			if (field_width < 0) {
				flags |= FLAG_LEFT_ADJUST;
				field_width = -field_width;
			}
			p++;
		} else {
			for (field_width = 0; isdigit(*p); p++)
				field_width = field_width * 10 + (*p - '0');
		}

		// read precision (optional)

		if (*p == '.') {
			if (*++p == '*') {
				precision = va_arg(args, int);
				if (precision < 0 )
					precision = 0;
				p++;
			} else if (isdigit(*p)) {
				precision = 0;
				do {
					precision = precision * 10 + (*p - '0');
				} while (isdigit(*++p));
			} else {
				precision = -1;
			}
		} else
			precision = -1;

		// read length modifier (optional)

		switch (*p) {
		case 'z':
			arg = va_arg(args, ssize_t);
			argu = (size_t)arg;
			p++;
			break;

		case 'l':
			if (p[1] == 'l') {
				p += 2;
				arg = va_arg(args, long long);
				argu = (unsigned long long)arg;
			} else {
				p++;
				arg = (long)va_arg(args, long long);
				argu = (unsigned long)arg;
			}
			break;

		case 'L':
			p++;
			arg = va_arg(args, long long);
			argu = (unsigned long long)arg;
			break;

		case 'h':
			if (p[1] == 'h') {
				p += 2;
				arg = (char)va_arg(args, /* char */ int);
				argu = (unsigned char)arg;
			} else {
				p++;
				arg = (short)va_arg(args, /* short */ int);
				argu = (unsigned short)arg;
			}
			break;

		case 's':
			p++;
			_copy_string(buf, flags, field_width, precision, va_arg(args, const char *));
			continue;

		case 'p': // %p shortcut
			p++;
			_convert_dec(buf, flags | FLAG_HEXLO | FLAG_ALTERNATE, field_width, precision, (long long)va_arg(args, void *));
			continue;

		default:
			arg = va_arg(args, int);
			argu = (unsigned int)arg;
		}

		// info_printf("*p = %c arg = %lld\n", *p, arg);

		if (*p == 'd' || *p == 'i')
			_convert_dec(buf, flags | FLAG_SIGNED | FLAG_DECIMAL, field_width, precision, arg);
		else if (*p == 'u')
			_convert_dec(buf, flags | FLAG_DECIMAL, field_width, precision, argu);
		else if (*p == 'x')
			_convert_dec(buf, flags | FLAG_HEXLO, field_width, precision, argu);
		else if (*p == 'X')
			_convert_dec(buf, flags | FLAG_HEXUP, field_width, precision, argu);
		else if (*p == 'o')
			_convert_dec(buf, flags | FLAG_OCTAL, field_width, precision, argu);
		else {
			// err_printf("Internal error: Unknown conversion specifier '%c'\n", *p);
			mget_buffer_memset_append(buf, '%', 1);
			p = begin + 1;
			continue;
		}

		p++;
	}

	return buf->length;
}

size_t mget_buffer_vprintf2(mget_buffer_t *buf, const char *fmt, va_list args)
{
	buf->length = 0;

	return mget_buffer_vprintf_append2(buf, fmt, args);
}

size_t mget_buffer_printf_append2(mget_buffer_t *buf, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	mget_buffer_vprintf_append2(buf, fmt, args);
	va_end(args);

	return buf->length;
}

size_t mget_buffer_printf2(mget_buffer_t *buf, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	return mget_buffer_vprintf2(buf, fmt, args);
	va_end(args);
}

size_t mget_buffer_vprintf_append(mget_buffer_t *buf, const char *fmt, va_list args)
{
	ssize_t length;
	va_list args2;

	// vsnprintf destroys args, so we need a copy for the realloc case
	va_copy(args2, args);

	// first try
	length = vsnprintf(buf->data + buf->length, buf->size - buf->length, fmt, args);

	if (length == -1 || (size_t)length >= buf->size - buf->length) {
		mget_buffer_realloc(buf, buf->size * 2 + length);
		buf->length += vsnprintf(buf->data + buf->length, buf->size - buf->length, fmt, args2);
	} else
		buf->length += length;

	return buf->length;
}

size_t mget_buffer_printf_append(mget_buffer_t *buf, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	mget_buffer_vprintf_append(buf, fmt, args);
	va_end(args);

	return buf->length;
}

size_t mget_buffer_vprintf(mget_buffer_t *buf, const char *fmt, va_list args)
{
	buf->length = 0;

	return mget_buffer_vprintf_append(buf, fmt, args);
}

size_t mget_buffer_printf(mget_buffer_t *buf, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	mget_buffer_vprintf(buf, fmt, args);
	va_end(args);

	return buf->length;
}
