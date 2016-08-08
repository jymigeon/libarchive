/*-
 * Copyright (c) 2013 Jean-Yves Migeon
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MTREE_UTIL_H_INCLUDED
#define _MTREE_UTIL_H_INCLUDED

#include "mtree_platform.h"

#ifndef HAVE_FPARSELN
#define FPARSELN_UNESCALL	0xf
#define FPARSELN_UNESCCOMM	0x1
#define FPARSELN_UNESCCONT	0x2
#define FPARSELN_UNESCESC	0x4
#define FPARSELN_UNESCREST	0x8
char *fparseln(FILE *, size_t *, size_t *, const char delim[3], int);
#else
#include <libutil.h>
#endif

#ifndef HAVE_FGETLN
char *fgetln(FILE *, size_t *);
#endif

#endif /* _MTREE_UTIL_H_INCLUDED */
