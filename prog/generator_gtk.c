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

#include <gtk/gtk.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#include "gtk_common.h"
#include "libgenerator.h"

static struct generator *generator = NULL;
static int fd_tag;
static char dev[128] = "/dev/ttyUSB0";

/* static global gtk widgets */
static GtkWidget *freq_entry;
static GtkWidget *filters[4];
static GtkWidget *waves[8];
static GtkWidget *memory[8];

static void destroy(GtkWidget *widget, gpointer data)
{
	generator_destroy(generator);
	gtk_exit(0);
}

static void settings(GtkWidget *widget, gpointer data)
{
	
	gtk_run_serial_cfg(NULL, dev, dev, sizeof(dev));
}

/*
 * Callback called from generator library, on change.
 */
static void generator_update(struct generator *self)
{
	char buf[128];

	printf("Generator update\n");

	/* set frequency */
	snprintf(buf, sizeof(buf), "%6.2f", fabs(generator_convert_freq(self)));
	gtk_entry_set_text(GTK_ENTRY(freq_entry), buf);

	/* set filter */
	if (self->filter < 4)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filters[self->filter]), TRUE);

	/* set wave */
	switch (self->wave) {
	case GENERATOR_WAVE_SINE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(waves[0]), TRUE);
	break;
	case GENERATOR_WAVE_TRIANGLE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(waves[1]), TRUE);
	break;
	case GENERATOR_WAVE_SAWTOOTH:
		if (generator_convert_freq(self) > 0)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(waves[2]), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(waves[3]), TRUE);
	break;
	case GENERATOR_WAVE_SQUARE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(waves[4]), TRUE);
	break;
	case GENERATOR_WAVE_BW_VIDEO:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(waves[5]), TRUE);
	break;
	case GENERATOR_WAVE_SERIAL:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(waves[6]), TRUE);
	break;
	case GENERATOR_WAVE_SERIAL_INV:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(waves[7]), TRUE);
	break;
	default:
	break;
	}

	/* set memory */
	if (self->mem < 8)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(memory[self->mem]), TRUE);
	else
		printf("Invalid memory %u\n", self->mem);
}

/*
 * Callback that is called when data are ready on serial port.
 */
static void generator_callback(gpointer data __attribute__((unused)),
                               gint source __attribute__((unused)),
                               GdkInputCondition condition __attribute__((unused)))
{
	generator_read(generator);
}

static void connect(GtkWidget *widget, gpointer data)
{
	generator = generator_create(dev, generator_update);

	if (generator != NULL) {
		fd_tag = gdk_input_add(generator->port->fd, GDK_INPUT_READ, generator_callback, NULL);
		generator_load_state(generator);
		return;
	}

	printf("Failed to connect\n");
}

static void disconnect(GtkWidget *widget, gpointer data)
{
	if (generator == NULL) {
		printf("Nothing to disconnect\n");
		return;
	}

	gdk_input_remove(fd_tag);
	generator_destroy(generator);
	generator = NULL;
}

static GtkItemFactoryEntry menu_items[] = {
	{"/_File",            NULL,      NULL,       0, "<Branch>",     NULL},
	{"/File/_Settings",   NULL,      settings,   0, "<StockItem>",  GTK_STOCK_PREFERENCES},
	{"/File/_Connect",    NULL,      connect,    0, "<StockItem>",  GTK_STOCK_CONNECT},
	{"/File/_Disconnect", NULL,      disconnect, 0, "<StockItem>",  GTK_STOCK_DISCONNECT},
	{ "/File/sep1",       NULL,      NULL,       0, "<Separator>",  NULL},
	{"/File/_Quit",       "<CTRL>Q", destroy,    0, "<StockItem>",  GTK_STOCK_QUIT},
	{"/_Help",            NULL,      NULL,       0, "<LastBranch>", NULL},
	{"/Help/_About",      NULL,      NULL,       0, "<StockItem>",  GTK_STOCK_ABOUT}
};

#define array_size(array) (sizeof(array)/sizeof((array)[0]))

