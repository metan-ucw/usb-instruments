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

#include <gtk/gtk.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#include "gtk_common.h"
#include "libcounter.h"

static struct counter *counter = NULL;
static int fd_tag;
static char dev[128] = "/dev/ttyUSB0";
static GtkWidget *freq_label;

static void destroy(GtkWidget *widget, gpointer data)
{
	counter_destroy(counter);
	gtk_exit(0);
}

static void settings(GtkWidget *widget, gpointer data)
{
	
	gtk_run_serial_cfg(NULL, dev, dev, sizeof(dev));
}

static void measure(float freq)
{
	char buf[64];
	
	/* Mhz range */
	if (freq > 1000000) {
		snprintf(buf, sizeof(buf), "%.3f MHz", roundf((float)freq/1000)/1000);
	/* Khz range */
	} else if (freq > 1000) {
		snprintf(buf, sizeof(buf), "%.3f KHz", (float)freq/1000);
	/* Hz range */
	} else {
		snprintf(buf, sizeof(buf), "%.2f Hz", freq);
	}

	gtk_label_set_text(GTK_LABEL(freq_label), buf);
}

static void range(unsigned char range)
{
	printf("range %c\n", range);
}

/*
 * Callback that is called when data are ready on serial port.
 */
static void counter_callback(gpointer data __attribute__((unused)),
                             gint source __attribute__((unused)),
                             GdkInputCondition condition __attribute__((unused)))
{
	counter_read(counter);
}

static void connect(GtkWidget *widget, gpointer data)
{
	counter = counter_create(dev, measure, range);

	if (counter != NULL)
		fd_tag = gdk_input_add(counter->port->fd, GDK_INPUT_READ, counter_callback, NULL);
}

static void disconnect(GtkWidget *widget, gpointer data)
{
	gdk_input_remove(fd_tag);
	counter_destroy(counter);
	counter = NULL;
	gtk_label_set_text(GTK_LABEL(freq_label), "--- Mhz");
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

static GtkWidget *create_counter(void)
{
	GtkWidget *table = gtk_table_new(2, 1, TRUE);
	GtkWidget *freq_frame = gtk_frame_new("Frequency");
	PangoFontDescription *font;
	
	freq_label = gtk_label_new("--- Mhz");

	font = pango_font_description_from_string("Sans 16");
        gtk_widget_modify_font(freq_label, font);

	gtk_container_add(GTK_CONTAINER(freq_frame), freq_label);

	gtk_table_attach_defaults(GTK_TABLE(table), freq_frame, 0, 1, 0, 1);

	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	return table;
}

int main(int argc, char *argv[])
{
	GtkWidget *window, *vbox;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "USB Counter");
	
	vbox = gtk_vbox_new(FALSE, 1);

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

	gtk_box_pack_start(GTK_BOX(vbox), create_menubar(window), FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), create_counter(), TRUE, TRUE, 0);


	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show_all(window);
	
	gtk_main();

	return 0;
}
