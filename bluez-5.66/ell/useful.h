/*
 *
 *  Embedded Linux library
 *
 *  Copyright (C) 2021  Intel Corporation. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <unistd.h>
#include <errno.h>

#include <ell/util.h>

#define align_len(len, boundary) (((len)+(boundary)-1) & ~((boundary)-1))

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static inline size_t minsize(size_t a, size_t b)
{
	if (a <= b)
		return a;

	return b;
}

static inline size_t maxsize(size_t a, size_t b)
{
	if (a >= b)
		return a;

	return b;
}

static inline void set_bit(void *addr, unsigned int bit)
{
	unsigned char *field = addr;
	field[bit / 8] |= 1U << (bit % 8);
}

static inline int test_bit(const void *addr, unsigned int bit)
{
	const unsigned char *field = addr;
	return (field[bit / 8] & (1U << (bit % 8))) != 0;
}

static inline unsigned char bit_field(const unsigned char oct,
					unsigned int start, unsigned int n_bits)
{
	unsigned char mask = (1U << n_bits) - 1U;
	return (oct >> start) & mask;
}

#define DIV_ROUND_CLOSEST(x, divisor)			\
({							\
	typeof(divisor) _d = (divisor);			\
	typeof(x) _x = (x) + _d / 2;			\
	_x / _d;					\
})

#define __AUTODESTRUCT(func)				\
	__attribute((cleanup(func ## _cleanup)))

#define _auto_(func)					\
	__AUTODESTRUCT(func)

/* Enables declaring _auto_(close) int fd = <-1 or L_TFR(open(...))>; */
inline __attribute__((always_inline)) void close_cleanup(void *p)
{
	int fd = *(int *) p;

	if (fd >= 0)
		L_TFR(close(fd));
}

/*
 * Trick the compiler into thinking that var might be changed somehow by
 * the asm
 */
#define DO_NOT_OPTIMIZE(var) \
	__asm__ ("" : "=r" (var) : "0" (var));

static inline int secure_select(int select_left, int l, int r)
{
	int mask = -(!!select_left);

	return r ^ ((l ^ r) & mask);
}

#define struct_alloc(structname, ...) \
	(struct structname *) l_memdup(&(struct structname) { __VA_ARGS__ }, \
					sizeof(struct structname))
