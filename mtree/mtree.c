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

#include "archive_platform.h"
__RCSID("$FreeBSD$");

#include <sys/param.h>

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "err.h"
#include "mtree.h"
#include "util.h"

static struct {
	enum flavor flavor;
	const char name[9];
} flavors[] = {
	{F_MTREE, "mtree"},
	{F_FREEBSD9, "freebsd9"},
	{F_NETBSD6, "netbsd6"},
};

static int	check_excludes(struct archive_entry *, struct mtree *);
static int	mtree_archive_read_disk_create(struct mtree *);
static int	mtree_archive_read_disk_destroy(struct mtree *);
static void	mtree_read_excludes_file(slist_t *, const char *);
static int	mtree_set_option(struct archive *, const char *, const char *);
static int	mtree_write_data(struct mtree *, const char *);
static void	parsetags(slist_t *, char *);
__LA_DEAD static void	usage(void);

int	mtree_convert(struct mtree *);
int	mtree_cwalk(struct mtree *);
int	mtree_verify(struct mtree *);

int main(int argc, char **argv)
{
	int ch, status;
	unsigned int i;
	long psz;
	char *p;

	struct mtree _mtree; /* Stack allocation. */
	struct mtree *mt;

	/* Initialization and default values */
	mt = &_mtree;
	memset(mt, 0, sizeof(*mt));
	mt->symlink_mode = 'P'; /* 'P'hysical */
	mt->spec1 = stdin;

	/* Set lafe_progname before calling lafe_warnc. */
	lafe_setprogname(*argv, "mtree");

	psz = pathconf(".", _PC_PATH_MAX);
	if ((mt->fullpath = malloc((size_t)psz)) == NULL)
		lafe_errc(1, 0, "Failed to allocated archive object");

	if ((mt->writer = archive_write_new()) == NULL)
		lafe_errc(1, 0, "Failed to allocate archive object");

	if ((mt->match = archive_match_new()) == NULL)
		lafe_errc(1, 0, "Failed to allocated archive object");

	if (archive_write_set_format_mtree_classic(mt->writer) != ARCHIVE_OK)
		lafe_errc(1, 0, "%s", archive_error_string(mt->writer));

	while ((ch = getopt(argc, argv,
	    "bcCdDeE:f:F:I:ijk:K:lLmMnN:O:p:PqrR:s:StuUwWxX:"))
	    != -1) {
		switch ((char)ch) {
		case 'b':
			mt->bflag = 1;
			break;
		case 'c':
			mt->cflag = 1;
			break;
		case 'C':
			mt->Cflag = 1;
			break;
		case 'd':
			mt->dflag = 1;
			break;
		case 'D':
			mt->Dflag = 1;
			break;
		case 'E':
			// XXX  TODO tags exclusion when parsing specs
			parsetags(&mt->excludetags, optarg);
			break;
		case 'e':
			mt->eflag = 1;
			// XXX TODO Don't complain about files that are in the file
			// XXX hierarchy, but not in the specification.
			break;

		case 'f':
			// XXX TODO comm(1) output
			if (mt->spec1 == stdin) {
				mt->spec1 = fopen(optarg, "r");
				if (mt->spec1 == NULL)
					lafe_errc(1, errno,
					    "could not open `%s'", optarg);
			} else if (mt->spec2 == NULL) {
				mt->spec2 = fopen(optarg, "r");
				if (mt->spec2 == NULL)
					lafe_errc(1, errno,
					    "could not open `%s'", optarg);
			} else
				usage();
			break;
		case 'F':
			// XXX TODO test completeness
			for (i = 0; i < __arraycount(flavors); i++)
				if (strcmp(optarg, flavors[i].name) == 0) {
					mt->flavor = flavors[i].flavor;
					break;
				}
			if (i == __arraycount(flavors))
				usage();
			break;
		case 'i':
			mt->iflag = 1;
			// XXX TODO set the schg and/or sappnd flags.
			break;
		case 'I':
			// XXX TODO implement include tags
			parsetags(&mt->includetags, optarg);
			break;
		case 'j':
			mt->jflag = 1;
			break;
		case 'k':
			/* clear all options */
			mtree_set_option(mt->writer, "all", "0");
			/* add "type" option */
			mtree_set_option(mt->writer, "type", "1");
			/* FALLTHROUGH */
		case 'K':
			while ((p = strsep(&optarg, " \t,")) != NULL)
				if (*p != '\0')
					mtree_set_option(mt->writer, p, "1");
			break;
		case 'l':
			mt->lflag = 1;
			// XXX TODO loosse permissions checks
			break;
		case 'L':
			mt->symlink_mode = 'L';
			break;
		case 'm':
			mt->mflag = 1;
			// XXX TODO reset schg/sappnd flags
			break;
		case 'M':
			mt->Mflag = 1;
			// XXX TODO Permit merging of spec entries with different types
			break;
		case 'n':
			mt->nflag = 1;
			break;
		case 'N':
#if HAVE_PWCACHE
			// XXX TODO test under BSD
			if (! setup_getid(optarg))
				lafe_errc(1, 0,
				    "Unable to use user and group "
				    "databases in `%s'", optarg);
#else
			lafe_errc(1, 0, "Option -N not supported on this "
			    "platform");
#endif
			break;
		case 'O':
			mtree_read_load_only_file(optarg);
			break;
		case 'p':
			mt->dir = optarg;
			break;
		case 'P':
			mt->symlink_mode = 'P';
			break;
		case 'q':
			// XXX TODO quiet mode
			mt->qflag = 1;
			break;
		case 'r':
			// XXX TODO remove files not described in the spec
			mt->rflag = 1;
			break;
		case 'R':
			while ((p = strsep(&optarg, " \t,")) != NULL)
				if (*p != '\0')
					mtree_set_option(mt->writer, p, "0");
			break;
		case 's':
			mt->sflag = 1;
			mt->seed = ~strtol(optarg, &p, 0);
			mtree_set_option(mt->writer, "seed", optarg);
			break;
		case 'S':
			// XXX TODO sort entries
			mt->Sflag = 1;
			break;
		case 't':
			// XXX TODO modify the modified time of existing files
			mt->tflag = 1;
			break;
		case 'u':
			// XXX TODO modify owner/group/perms/flags of existing files
			mt->uflag = 1;
			break;
		case 'U':
			// XXX TODO same as 'u' but not considered error when fixed
			mt->Uflag = mt->uflag = 1;
			break;
		case 'w':
			// XXX TODO Don't attempt to set various file attributes such as
                        // the ownership, mode, flags
			mt->wflag = 1;
			break;
		case 'W':
			// XXX TODO don't attempt to set various file attr when creating
			// new directories
			mt->Wflag = 1;
			break;
		case 'x':
			mt->diskreader_flags |=
			    ARCHIVE_READDISK_NO_TRAVERSE_MOUNTS;
			break;
		case 'X':
			mtree_read_excludes_file(&mt->excludes, optarg);
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	switch (mt->flavor) {
	case F_FREEBSD9:
		if (mt->cflag && mt->iflag) {
			lafe_warnc(0, "-c and -i passed, replacing -i with "
			    "-j for FreeBSD compatibility");
			mt->iflag = 0;
			mt->jflag = 1;
		}
		if (mt->dflag && !mt->bflag) {
			lafe_warnc(0,
			    "Adding -b to -d for FreeBSD compatibility");
			mt->bflag = 1;
		}
		if (mt->uflag && !mt->iflag) {
			lafe_warnc(0,
			    "Adding -i to -%c for FreeBSD compatibility",
			    mt->Uflag ? 'U' : 'u');
			mt->iflag = 1;
		}
		if (mt->uflag && !mt->tflag) {
			lafe_warnc(0,
			    "Adding -t to -%c for FreeBSD compatibility",
			    mt->Uflag ? 'U' : 'u');
			mt->tflag = 1;
		}
		if (mt->wflag)
			lafe_warnc(0, "The -w flag is a no-op");
		break;
	default:
		if (mt->wflag)
			usage();
	}

	if (mt->spec2 && (mt->cflag || mt->Cflag || mt->Dflag))
		lafe_errc(1, 0,
		    "Double -f, -c, -C and -D flags are mutually exclusive");

	if (mt->dir != NULL && mt->spec2 != NULL)
		lafe_errc(1, 0,
		    "Double -f and -p flags are mutually exclusive");

	if (mt->dir != NULL && chdir(mt->dir))
		lafe_errc(1, errno,
		    "Failed to chdir() to %s", mt->dir);

	if ((mt->cflag || mt->sflag) && getcwd(mt->fullpath, psz) == NULL)
		lafe_errc(1, errno,
		    "Failed to getcwd() for %s", mt->fullpath);

#if 0 // XXX JYM see if this is really needed with libarchive
	if ((mt->cflag && mt->Cflag)
	    || (mt->cflag && mt->Dflag)
	    || (mt->Cflag && mt->Dflag))
		lafe_errc(1, 0, "-c, -C and -D flags are mutually exclusive");
#else
	if ((mt->Cflag && mt->Dflag))
		lafe_errc(1, 0, "-C and -D flags are mutually exclusive");
#endif

	if (mt->iflag && mt->mflag)
		lafe_errc(1, 0, "-i and -m flags are mutually exclusive");

	if (mt->lflag && mt->uflag)
		lafe_errc(1, 0, "-l and -u flags are mutually exclusive");

	if (mt->Cflag) {
		if (archive_write_set_format_mtree_C(mt->writer)
		    != ARCHIVE_OK)
			lafe_errc(1, 0, "%s",
			    archive_error_string(mt->writer));
	}
	if (mt->Dflag) {
		if (archive_write_set_format_mtree_D(mt->writer)
		    != ARCHIVE_OK)
			lafe_errc(1, 0, "%s",
			    archive_error_string(mt->writer));
	}

	/* Set output options */
	if (mt->bflag)
		mtree_set_option(mt->writer, "blanklines", "0");
	if (mt->dflag)
		mtree_set_option(mt->writer, "dironly", "1");
	if (mt->jflag)
		mtree_set_option(mt->writer, "indent", "1");
	if (mt->nflag) {
		mtree_set_option(mt->writer, "comments", "0");
	} else {
		/*
		 * pass down to libarchive the metadata used to generate
		 * the first mtree comment
		 */
		const char *user;
		time_t clocktime;
		char host[MAXHOSTNAMELEN + 1];
		struct passwd *pw;

		user = getlogin();
		if (user == NULL) {
			pw = getpwuid(getuid());
			user = (pw != NULL) ? pw->pw_name : "<unknown>";
		}
		time(&clocktime);
		gethostname(host, sizeof(host));
		host[sizeof(host) - 1] = '\0';

		mtree_set_option(mt->writer, "header-user", user);
		mtree_set_option(mt->writer, "header-machine", host);
		mtree_set_option(mt->writer, "header-tree", mt->fullpath);
		mtree_set_option(mt->writer, "header-date", ctime(&clocktime));
	}

	status = archive_write_open_filename(mt->writer, NULL);
	if (status != ARCHIVE_OK)
		lafe_errc(1, 0, "%s", archive_error_string(mt->writer));

	mt->bytes_per_block = archive_write_get_bytes_per_block(mt->writer);
	/* default buf_size */
	mt->buf_size = DEFAULT_BYTES_PER_BLOCK;
	while (mt->buf_size < (size_t)mt->bytes_per_block)
		mt->buf_size *= 2;

	/* allocate buffer for cksum/hash computation */
	if ((mt->buf = malloc(mt->buf_size)) == NULL)
		lafe_errc(1, 0, "cannot allocate memory (%zd B)",
		    mt->buf_size);

	/*
	 * A specification is read by mtree unless we are creating
	 * one through the 'c' option.
	 */
	if (mt->cflag == 0) {
		mt->ar_spec1 = archive_read_new();
		if (archive_read_support_format_mtree(mt->ar_spec1) != ARCHIVE_OK)
			lafe_errc(1, 0, "%s", archive_error_string(mt->ar_spec1));

		if (archive_read_open_FILE(mt->ar_spec1, mt->spec1) != ARCHIVE_OK)
			lafe_errc(1, 0, "%s", archive_error_string(mt->ar_spec1));
	}

	if (mt->cflag != 0) {
		/* Outputs the specification of the current file hierarchy. */

		/* Initialize disk reader for path traversal. */
		mtree_archive_read_disk_create(mt);
		/* print specification of the file hierarchy */
		status = mtree_cwalk(mt);
		mtree_archive_read_disk_destroy(mt);
	} else if (mt->Cflag || mt->Dflag) {
		/* Read specification and print it into C or D format. */
		status = mtree_convert(mt);
	} else if (mt->spec2 != NULL) {
		/* Compare the two specs together */
#if 0
		status = mtree_specspec(spec1, spec2);
#endif
	} else {
		/* Verify the file hierarchy according to specification */

		mtree_archive_read_disk_create(mt);
		/* verify tree according to spec */
		mtree_verify(mt);
		mtree_archive_read_disk_destroy(mt);
	}

	if (mt->Uflag && (status == MISMATCHEXIT))
		status = 0;

	if (mt->spec1 != stdin)
		fclose(mt->spec1);
	if (mt->ar_spec1 != NULL) {
		archive_read_close(mt->ar_spec1);
		archive_read_free(mt->ar_spec1);
	}

	if (mt->spec2 != NULL)
		fclose(mt->spec2);
	if (mt->ar_spec2 != NULL) {
		archive_read_close(mt->ar_spec2);
		archive_read_free(mt->ar_spec2);
	}

	free(mt->buf);
	archive_write_close(mt->writer);
	archive_write_free(mt->writer);
	archive_match_free(mt->match);
	free(mt->fullpath);
	list_free(&mt->excludetags);
	list_free(&mt->includetags);
	list_free(&mt->excludes);
	mtree_only_fini();
	exit(status);
}

static void
usage(void)
{
	unsigned int i;

	fprintf(stderr,
	    "usage: %s [-bCcDdejLlMnPqrStUuWx] [-i|-m] [-E tags]\n"
	    "\t\t[-f spec] [-f spec]\n"
	    "\t\t[-I tags] [-K keywords] [-k keywords] [-N dbdir] [-p path]\n"
	    "\t\t[-R keywords] [-s seed]\n"
	    "\t\t[-O onlyfile] [-X exclude-file]\n"
	    "\t\t[-F flavor]\n",
	    lafe_getprogname());
	fprintf(stderr, "\nflavors:");
	for (i = 0; i < __arraycount(flavors); i++) {
		fprintf(stderr, " %s", flavors[i].name);
	}
	fprintf(stderr, "\n");
	exit(1);
}

/* read file that contains fnmatch(3) exclude patterns */
static void
mtree_read_excludes_file(slist_t *list, const char *name)
{
	FILE *fp;
	char *line;
	size_t len;

	fp = fopen(name, "r");
	if (fp == NULL)
		lafe_errc(1, errno, "could not open `%s'", name);

	while ((line = fparseln(fp, &len, NULL, NULL,
	    FPARSELN_UNESCCOMM | FPARSELN_UNESCCONT | FPARSELN_UNESCESC))
	    != NULL) {

		if (line[0] == '\0') {
			free(line);
			continue;
		}

		list_insert(list, line);
	}
	fclose(fp);
}

/*
 * Check that the entry is not part of the excluded list.
 * If the pattern does not contain a '/', check against the filename only.
 */
static int
check_excludes(struct archive_entry *entry, struct mtree *mt)
{
	int i;
	const char *c, *s, *p;
	slist_t *excludes;

	s = archive_entry_pathname(entry);
	excludes = &mt->excludes;
	for (i = 0; i < excludes->count; i++) {
		p = excludes->list[i];
		/*
		 * Might be interesting to cache the '/' lookup in pattern for
		 * subsequent calls to that function
		 */
		if (strchr(p, '/') == NULL) {
			/* check against filename only */
			if ((c = strrchr(s, '/')) != NULL)
				s = c + 1;
		}

		if (fnmatch(p, s, FNM_PATHNAME) == 0)
			return 1;
	}

	return 0;
}

void
list_insert(slist_t *list, char *elem)
{
	char **new;

#define TAG_CHUNK 20
	if ((list->count % TAG_CHUNK) == 0) {
		new = realloc(list->list, (list->count + TAG_CHUNK)
		    * sizeof(*new));
		if (new == NULL)
			lafe_errc(1, 0, "could not allocate memory for tags");
		list->list = new;
	}
	list->list[list->count] = elem;
	list->count++;
#undef TAG_CHUNK
}

/* tags parsing for command line arguments, then add them to the linked list */
static void
parsetags(slist_t *list, char *args)
{
	size_t len;
	char *p, *e;

	if (args == NULL) {
		list_insert(list, NULL);
		return;
	}
	while ((p = strsep(&args, ",")) != NULL) {
		if (*p == '\0')
			continue;
		len = strlen(p) + 3;
		if ((e = malloc(len)) == NULL) {
			lafe_errc(1, 0, "could not allocate memory for tags");
		}
		snprintf(e, len, ",%s,", p);
		list_insert(list, e);
	}
}

/* free all slist_t related structures */
void
list_free(slist_t *list)
{
	int i;

	for (i = 0; i < list->count; i++) {
		free(list->list[i]);
	}

	free(list->list);
	list->count = 0;
}

/*
 * Pass down the option to libarchive. A "0" or empty string value
 * turns off the corresponding mtree option.
 */
static int
mtree_set_option(struct archive *a, const char *option, const char *value)
{
	int r;

	if (value == NULL) {
		lafe_errc(1, 0, "Value cannot be NULL for option `%s'",
		    option);
	}

	/* for convenience */
	if (strncmp(value, "0", sizeof("0")) == 0)
		value = NULL;

	r = archive_write_set_option(a, NULL, option, value);
	if (r < ARCHIVE_OK)
		lafe_errc(1, 0, "Error setting option `%s' (%s)",
		    option, archive_error_string(a));

	return r;
}

int
mtree_verify(struct mtree *mt)
{
	int r;
	struct archive_entry *ar_entry, *disk_entry;

	/*
	 * Compare each disk entry with the one in spec1 and
	 * print the relevant mismatches
	 */
	ar_entry = archive_entry_new();
	disk_entry = archive_entry_new();
	for (;;) {
		archive_entry_clear(ar_entry);
		archive_entry_clear(disk_entry);

		/* Traverse file hierarchy and lookup the proper entry in spec.
		 * if missing, warn, if not missing, print mismatches */
		r = archive_read_next_header2(mt->ar_spec1, ar_entry);
		r = archive_read_next_header2(mt->diskreader, disk_entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r < ARCHIVE_OK) {
			lafe_errc(1, 0, "%s",
			    archive_error_string(mt->diskreader));
		}

		if (check_excludes(disk_entry, mt) == 1)
			continue;

		if (!mtree_find_only(disk_entry))
			continue;

		archive_read_disk_descend(mt->diskreader);

		/* XXX implement verify: sort archive_read_disk entries first */

	}
	archive_entry_free(ar_entry);
	archive_entry_free(disk_entry);

	return 0;
}

int
mtree_convert(struct mtree *mt)
{
	int r;
	struct archive_entry *entry;

	entry = archive_entry_new();
	for (;;) {
		archive_entry_clear(entry);
		r = archive_read_next_header2(mt->ar_spec1, entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r < ARCHIVE_OK) {
			lafe_errc(1, 0, "%s",
			    archive_error_string(mt->ar_spec1));
		}

		r = archive_write_header(mt->writer, entry);
		if (r <= ARCHIVE_FAILED) {
			lafe_errc(1, 0, "%s",
			    archive_error_string(mt->writer));
		}
		if (r < ARCHIVE_OK) {
			lafe_warnc(0, "%s", archive_error_string(mt->writer));
		}
	}
	archive_entry_free(entry);

	return 0;
}

int
mtree_cwalk(struct mtree *mt)
{
	int r;
	struct archive_entry *entry;

	/*
	 * Navigate the file hierarchy and output its specification
	 */
	entry = archive_entry_new();
	for (;;) {
		archive_entry_clear(entry);
		r = archive_read_next_header2(mt->diskreader, entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r < ARCHIVE_OK) {
			lafe_errc(1, 0, "%s",
			    archive_error_string(mt->diskreader));
		}

		if (check_excludes(entry, mt) == 1)
			continue;

		if (!mtree_find_only(entry))
			continue;

		archive_read_disk_descend(mt->diskreader);

		r = archive_write_header(mt->writer, entry);
		if (r <= ARCHIVE_FAILED) {
			lafe_errc(1, 0, "%s",
			    archive_error_string(mt->writer));
		}
		if (r < ARCHIVE_OK) {
			lafe_warnc(0, "%s", archive_error_string(mt->writer));
		}

		/* only compute cksums if entry is a regular file */
		if (archive_entry_filetype(entry) == AE_IFREG) {
			mtree_write_data(mt, archive_entry_sourcepath(entry));
		}
	}
	archive_entry_free(entry);

	return 0;
}

static int
mtree_write_data(struct mtree *mt, const char *path)
{
	int fd;
	ssize_t len, written;

	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		lafe_warnc(errno, "Cannot open %s", path);
		return -1;
	}

	do {
		len = read(fd, mt->buf, mt->buf_size);
		written = archive_write_data(mt->writer, mt->buf, len);
		if (written < 0 || written < len) {
			/*
			 * Write error or truncated write. Warn and
			 * move on to next entry.
			 */
			lafe_warnc(0, "%s", archive_error_string(mt->writer));
			close(fd);
			return -1;
		}
	} while (len > 0);

	close(fd);
	return 0;
}

static int
mtree_archive_read_disk_create(struct mtree *mt)
{

	mt->diskreader = archive_read_disk_new();
	if (mt->diskreader == NULL)
		lafe_errc(1, 0,
		    "Failed to allocate disk reader archive object");

	if (mt->symlink_mode == 'P')
		archive_read_disk_set_symlink_physical(mt->diskreader);
	else
		archive_read_disk_set_symlink_logical(mt->diskreader);

	archive_read_disk_set_behavior(mt->diskreader,
	    mt->diskreader_flags);
	archive_read_disk_set_standard_lookup(mt->diskreader);

	/* Open the disk for reading */
	if (archive_read_disk_open(mt->diskreader, ".") != ARCHIVE_OK)
		lafe_errc(1, 0, "%s",
		    archive_error_string(mt->diskreader));

	return ARCHIVE_OK;
}

static int
mtree_archive_read_disk_destroy(struct mtree *mt)
{

	archive_read_close(mt->diskreader);
	archive_read_free(mt->diskreader);

	return ARCHIVE_OK;
}
