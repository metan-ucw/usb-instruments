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

#include "libgenerator.h"

static int ready = 1;

static void sighandler(int signum)
{
	(void) signum;
	ready = 0;
}

static void dump_generator_state(struct generator *generator)
{
	printf("------ Generator state ------\n");
	printf("Output Wave:      %s\n",
	       generator_wave_names[generator->out_wave]);
	printf("Output Frequency: %6.2f Hz/Baud\n",
	       generator_convert_freq(generator));
	printf("Output Amplitude: %f V\n",
	       generator_convert_amplitude(generator));
	printf("Output Offset:    %f V\n",
	       generator_convert_offset(generator));
	printf("Output Filter:    %s\n",
	       generator_filter_names[generator->out_filter]);
	printf("Memory:           %u\n", generator->mem);
	printf("-----------------------------\n");
}

int main(int argc, char *argv[])
{
	struct generator *generator;

	if (argc != 2) {
		printf("usage: ./counter /dev/serial\n");
		return 1;
	}

	generator = generator_create(argv[1], dump_generator_state);

	if (generator == NULL) {
		printf("failed to initalize generator: %s\n", strerror(errno));
		return 1;
	}

	signal(SIGINT, sighandler);

	generator_load_state(generator);
	
	while (ready)
		generator_read(generator);
	
	generator_destroy(generator);

	return 0;
}
