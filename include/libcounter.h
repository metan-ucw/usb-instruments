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

#include "libserial.h"

struct counter {
	struct libserial_port *port;
	
	int stream_pos;
	unsigned char range;
	unsigned int val;

	void (*measure_ev)(unsigned int val);
	void (*range_ev)(unsigned char range);
};

struct counter *counter_create(const char *port, void (*measure)(unsigned int),
                               void (*range)(unsigned char));

void            counter_destroy(struct counter *counter);

void            counter_read(struct counter *counter);

void            counter_cmd(struct counter *counter, int cmd);

#endif /* __LIBCOUNTER_H__ */
