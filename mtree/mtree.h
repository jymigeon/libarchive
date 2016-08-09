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

#ifndef _MTREE_H_INCLUDED
#define _MTREE_H_INCLUDED

#include "mtree_platform.h"

#define DEFAULT_BYTES_PER_BLOCK (64*1024)

/* array of strings */
typedef struct {
	char **list;
	int count;
} slist_t;
void	list_insert(slist_t *, char *);
void	list_free(slist_t *);

enum flavor {
	F_MTREE,
	F_FREEBSD9,
	F_NETBSD6
};

/*
 * The internal state for the "mtree" program.
 *
 * Keeping all of the state in a structure like this simplifies memory
 * leak testing (at exit, anything left on the heap is suspect).  A
 * pointer to this structure is passed to most mtree internal
 * functions.
 */

struct mtree {
	/* option flags */
	int cflag; /* output specification to stdout */
	int Cflag; /* turn output into a format easier to parse with tools */
	int bflag; /* suppress \n when entering/leaving directories */
	int dflag; /* ignore everything except directories */
	int Dflag; /* like Cflag but path name is printed last */
	int eflag; /* do not complain for files present on system
		    * but not in spec */
	int iflag; /* set schg and/or sappnd flags */
	int jflag; /* 4 space indentation for each fts_level increase */
	int lflag; /* enable ``loose'' permission checks */
	int mflag; /* reset schg and/or sappnd flags */
	int Mflag; /* merge duplicate entries */
	int nflag; /* disable path name comments */
	int qflag; /* quiet mode */
	int rflag; /* remove files that are not in specification */
	int sflag; /* checksum seed for files with `cksum' keyword specified */
	int Sflag; /* sort entries */
	int tflag; /* modify files' time, device type and symlink according
		    * to specification */
	int uflag; /* modify owner, group, perms, flags, device type and
		    * symlink target according to specification */
	int Uflag; /* like uflag but will not raise an error if the metadata
		    * has been successfully modified */
	int wflag; /* no-op */
	int Wflag; /* Don't "whack" permissions */

	slist_t excludetags; /* excluded tags for mtree entries */
	slist_t includetags; /* included tags for mtree entries */
	enum flavor flavor;/* flavor used for the mtree program */

	const char *dir; /* root path to use instead of CWD */
	uint32_t seed; /* CRC seed */

	slist_t excludes; /* excluded fnmatch() patterns for files */
	FILE *spec1;
	struct archive *ar_spec1; /* archive structure of first mtree spec */
	FILE *spec2;
	struct archive *ar_spec2; /* archive structure of optional second mtree
				   * spec (used for comparing) */

	struct archive *writer; /* archive structure for mtree spec output */
	struct archive *match;  /* archive structure for the exclude list */

	char symlink_mode; /* 'L'ogical or 'P'hysical symlink mode */
	struct archive *diskreader; /* archive structure that controls
				     * read disk operations */
	int diskreader_flags; /* flags used for read disk operations */

	int bytes_per_block; /* bytes per block for archive */
	char *buf; /* buffer for cksum/hash computation */
	size_t buf_size; /* size of buf */

	char *fullpath;
};

#define MISMATCHEXIT 2

extern enum flavor flavor;

#if HAVE_PWCACHE
int	setup_getid(const char *);
#endif

void	mtree_read_load_only_file(const char *);
bool	mtree_find_only(struct archive_entry *);
void	mtree_only_fini(void);
#endif
