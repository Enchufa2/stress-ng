/*
 * Copyright (C) 2013-2014 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * This code is a complete clean re-write of the stress tool by
 * Colin Ian King <colin.king@canonical.com> and attempts to be
 * backwardly compatible with the stress tool by Amos Waterland
 * <apw@rossby.metr.ou.edu> but has more stress tests and more
 * functionality.
 *
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "stress-ng.h"

/*
 *  stress_fault()
 *	stress min and max page faulting
 */
int stress_fault(
	uint64_t *const counter,
	const uint32_t instance,
	const uint64_t max_ops,
	const char *name)
{
	struct rusage usage;
	char filename[128];
	int i = 0;

	(void)stress_temp_filename(filename, sizeof(filename),
		name, getpid(), instance, mwc());
	(void)umask(0077);

	do {
		char *ptr;
		int fd;

		fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd < 0) {
			pr_err(stderr, "%s: open failed: errno=%d (%s)\n",
				name, errno, strerror(errno));
			break;
		}
		if (fallocate(fd, 0, 0, 1) < 0) {
			close(fd);
			pr_err(stderr, "%s: fallocate failed: errno=%d (%s)\n",
				name, errno, strerror(errno));
			break;
		}

		/*
		 * Removing file here causes major fault when we touch
		 * ptr later
		 */
		if (i & 1)
			(void)unlink(filename);

		ptr = mmap(NULL, 1, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);
		(void)close(fd);

		if (ptr == MAP_FAILED) {
			pr_err(stderr, "%s: mmap failed: errno=%d (%s)\n",
				name, errno, strerror(errno));
			break;

		}
		*ptr = 0;	/* Cause the page fault */

		if (munmap(ptr, 1) < 0) {
			pr_err(stderr, "%s: munmap failed: errno=%d (%s)\n",
				name, errno, strerror(errno));
			break;
		}

		/* Remove file on-non major fault case */
		if (!i & 1)
			(void)unlink(filename);

		i++;
		(*counter)++;
	} while (opt_do_run && (!max_ops || *counter < max_ops));
	/* Clean up, most times this is redundant */
	(void)unlink(filename);

	if (!getrusage(RUSAGE_SELF, &usage)) {
		pr_dbg(stderr, "page faults: minor: %lu, major: %lu\n",
			usage.ru_minflt, usage.ru_majflt);
	}

	return EXIT_SUCCESS;
}