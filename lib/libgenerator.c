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
#include <math.h>

#include "libgenerator.h"

const char *generator_wave_names[] = {
	"Unknown",
	"Sine",
	"Triangle",
	"Sawtooth",
	"Square",
	"B/W Video",
	"Serial",
	"Serial inv.",
	NULL
};

const char *generator_filter_names[] = {
	"None",
	"100kHz",
	"8kHz",
	"600Hz",
	"Unknown",
	NULL
};

struct generator *generator_create(const char *port,
                                   void (*update)(struct generator *self))
{
	struct generator *generator = malloc(sizeof (struct generator));

	if (generator == NULL)
		return NULL;

	generator->port = libserial_open(port, B19200);

	if (generator->port == NULL) {
		free(generator);
		return NULL;
	}

	/* callback */
	generator->update = update;

	/* state initalization */
	generator->wave      = GENERATOR_WAVE_UNKNOWN;
	generator->filter    = GENERATOR_FILTER_UNKNOWN;
	generator->amplitude = 0;
	generator->offset    = 0;
	generator->freq      = 0;
	generator->mem       = 0;

	/* serial data buffer */
	generator->data_pos  = 0;
	generator->data_flag = 0;

	return generator;
}

void generator_destroy(struct generator *self)
{
	if (self == NULL)
		return;

	libserial_close(self->port);
	free(self);
}

/*
 * We have several packed types here:
 *
 * 0x3X             - button was pressed, wave X loaded
 * 0xd3             - request was succesfull
 * 0xd2, ...., 0x0a - 10 bytes, generator state:
 *                    wave_type, 3xfreq, offset, amp, filter, mem
 */
static void generator_parse_state(struct generator *self)
{
	unsigned int i;

	for (i = 0; i < self->data_pos; i++)
		printf("0x%02x ", self->data[i]);

	printf("\n");

	self->wave      = self->data[1];
	self->freq      = self->data[2]<<16 | self->data[3]<<8 | self->data[4];
	self->offset    = self->data[5];
	self->amplitude = self->data[6];
	self->filter    = self->data[7];
	self->mem       = self->data[8];

	/* 24 bit negative two's complement */
	if (self->freq & 0x800000) {
		self->freq = (~self->freq & 0xffffff) + 1;
		self->freq = -self->freq;
	}

	/* end of data packet */
	self->data_pos  = 0;
	self->data_flag = 0;

	/* Call update if set */
	if (self->update != NULL)
		self->update(self);
}

static void dump(uint8_t *buf, int len)
{
	int i;

	printf("RAW: ");

	for (i = 0; i < len; i++)
		printf("0x%02x ", buf[i]);

	printf("\n");
}

void generator_read(struct generator *self)
{
	int len, i;

	if ((len = read(self->port->fd, self->data + self->data_pos,
	    sizeof(self->data) - self->data_pos)) > 0) {

	//	dump(self->data + self->data_pos, len);

		self->data_pos += len;

	}

	/* in the middle of generator state packet */
	if (self->data_flag) {
		if (self->data_pos == 10)
			generator_parse_state(self);

		return;
	}

	for (i = 0; i < self->data_pos; i++) {
		switch (self->data[i]) {
		/* memory loaded state */
		case 0x30 ... 0x37:
			printf("Memory %i\n", self->data[i] & 0x07);
			generator_load_state(self);
		break;
		/* ack from generator */
		case 0xd3:
			printf("Operation successful\n");
		break;
		/* generator state is send */
		case 0xd2:
			printf("Start of state packet\n");
			self->data_flag = 1;
			memmove(self->data, self->data+i, self->data_pos - i);
			self->data_pos -= i;
			return;
		break;
		default:
			printf("Lost 0x%02x\n", self->data[i]);
		break;
		}
	}

	self->data_pos = 0;
}

#define SAVE(x) (0x60 | (0x07 & (x)))
#define LOAD(x) (0x70 | (0x07 & (x)))

void generator_save(struct generator *self, uint8_t pos)
{
	uint8_t s = SAVE(pos);

	if (write(self->port->fd, &s, 1) != 1)
		printf("Error saving state: %s\n.", strerror(errno));
}

