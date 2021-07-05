#ifndef RANDOM_H_INCLUDED
#define _GNU_SOURCE
#include <float.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <ntsecapi.h>
int
trng_write(void *ptr, size_t n)
{
	unsigned char *p;
	for (p = ptr; n > ULONG_MAX; n -= ULONG_MAX, p += ULONG_MAX)
		if (!RtlGenRandom(p, n))
			return 0;
	RtlGenRandom(p, n);
	return 1;
}
#elif defined(__OpenBSD__) || defined(__CloudABI__) || defined(__wasi__)
#include <stdlib.h>
int
trng_write(void *ptr, size_t n)
{
	arc4random_buf(ptr, n);
	return 1;
}
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
int
trng_write(void *ptr, size_t n)
{
	static int urandomFd = -1;
	unsigned char *p;
	ssize_t r;

	for (p = ptr, r = 0; n > 0; n -= (size_t)r, p += r) {
		#ifdef SYS_getrandom
		if ((r = syscall(SYS_getrandom, p, n, 0)) > 0)
			continue;
		#endif
		if (urandomFd < 0)
			if ((urandomFd = open("/dev/urandom", O_RDONLY)) < 0)
				return 0;
		if ((r = read(urandomFd, p, n)) < 0)
			return 0;
	}
	return 1;
}
#else
#error "platform not supported"
#endif

size_t
dist_uniform(size_t range)
 /* [0,range) */
{
	size_t x, r;
	do {
		trng_write(&x, sizeof x);
		r = x % range;
	} while (x - r > -range);
	return r;
}

float
dist_uniformf(void)
{
	uint_least32_t x;
	trng_write(&x, sizeof x);
	return (x >> (32 - FLT_MANT_DIG)) *
	       (1.0f / (UINT32_C(1) << FLT_MANT_DIG));
}

#define RANDOM_H_INCLUDED
#endif

/*
--------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
--------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Olaf Berstein
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
--------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
For more information, please refer to <http://unlicense.org/>
--------------------------------------------------------------------------------
*/

