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
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "stress-ng.h"

/*
 *  stress_hdd
 *	stress I/O via writes
 */
int stress_hdd(
	uint64_t *const counter,
	const uint32_t instance,
	const uint64_t max_ops,
	const char *name)
{
	uint8_t *buf;
	uint64_t i;
	const pid_t pid = getpid();
	int rc = EXIT_FAILURE;

	(void)instance;

	if ((buf = malloc((size_t)opt_hdd_write_size)) == NULL) {
		pr_err(stderr, "%s: cannot allocate buffer\n", name);
		return EXIT_FAILURE;
	}

	for (i = 0; i < opt_hdd_write_size; i++)
		buf[i] = (uint8_t)mwc();

	do {
		int fd;
		char filename[64];

		snprintf(filename, sizeof(filename), "./%s-%i.XXXXXXX", name, pid);

		(void)umask(0077);
		if ((fd = mkstemp(filename)) < 0) {
			pr_failed_err(name, "mkstemp");
			goto finish;
		}
		if (!(opt_flags & OPT_FLAGS_NO_CLEAN))
			(void)unlink(filename);

		for (i = 0; i < opt_hdd_bytes; i += opt_hdd_write_size) {
			if (write(fd, buf, (size_t)opt_hdd_write_size) < 0) {
				pr_failed_err(name, "write");
				goto finish;
			}
			(*counter)++;
			if (!opt_do_run || (max_ops && *counter >= max_ops))
				break;
		}
		(void)close(fd);
		if (!(opt_flags & OPT_FLAGS_NO_CLEAN))
			(void)unlink(filename);
	} while (opt_do_run && (!max_ops || *counter < max_ops));

	rc = EXIT_SUCCESS;
finish:
	free(buf);
	return rc;
}