static GtkWidget *create_menubar(GtkWidget *window)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new();

	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>",
	                                    accel_group);

	gtk_item_factory_create_items(item_factory, array_size(menu_items),
                                      menu_items, NULL);

	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	return gtk_item_factory_get_widget(item_factory, "<main>");
}

static void wave_radio_button_callback(GtkWidget *widget,
                                       gpointer data)
{
	int idx = (int)data;
	enum generator_wave wave = GENERATOR_WAVE_UNKNOWN;

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	if (generator == NULL)
		return;

	/* set wave */
	switch (idx) {
	case 0:
		wave = GENERATOR_WAVE_SINE;
	break;
	case 1:
		wave = GENERATOR_WAVE_TRIANGLE;
	break;
	case 2:
		wave = GENERATOR_WAVE_SAWTOOTH;
	break;
	case 3:
		//TODO: negative freq
		wave = GENERATOR_WAVE_SAWTOOTH;
	break;
	case 4:
		wave = GENERATOR_WAVE_SQUARE;
	break;
	case 5:
		wave = GENERATOR_WAVE_BW_VIDEO;
	break;
	case 6:
		wave = GENERATOR_WAVE_SERIAL;
	break;
	case 7:
		wave = GENERATOR_WAVE_SERIAL_INV;
	break;
	default:
		return;
	}

	printf("Setting wave\n");

	generator_set_wave(generator, wave);
	generator_load_state(generator);
}

static void filter_radio_button_callback(GtkWidget *widget,
                                         gpointer data)
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	if (generator == NULL)
		return;

	enum generator_filter filter = (enum generator_filter) data;

	printf("Setting filter\n");
	generator_set_filter(generator, filter);
	generator_load_state(generator);
}

static void memory_radio_button_callback(GtkWidget *widget,
                                       gpointer data)
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	if (generator == NULL)
		return;

	uint8_t mem = (int)data;

	printf("Loading memory %u\n", mem);

	generator_load(generator, mem);
	generator_load_state(generator);
}

static void amplitude_slider_callback(GtkRange *range,
                                      gpointer *priv __attribute__((unused)))
{
	int val = gtk_range_get_value(range);
	
	printf("Amplitude callback\n");
	
//	if (counter != NULL)
//		counter_trigger(counter, val);
}

static void offset_slider_callback(GtkRange *range,
                                   gpointer *priv __attribute__((unused)))
{
	int val = gtk_range_get_value(range);
	
	printf("Offset callback\n");

//	if (counter != NULL)
//		counter_trigger(counter, val);
}

static void freq_entry_callback(GtkWidget *widget, GtkEntry *entry)
{
	printf("Entry %s\n", gtk_entry_get_text(GTK_ENTRY(entry)));
}

