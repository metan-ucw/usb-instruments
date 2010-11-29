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

#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "libcounter.h"

#define PACKET_START 0xC9

struct counter *counter_create(const char *dev, void (*measure)(float),
                               void (*range)(unsigned char))
{
	struct counter *counter = malloc(sizeof (struct counter));

	if (counter == NULL)
		return NULL;

	counter->port = libserial_open(dev, B19200);

	if (counter->port == NULL) {
		free(counter);
		return NULL;
	}
	
	/* event callbacks */
	counter->measure_ev = measure;
	counter->range_ev = range;
	
	/* initalization */
	counter->stream_pos = -2;
	counter->val   = 0;
	counter->range = 0;

	return counter;
}

void counter_destroy(struct counter *counter)
{
	if (counter == NULL)
		return;

	libserial_close(counter->port);
	free(counter);
}

static void counter_parse(struct counter *counter, unsigned char byte)
{
	switch (counter->stream_pos) {
		/* synchronize */
		case -2:
			if (byte == PACKET_START)
				counter->stream_pos = -1;
			else
				printf("LOST %x\n", byte);
		break;
		/* range */
		case -1:
			//TODO: check for correct range
			if (counter->range != byte) {
				counter->range = byte;
				counter->range_ev(counter->range);
			}
			counter->stream_pos = 0;
		break;
		/* data */
		default:
			/* last sample */
			if (counter->stream_pos == 5) {
				
				counter->val += (byte & 0x0f)<<(4*(counter->stream_pos++));
				
				switch (counter->range) {
					case 'A':
						counter->measure_ev(2.00 * 128 * counter->val);
					break;
					case 'B':
						counter->measure_ev(2.00 * counter->val);
					break;
					case 'C':
						counter->measure_ev(2000000.00 / counter->val);
					break;
					case 'a':
						counter->measure_ev(128.00 * counter->val / 5);
					break;
					case 'b':
						counter->measure_ev(1.00 * counter->val / 5);
					break;
				}
					
				counter->val = 0;
				counter->stream_pos = -2;
				return;
			}
		/* add data */
		counter->val += (byte & 0x0f)<<(4*(counter->stream_pos++));
	}
}

void counter_read(struct counter *counter)
{
	char buf[64];
	int len;
	int i;

	len = read(counter->port->fd, &buf, sizeof (buf));

	//TODO: error
	if (len < 0)
		return;

	for (i = 0; i < len; i++)
		counter_parse(counter, buf[i]);
}

static const char modes[] = {
	0x30, /* 0.5 sec period on  */
	0x31, /* 0.5 sec period off */
	0x32  /* 5 sec period off   */
};

void counter_mode(struct counter *counter, enum counter_mode mode)
{
	if (mode > COUNTER_5SEC) {
		printf("Invalid mode\n");
		return;
	}

	if (write(counter->port->fd, &modes[mode], 1) != 1)
		printf("Error setting mode %s\n.", strerror(errno));
}

/*
 * 0x81 - 0xbf
 *
 * 0x10 trigger_level
 */
void counter_trigger(struct counter *counter, int8_t trig)
{
	/* round up trigger value */
	if (trig > 31)
		trig = 31;

	if (trig < -31)
		trig = -31;
	
	/* move the trigger value to 1 - 63 range (six bits) */
	trig += 32;

	/* set trigger command bit */
	trig |= 0x80;

	if (write(counter->port->fd, &trig, 1) != 1)
		printf("Error setting trigger %s\n.", strerror(errno));
}