void generator_load(struct generator *self, uint8_t pos)
{
	uint8_t l = LOAD(pos);

	if (write(self->port->fd, &l, 1) != 1)
		printf("Error loading state: %s\n.", strerror(errno));
}

#define WAVE(x) (0x30 | (0x07 & (x)))

void generator_set_wave(struct generator *self, enum generator_wave wave)
{
	uint8_t w = WAVE(wave);

	if (wave == 0)
		return;

	if (write(self->port->fd, &w, 1) != 1)
		printf("Error setting output wave: %s\n.", strerror(errno));
}

#define FILTER(x) (0x03 & (x))

void generator_set_filter(struct generator *self, enum generator_filter filter)
{
	uint8_t f[] = {'F', FILTER(filter)};

	if (write(self->port->fd, f, 2) != 2)
		printf("Error setting output filter: %s\n.", strerror(errno));
}

void generator_set_amplitude(struct generator *self, uint8_t amplitude)
{
	uint8_t a[] = {'V', amplitude};

	if (write(self->port->fd, a, 2) != 2)
		printf("Error setting output amplitude: %s\n.", strerror(errno));
}

void generator_set_offset(struct generator *self, uint8_t offset)
{
	uint8_t o[] = {'O', offset};

	if (write(self->port->fd, o, 2) != 2)
		printf("Error setting output offset: %s\n.", strerror(errno));
}

#define F1(x) ((uint8_t)(((x)>>16) & 0xff))
#define F2(x) ((uint8_t)(((x)>>8) & 0xff))
#define F3(x) ((uint8_t)((x) & 0xff))

void generator_set_freq(struct generator *self, uint32_t freq)
{
	uint8_t f[] = {'S', F1(freq), F2(freq), F3(freq)};

	if (write(self->port->fd, f, 4) != 4)
		printf("Error setting output frequency: %s\n.", strerror(errno));
}

void generator_set_freq_float(struct generator *self, float freq)
{
	uint32_t fval;

	switch (self->wave) {
	case GENERATOR_WAVE_UNKNOWN:
	case GENERATOR_WAVE_BW_VIDEO:
		printf("Cannot set frequency for BW video\n");
		return;
	case GENERATOR_WAVE_TRIANGLE:
	case GENERATOR_WAVE_SINE:
		fval = round((16777216 * 9 * freq) / 20000000);
	break;
	case GENERATOR_WAVE_SAWTOOTH:
		fval = round((16777216 * 6 * freq) / 20000000);
	break;
	case GENERATOR_WAVE_SQUARE:
		fval = round((16777216 * 7 * freq) / 20000000);
	break;
	case GENERATOR_WAVE_SERIAL:
	case GENERATOR_WAVE_SERIAL_INV:
		//return 200000000.00 / (self->freq >> 8);
		printf("TODO\n");
		return;
	break;
	}

	printf("%f %u\n", freq, fval);

	generator_set_freq(self, fval);
}

void generator_load_state(struct generator *self)
{
	uint8_t q = '?';

	if (write(self->port->fd, &q, 1) != 1)
		printf("Error requesting state: %s\n.", strerror(errno));
}

float generator_convert_freq(struct generator *self)
{
	switch (self->wave) {
	case GENERATOR_WAVE_UNKNOWN:
	case GENERATOR_WAVE_BW_VIDEO:
		return 0;
	break;
	case GENERATOR_WAVE_TRIANGLE:
	case GENERATOR_WAVE_SINE:
		return (float)self->freq * 20000000 / 9 / 16777216;
	break;
	case GENERATOR_WAVE_SAWTOOTH:
		return (float)self->freq * 20000000 / 6 / 16777216;
	break;
	case GENERATOR_WAVE_SQUARE:
		return (float)self->freq * 20000000 / 7 / 16777216;
	break;
	case GENERATOR_WAVE_SERIAL:
	case GENERATOR_WAVE_SERIAL_INV:
		return 200000000.00 / (self->freq >> 8);
	break;
	}
}

float generator_convert_amplitude(struct generator *self)
{
	return (float)self->amplitude * 4.81 / 255;
}

float generator_convert_offset(struct generator *self)
{
	return -(float)self->amplitude * 4.81 / 255 / 2;
}
