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
#include "libvameter.h"

static GtkWidget *current_label, *voltage_label;
static GtkWidget *current_range_label, *voltage_range_label;
static struct VAmeter *meter;

static void voltage_sample(char acdc, float sample)
{
	char buf[20];
	
	buf[0] = acdc;

	if (sample < 1) {
		sample = roundf(10000*sample)/10;
		snprintf(buf + 1, 19, "%.1fmV", sample);
	} else {
		sample = roundf(100*sample)/100;
		snprintf(buf + 1, 19, "%.3fV", sample);
	}

	gtk_label_set_text (GTK_LABEL (voltage_label), (gchar*) buf);
}

static void current_sample(char acdc, float sample)
{
	char buf[20];

	buf[0] = acdc;

	if (sample < 1) {
		sample = roundf(10000*sample)/10;
		snprintf(buf + 1, 19, "%.1fmA", sample);
	} else {
		sample = roundf(100*sample) / 100;
		snprintf(buf + 1, 19, "%.3fA", sample);
	}
	
	gtk_label_set_text(GTK_LABEL(current_label), buf);
}

static void voltage_range(uint8_t range, const char *str_range)
{
	gtk_label_set_text(GTK_LABEL(voltage_range_label), str_range);
}

static void current_range(uint8_t hw_switch, uint8_t range, const char *str_range)
{
	gtk_label_set_text(GTK_LABEL(current_range_label), str_range);
}

/*
 * Set callbacks.
 */
static void setup_vameter(struct VAmeter *meter)
{
	meter->voltage_sample = voltage_sample;
	meter->current_sample = current_sample;
	meter->voltage_range  = voltage_range;
	meter->current_range  = current_range;
}

/*
 * Clean up and exit.
 */
static void destroy(GtkWidget *widget, gpointer data)
{
	vameter_exit(meter);
	gtk_exit(0);
}

/*
 * Show about dialog.
 */
static void ShowAboutDialog(GtkWindow *parent)
{
	GtkWidget *dialog;

	const gchar *authors[] = {
		"Cyril Hrubis",
		NULL,
	};

	dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG (dialog), "vameter_gtk");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG (dialog), "first alpha");
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG (dialog), "(C) 2009 Cyril Hrubis");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG (dialog), "This is graphical frontend to vameter library for usb voltmeter/ampermeter.");
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG (dialog), "GPLv2 or any later");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG (dialog), "http://todo");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG (dialog), authors);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/*
 * Shows settings dialog.
 */
static void RunSettingsDialog(GtkWindow *parent)
{
	GtkWidget *dialog, *label, *entry, *table;
	gint response;
	const char *def_dev = "/dev/ttyUSB0";

	if (meter != NULL)
		def_dev = vameter_get_dev(meter);

	dialog = gtk_dialog_new_with_buttons("Serial port settings", parent,
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	label = gtk_label_new("Serial port device:");
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), def_dev);
	table = gtk_table_new(1, 2, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG (dialog)->vbox), table);

	gtk_widget_show_all(dialog);
	
	response = gtk_dialog_run(GTK_DIALOG (dialog));
	
	gtk_widget_hide_all(dialog);

	if (response == GTK_RESPONSE_OK) {
		int no;

		vameter_exit(meter);
		meter = vameter_init(gtk_entry_get_text (GTK_ENTRY (entry)));
		no = errno;

		if (meter == NULL) {
			g_print ("Cannot initalize vameter: %s: %s\n", gtk_entry_get_text(GTK_ENTRY(entry)), strerror(no));
			//TODO
			return;
		}
		
		vameter_read_blocked(meter, false);
		setup_vameter(meter);
	}

	gtk_widget_destroy(dialog);
}

static GtkWidget *CreateMenuBar(GtkWidget *parent)
{
	GtkWidget *menubar, *file, *filemenu, *help, *helpmenu;
	GtkWidget *settings, *about;
	GtkAccelGroup *group;

	group    = gtk_accel_group_new ();
	menubar  = gtk_menu_bar_new ();
	file     = gtk_menu_item_new_with_label ("File");
	help     = gtk_menu_item_new_with_label ("Help");
	filemenu = gtk_menu_new();
	helpmenu = gtk_menu_new();

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file), filemenu);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (help), helpmenu);

	gtk_menu_shell_append ( GTK_MENU_SHELL (menubar), file);
	gtk_menu_shell_append ( GTK_MENU_SHELL (menubar), help);

	settings = gtk_image_menu_item_new_with_label ("Settings");
	about    = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, group);
	
	gtk_menu_shell_append (GTK_MENU_SHELL (helpmenu), about);
	gtk_menu_shell_append (GTK_MENU_SHELL (filemenu), settings);

	g_signal_connect (G_OBJECT (about), "activate", G_CALLBACK (ShowAboutDialog), GTK_WINDOW (parent));
	g_signal_connect (G_OBJECT (settings), "activate", G_CALLBACK (RunSettingsDialog), GTK_WINDOW (parent));
	
	return menubar;
}

static GtkWidget *CreateVAmeter(void)
{
	GtkWidget *table;
	GtkWidget *voltage_frame, *current_frame;
	GtkWidget *voltage_range_frame, *current_range_frame;
	PangoFontDescription *initial_font;
	
	table = gtk_table_new (2, 2, TRUE);

	voltage_frame = gtk_frame_new ("Voltage");
	current_frame = gtk_frame_new ("Current");

	voltage_label = gtk_label_new ("---");
	current_label = gtk_label_new ("---");
	
	gtk_label_set_width_chars(GTK_LABEL (voltage_label), 10);
	gtk_label_set_width_chars(GTK_LABEL (current_label), 10);

	initial_font = pango_font_description_from_string("Sans 16");
	gtk_widget_modify_font(voltage_label, initial_font);
	gtk_widget_modify_font(current_label, initial_font);

	gtk_container_add (GTK_CONTAINER (voltage_frame), voltage_label);
	gtk_container_add (GTK_CONTAINER (current_frame), current_label);
	
	voltage_range_frame = gtk_frame_new ("Voltage range");
	current_range_frame = gtk_frame_new ("Current range");

	voltage_range_label = gtk_label_new ("---");
	current_range_label = gtk_label_new ("---");

	gtk_container_add (GTK_CONTAINER (voltage_range_frame), voltage_range_label);
	gtk_container_add (GTK_CONTAINER (current_range_frame), current_range_label);
	
	gtk_table_attach_defaults (GTK_TABLE (table), voltage_frame, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), current_frame, 1, 2, 0, 1);
	
	gtk_table_attach_defaults (GTK_TABLE (table), voltage_range_frame, 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), current_range_frame, 1, 2, 1, 2);

	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);

	return table;
}


int main(int argc, char *argv[])
{
	GtkWidget *window, *vbox;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "VAmeter");
	
	vbox = gtk_vbox_new(FALSE, 2);

	gtk_box_pack_start_defaults(GTK_BOX (vbox), CreateMenuBar(window));
	gtk_box_pack_start_defaults(GTK_BOX (vbox), CreateVAmeter());

	gtk_container_add(GTK_CONTAINER (window), vbox);

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
	gtk_widget_show_all(window);
	
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	for (;;) {
		while (gtk_events_pending())
			gtk_main_iteration();
	
		if (meter != NULL) {
			if (vameter_read(meter) < 0) {
				vameter_exit(meter);
				meter = NULL;
			}
		}
		
		g_usleep(500);
	}
	
	return 0;
}
