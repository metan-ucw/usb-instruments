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
#include <fcntl.h>
#include <math.h>

#include "libvameter.h"

#define DPRINT(...) { fprintf(stderr, "%s: %i: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); }

/***************************************
 *                                     *
 *  Communication protocol constants.  *
 *                                     *
 ***************************************/  

/*
 * Voltage part.
 */
#define V_RANGE       0x8A /* 'A' - 'H' */
#define V_ZERO_REF    0x9A /* followed by 32 two byte samples */
#define V_REF         0x8D /* followed by 32 two byte samples */
#define V_SAMPLE      0x9D /* followed by 32 two byte samples */

#define V_RANGE_MIN 'A'
#define V_RANGE_MAX 'H'

/*
 * Current part.
 */

/* This frames depends on switch on board. */
#define A_RANGE_2A    0x8E /* 'A' - 'D' */
#define A_RANGE_600mA 0x8F /* 'A' - 'D' */

#define A_ZERO_REF    0x9E /* followed by 32 two byte samples */
#define A_REF         0xF8 /* followed by 32 two byte samples */
#define A_SAMPLE      0xD8 /* followed by 32 two byte samples */

#define A_RANGE_MIN 'A'
#define A_RANGE_MAX 'D'

/*
 * All control command have MSB set to 1
 */
#define CONTROL_CMD 0x80

static char *current_range_A[] = 
{
	"220mA",
	"700mA",
	"2A"   ,
	"6A"   ,
};

static char *current_range_B[] =
{
	"22mA" ,
	"70mA" ,
	"200mA",
	"600mA",
};

static char *voltage_range[] =
{
	"600mV",
	"2V"   ,
	"5V"   ,
	"11V"  ,
	"17V"  ,
	"36V"  ,
	"100V" ,
	"330V" ,
};

/*
 * Voltage input amplifiers.
 */
static float voltage_magick[] =
{
	  0.530,
	  1.696, // 0.53 * 3.2
	  5.300,
	 10.100,
	 13.250, // 5.3  * 2.5
	 32.320, // 10.1 * 3.2
	101.000,
	1,       //fixme 330V max
};

/*
 * Current input amplifiers.
 */
static float current_magick[] = 
{
	0.20,
	0.64,
	2.00,
	5.00,
};

struct VAmeter *vameter_init(const char *device_path)
{
	struct VAmeter *new; 
	struct libserial_port *port;

	port = libserial_open(device_path, B19200); 

	if (port == NULL)
		return NULL;

	new = malloc(sizeof (struct VAmeter) + strlen(device_path) + 1);

	if (new == NULL) {
		libserial_close(port);		
		return NULL;
	}

	new->port = port;

	new->cur_voltage_range    = 0xff;
	new->cur_current_range    = 0xff;
	new->sample_low           = 0;
	new->command              = 0x00;
	new->neg_volt_samp        = 0;
	new->neg_curr_samp        = 0;
	new->sample_cnt           = 0;

	/* set callbacks to NULL */
	new->current_range        = NULL;
	new->voltage_range        = NULL;
	new->voltage_sample       = NULL;
	new->current_sample       = NULL;

	/* set callibrations to 1 */
	vameter_unload_callib(new);

	return new;
}

void vameter_exit(struct VAmeter *meter)
{
	if (meter == NULL)
		return;
	
	libserial_close(meter->port);
	free(meter);
}

/*
 * Wrappers.
 */
const char *vameter_get_dev(struct VAmeter *meter)
{
	return meter->port->dev;
}

int vameter_get_fd(struct VAmeter *meter)
{
	return meter->port->fd;
}


/*
 * Test for negative/possitive or AC.
 */
static char calc_acdc(uint8_t neg_samp, uint8_t samp_cnt)
{
	if (neg_samp <= 2)
		return VAMETER_DC_POS;
	
	if (neg_samp > samp_cnt - 2)
		return VAMETER_DC_NEG;
	
	return VAMETER_AC;
}

/*
 * Process next part of the buffer. Current possition in data packet is
 * remebered in struct vameter.
 */
