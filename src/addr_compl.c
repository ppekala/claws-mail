/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 *
 * Copyright (c) 2000-2001 by Alfons Hoogervorst <alfons@proteus.demon.nl>
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
#include "intl.h"
#include "defs.h"

#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkscrolledwindow.h>

#include <string.h>
#include <ctype.h>
#if (HAVE_WCTYPE_H && HAVE_WCHAR_H)
#  include <wchar.h>
#  include <wctype.h>
#endif

#include "addressbook.h"
#include "addr_compl.h"
#include "utils.h"

/* How it works:
 *
 * The address book is read into memory. We set up an address list
 * containing all address book entries. Next we make the completion
 * list, which contains all the completable strings, and store a
 * reference to the address entry it belongs to.
 * After calling the g_completion_complete(), we get a reference
 * to a valid email address.  
 *
 * Completion is very simplified. We never complete on another prefix,
 * i.e. we neglect the next smallest possible prefix for the current
 * completion cache. This is simply done so we might break up the
 * addresses a little more (e.g. break up alfons@proteus.demon.nl into
 * something like alfons, proteus, demon, nl; and then completing on
 * any of those words).
 */ 
	
/* address_entry - structure which refers to the original address entry in the
 * address book 
 */
typedef struct
{
	gchar *name;
	gchar *address;
} address_entry;

/* completion_entry - structure used to complete addresses, with a reference
 * the the real address information.
 */
typedef struct
{
	gchar		*string; /* string to complete */
	address_entry	*ref;	 /* address the string belongs to  */
} completion_entry;

/*******************************************************************************/

static gint	    g_ref_count;	/* list ref count */
static GList 	   *g_completion_list;	/* list of strings to be checked */
static GList 	   *g_address_list;	/* address storage */
static GCompletion *g_completion;	/* completion object */

/* To allow for continuing completion we have to keep track of the state
 * using the following variables. No need to create a context object. */

static gint	    g_completion_count;		/* nr of addresses incl. the prefix */
static gint	    g_completion_next;		/* next prev address */
static GSList	   *g_completion_addresses;	/* unique addresses found in the
						   completion cache. */
static gchar	   *g_completion_prefix;	/* last prefix. (this is cached here
						 * because the prefix passed to g_completion
						 * is g_strdown()'ed */

/*******************************************************************************/

/* completion_func() - used by GTK to find the string data to be used for 
 * completion 
 */
static gchar *completion_func(gpointer data)
{
	g_return_val_if_fail(data != NULL, NULL);

	return ((completion_entry *)data)->string;
} 

static void init_all(void)
{
	g_completion = g_completion_new(completion_func);
	g_return_if_fail(g_completion != NULL);
}

static void free_all(void)
{
	GList *walk;
	
	walk = g_list_first(g_completion_list);
	for (; walk != NULL; walk = g_list_next(walk)) {
		completion_entry *ce = (completion_entry *) walk->data;
		g_free(ce->string);
		g_free(walk->data);
	}
	g_list_free(g_completion_list);
	g_completion_list = NULL;
	
	walk = g_address_list;
	for (; walk != NULL; walk = g_list_next(walk)) {
		address_entry *ae = (address_entry *) walk->data;
		g_free(ae->name);
		g_free(ae->address);
		g_free(walk->data);
	}
	g_list_free(g_address_list);
	g_address_list = NULL;
	
	g_completion_free(g_completion);
	g_completion = NULL;
}

static void add_address1(const char *str, address_entry *ae)
{
	completion_entry *ce1;
	ce1 = g_new0(completion_entry, 1),
	ce1->string = g_strdup(str);
	/* GCompletion list is case sensitive */
	g_strdown(ce1->string);
	ce1->ref = ae;

	g_completion_list = g_list_append(g_completion_list, ce1);
}

/* add_address() - adds address to the completion list. this function looks
 * complicated, but it's only allocation checks.
 */
static gint add_address(const gchar *name, const gchar *address, const gchar *alias)
{
	address_entry    *ae;

	if (!name || !address) return -1;

	debug_print( "completion: add_address: %s - %s\n", name, address );

	ae = g_new0(address_entry, 1);

	g_return_val_if_fail(ae != NULL, -1);

	ae->name    = g_strdup(name);
	ae->address = g_strdup(address);		

	g_address_list 	  = g_list_append(g_address_list,    ae);

	add_address1(name, ae);
	add_address1(address, ae);
	add_address1(alias, ae);

	return 0;
}

