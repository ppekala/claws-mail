/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999,2000 Hiroyuki Yamamoto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#if HAVE_LOCALE_H
#  include <locale.h>
#endif

#include "sylpheed.h"
#include "intl.h"
#include "defs.h"
#include "utils.h"
#include "ssl.h"
#include "version.h"

static gboolean sylpheed_initialized = FALSE;

gboolean sylpheed_init(int *argc, char ***argv)
{
#ifdef WIN32
	gchar *locale_dir;
#endif
	if (sylpheed_initialized)
		return TRUE;

	setlocale(LC_ALL, "");
#ifdef WIN32
	locale_dir = g_strconcat(get_installed_dir(), G_DIR_SEPARATOR_S,
				LOCALEDIR, NULL);
	bindtextdomain(PACKAGE, locale_dir);
	g_free(locale_dir);
#else
	bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	textdomain(PACKAGE);

	/* backup if old rc file exists */
	if (is_file_exist(RC_DIR)) {
		if (rename(RC_DIR, RC_DIR ".bak") < 0) {
			FILE_OP_ERROR(RC_DIR, "rename");
			return FALSE;
		}
	}

#if USE_OPENSSL
	ssl_init();
#endif

	srandom((gint)time(NULL));

	sylpheed_initialized = TRUE;

	return TRUE;
}
