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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libserial.h"

#define FHS_LOCK_PREFIX "/var/lock/LCK.."

#define DEBUG(...) printf(__VA_ARGS__);

/*
 * Initalize serial port sppeed.
 */
static int ser_init(int fd, tcflag_t baudrate)
{
	struct termios t;
	
	tcgetattr(fd, &t);

	t.c_cflag = baudrate;
	t.c_iflag = baudrate;
	
	cfmakeraw(&t);
	
	t.c_cc[VMIN]  = 1;
	t.c_cc[VTIME] = 0;

	tcsetattr(fd, TCSANOW, &t);

	tcflush(fd, TCIFLUSH);

	return 0;
}

/*
 * Returns last part of the path
 *
 * eg from "/dev/ttyS0" only "ttyS0"
 */
static const char *ser_name(const char *dev)
{
	int i;

	for (i = strlen(dev); i >= 0; i--) {
		if (dev[i] == '/')
			return dev + i + 1;
	}

	return dev;
}

/*
 * Try to create lockfile, fail if exists.
 */
static int try_lock(const char *dev)
{
	int lock_fd;
	char lock[255];
	FILE *lock_file;

	return 0;

	snprintf(lock, 255, "%s%s", FHS_LOCK_PREFIX, ser_name(dev));

	DEBUG("Trying lock on %s\n", lock);

	/* try to open lock exclusively */
	lock_fd = open(lock, O_CREAT | O_WRONLY | O_EXCL,
	               S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR);

	/* lock failed */
	if (lock_fd < 0)
		return 1;

	/* write pid */
	lock_file = fdopen(lock_fd, "w");

	if (lock_file == NULL)
		return 1;

	fprintf(lock_file, "%i\n", getpid());

	if (fclose(lock_file))
		return 1;

	return 0;
}

/*
 * Delete lockfile.
 */
static int release_lock(const char *dev)
{
	char lock[255];
	
	snprintf(lock, 255, "%s%s", FHS_LOCK_PREFIX, ser_name(dev));
	
	DEBUG("Releasing lock on %s\n", lock);

	return unlink(lock);
}


struct libserial_port *libserial_open(const char *dev, tcflag_t baudrate)
{
	struct libserial_port *port;

	port = malloc(sizeof (struct libserial_port) + strlen(dev) + 1);
	
	/* malloc failed */
	if (port == NULL)
		return NULL;

	/* look for file type */
	if (stat(dev, &port->st))
		goto err;

	/* try to lock serial port (char device) */
	if (S_ISCHR(port->st.st_mode) && try_lock(dev))
			goto err;

	/* open serial port */
	port->fd = open(dev, O_RDWR);

	if (port->fd < 0)
		goto err1;

	if (ser_init(port->fd, baudrate))
		goto err2;

	strcpy(port->dev, dev);

	return port;

err2:
	close(port->fd);
err1:
	if (S_ISCHR(port->st.st_mode))
		release_lock(dev);
err:
	free(port);
	return NULL;
}

void libserial_close(struct libserial_port *port)
{
	if (port == NULL)
		return;
	
	close(port->fd);
	
	if (S_ISCHR(port->st.st_mode))
		release_lock(port->dev);
	
	free(port);
}
