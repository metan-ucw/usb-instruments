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

#ifndef __LIBVAMETER_H__
#define __LIBVAMETER_H__

#include <stdint.h>
#include <stdbool.h>

#include "libserial.h"

#define VAMETER_DC_POS '+'
#define VAMETER_DC_NEG '-'
#define VAMETER_AC     '~'

struct VAmeter {
	/*
	 * VA meter state.
	 */
	uint8_t  command;          /* where are we now       */
	
	uint8_t  sample_low;       /* low part of sample     */
	uint8_t  sample_cnt;       /* number of read samples */
	float    sample_sum;       /* sum of read samples    */

	uint8_t cur_voltage_range; /* voltage range          */
	uint8_t cur_current_range; /* current range          */
	uint8_t hw_switch;         /* hw switch on the board */

	/*
	 * Autocallibration values.
	 */
	float   voltage_ref;
	float   voltage_zero;
	float   current_ref;
	float   current_zero;

	/*
	 * Measured values
	 */
	float voltage;
	float current;

	/*
	 * Static callibration in order to fix non precise parts.
	 */
	float voltage_callib[8];
	float current_callib[4];

	/*
	 * Number of samples less than zero. Used to determine AC/DC.
	 */
	uint8_t neg_volt_samp;
	uint8_t neg_curr_samp;

	/*
	 * Callbacks, that could be set by application.
	 */
	void (*voltage_range)(uint8_t range, const char *str_range);
	void (*current_range)(uint8_t hw_switch, uint8_t range, const char *str_range);
	void (*voltage_sample)(char acdc, float sample);
	void (*current_sample)(char acdc, float sample);

	/*
	 * File descriptor and path to device file. 
	 */
	struct libserial_port *port;
};

/*
 * Allocate struct AVmeter, open device.
 */
struct VAmeter *vameter_init(const char *device_path);

/*
 * Free memory and close device.
 */
void            vameter_exit(struct VAmeter *meter);

/*
 * Returns device name.
 */
const char     *vameter_get_dev(struct VAmeter *meter);


/*
 * Returns file descriptor.
 */
int             vameter_get_fd(struct VAmeter *meter);

/*
 * Process bytes from buffer.
 */
void            vameter_process(struct VAmeter *meter, uint8_t *buf, uint32_t buf_len);

/*
 * Read and process data.
 */
int             vameter_read(struct VAmeter *meter);

/*
 * Set/reset blocking mode on filedescriptor.
 */
void            vameter_read_blocked(struct VAmeter *meter, bool blocked);


/*
 * Load static callibration from file. Returns -1 on failure with errno set
 * and -2 when fscanf wasn't able to parse the file.
 *
 * File consist of line with numbers, every number has four ditits and it's
 * treated as it has has decimal point after the first one. Order of o the
 * callibration constants is following.
 *
 * V-A V-B V-C V-D V-E V-F V-G V-H mA-A mA-B mA-C mA-D
 *
 */
int             vameter_load_callib(struct VAmeter *meter, const char *file);

/*
 * Just reset callibration.
 */
void            vameter_unload_callib(struct VAmeter *meter);

#endif /* __LIBVAMETER_H__ */
