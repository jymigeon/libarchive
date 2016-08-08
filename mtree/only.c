/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>

#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: only.c,v 1.2 2013/02/05 00:59:03 christos Exp $");
#endif

#include <sys/param.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "err.h"
#include "mtree.h"
#include "util.h"

struct hentry {
	char *str;
	uint32_t hash;
	struct hentry *next;
};

#define TABLE_SIZE 1024		/* hash table size */
static struct hentry **table = NULL;
static bool loaded = false;

static uint32_t	hash_str(const char *);
static bool	hash_find(const char *, uint32_t *);
static void	hash_insert(char *, uint32_t);
static void	fill(char *);

static uint32_t
hash_str(const char *str)
{
	const uint8_t *s = (const uint8_t *)str;
	uint8_t c;
	uint32_t hash = 0;
	while ((c = *s++) != '\0') 
		hash = hash * 33 + c;           /* "perl": k=33, r=r+r/32 */
	return hash + (hash >> 5);
}

static bool
hash_find(const char *str, uint32_t *h)
{
	struct hentry *e;
	*h = hash_str(str) % TABLE_SIZE;

	for (e = table[*h]; e; e = e->next)
		if (e->hash == *h && strcmp(e->str, str) == 0)
			return true;
	return false;
}

static void
hash_insert(char *str, uint32_t h)
{
	struct hentry *e;

	if ((e = malloc(sizeof(*e))) == NULL)
		lafe_errc(1, errno, "memory allocation error");

	e->str = str;
	e->hash = h;
	e->next = table[h];
	table[h] = e;
}

/* Fill an entry into the hash table by recursively adding its path components to table */
static void
fill(char *str)
{
	uint32_t h;
	char *ptr = strrchr(str, '/');

	if (ptr == NULL)
		return;

	*ptr = '\0';
	if (!hash_find(str, &h)) {
		char *x = strdup(str);
		if (x == NULL)
			lafe_errc(1, errno, "memory allocation error");
		hash_insert(x, h);
		fill(str);
	}
	*ptr = '/';
}

/* read file that contains include-only names, and perform table init */
void
mtree_read_load_only_file(const char *fname)
{
	FILE *fp;
	char *line;
	size_t len, lineno;

	if ((fp = fopen(fname, "r")) == NULL)
		lafe_errc(1, errno, "Cannot open `%s'", fname);

	if ((table = calloc(TABLE_SIZE, sizeof(*table))) == NULL)
		lafe_errc(1, errno, "memory allocation error");

	while ((line = fparseln(fp, &len, &lineno, NULL, FPARSELN_UNESCALL))) {
		uint32_t h;
		if (hash_find(line, &h))
			lafe_errc(1, 0, "Duplicate entry `%s'", line);
		hash_insert(line, h);
		fill(line);
	}

	fclose(fp);
	loaded = true;
}

/* Lookup the path inside the hash table */
bool
mtree_find_only(struct archive_entry *entry)
{
	uint32_t h;

	if (!loaded)
		return true;

	return hash_find(archive_entry_pathname(entry), &h);
}

/* Clean-up allocations */
void
mtree_only_fini(void)
{

	free(table);
}