/* read_address_book()
 */ 
static void read_address_book(void) {	
	addressbook_load_completion( add_address );
}

/* start_address_completion() - returns the number of addresses 
 * that should be matched for completion.
 */
gint start_address_completion(void)
{
	clear_completion_cache();
	if (!g_ref_count) {
		init_all();
		/* open the address book */
		read_address_book();
		/* merge the completion entry list into g_completion */
		if (g_completion_list)
			g_completion_add_items(g_completion, g_completion_list);
	}
	g_ref_count++;
	debug_print("start_address_completion ref count %d\n", g_ref_count);

	return g_list_length(g_completion_list);
}

/* get_address_from_edit() - returns a possible address (or a part)
 * from an entry box. To make life easier, we only look at the last valid address 
 * component; address completion only works at the last string component in
 * the entry box. 
 */ 
gchar *get_address_from_edit(GtkEntry *entry, gint *start_pos)
{
	const gchar *edit_text;
	gint cur_pos;
	wchar_t *wtext;
	wchar_t *wp;
	wchar_t rfc_mail_sep;
	wchar_t quote;
	wchar_t lt;
	wchar_t gt;
	gboolean in_quote = FALSE;
	gboolean in_bracket = FALSE;
	gchar *str;

	if (mbtowc(&rfc_mail_sep, ",", 1) < 0) return NULL;
	if (mbtowc(&quote, "\"", 1) < 0) return NULL;
	if (mbtowc(&lt, "<", 1) < 0) return NULL;
	if (mbtowc(&gt, ">", 1) < 0) return NULL;

	edit_text = gtk_entry_get_text(entry);
	if (edit_text == NULL) return NULL;

	wtext = strdup_mbstowcs(edit_text);
	g_return_val_if_fail(wtext != NULL, NULL);

	cur_pos = gtk_editable_get_position(GTK_EDITABLE(entry));

	/* scan for a separator. doesn't matter if walk points at null byte. */
	for (wp = wtext + cur_pos; wp > wtext; wp--) {
		if (*wp == quote)
			in_quote ^= TRUE;
		else if (!in_quote) {
			if (!in_bracket && *wp == rfc_mail_sep)
				break;
			else if (*wp == gt)
				in_bracket = TRUE;
			else if (*wp == lt)
				in_bracket = FALSE;
		}
	}

	/* have something valid */
	if (wcslen(wp) == 0) {
		g_free(wtext);
		return NULL;
	}

#define IS_VALID_CHAR(x) \
	(iswalnum(x) || (x) == quote || (x) == lt || ((x) > 0x7f))

	/* now scan back until we hit a valid character */
	for (; *wp && !IS_VALID_CHAR(*wp); wp++)
		;

#undef IS_VALID_CHAR

	if (wcslen(wp) == 0) {
		g_free(wtext);
		return NULL;
	}

	if (start_pos) *start_pos = wp - wtext;

	str = strdup_wcstombs(wp);
	g_free(wtext);

	return str;
} 

/* replace_address_in_edit() - replaces an incompleted address with a completed one.
 */
void replace_address_in_edit(GtkEntry *entry, const gchar *newtext,
			     gint start_pos)
{
	if (!newtext) return;
	gtk_editable_delete_text(GTK_EDITABLE(entry), start_pos, -1);
	gtk_editable_insert_text(GTK_EDITABLE(entry), newtext, strlen(newtext),
				 &start_pos);
	gtk_editable_set_position(GTK_EDITABLE(entry), -1);
}

/* complete_address() - tries to complete an addres, and returns the
 * number of addresses found. use get_complete_address() to get one.
 * returns zero if no match was found, otherwise the number of addresses,
 * with the original prefix at index 0. 
 */