static GtkWidget *create_generator(void)
{
	GtkWidget *table = gtk_table_new(3, 4, FALSE), *ftable;
	GtkWidget *freq_frame = gtk_frame_new("Freq Hz/Baud");
	GtkWidget *filter_frame = gtk_frame_new("Filter");
	GtkWidget *wave_frame = gtk_frame_new("Output wave");
	GtkWidget *amplitude_frame = gtk_frame_new("Amplitude");
	GtkWidget *offset_frame = gtk_frame_new("Offset");
	GtkWidget *memory_frame = gtk_frame_new("Memory");
	GtkWidget *box, *button, *slider;
	GSList *group;

	gtk_container_set_border_width(GTK_CONTAINER(table), 5);

	/* frequency edit box */
	freq_entry = gtk_entry_new_with_max_length(8);
	gtk_entry_set_text(GTK_ENTRY(freq_entry), "---");

	gtk_signal_connect(GTK_OBJECT(freq_entry), "activate",
	                   GTK_SIGNAL_FUNC(freq_entry_callback),
			   freq_entry);

	gtk_container_add(GTK_CONTAINER(freq_frame), freq_entry);

	/* filter radio buttons */
	ftable = gtk_table_new(2, 2, TRUE);
	
	button = gtk_radio_button_new_with_label(NULL, "None");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_table_attach_defaults(GTK_TABLE(ftable), button, 0, 1, 0, 1);
	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(filter_radio_button_callback),
			 (void*)GENERATOR_FILTER_NONE);
	filters[0] = button;

	button = gtk_radio_button_new_with_label(group, "600 Hz");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_table_attach_defaults(GTK_TABLE(ftable), button, 1, 2, 0, 1);
	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(filter_radio_button_callback),
			 (void*)GENERATOR_FILTER_600HZ);
	filters[3] = button;
	
	button = gtk_radio_button_new_with_label(group, "8 kHz");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_table_attach_defaults(GTK_TABLE(ftable), button, 0, 1, 1, 2);
	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(filter_radio_button_callback),
			 (void*)GENERATOR_FILTER_8KHZ);
	filters[2] = button;
	
	button = gtk_radio_button_new_with_label(group, "100 kHz");
	gtk_table_attach_defaults(GTK_TABLE(ftable), button, 1, 2, 1, 2);
	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(filter_radio_button_callback),
			 (void*)GENERATOR_FILTER_100KHZ);

	gtk_container_add(GTK_CONTAINER(filter_frame), ftable);
	filters[1] = button;
	
	/* wave radio buttons */
	box = gtk_vbox_new(FALSE, 5);

	char *wave_names[] = {
		"Sinus", "Triangle", "Saw", "Saw inv.",
		"Square", "B/W video", "Serial", "Serial inv."
	};
	
	int i;
	
	group = NULL;

	for (i = 0; i < 8; i++) {
		waves[i] = gtk_radio_button_new_with_label(group, wave_names[i]);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(waves[i]));
		gtk_box_pack_start(GTK_BOX(box), waves[i], FALSE, TRUE, 0);
		g_signal_connect(G_OBJECT(waves[i]), "toggled",
		                 G_CALLBACK(wave_radio_button_callback), (void*)i);
	}

	gtk_container_add(GTK_CONTAINER(wave_frame), box);

	/* memory radio buttons */
	box = gtk_hbox_new(FALSE, 5);

	group = NULL;

	for (i = 0; i < 8; i++) {
		char buf[2] = {0, 0};

		buf[0] = i + '1';

		memory[i] = gtk_radio_button_new_with_label(group, buf);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(memory[i]));
		gtk_box_pack_start(GTK_BOX(box), memory[i], FALSE, TRUE, 0);
		g_signal_connect(G_OBJECT(memory[i]), "toggled",
		                 G_CALLBACK(memory_radio_button_callback), (void*)i);
	}
	
	gtk_container_add(GTK_CONTAINER(memory_frame), box);

	/* amplitude slider */
	slider = gtk_vscale_new_with_range(0, 4.81, 0.01);
	gtk_range_set_value(GTK_RANGE(slider), 0);
	g_signal_connect(slider, "value_changed",
	                 G_CALLBACK(amplitude_slider_callback), NULL);
	gtk_container_add(GTK_CONTAINER(amplitude_frame), slider);

	/* offset slider */
	slider = gtk_vscale_new_with_range(-4.81, 0, 0.01);
	gtk_range_set_value(GTK_RANGE(slider), 0);
	g_signal_connect(slider, "value_changed",
	                 G_CALLBACK(offset_slider_callback), NULL);
	gtk_container_add(GTK_CONTAINER(offset_frame), slider);


	gtk_table_attach_defaults(GTK_TABLE(table), wave_frame, 0, 1, 0, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), freq_frame, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), filter_frame, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), amplitude_frame, 2, 3, 0, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), offset_frame, 3, 4, 0, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), memory_frame, 1, 4, 2, 3);

	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	return table;
}

int main(int argc, char *argv[])
{
	GtkWidget *window, *vbox;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "USB Generator");
	
	vbox = gtk_vbox_new(FALSE, 1);

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

	gtk_box_pack_start(GTK_BOX(vbox), create_menubar(window), FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), create_generator(), TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show_all(window);
	
	gtk_main();

	return 0;
}
