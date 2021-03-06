/******************************************************************************
 * This file is part of usb-instruments.                                      *
 *                                                                            *
 * Usb-instruments is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 2 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Usb-instruments is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with usb-instruments; if not, write to the Free Software             *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA *
 *                                                                            *
 * Copyright (C) 2009-2010 Cyril Hrubis <metan@ucw.cz>                        *
 *                                                                            *
 ******************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "libcounter.h"

static int ready = 1;

static void measure(float val)
{
	printf("%.3f\n", val);
}

static void range(unsigned char range)
{
	printf("range %c\n", range);
}

static void sighandler(int signum)
{
	(void) signum;
	ready = 0;
}

int main(int argc, char *argv[])
{
	struct counter *counter;
	
	if (argc != 2) {
		printf("usage: ./counter /dev/serial\n");
		return 1;
	}

	counter = counter_create(argv[1], measure, range);

	if (counter == NULL) {
		printf("failed to initalize counter: %s\n", strerror(errno));
		return 1;
	}

	signal(SIGINT, sighandler);

	counter_trigger(counter, 10);

	while (ready)
		counter_read(counter);

	counter_destroy(counter);

	return 0;
}