guint complete_address(const gchar *str)
{
	GList *result;
	gchar *d;
	guint  count, cpl;
	completion_entry *ce;

	g_return_val_if_fail(str != NULL, 0);

	Xstrdup_a(d, str, return 0);

	clear_completion_cache();
	g_completion_prefix = g_strdup(str);

	/* g_completion is case sensitive */
	g_strdown(d);
	result = g_completion_complete(g_completion, d, NULL);

	count = g_list_length(result);
	if (count) {
		/* create list with unique addresses  */
		for (cpl = 0, result = g_list_first(result);
		     result != NULL;
		     result = g_list_next(result)) {
			ce = (completion_entry *)(result->data);
			if (NULL == g_slist_find(g_completion_addresses,
						 ce->ref)) {
				cpl++;
				g_completion_addresses =
					g_slist_append(g_completion_addresses,
						       ce->ref);
			}
		}
		count = cpl + 1;	/* index 0 is the original prefix */
		g_completion_next = 1;	/* we start at the first completed one */
	} else {
		g_free(g_completion_prefix);
		g_completion_prefix = NULL;
	}

	g_completion_count = count;
	return count;
}

/* get_complete_address() - returns a complete address. the returned
 * string should be freed 
 */
gchar *get_complete_address(gint index)
{
	const address_entry *p;
	gchar *address = NULL;

	if (index < g_completion_count) {
		if (index == 0)
			address = g_strdup(g_completion_prefix);
		else {
			/* get something from the unique addresses */
			p = (address_entry *)g_slist_nth_data
				(g_completion_addresses, index - 1);
			if (p != NULL) {
				if (!p->name || p->name[0] == '\0')
					address = g_strdup_printf(p->address);
				else if (strchr_with_skip_quote(p->name, '"', ','))
					address = g_strdup_printf
						("\"%s\" <%s>", p->name, p->address);
				else
					address = g_strdup_printf
						("%s <%s>", p->name, p->address);
			}
		}
	}

	return address;
}

gchar *get_next_complete_address(void)
{
	if (is_completion_pending()) {
		gchar *res;

		res = get_complete_address(g_completion_next);
		g_completion_next += 1;
		if (g_completion_next >= g_completion_count)
			g_completion_next = 0;

		return res;
	} else
		return NULL;
}

gchar *get_prev_complete_address(void)
{
	if (is_completion_pending()) {
		int n = g_completion_next - 2;

		/* real previous */
		n = (n + (g_completion_count * 5)) % g_completion_count;

		/* real next */
		g_completion_next = n + 1;
		if (g_completion_next >=  g_completion_count)
			g_completion_next = 0;
		return get_complete_address(n);
	} else
		return NULL;
}

guint get_completion_count(void)
{
	if (is_completion_pending())
		return g_completion_count;
	else
		return 0;
}

/* should clear up anything after complete_address() */
void clear_completion_cache(void)
{
	if (is_completion_pending()) {
		if (g_completion_prefix)
			g_free(g_completion_prefix);

		if (g_completion_addresses) {
			g_slist_free(g_completion_addresses);
			g_completion_addresses = NULL;
		}

		g_completion_count = g_completion_next = 0;
	}
}

gboolean is_completion_pending(void)
{
	/* check if completion pending, i.e. we might satisfy a request for the next
	 * or previous address */
	 return g_completion_count;
}

/* invalidate_address_completion() - should be called if address book
 * changed; 
 */
gint invalidate_address_completion(void)
{
	if (g_ref_count) {
		/* simply the same as start_address_completion() */
		debug_print("Invalidation request for address completion\n");
		free_all();
		init_all();
		read_address_book();
		if (g_completion_list)
			g_completion_add_items(g_completion, g_completion_list);
		clear_completion_cache();
	}

	return g_list_length(g_completion_list);
}

gint end_address_completion(void)
{
	clear_completion_cache();

	if (0 == --g_ref_count)
		free_all();

	debug_print("end_address_completion ref count %d\n", g_ref_count);

	return g_ref_count; 
}


/* address completion entry ui. the ui (completion list was inspired by galeon's
 * auto completion list). remaining things powered by sylpheed's completion engine.
 */

#define ENTRY_DATA_TAB_HOOK	"tab_hook"			/* used to lookup entry */
#define WINDOW_DATA_COMPL_ENTRY	"compl_entry"	/* used to store entry for compl. window */
#define WINDOW_DATA_COMPL_CLIST "compl_clist"	/* used to store clist for compl. window */

static void address_completion_mainwindow_set_focus	(GtkWindow   *window,
							 GtkWidget   *widget,
							 gpointer     data);
