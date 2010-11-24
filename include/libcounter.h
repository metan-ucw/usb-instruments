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

#ifndef __LIBCOUNTER_H__
#define __LIBCOUNTER_H__

#include <stdint.h>

#include "libserial.h"

enum counter_mode {
	COUNTER_05SEC_PERIOD, /* 0.5 sec period on  */
	COUNTER_05SEC,        /* 0.5 sec period off */
	COUNTER_5SEC,         /* 5 sec period off */
};

struct counter {
	struct libserial_port *port;
	
	int stream_pos;
	unsigned char range;
	unsigned int val;

	void (*measure_ev)(unsigned int val);
	void (*range_ev)(unsigned char range);
};

/*
 * Lock serial port. initalize counter.
 */
struct counter *counter_create(const char *port, void (*measure)(unsigned int),
                               void (*range)(unsigned char));

/*
 * Unlock serial port, free memory.
 */
void            counter_destroy(struct counter *counter);

/*
 * Read and parse data.
 */
void            counter_read(struct counter *counter);

/*
 * Set measurment mode
 */
void            counter_mode(struct counter *counter, enum counter_mode mode);

/*
 * Set trigger level.
 *
 * The value is six bits with zero, so values > +31 or < -31 are rounded.
 */
void            counter_trigger(struct counter *counter, int8_t trig);

#endif /* __LIBCOUNTER_H__ */