void vameter_process(struct VAmeter *meter, uint8_t *buf, uint32_t buf_len)
{
	uint32_t i;
	uint8_t range;

	for (i = 0; i < buf_len; i++) {
		
		/* Parse control character from the stream. */
		if (buf[i] & CONTROL_CMD) {
			
			switch (meter->command) {
				
				case 0x00:
				case V_RANGE:
				case A_RANGE_2A:
				case A_RANGE_600mA:
				break;

				case V_ZERO_REF:
					meter->voltage_zero = meter->sample_sum / meter->sample_cnt;
				break;
				
				case V_REF:
					meter->voltage_ref = meter->sample_sum / meter->sample_cnt;
				break;

				case V_SAMPLE:
					range = meter->cur_voltage_range;

					meter->voltage  = sqrtf(meter->sample_sum/meter->sample_cnt) * voltage_magick[range];
					meter->voltage /= fabsf(meter->voltage_ref - meter->voltage_zero);
					meter->voltage *= meter->voltage_callib[range];

					if (meter->voltage_sample != NULL)
						meter->voltage_sample(calc_acdc(meter->neg_volt_samp, meter->sample_cnt), meter->voltage);

					meter->neg_volt_samp = 0;
				break;

				case A_ZERO_REF:
					meter->current_zero = meter->sample_sum / meter->sample_cnt;
				break;
				case A_REF:
					meter->current_ref  = meter->sample_sum / meter->sample_cnt;
				break;
				case A_SAMPLE:
					range = meter->cur_current_range;

					meter->current  = sqrtf(meter->sample_sum/meter->sample_cnt) * current_magick[range];
					meter->current /= fabsf(meter->current_ref - meter->current_zero);
					meter->current *= meter->current_callib[range];

					if (meter->current_sample != NULL)
						meter->current_sample(calc_acdc(meter->neg_curr_samp, meter->sample_cnt), meter->current);
					
					meter->neg_curr_samp = 0;
				break;

				default:
					printf("ERROR: Unknown command %x.\n", buf[i]);
			}

			meter->command     = buf[i];
			meter->sample_sum  = 0;
			meter->sample_cnt  = 0;

			continue;
		}

		switch (meter->command) {
			case V_RANGE:
				/* Invalid voltage range */
				if (buf[i] < V_RANGE_MIN || buf[i] > V_RANGE_MAX) {
					printf("ERROR: Invalid voltage range %x\n", buf[i]);
					continue;
				}
				
				/* voltage range has changed */
				if (meter->cur_voltage_range != buf[i] - V_RANGE_MIN) {
					range = buf[i] - V_RANGE_MIN;
					meter->cur_voltage_range = range;
					if (meter->voltage_range != NULL)
						meter->voltage_range(range, voltage_range[range]);
				}
			break;
			
			case A_RANGE_600mA:
			case A_RANGE_2A:
				/* invalid current range */
				if (buf[i] < A_RANGE_MIN || buf[i] > A_RANGE_MAX) {
					printf("ERROR: Invalid current range %x\n", buf[i]);
					continue;
				}
				
				/* current range has changed */
				if (meter->cur_current_range != buf[i] - A_RANGE_MIN || meter->command - A_RANGE_2A != meter->hw_switch) {
					range = buf[i] - A_RANGE_MIN;
					meter->cur_current_range = range;
					meter->hw_switch         = meter->command - A_RANGE_2A; 
					if (meter->current_range != NULL)
						meter->current_range(meter->hw_switch, range, meter->hw_switch ? current_range_B[range] : current_range_A[range]);
				}
			break;

			case V_ZERO_REF:
			case V_REF:
			case A_ZERO_REF:
			case A_REF:
				if (meter->sample_low == 0) {
					meter->sample_low = buf[i];
				} else {
					meter->sample_sum += (buf[i] & 0x0F)<<6 | ((0x3F & meter->sample_low));
					meter->sample_cnt++;
					meter->sample_low = 0;
				}
			break;
			
			case V_SAMPLE:
			case A_SAMPLE:
				if (meter->sample_low == 0) {
					meter->sample_low = buf[i];
				} else {
					float sample = (buf[i] & 0x0F)<<6 | ((0x3F & meter->sample_low));
	
					if (meter->command == V_SAMPLE) {
						if (sample - meter->voltage_zero < 0)
							meter->neg_volt_samp++;
						sample -= meter->voltage_zero;
					} else {
						if (sample - meter->current_zero < 0)
							meter->neg_curr_samp++;
						sample -= meter->current_zero;
					}
					
					meter->sample_sum += sample*sample;
					
					meter->sample_cnt++;
					meter->sample_low = 0;
				}
			break;
			
			default:
			break;
		}
	}
}

void vameter_read_blocked(struct VAmeter *meter, bool blocked)
{
	long flags;

	flags = fcntl(meter->port->fd, F_GETFL);

	if (blocked)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
	
	fcntl(meter->port->fd, F_SETFL, flags);
}

int vameter_read(struct VAmeter *meter)
{
	uint8_t buf[512];
	int32_t len;
	
	len = read(meter->port->fd, &buf, 512);

	/* end of file */
	if (len == 0)
		return 0;

	if (len < 0) {
		if (errno == EAGAIN)
			return 1;

		printf("ERROR: %s read: %s\n", meter->port->dev, strerror(errno));
		return len;
	}

	vameter_process(meter, buf, len);

	return 1;
}

/*
 * Load static callibration from file. Returns zero or errno.
 *
 * File consist of line with numbers, every number has four ditits and it's
 * treated as it has has decimal point after the first one. Order of o the
 * callibration constants is following.
 *
 * V-A V-B V-C V-D V-E V-F V-G V-H mA-A mA-B mA-C mA-D
 *
 */
int vameter_load_callib(struct VAmeter *meter, const char *file)
{
	FILE *f;
	unsigned int V[8], A[4], i;

	f = fopen(file, "r");
	
	if (f == NULL)
		return -1;
	
	i  = fscanf(f, "%4u %4u %4u %4u %4u %4u %4u %4u", V, V+1, V+2, V+3, V+4, V+5, V+6, V+7);
	i += fscanf(f, "%4u %4u %4u %4u", A, A+1, A+2, A+3);

	if (i < 12)
		return -2;

	for (i = 0; i < 8; i++) {
		meter->voltage_callib[i] = V[i];
		meter->voltage_callib[i] /= 1000;
	}

	for (i = 0; i < 4; i++) {
		meter->current_callib[i] = A[i];
		meter->current_callib[i] /= 1000;
	}

	return 0;
}

/*
 * Just reset callibration.
 */
void vameter_unload_callib(struct VAmeter *meter)
{
	int i;

	for (i = 0; i < 8; i++)
		meter->voltage_callib[i] = 1;

	for (i = 0; i < 4; i++)
		meter->current_callib[i] = 1;
}
