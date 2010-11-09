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
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "libvameter.h"

static char volt_range[64];
static char curr_range[64];
static int nr_samples = -1;

static void voltage_range(uint8_t range, const char *str_range)
{
	(void) range;
	snprintf(volt_range, 64, " (%s)", str_range);
}

static void current_range(uint8_t hw_switch, uint8_t range, const char *str_range)
{
	(void) range;
	(void) hw_switch;
	snprintf(curr_range, 64, " (%s)", str_range);
}

static void voltage_sample_raw(char acdc, float val)
{
	printf("%c%fV%s\n", acdc, val, volt_range);
	
	fflush(stdout);
	
	if (nr_samples > 0)
		nr_samples--;
	//todo else exit
}

static void current_sample_raw(char acdc, float val)
{
	printf("%c%fA%s\n", acdc, val, curr_range);
	fflush(stdout);
	
	if (nr_samples > 0)
		nr_samples--;
	//todo else exit
}

static void voltage_sample(char acdc, float val)
{	
	if (val < 1) {
		printf("%c%.0fmV%s\n", acdc, val*1000, volt_range);
		fflush(stdout);
		return;
	}

	if (val < 10) {
		printf("%c%.2fV%s\n", acdc, val, volt_range);
		fflush(stdout);
		return;
	}

	printf("%c%.1fV%s\n", acdc, val, volt_range);
	fflush(stdout);
	
	if (nr_samples > 0)
		nr_samples--;
}

static void current_sample(char acdc, float val)
{
	if (val < 1) {
		printf("%c%.0fmA%s\n", acdc, val*1000, curr_range);
		fflush(stdout);
		return;
	}

	if (val < 10) {
		printf("%c%.2fA%s\n", acdc, val, curr_range);
		fflush(stdout);
		return;
	}

	printf("%c%.1fA\n%s", acdc, val, curr_range);
	fflush(stdout);
	
	if (nr_samples > 0)
		nr_samples--;
}

static char *help = 
	"Usage: %s -d /dev/ttyXXX [-c callibration_file.cal]\n\n"
	" -A print current\n"
	" -a print current range\n"
	" -V print voltage data\n"
	" -v print voltage\n"
	" -r print raw data\n"
	" -n print number of samples\n"
	" -h prints this help\n"

	"\nWritten by (bugs to):\n"
	"\tmetan{at}ucw.cz\n";

static void print_help(const char *name, int ret)
{
	fprintf(stderr, help, name);

	exit(ret);
}

static int ready = 1;

static void sighandler(int signum)
{
	(void) signum;
	ready = 0;
}

int main(int argc, char *argv[])
{
	struct VAmeter *meter;
	int opt;
	char *dev = NULL, *callib = NULL;
	int ret, raw = 0, p_volt = 0, p_curr = 0, p_vrange = 0, p_crange = 0;

	while ((opt = getopt(argc, argv, "Aac:d:hn:rVv")) != -1) {
		switch (opt) {
			case 'd':
				dev = optarg;
			break;
			case 'c':
				callib = optarg;
			break;
			case 'h':
				print_help(argv[0], 0);
			break;
			case 'V':
				p_volt = 1;
			break;
			case 'v':
				p_vrange = 1;
			break;
			case 'a':
				p_crange = 1;
			break;
			case 'A':
				p_curr = 1;
			break;
			case 'r':
				raw = 1;
			break;
			case 'n':
				nr_samples = atoi(optarg);
			break;
			default:
				print_help(argv[0], 1);
		}
	}
	
	nr_samples *= (p_volt + p_curr);

	if (optind < argc || dev == NULL)
		print_help(argv[0], 1);

	meter = vameter_init(dev);

	if (meter == NULL) {
		fprintf(stderr, "%s: %s\n", dev, strerror(errno));
		return 1;
	}

	signal(SIGINT, sighandler);

	if (callib != NULL)
		if ((ret = vameter_load_callib(meter, callib)) < 0) {
			if (ret == -1)
				fprintf(stderr, "Cannot load callibration: %s: %s\n", callib, strerror(errno));
			else
				fprintf(stderr, "Cannot load callibration: %s: Invalid file format\n", callib);
		}

	if (p_vrange)
		meter->voltage_range = voltage_range;

	if (p_crange)
		meter->current_range = current_range;

	if (p_volt) {
		if (raw)
			meter->voltage_sample = voltage_sample_raw;
		else
			meter->voltage_sample = voltage_sample;
	}

	if (p_curr) {
		if (raw)
			meter->current_sample = current_sample_raw;
		else
			meter->current_sample = current_sample;
	}

	while (ready) {
		int ret;

		if ((ret = vameter_read(meter)) < 0) {
			/* end of file*/
			if (ret == 0)
				return 0;
			
			if (nr_samples == 0)
				return 0;

			fprintf(stderr, "Error reading from device: %s\n", strerror(errno)); 
			vameter_exit(meter);
			return 1;
		}
	}

	vameter_exit(meter);
	return 0;
}