static gboolean address_completion_entry_key_pressed	(GtkEntry    *entry,
							 GdkEventKey *ev,
							 gpointer     data);
static gboolean address_completion_complete_address_in_entry
							(GtkEntry    *entry,
							 gboolean     next);
static void address_completion_create_completion_window	(GtkEntry    *entry);

static void completion_window_select_row(GtkCList	 *clist,
					 gint		  row,
					 gint		  col,
					 GdkEvent	 *event,
					 GtkWidget	**completion_window);
static gboolean completion_window_button_press
					(GtkWidget	 *widget,
					 GdkEventButton  *event,
					 GtkWidget	**completion_window);
static gboolean completion_window_key_press
					(GtkWidget	 *widget,
					 GdkEventKey	 *event,
					 GtkWidget	**completion_window);


static void completion_window_advance_to_row(GtkCList *clist, gint row)
{
	g_return_if_fail(row < g_completion_count);
	gtk_clist_select_row(clist, row, 0);
}

static void completion_window_advance_selection(GtkCList *clist, gboolean forward)
{
	int row;

	g_return_if_fail(clist != NULL);
	g_return_if_fail(clist->selection != NULL);

	row = GPOINTER_TO_INT(clist->selection->data);

	row = forward ? (row + 1) % g_completion_count :
			(row - 1) < 0 ? g_completion_count - 1 : row - 1;

	gtk_clist_freeze(clist);
	completion_window_advance_to_row(clist, row);					
	gtk_clist_thaw(clist);
}

#if 0
/* completion_window_accept_selection() - accepts the current selection in the
 * clist, and destroys the window */
static void completion_window_accept_selection(GtkWidget **window,
					       GtkCList *clist,
					       GtkEntry *entry)
{
	gchar *address = NULL, *text = NULL;
	gint   cursor_pos, row;

	g_return_if_fail(window != NULL);
	g_return_if_fail(*window != NULL);
	g_return_if_fail(clist != NULL);
	g_return_if_fail(entry != NULL);
	g_return_if_fail(clist->selection != NULL);

	/* FIXME: I believe it's acceptable to access the selection member directly  */
	row = GPOINTER_TO_INT(clist->selection->data);

	/* we just need the cursor position */
	address = get_address_from_edit(entry, &cursor_pos);
	g_free(address);
	gtk_clist_get_text(clist, row, 0, &text);
	replace_address_in_edit(entry, text, cursor_pos);

	clear_completion_cache();
	gtk_widget_destroy(*window);
	*window = NULL;
}
#endif

/* completion_window_apply_selection() - apply the current selection in the
 * clist */
static void completion_window_apply_selection(GtkCList *clist, GtkEntry *entry)
{
	gchar *address = NULL, *text = NULL;
	gint   cursor_pos, row;

	g_return_if_fail(clist != NULL);
	g_return_if_fail(entry != NULL);
	g_return_if_fail(clist->selection != NULL);

	row = GPOINTER_TO_INT(clist->selection->data);

	address = get_address_from_edit(entry, &cursor_pos);
	g_free(address);
	gtk_clist_get_text(clist, row, 0, &text);
	replace_address_in_edit(entry, text, cursor_pos);
}

/* should be called when creating the main window containing address
 * completion entries */
void address_completion_start(GtkWidget *mainwindow)
{
	start_address_completion();

	/* register focus change hook */
	gtk_signal_connect(GTK_OBJECT(mainwindow), "set_focus",
			   GTK_SIGNAL_FUNC(address_completion_mainwindow_set_focus),
			   mainwindow);
}

/* Need unique data to make unregistering signal handler possible for the auto
 * completed entry */
#define COMPLETION_UNIQUE_DATA (GINT_TO_POINTER(0xfeefaa))

void address_completion_register_entry(GtkEntry *entry)
{
	g_return_if_fail(entry != NULL);
	g_return_if_fail(GTK_IS_ENTRY(entry));

	/* add hooked property */
	gtk_object_set_data(GTK_OBJECT(entry), ENTRY_DATA_TAB_HOOK, entry);

	/* add keypress event */
	gtk_signal_connect_full(GTK_OBJECT(entry), "key_press_event",
				GTK_SIGNAL_FUNC(address_completion_entry_key_pressed),
				NULL,
				COMPLETION_UNIQUE_DATA,
				NULL,
				0,
				0); /* magic */
}

