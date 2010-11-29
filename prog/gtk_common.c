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

#include <string.h>
#include <gtk/gtk.h>

#define DEF_DEV "/dev/ttyUSB0"

int gtk_run_serial_cfg(GtkWindow *parent, const char *old_dev, char *dev, size_t size)
{

	GtkWidget *dialog, *label, *entry, *table;
	gint response;

	if (old_dev == NULL)
		old_dev = DEF_DEV;
	
	dialog = gtk_dialog_new_with_buttons("Serial port settings", parent,
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	label = gtk_label_new("Serial port device:");
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), old_dev);
	table = gtk_table_new(1, 2, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG (dialog)->vbox), table);

	gtk_widget_show_all(dialog);
	
	response = gtk_dialog_run(GTK_DIALOG (dialog));
	
	gtk_widget_hide_all(dialog);

	if (response == GTK_RESPONSE_OK) {
		strncpy(dev, gtk_entry_get_text (GTK_ENTRY (entry)), size);
		dev[size - 1] = '\0';
		return 1;
	}

	gtk_widget_destroy(dialog);

	return 0;
}
