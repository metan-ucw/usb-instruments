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
	printf("Generator update\n");

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

	if (generator != NULL)
		fd_tag = gdk_input_add(generator->port->fd, GDK_INPUT_READ, generator_callback, NULL);
}

static void disconnect(GtkWidget *widget, gpointer data)
{
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

static void radio_button_callback(GtkWidget *widget,
                                  gpointer data __attribute__((unused)))
{
	printf("radio callback\n");

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	if (generator == NULL)
		return;

//	counter_mode(counter, COUNTER_05SEC_PERIOD);
}

static void filter_radio_button_callback(GtkWidget *widget,
                                         gpointer data __attribute__((unused)))
{
	printf("filter callback\n");

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	if (generator == NULL)
		return;

//	counter_mode(counter, COUNTER_05SEC_PERIOD);
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

static GtkWidget *create_generator(void)
{
	GtkWidget *table = gtk_table_new(2, 4, TRUE), *ftable;
	GtkWidget *freq_frame = gtk_frame_new("Frequency/Baud");
	GtkWidget *filter_frame = gtk_frame_new("Filter");
	GtkWidget *wave_frame = gtk_frame_new("Output wave");
	GtkWidget *amplitude_frame = gtk_frame_new("Amplitude");
	GtkWidget *offset_frame = gtk_frame_new("Offset");
	GtkWidget *box, *button, *slider;
	GSList *group;
	PangoFontDescription *font;
	
	gtk_container_set_border_width(GTK_CONTAINER(table), 5);
	
	/*
	freq_label = gtk_label_new("--- Mhz");
	range_label = gtk_label_new("-");

	font = pango_font_description_from_string("Monospace 16");
        gtk_widget_modify_font(freq_label, font);
	gtk_widget_modify_font(range_label, font);

	gtk_container_add(GTK_CONTAINER(freq_frame), freq_label);
	gtk_container_add(GTK_CONTAINER(range_frame), range_label);
	*/

	/* filter radio buttons */
	ftable = gtk_table_new(2, 2, TRUE);
	
	button = gtk_radio_button_new_with_label(NULL, "None");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_table_attach_defaults(GTK_TABLE(ftable), button, 0, 1, 0, 1);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(filter_radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "600 Hz");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_table_attach_defaults(GTK_TABLE(ftable), button, 1, 2, 0, 1);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(filter_radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "8 kHz");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_table_attach_defaults(GTK_TABLE(ftable), button, 0, 1, 1, 2);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(filter_radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "100 kHz");
	gtk_table_attach_defaults(GTK_TABLE(ftable), button, 1, 2, 1, 2);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(filter_radio_button_callback), NULL);

	gtk_container_add(GTK_CONTAINER(filter_frame), ftable);
	
	/* wave radio buttons */
	box = gtk_vbox_new(FALSE, 5);
	
	button = gtk_radio_button_new_with_label(NULL, "Sinus");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(radio_button_callback), NULL);

	button = gtk_radio_button_new_with_label(group, "Triangle");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
	
	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "Saw");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
	
	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "Saw inv.");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "Square");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "B/W video");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "Serial");
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(radio_button_callback), NULL);
	
	button = gtk_radio_button_new_with_label(group, "Serial inv.");
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(button), "toggled",
	                 G_CALLBACK(radio_button_callback), NULL);


	gtk_container_add(GTK_CONTAINER(wave_frame), box);

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
