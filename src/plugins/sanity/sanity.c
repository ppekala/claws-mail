/*
 * Claws Mail -- a GTK based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2026 the Claws Mail Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "version.h"
#include "claws.h"
#include "plugin.h"
#include "utils.h"
#include "prefs_common.h"
#include "mainwindow.h"
#include "folderview.h"

#define PLUGIN_NAME (_("Sanity"))

struct {
	gboolean always_show_msg;
	gboolean autochk_newmail;
	GdkRGBA col_log_in;
	GdkRGBA col_log_snok;
	GdkRGBA col_uri;
	gint folderview_vscrollbar_policy;
	ToolbarStyle toolbar_style;
} defaults;

void sanity_reflect_prefs(PrefsCommon *prefs)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	GtkTextTag *tag = NULL;

	if (!mainwin)
		return;

	main_window_reflect_prefs_all();

	gtk_scrolled_window_set_policy(
			GTK_SCROLLED_WINDOW(mainwin->folderview->scrolledwin),
			GTK_POLICY_AUTOMATIC,
			prefs->folderview_vscrollbar_policy);
	gtk_widget_queue_draw(GTK_WIDGET(mainwin->folderview->scrolledwin));

	GtkTextBuffer *buffer = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(mainwin->logwin->text));
	GtkTextTagTable *tags = gtk_text_buffer_get_tag_table(buffer);

	if ((tag = gtk_text_tag_table_lookup(tags, "input")))
		g_object_set(G_OBJECT(tag), "foreground-rgba",
				&prefs->color[COL_LOG_IN], NULL);
	if ((tag = gtk_text_tag_table_lookup(tags, "status_nok")))
		g_object_set(G_OBJECT(tag), "foreground-rgba",
				&prefs->color[COL_LOG_STATUS_NOK], NULL);

	toolbar_set_style(mainwin->toolbar->toolbar, mainwin->handlebox,
			prefs->toolbar_style);
}

gint plugin_init(gchar **error)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	PrefsCommon *prefs = prefs_common_get_prefs();

	/* it's not 90s anymore, we have bandwidth to download/open */
	defaults.always_show_msg = prefs->always_show_msg;
	prefs->always_show_msg = TRUE;

	/* autocheck for newmail */
	defaults.autochk_newmail = prefs->autochk_newmail;
	prefs->autochk_newmail = TRUE;

	/* remove visual clutter in folderview - hide scrollbar when not needed */
	defaults.folderview_vscrollbar_policy = prefs->folderview_vscrollbar_policy;
	prefs->folderview_vscrollbar_policy = 1;

	/* Some logwindow colors clash with used theme */
	memcpy(&defaults.col_log_in, &prefs->color[COL_LOG_IN], sizeof(GdkRGBA));
	memcpy(&defaults.col_log_snok, &prefs->color[COL_LOG_STATUS_NOK], sizeof(GdkRGBA));
	/* green links, don't see why be different from everyone else */
	memcpy(&defaults.col_uri, &prefs->color[COL_URI], sizeof(GdkRGBA));

	GtkStyleContext *ctx = gtk_widget_get_style_context(mainwin->window);
	gtk_style_context_get_color(ctx, GTK_STATE_FLAG_NORMAL, &prefs->color[COL_LOG_IN]);
	gtk_style_context_get_color(ctx, GTK_STATE_FLAG_FOCUSED, &prefs->color[COL_LOG_STATUS_NOK]);
	gtk_style_context_get_color(ctx, GTK_STATE_FLAG_LINK, &prefs->color[COL_URI]);

	/* show icons and text beside for compactness */
	defaults.toolbar_style = prefs->toolbar_style;
	prefs->toolbar_style = 4;

	sanity_reflect_prefs(prefs);
	return 0;
}

gboolean plugin_done(void)
{
	PrefsCommon *prefs = prefs_common_get_prefs();

	prefs->always_show_msg = defaults.always_show_msg;
	prefs->autochk_newmail = defaults.autochk_newmail;
	prefs->folderview_vscrollbar_policy = defaults.folderview_vscrollbar_policy;
	memcpy(&prefs->color[COL_LOG_IN], &defaults.col_log_in, sizeof(GdkRGBA));
	memcpy(&prefs->color[COL_LOG_STATUS_NOK], &defaults.col_log_snok, sizeof(GdkRGBA));
	memcpy(&prefs->color[COL_URI], &defaults.col_uri, sizeof(GdkRGBA));
	prefs->toolbar_style = defaults.toolbar_style;

	sanity_reflect_prefs(prefs);
	return TRUE;
}

const gchar *plugin_name(void)
{
	return PLUGIN_NAME;
}

const gchar *plugin_desc(void)
{
	return _("This plugin tries to set sane defaults for Claws Mail. "
	         "It's main objective is to speed up configuring new installations.\n\n"
	         "Default configuration is saved on plugin load and restored on unload.");
}

const gchar *plugin_type(void)
{
	return "GTK3";
}

const gchar *plugin_licence(void)
{
	return "GPL3+";
}

const gchar *plugin_version(void)
{
	return VERSION;
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] =
		{ {PLUGIN_OTHER, N_("Sanity")},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}