void address_completion_unregister_entry(GtkEntry *entry)
{
	GtkObject *entry_obj;

	g_return_if_fail(entry != NULL);
	g_return_if_fail(GTK_IS_ENTRY(entry));

	entry_obj = gtk_object_get_data(GTK_OBJECT(entry), ENTRY_DATA_TAB_HOOK);
	g_return_if_fail(entry_obj);
	g_return_if_fail(entry_obj == GTK_OBJECT(entry));

	/* has the hooked property? */
	gtk_object_set_data(GTK_OBJECT(entry), ENTRY_DATA_TAB_HOOK, NULL);

	/* remove the hook */
	gtk_signal_disconnect_by_func(GTK_OBJECT(entry), 
		GTK_SIGNAL_FUNC(address_completion_entry_key_pressed),
		COMPLETION_UNIQUE_DATA);
}

/* should be called when main window with address completion entries
 * terminates.
 * NOTE: this function assumes that it is called upon destruction of
 * the window */
void address_completion_end(GtkWidget *mainwindow)
{
	/* if address_completion_end() is really called on closing the window,
	 * we don't need to unregister the set_focus_cb */
	end_address_completion();
}

/* if focus changes to another entry, then clear completion cache */
static void address_completion_mainwindow_set_focus(GtkWindow *window,
						    GtkWidget *widget,
						    gpointer   data)
{
	if (widget)
		clear_completion_cache();
}

/* watch for tabs in one of the address entries. if no tab then clear the
 * completion cache */
static gboolean address_completion_entry_key_pressed(GtkEntry    *entry,
						     GdkEventKey *ev,
						     gpointer     data)
{
	if (ev->keyval == GDK_Tab) {
		if (address_completion_complete_address_in_entry(entry, TRUE)) {
			address_completion_create_completion_window(entry);
			/* route a void character to the default handler */
			/* this is a dirty hack; we're actually changing a key
			 * reported by the system. */
			ev->keyval = GDK_AudibleBell_Enable;
			ev->state &= ~GDK_SHIFT_MASK;
			gtk_signal_emit_stop_by_name(GTK_OBJECT(entry),
						     "key_press_event");
		} else {
			/* old behaviour */
		}
	} else if (ev->keyval == GDK_Shift_L
		|| ev->keyval == GDK_Shift_R
		|| ev->keyval == GDK_Control_L
		|| ev->keyval == GDK_Control_R
		|| ev->keyval == GDK_Caps_Lock
		|| ev->keyval == GDK_Shift_Lock
		|| ev->keyval == GDK_Meta_L
		|| ev->keyval == GDK_Meta_R
		|| ev->keyval == GDK_Alt_L
		|| ev->keyval == GDK_Alt_R) {
		/* these buttons should not clear the cache... */
	} else
		clear_completion_cache();

	return TRUE;
}

/* initialize the completion cache and put first completed string
 * in entry. this function used to do back cycling but this is not
 * currently used. since the address completion behaviour has been
 * changed regularly, we keep the feature in case someone changes
 * his / her mind again. :) */
static gboolean address_completion_complete_address_in_entry(GtkEntry *entry,
							     gboolean  next)
{
	gint ncount, cursor_pos;
	gchar *address, *new = NULL;
	gboolean completed = FALSE;

	g_return_val_if_fail(entry != NULL, FALSE);

	if (!GTK_WIDGET_HAS_FOCUS(entry)) return FALSE;

	/* get an address component from the cursor */
	address = get_address_from_edit(entry, &cursor_pos);
	if (!address) return FALSE;

	/* still something in the cache */
	if (is_completion_pending()) {
		new = next ? get_next_complete_address() :
			get_prev_complete_address();
	} else {
		if (0 < (ncount = complete_address(address)))
			new = get_next_complete_address();
	}

	if (new) {
		/* prevent "change" signal */
		/* replace_address_in_edit(entry, new, cursor_pos); */
		g_free(new);
		completed = TRUE;
	}

	g_free(address);

	return completed;
}

