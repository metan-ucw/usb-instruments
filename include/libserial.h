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

#ifndef __LIBSERIAL_H__
#define __LIBSERIAL_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

struct libserial_port {
	int fd;
	struct stat st;
	char dev[];
};

/*
 * Opens serial port. Retruns file descriptor or negative value (-1) in case of
 * failure and errno set.
 * 
 * Dev should be device path in /dev/ for example "/dev/ttyS0" or "/dev/ttyUSB0"
 *
 * Baud rate is one of following:
 *
 * B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800,
 * B9600, B19200, B3840
 *
 * You can also pass file instead of serial port as a dev. In this case
 * baudrate is ignored and also no serial port locking is done.
 */
struct libserial_port *libserial_open(const char *dev, tcflag_t baudrate);

/*
 * Close serial port, removes lock.
 */
void libserial_close(struct libserial_port *port);

#endif /* __LIBSERIAL_H__ */
