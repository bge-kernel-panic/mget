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
 * Memory allocation routines
 *
 * Changelog
 * 25.06.2012  Tim Ruehsen  created
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <libmget.h>
#include "private.h"

/**
 * SECTION:libmget-xalloc
 * @short_description: Memory allocation functions
 * @title: libmget-xalloc
 * @stability: stable
 * @include: libmget.h
 *
 * The provided memory allocation functions are used by explicit libmget memory
 * allocations.
 * They differ from the standard ones in that they exit the program in an
 * out-of-memory situation with %EXIT_FAILURE. That means, you don't have to
 * check the returned value against %NULL.
 *
 * You can provide a out-of-memory function that will be called before exit(),
 * e.g. to print out a "No memory" message.
 *
 * To work around this behavior, either provide your own allocation routines,
 * namely malloc(), calloc(), realloc().
 */

static void
	(*_oom_func)(void);

static inline void G_GNUC_MGET_NORETURN _no_memory(void)
{
	if (_oom_func)
		_oom_func();

	exit(EXIT_FAILURE);
}

/**
 * mget_set_oomfunc:
 * @oom_func: Pointer to your custom out-of-memory function.
 *
 * Set a custom out-of-memory function.
 */
void mget_set_oomfunc(void (*oom_func)(void))
{
	_oom_func = oom_func;
}

/**
 * mget_malloc:
 * @size: Number of bytes to allocate.
 *
 * Like the standard malloc(), except that it doesn't return %NULL values.
 *
 * Return: A pointer to the allocated (uninitialized) memory.
 */
void *mget_malloc(size_t size)
{
	void *p = malloc(size);
	if (!p)
		_no_memory();
	return p;
}

/**
 * mget_calloc:
 * @nmemb: Number of elements (each of size @size) to allocate.
 * @size: Size of element.
 *
 * Like the standard calloc(), except that it doesn't return %NULL values.
 *
 * Return: A pointer to the allocated (initialized) memory.
 */
void *mget_calloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);
	if (!p)
		_no_memory();
	return p;
}

/**
 * mget_realloc:
 * @ptr: Pointer to old memory area.
 * @size: Number of bytes to allocate for the new memory area.
 *
 * Like the standard realloc(), except that it doesn't return %NULL values.
 *
 * Return: A pointer to the new memory area.
 */
void *mget_realloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);
	if (!p)
		_no_memory();
	return p;
}

/*void mget_free(const void **p)
{
	if (p && *p) {
		free(*p);
		*p = NULL;
	}
}*/