static void address_completion_create_completion_window(GtkEntry *entry_)
{
	static GtkWidget *completion_window;
	gint x, y, height, width, depth;
	GtkWidget *scroll, *clist;
	GtkRequisition r;
	guint count = 0;
	GtkWidget *entry = GTK_WIDGET(entry_);

	if (completion_window) {
		gtk_widget_destroy(completion_window);
		completion_window = NULL;
	}

	scroll = gtk_scrolled_window_new(NULL, NULL);
	clist  = gtk_clist_new(1);
	gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_SINGLE);
	
	completion_window = gtk_window_new(GTK_WINDOW_POPUP);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(completion_window), scroll);
	gtk_container_add(GTK_CONTAINER(scroll), clist);

	/* set the unique data so we can always get back the entry and
	 * clist window to which this completion window has been attached */
	gtk_object_set_data(GTK_OBJECT(completion_window),
			    WINDOW_DATA_COMPL_ENTRY, entry_);
	gtk_object_set_data(GTK_OBJECT(completion_window),
			    WINDOW_DATA_COMPL_CLIST, clist);

	gtk_signal_connect(GTK_OBJECT(clist), "select_row",
			   GTK_SIGNAL_FUNC(completion_window_select_row),
			   &completion_window);

	for (count = 0; count < get_completion_count(); count++) {
		gchar *text[] = {NULL, NULL};

		text[0] = get_complete_address(count);
		gtk_clist_append(GTK_CLIST(clist), text);
		g_free(text[0]);
	}

	gdk_window_get_geometry(entry->window, &x, &y, &width, &height, &depth);
	gdk_window_get_deskrelative_origin (entry->window, &x, &y);
	y += height;
	gtk_widget_set_uposition(completion_window, x, y);

	gtk_widget_size_request(clist, &r);
	gtk_widget_set_usize(completion_window, width, r.height);
	gtk_widget_show_all(completion_window);
	gtk_widget_size_request(clist, &r);

	if ((y + r.height) > gdk_screen_height()) {
		gtk_window_set_policy(GTK_WINDOW(completion_window),
				      TRUE, FALSE, FALSE);
		gtk_widget_set_usize(completion_window, width,
				     gdk_screen_height () - y);
	}

	gtk_signal_connect(GTK_OBJECT(completion_window),
			   "button-press-event",
			   GTK_SIGNAL_FUNC(completion_window_button_press),
			   &completion_window);
	gtk_signal_connect(GTK_OBJECT(completion_window),
			   "key-press-event",
			   GTK_SIGNAL_FUNC(completion_window_key_press),
			   &completion_window);
	gdk_pointer_grab(completion_window->window, TRUE,
			 GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
			 GDK_BUTTON_RELEASE_MASK,
			 NULL, NULL, GDK_CURRENT_TIME);
	gtk_grab_add(completion_window);

	/* this gets rid of the irritating focus rectangle that doesn't
	 * follow the selection */
	GTK_WIDGET_UNSET_FLAGS(clist, GTK_CAN_FOCUS);
	gtk_clist_select_row(GTK_CLIST(clist), 1, 0);
}


/* row selection sends completed address to entry.
 * note: event is NULL if selected by anything else than a mouse button. */
static void completion_window_select_row(GtkCList *clist, gint row, gint col,
					 GdkEvent *event,
					 GtkWidget **completion_window)
{
	GtkEntry *entry;

	g_return_if_fail(completion_window != NULL);
	g_return_if_fail(*completion_window != NULL);

	entry = GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(*completion_window),
					      WINDOW_DATA_COMPL_ENTRY));
	g_return_if_fail(entry != NULL);

	completion_window_apply_selection(clist, entry);

	if (!event || event->type != GDK_BUTTON_RELEASE)
		return;

	clear_completion_cache();
	gtk_widget_destroy(*completion_window);
	*completion_window = NULL;
}

/* completion_window_button_press() - check is mouse click is anywhere
 * else (not in the completion window). in that case the completion
 * window is destroyed, and the original prefix is restored */
