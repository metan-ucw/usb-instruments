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
 * Copyright (C) 2009-2011 Cyril Hrubis <metan@ucw.cz>                        *
 *                                                                            *
 ******************************************************************************/

#ifndef __LIBGENERATOR_H__
#define __LIBGENERATOR_H__

#include <stdint.h>

#include "libserial.h"

/* Generator output wave */
enum generator_wave {
	GENERATOR_WAVE_UNKNOWN = 0,
	GENERATOR_WAVE_SINE = 1,
	GENERATOR_WAVE_TRIANGLE = 2,
	GENERATOR_WAVE_SAWTOOTH = 3,
	GENERATOR_WAVE_SQUARE = 4,
	GENERATOR_WAVE_BW_VIDEO = 5,
	GENERATOR_WAVE_SERIAL = 6,
	GENERATOR_WAVE_SERIAL_INV = 7,
};

/* Null terminated array of strings */
extern const char *generator_wave_names[];

/* Output filter */
enum generator_filter {
	GENERATOR_FILTER_NONE = 0,
	GENERATOR_FILTER_100KHZ = 1,
	GENERATOR_FILTER_8KHZ = 2,
	GENERATOR_FILTER_600HZ = 3,
	GENERATOR_FILTER_UNKNOWN = 4,
};

/* Null terminated array of strings */
extern const char *generator_filter_names[];

struct generator {
	struct libserial_port *port;

	void (*update)(struct generator *self);

	/* generator internal state */
	enum generator_wave   wave;
	enum generator_filter filter;
	uint8_t amplitude;
	uint8_t offset;
	int32_t freq;
	uint8_t mem;

	/* serial data buffer */
	uint8_t data_flag;
	uint8_t data_pos;
	uint8_t data[10];
};

/*
 * Lock serial port. initalize generator.
 *
 * The update callback is called once generator
 * succesfully has changed the settings.
 */
struct generator *generator_create(const char *port,
                                   void (*update)(struct generator *self));

/*
 * Unlock serial port, free memory.
 */
void generator_destroy(struct generator *self);

/*
 * Call either to sleep in read, or when data are ready on fd.
 */
void generator_read(struct generator *self);

/*
 * Generator can save up to 8 signals that can be later loaded.
 */
void generator_save(struct generator *self, uint8_t pos);
void generator_load(struct generator *self, uint8_t pos);

/*
 * Set generator output wave.
 */
void generator_set_wave(struct generator *self, enum generator_wave wave);

/*
 * Set generator output filter.
 */
void generator_set_filter(struct generator *self, enum generator_filter filter);

/*
 * Set generator output amplitude.
 */
void generator_set_amplitude(struct generator *self, uint8_t amplitude);

/*
 * Set generator output offset.
 */
void generator_set_offset(struct generator *self, uint8_t offset);

/*
 * Set generator output frequency lower three bytes are used.
 */
void generator_set_freq(struct generator *self, uint32_t freq);

/*
 * Dtto but converts frequency in hertz to 24 bit two complement value.
 */
void generator_set_freq_float(struct generator *self, float freq);

/*
 * Tells generator to send it's state.
 */
void generator_load_state(struct generator *self);

/*
 * Converts 24 bit freq value to Hz.
 *
 * Returns period in usec for serial waves
 */
float generator_convert_freq(struct generator *self);

/*
 * Returns amplitude in volts.
 */
float generator_convert_amplitude(struct generator *self);

/*
 * Returns offset in volts.
 */
float generator_convert_offset(struct generator *self);

#endif /* __LIBGENERATOR_H__ */
