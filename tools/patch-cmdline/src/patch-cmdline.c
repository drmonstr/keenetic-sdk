/*
 * patch-cmdline.c - patch the kernel command line on rb532
 *
 * Copyright (C) 2006 Felix Fietkau <nbd@openwrt.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

#define SEARCH_SPACE	(64 * 1024)
#define CMDLINE_MAX		512

int main(int argc, char **argv)
{
	int fd, found = 0, len, ret = -1;
	char *ptr, *p;
	struct stat sbuf;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <file> <cmdline>\n", argv[0]);
		goto err1;
	}
	len = strlen(argv[2]);
	if (len > (CMDLINE_MAX - 1)) {
		fprintf(stderr, "Command line string too long\n");
		goto err1;
	}
	
	if ((fd = open(argv[1], O_RDWR)) < 0) {
		fprintf(stderr, "Could not open kernel image");
		goto err2;
	}
	
	if (fstat(fd, &sbuf) < 0) {
		fprintf(stderr, "Can't stat %s: %s\n", argv[1], strerror(errno));
		goto err2;
	}

	if((ptr = (char *) mmap(0, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == (void *) (-1)) {
		fprintf(stderr, "Can't read %s: %s\n", argv[1], strerror(errno));
		goto err2;
	}

	for (p = ptr; p < (ptr + (sbuf.st_size - 1)); p++) {
		if (memcmp(p, "CMDLINE:", 8) == 0) {
			found = 1;
			p += 8;
			break;
		}
	}

	if (!found) {
		fprintf(stderr, "Command line marker not found!\n");
		goto err3;
	}

	memset(p, 0, CMDLINE_MAX - 8);
	strcpy(p, argv[2]);
	msync(p, CMDLINE_MAX, MS_SYNC|MS_INVALIDATE);
	ret = 0;

err3:
	munmap((void *) ptr, sbuf.st_size);
err2:
	if (fd > 0)
		close(fd);
err1:
	return ret;
}