static gboolean completion_window_button_press(GtkWidget *widget,
					       GdkEventButton *event,
					       GtkWidget **completion_window)
{
	GtkWidget *event_widget, *entry;
	gchar *prefix;
	gint cursor_pos;
	gboolean restore = TRUE;

	g_return_val_if_fail(completion_window != NULL, FALSE);
	g_return_val_if_fail(*completion_window != NULL, FALSE);

	entry = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(*completion_window),
					       WINDOW_DATA_COMPL_ENTRY));
	g_return_val_if_fail(entry != NULL, FALSE);

	event_widget = gtk_get_event_widget((GdkEvent *)event);
	if (event_widget != widget) {
		while (event_widget) {
			if (event_widget == widget)
				return FALSE;
			else if (event_widget == entry) {
				restore = FALSE;
				break;
			}
		    event_widget = event_widget->parent;
		}
	}

	if (restore) {
		prefix = get_complete_address(0);
		g_free(get_address_from_edit(GTK_ENTRY(entry), &cursor_pos));
		replace_address_in_edit(GTK_ENTRY(entry), prefix, cursor_pos);
		g_free(prefix);
	}

	clear_completion_cache();
	gtk_widget_destroy(*completion_window);
	*completion_window = NULL;

	return TRUE;
}

static gboolean completion_window_key_press(GtkWidget *widget,
					    GdkEventKey *event,
					    GtkWidget **completion_window)
{
	GdkEventKey tmp_event;
	GtkWidget *entry;
	gchar *prefix;
	gint cursor_pos;
	GtkWidget *clist;

	g_return_val_if_fail(completion_window != NULL, FALSE);
	g_return_val_if_fail(*completion_window != NULL, FALSE);

	entry = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(*completion_window),
					       WINDOW_DATA_COMPL_ENTRY));
	clist = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(*completion_window),
					       WINDOW_DATA_COMPL_CLIST));
	g_return_val_if_fail(entry != NULL, FALSE);

	/* allow keyboard navigation in the alternatives clist */
	if (event->keyval == GDK_Up || event->keyval == GDK_Down ||
	    event->keyval == GDK_Page_Up || event->keyval == GDK_Page_Down) {
		completion_window_advance_selection
			(GTK_CLIST(clist),
			 event->keyval == GDK_Down ||
			 event->keyval == GDK_Page_Down ? TRUE : FALSE);
		return FALSE;
	}		

	/* also make tab / shift tab go to next previous completion entry. we're
	 * changing the key value */
	if (event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab) {
		event->keyval = (event->state & GDK_SHIFT_MASK)
			? GDK_Up : GDK_Down;
		/* need to reset shift state if going up */
		if (event->state & GDK_SHIFT_MASK)
			event->state &= ~GDK_SHIFT_MASK;
		completion_window_advance_selection(GTK_CLIST(clist), 
			event->keyval == GDK_Down ? TRUE : FALSE);
		return FALSE;
	}

	/* look for presses that accept the selection */
	if (event->keyval == GDK_Return || event->keyval == GDK_space) {
		clear_completion_cache();
		gtk_widget_destroy(*completion_window);
		*completion_window = NULL;
		return FALSE;
	}

	/* key state keys should never be handled */
	if (event->keyval == GDK_Shift_L
		 || event->keyval == GDK_Shift_R
		 || event->keyval == GDK_Control_L
		 || event->keyval == GDK_Control_R
		 || event->keyval == GDK_Caps_Lock
		 || event->keyval == GDK_Shift_Lock
		 || event->keyval == GDK_Meta_L
		 || event->keyval == GDK_Meta_R
		 || event->keyval == GDK_Alt_L
		 || event->keyval == GDK_Alt_R) {
		return FALSE;
	}

	/* other key, let's restore the prefix (orignal text) */
	prefix = get_complete_address(0);
	g_free(get_address_from_edit(GTK_ENTRY(entry), &cursor_pos));
	replace_address_in_edit(GTK_ENTRY(entry), prefix, cursor_pos);
	g_free(prefix);
	clear_completion_cache();

	/* make sure anything we typed comes in the edit box */
	tmp_event.type       = event->type;
	tmp_event.window     = entry->window;
	tmp_event.send_event = TRUE;
	tmp_event.time       = event->time;
	tmp_event.state      = event->state;
	tmp_event.keyval     = event->keyval;
	tmp_event.length     = event->length;
	tmp_event.string     = event->string;
	gtk_widget_event(entry, (GdkEvent *)&tmp_event);

	/* and close the completion window */
	gtk_widget_destroy(*completion_window);
	*completion_window = NULL;

	return TRUE;
}
