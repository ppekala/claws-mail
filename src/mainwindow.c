/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkstatusbar.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkvpaned.h>
#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkitemfactory.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkhandlebox.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktooltips.h>
#include <string.h>

#include "main.h"
#include "mainwindow.h"
#include "folderview.h"
#include "foldersel.h"
#include "summaryview.h"
#include "summary_search.h"
#include "messageview.h"
#include "message_search.h"
#include "headerview.h"
#include "menu.h"
#include "stock_pixmap.h"
#include "folder.h"
#include "inc.h"
#include "compose.h"
#include "procmsg.h"
#include "import.h"
#include "export.h"
#include "prefs_common.h"
#include "prefs_actions.h"
#include "prefs_filtering.h"
#include "prefs_account.h"
#include "prefs_summary_column.h"
#include "prefs_folder_column.h"
#include "prefs_template.h"
#include "action.h"
#include "account.h"
#include "addressbook.h"
#include "logwindow.h"
#include "manage_window.h"
#include "alertpanel.h"
#include "statusbar.h"
#include "inputdialog.h"
#include "utils.h"
#include "gtkutils.h"
#include "codeconv.h"
#include "about.h"
#include "manual.h"
#include "version.h"
#include "ssl_manager.h"
#include "sslcertwindow.h"
#include "prefs_gtk.h"
#include "pluginwindow.h"
#include "hooks.h"
#include "progressindicator.h"
#include "localfolder.h"
#include "filtering.h"
#include "folderutils.h"
#include "foldersort.h"

#define AC_LABEL_WIDTH	240

/* list of all instantiated MainWindow */
static GList *mainwin_list = NULL;

static GdkCursor *watch_cursor;

static void main_window_menu_callback_block	(MainWindow	*mainwin);
static void main_window_menu_callback_unblock	(MainWindow	*mainwin);

static void main_window_show_cur_account	(MainWindow	*mainwin);

static void main_window_set_widgets		(MainWindow	*mainwin,
						 SeparateType	 type);

#if 0
static gboolean toolbar_account_button_pressed	(GtkWidget	*widget,
						 GdkEventButton	*event,
						 gpointer	 data);
#endif

static void toolbar_child_attached		(GtkWidget	*widget,
						 GtkWidget	*child,
						 gpointer	 data);
static void toolbar_child_detached		(GtkWidget	*widget,
						 GtkWidget	*child,
						 gpointer	 data);

static gboolean ac_label_button_pressed		(GtkWidget	*widget,
						 GdkEventButton	*event,
						 gpointer	 data);

static gint main_window_close_cb		(GtkWidget	*widget,
						 GdkEventAny	*event,
						 gpointer	 data);
static gint folder_window_close_cb		(GtkWidget	*widget,
						 GdkEventAny	*event,
						 gpointer	 data);
static gint message_window_close_cb		(GtkWidget	*widget,
						 GdkEventAny	*event,
						 gpointer	 data);

static void main_window_size_allocate_cb	(GtkWidget	*widget,
						 GtkAllocation	*allocation,
						 gpointer	 data);
static void folder_window_size_allocate_cb	(GtkWidget	*widget,
						 GtkAllocation	*allocation,
						 gpointer	 data);
static void message_window_size_allocate_cb	(GtkWidget	*widget,
						 GtkAllocation	*allocation,
						 gpointer	 data);

static void update_folderview_cb (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void add_mailbox_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void foldersort_cb	 (MainWindow 	*mainwin,
				  guint		 action,
                        	  GtkWidget 	*widget);
static void import_mbox_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void export_mbox_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void export_list_mbox_cb  (MainWindow 	*mainwin, 
				  guint 	 action,
				  GtkWidget 	*widget);
static void empty_trash_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void save_as_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void print_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void app_exit_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void search_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void toggle_folder_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void toggle_message_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void toggle_toolbar_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void toggle_statusbar_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void separate_widget_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void addressbook_open_cb	(MainWindow	*mainwin,
				 guint		 action,
				 GtkWidget	*widget);
static void log_window_show_cb	(MainWindow	*mainwin,
				 guint		 action,
				 GtkWidget	*widget);

static void inc_cancel_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void open_msg_cb			(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void view_source_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void show_all_header_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void move_to_cb			(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);
static void copy_to_cb			(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);
static void delete_cb			(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);
static void delete_trash_cb			(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void cancel_cb                   (MainWindow     *mainwin,
					 guint           action,
					 GtkWidget      *widget);

static void mark_cb			(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);
static void unmark_cb			(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void mark_as_unread_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);
static void mark_as_read_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);
static void mark_all_read_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void reedit_cb			(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void add_address_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void set_charset_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void set_decode_cb		(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void hide_read_messages   (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void thread_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void expand_threads_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void collapse_threads_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void set_summary_display_item_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void set_folder_display_item_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void sort_summary_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void sort_summary_type_cb (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void attract_by_subject_cb(MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void delete_duplicated_cb (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void delete_duplicated_all_cb (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void filter_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void execute_summary_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void update_summary_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void prev_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void next_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void next_unread_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void prev_unread_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void prev_new_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void next_new_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void prev_marked_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void next_marked_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void prev_labeled_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void next_labeled_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void goto_folder_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void copy_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void allsel_cb		 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void select_thread_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void create_filter_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);
static void create_processing_cb (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void prefs_template_open_cb	(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);
static void prefs_actions_open_cb	(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);
static void prefs_account_open_cb	(MainWindow	*mainwin,
					 guint		 action,
					 GtkWidget	*widget);

static void prefs_pre_processing_open_cb  (MainWindow	*mainwin,
				  	   guint	 action,
				  	   GtkWidget	*widget);

static void prefs_post_processing_open_cb (MainWindow	*mainwin,
				  	   guint	 action,
				  	   GtkWidget	*widget);

static void prefs_filtering_open_cb 	(MainWindow	*mainwin,
				  	 guint		 action,
				  	 GtkWidget	*widget);
#ifdef USE_OPENSSL
static void ssl_manager_open_cb 	(MainWindow	*mainwin,
				  	 guint		 action,
				  	 GtkWidget	*widget);
#endif
static void new_account_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void account_selector_menu_cb	 (GtkMenuItem	*menuitem,
					  gpointer	 data);
static void account_receive_menu_cb	 (GtkMenuItem	*menuitem,
					  gpointer	 data);

static void prefs_open_cb	(GtkMenuItem	*menuitem,
				 gpointer 	 data);
static void plugins_open_cb	(GtkMenuItem	*menuitem,
				 gpointer 	 data);

static void online_switch_clicked(GtkButton     *btn, 
				  gpointer data);

static void manual_open_cb	 (MainWindow	*mainwin,
				  guint		 action,
				  GtkWidget	*widget);

static void scan_tree_func	 (Folder	*folder,
				  FolderItem	*item,
				  gpointer	 data);
				  
static void toggle_work_offline_cb(MainWindow *mainwin, guint action, GtkWidget *widget);

static void addr_harvest_cb	 ( MainWindow  *mainwin,
				   guint       action,
				   GtkWidget   *widget );

static void addr_harvest_msg_cb	 ( MainWindow  *mainwin,
				   guint       action,
				   GtkWidget   *widget );
static void sync_cb		 ( MainWindow *mainwin, 
				   guint action, 
				   GtkWidget *widget );

static gboolean mainwindow_focus_in_event	(GtkWidget	*widget, 
						 GdkEventFocus	*focus,
						 gpointer	 data);
void main_window_reply_cb			(MainWindow 	*mainwin, 
						 guint 		 action,
						 GtkWidget 	*widget);
gboolean mainwindow_progressindicator_hook	(gpointer 	 source,
						 gpointer 	 userdata);
#define  SEPARATE_ACTION 500 
static void mainwindow_quicksearch		(MainWindow 	*mainwin, 
						 guint 		 action, 
						 GtkWidget 	*widget);

static GtkItemFactoryEntry mainwin_entries[] =
{
	{N_("/_File"),				NULL, NULL, 0, "<Branch>"},
	{N_("/_File/_Add mailbox"),		NULL, NULL, 0, "<Branch>"},
	{N_("/_File/_Add mailbox/MH..."),	NULL, add_mailbox_cb, 0, NULL},
	{N_("/_File/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_File/Change folder order"),	NULL, foldersort_cb,  0, NULL},
	{N_("/_File/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_File/_Import mbox file..."),	NULL, import_mbox_cb, 0, NULL},
	{N_("/_File/_Export to mbox file..."),	NULL, export_mbox_cb, 0, NULL},
	{N_("/_File/Exp_ort selected to mbox file..."),	
						NULL, export_list_mbox_cb, 0, NULL},
	{N_("/_File/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_File/Empty all _Trash folders"),	"<shift>D", empty_trash_cb, 0, NULL},
	{N_("/_File/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_File/_Save as..."),		"<control>S", save_as_cb, 0, NULL},
	{N_("/_File/_Print..."),		"<control>P", print_cb, 0, NULL},
	{N_("/_File/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_File/_Work offline"),		"<control>W", toggle_work_offline_cb, 0, "<ToggleItem>"},
	{N_("/_File/Synchronise folders"),   	"<control><shift>S", sync_cb, 0, NULL},
	{N_("/_File/---"),			NULL, NULL, 0, "<Separator>"},
	/* {N_("/_File/_Close"),		"<alt>W", app_exit_cb, 0, NULL}, */
	{N_("/_File/E_xit"),			"<control>Q", app_exit_cb, 0, NULL},

	{N_("/_Edit"),				NULL, NULL, 0, "<Branch>"},
	{N_("/_Edit/_Copy"),			"<control>C", copy_cb, 0, NULL},
	{N_("/_Edit/Select _all"),		"<control>A", allsel_cb, 0, NULL},
	{N_("/_Edit/Select _thread"),		NULL, select_thread_cb, 0, NULL},
	{N_("/_Edit/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Edit/_Find in current message..."),
						"<control>F", search_cb, 0, NULL},
	{N_("/_Edit/_Search folder..."),	"<shift><control>F", search_cb, 1, NULL},
	{N_("/_Edit/_Quick search"),		"slash", mainwindow_quicksearch, 0, NULL},
	{N_("/_View"),				NULL, NULL, 0, "<Branch>"},
	{N_("/_View/Show or hi_de"),		NULL, NULL, 0, "<Branch>"},
	{N_("/_View/Show or hi_de/_Folder tree"),
						NULL, toggle_folder_cb, 0, "<ToggleItem>"},
	{N_("/_View/Show or hi_de/_Message view"),
						"V", toggle_message_cb, 0, "<ToggleItem>"},
	{N_("/_View/Show or hi_de/_Toolbar"),
						NULL, NULL, 0, "<Branch>"},
	{N_("/_View/Show or hi_de/_Toolbar/Icon _and text"),
						NULL, toggle_toolbar_cb, TOOLBAR_BOTH, "<RadioItem>"},
	{N_("/_View/Show or hi_de/_Toolbar/_Icon"),
						NULL, toggle_toolbar_cb, TOOLBAR_ICON, "/View/Show or hide/Toolbar/Icon and text"},
	{N_("/_View/Show or hi_de/_Toolbar/_Text"),
						NULL, toggle_toolbar_cb, TOOLBAR_TEXT, "/View/Show or hide/Toolbar/Icon and text"},
	{N_("/_View/Show or hi_de/_Toolbar/_None"),
						NULL, toggle_toolbar_cb, TOOLBAR_NONE, "/View/Show or hide/Toolbar/Icon and text"},
	{N_("/_View/Show or hi_de/Status _bar"),
						NULL, toggle_statusbar_cb, 0, "<ToggleItem>"},
	{N_("/_View/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_View/Separate f_older tree"),	NULL, separate_widget_cb, SEPARATE_FOLDER, "<ToggleItem>"},
	{N_("/_View/Separate _message view"),	NULL, separate_widget_cb, SEPARATE_MESSAGE, "<ToggleItem>"},
	{N_("/_View/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Sort"),			NULL, NULL, 0, "<Branch>"},
	{N_("/_View/_Sort/by _number"),		NULL, sort_summary_cb, SORT_BY_NUMBER, "<RadioItem>"},
	{N_("/_View/_Sort/by S_ize"),		NULL, sort_summary_cb, SORT_BY_SIZE, "/View/Sort/by number"},
	{N_("/_View/_Sort/by _Date"),		NULL, sort_summary_cb, SORT_BY_DATE, "/View/Sort/by number"},
	{N_("/_View/_Sort/by _From"),		NULL, sort_summary_cb, SORT_BY_FROM, "/View/Sort/by number"},
	{N_("/_View/_Sort/by _To"),		NULL, sort_summary_cb, SORT_BY_TO, "/View/Sort/by number"},
	{N_("/_View/_Sort/by S_ubject"),	NULL, sort_summary_cb, SORT_BY_SUBJECT, "/View/Sort/by number"},
	{N_("/_View/_Sort/by _color label"),
						NULL, sort_summary_cb, SORT_BY_LABEL, "/View/Sort/by number"},
	{N_("/_View/_Sort/by _mark"),		NULL, sort_summary_cb, SORT_BY_MARK, "/View/Sort/by number"},
	{N_("/_View/_Sort/by _status"),		NULL, sort_summary_cb, SORT_BY_STATUS, "/View/Sort/by number"},
	{N_("/_View/_Sort/by a_ttachment"),
						NULL, sort_summary_cb, SORT_BY_MIME, "/View/Sort/by number"},
	{N_("/_View/_Sort/by score"),		NULL, sort_summary_cb, SORT_BY_SCORE, "/View/Sort/by number"},
	{N_("/_View/_Sort/by locked"),		NULL, sort_summary_cb, SORT_BY_LOCKED, "/View/Sort/by number"},
	{N_("/_View/_Sort/D_on't sort"),	NULL, sort_summary_cb, SORT_BY_NONE, "/View/Sort/by number"},
	{N_("/_View/_Sort/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Sort/Ascending"),		NULL, sort_summary_type_cb, SORT_ASCENDING, "<RadioItem>"},
	{N_("/_View/_Sort/Descending"),		NULL, sort_summary_type_cb, SORT_DESCENDING, "/View/Sort/Ascending"},
	{N_("/_View/_Sort/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Sort/_Attract by subject"),
						NULL, attract_by_subject_cb, 0, NULL},
	{N_("/_View/Th_read view"),		"<control>T", thread_cb, 0, "<ToggleItem>"},
	{N_("/_View/E_xpand all threads"),	NULL, expand_threads_cb, 0, NULL},
	{N_("/_View/Co_llapse all threads"),	NULL, collapse_threads_cb, 0, NULL},
	{N_("/_View/_Hide read messages"),	NULL, hide_read_messages, 0, "<ToggleItem>"},
	{N_("/_View/Set displayed _columns"),	NULL, NULL, 0, "<Branch>"},
	{N_("/_View/Set displayed _columns/in _Folder list..."),	NULL, set_folder_display_item_cb, 0, NULL},
	{N_("/_View/Set displayed _columns/in _Message list..."),NULL, set_summary_display_item_cb, 0, NULL},

	{N_("/_View/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Go to"),			NULL, NULL, 0, "<Branch>"},
	{N_("/_View/_Go to/_Prev message"),	"P", prev_cb, 0, NULL},
	{N_("/_View/_Go to/_Next message"),	"N", next_cb, 0, NULL},
	{N_("/_View/_Go to/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Go to/P_rev unread message"),
						"<shift>P", prev_unread_cb, 0, NULL},
	{N_("/_View/_Go to/N_ext unread message"),
						"<shift>N", next_unread_cb, 0, NULL},
	{N_("/_View/_Go to/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Go to/Prev ne_w message"),	NULL, prev_new_cb, 0, NULL},
	{N_("/_View/_Go to/Ne_xt new message"),	NULL, next_new_cb, 0, NULL},
	{N_("/_View/_Go to/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Go to/Prev _marked message"),
						NULL, prev_marked_cb, 0, NULL},
	{N_("/_View/_Go to/Next m_arked message"),
						NULL, next_marked_cb, 0, NULL},
	{N_("/_View/_Go to/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Go to/Prev _labeled message"),
						NULL, prev_labeled_cb, 0, NULL},
	{N_("/_View/_Go to/Next la_beled message"),
						NULL, next_labeled_cb, 0, NULL},
	{N_("/_View/_Go to/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Go to/Other _folder..."),	"G", goto_folder_cb, 0, NULL},
	{N_("/_View/---"),			NULL, NULL, 0, "<Separator>"},

#define ENC_SEPARATOR \
	{N_("/_View/Character _encoding/---"),		NULL, NULL, 0, "<Separator>"}
#define ENC_ACTION(action) \
	 NULL, set_charset_cb, action, "/View/Character encoding/Auto detect"

	{N_("/_View/Character _encoding"),		NULL, NULL, 0, "<Branch>"},
	{N_("/_View/Character _encoding/_Auto detect"),
	 NULL, set_charset_cb, C_AUTO, "<RadioItem>"},
	{N_("/_View/Character _encoding/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/Character _encoding/7bit ascii (US-ASC_II)"),
	 ENC_ACTION(C_US_ASCII)},
	{N_("/_View/Character _encoding/Unicode (_UTF-8)"),
	 ENC_ACTION(C_UTF_8)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Western European (ISO-8859-_1)"),
	 ENC_ACTION(C_ISO_8859_1)},
	{N_("/_View/Character _encoding/Western European (ISO-8859-15)"),
	 ENC_ACTION(C_ISO_8859_15)},
	{N_("/_View/Character _encoding/Western European (Windows-1252)"),
	 ENC_ACTION(C_WINDOWS_1252)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Central European (ISO-8859-_2)"),
	 ENC_ACTION(C_ISO_8859_2)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/_Baltic (ISO-8859-13)"),
	 ENC_ACTION(C_ISO_8859_13)},
	{N_("/_View/Character _encoding/Baltic (ISO-8859-_4)"),
	 ENC_ACTION(C_ISO_8859_4)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Greek (ISO-8859-_7)"),
	 ENC_ACTION(C_ISO_8859_7)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Hebrew (ISO-8859-_8)"),
	 ENC_ACTION(C_ISO_8859_8)},
	{N_("/_View/Character _encoding/Hebrew (Windows-1255)"),
	 ENC_ACTION(C_CP1255)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Turkish (ISO-8859-_9)"),
	 ENC_ACTION(C_ISO_8859_9)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Cyrillic (ISO-8859-_5)"),
	 ENC_ACTION(C_ISO_8859_5)},
	{N_("/_View/Character _encoding/Cyrillic (KOI8-_R)"),
	 ENC_ACTION(C_KOI8_R)},
	{N_("/_View/Character _encoding/Cyrillic (KOI8-U)"),
	 ENC_ACTION(C_KOI8_U)},
	{N_("/_View/Character _encoding/Cyrillic (Windows-1251)"),
	 ENC_ACTION(C_CP1251)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Japanese (ISO-2022-_JP)"),
	 ENC_ACTION(C_ISO_2022_JP)},
	{N_("/_View/Character _encoding/Japanese (ISO-2022-JP-2)"),
	 ENC_ACTION(C_ISO_2022_JP_2)},
	{N_("/_View/Character _encoding/Japanese (_EUC-JP)"),
	 ENC_ACTION(C_EUC_JP)},
	{N_("/_View/Character _encoding/Japanese (_Shift__JIS)"),
	 ENC_ACTION(C_SHIFT_JIS)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Simplified Chinese (_GB2312)"),
	 ENC_ACTION(C_GB2312)},
	{N_("/_View/Character _encoding/Simplified Chinese (GBK)"),
	 ENC_ACTION(C_GBK)},
	{N_("/_View/Character _encoding/Traditional Chinese (_Big5)"),
	 ENC_ACTION(C_BIG5)},
	{N_("/_View/Character _encoding/Traditional Chinese (EUC-_TW)"),
	 ENC_ACTION(C_EUC_TW)},
	{N_("/_View/Character _encoding/Chinese (ISO-2022-_CN)"),
	 ENC_ACTION(C_ISO_2022_CN)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Korean (EUC-_KR)"),
	 ENC_ACTION(C_EUC_KR)},
	{N_("/_View/Character _encoding/Korean (ISO-2022-KR)"),
	 ENC_ACTION(C_ISO_2022_KR)},
	ENC_SEPARATOR,

	{N_("/_View/Character _encoding/Thai (TIS-620)"),
	 ENC_ACTION(C_TIS_620)},
	{N_("/_View/Character _encoding/Thai (Windows-874)"),
	 ENC_ACTION(C_WINDOWS_874)},

#undef ENC_SEPARATOR
#undef ENC_ACTION

#define DEC_SEPARATOR \
	{N_("/_View/Decode/---"),		NULL, NULL, 0, "<Separator>"}
#define DEC_ACTION(action) \
	 NULL, set_decode_cb, action, "/View/Decode/Auto detect"
	{N_("/_View/Decode"),		NULL, NULL, 0, "<Branch>"},
	{N_("/_View/Decode/_Auto detect"),
	 NULL, set_decode_cb, 0, "<RadioItem>"},
	{N_("/_View/Decode/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_View/Decode/_8bit"), 		DEC_ACTION(ENC_8BIT)},
	{N_("/_View/Decode/_Quoted printable"),	DEC_ACTION(ENC_QUOTED_PRINTABLE)},
	{N_("/_View/Decode/_Base64"), 		DEC_ACTION(ENC_BASE64)},
	{N_("/_View/Decode/_Uuencode"),		DEC_ACTION(ENC_X_UUENCODE)},

#undef DEC_SEPARATOR
#undef DEC_ACTION

	{N_("/_View/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_View/Open in new _window"),	"<control><alt>N", open_msg_cb, 0, NULL},
	{N_("/_View/Mess_age source"),		"<control>U", view_source_cb, 0, NULL},
	{N_("/_View/Show all headers"),		"<control>H", show_all_header_cb, 0, "<ToggleItem>"},
	{N_("/_View/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_View/_Update summary"),		"<control><alt>U", update_summary_cb,  0, NULL},

	{N_("/_Message"),			NULL, NULL, 0, "<Branch>"},
	{N_("/_Message/Recei_ve"),		NULL, NULL, 0, "<Branch>"},
	{N_("/_Message/Recei_ve/Get from _current account"),
						"<control>I",	inc_mail_cb, 0, NULL},
	{N_("/_Message/Recei_ve/Get from _all accounts"),
						"<shift><control>I", inc_all_account_mail_cb, 0, NULL},
	{N_("/_Message/Recei_ve/Cancel receivin_g"),
						NULL, inc_cancel_cb, 0, NULL},
	{N_("/_Message/Recei_ve/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_Message/_Send queued messages"), NULL, send_queue_cb, 0, NULL},
	{N_("/_Message/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Message/Compose a_n email message"),	"<control>M", compose_mail_cb, 0, NULL},
	{N_("/_Message/Compose a news message"),	NULL,	compose_news_cb, 0, NULL},
	{N_("/_Message/_Reply"),		"<control>R", 	main_window_reply_cb, COMPOSE_REPLY, NULL},
	{N_("/_Message/Repl_y to"),		NULL, NULL, 0, "<Branch>"},
	{N_("/_Message/Repl_y to/_all"),	"<shift><control>R", main_window_reply_cb, COMPOSE_REPLY_TO_ALL, NULL},
	{N_("/_Message/Repl_y to/_sender"),	NULL, main_window_reply_cb, COMPOSE_REPLY_TO_SENDER, NULL},
	{N_("/_Message/Repl_y to/mailing _list"),
						"<control>L", main_window_reply_cb, COMPOSE_REPLY_TO_LIST, NULL},
	{N_("/_Message/Follow-up and reply to"),NULL, main_window_reply_cb, COMPOSE_FOLLOWUP_AND_REPLY_TO, NULL},
	{N_("/_Message/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Message/_Forward"),		"<control><alt>F", main_window_reply_cb, COMPOSE_FORWARD_INLINE, NULL},
	{N_("/_Message/For_ward as attachment"),	NULL, main_window_reply_cb, COMPOSE_FORWARD_AS_ATTACH, NULL},
	{N_("/_Message/Redirect"),		NULL, main_window_reply_cb, COMPOSE_REDIRECT, NULL},
	{N_("/_Message/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Message/M_ove..."),		"<control>O", move_to_cb, 0, NULL},
	{N_("/_Message/_Copy..."),		"<shift><control>O", copy_to_cb, 0, NULL},
	{N_("/_Message/Move to _trash"),	"<control>D", delete_trash_cb,  0, NULL},
	{N_("/_Message/_Delete..."),		NULL, delete_cb,  0, NULL},
	{N_("/_Message/Cancel a news message"), "", cancel_cb,  0, NULL},
	{N_("/_Message/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Message/_Mark"),			NULL, NULL, 0, "<Branch>"},
	{N_("/_Message/_Mark/_Mark"),		"<shift>asterisk", mark_cb, 0, NULL},
	{N_("/_Message/_Mark/_Unmark"),		"U", unmark_cb, 0, NULL},
	{N_("/_Message/_Mark/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_Message/_Mark/Mark as unr_ead"),	"<shift>exclam", mark_as_unread_cb, 0, NULL},
	{N_("/_Message/_Mark/Mark as rea_d"),
						NULL, mark_as_read_cb, 0, NULL},
	{N_("/_Message/_Mark/Mark all _read"),	NULL, mark_all_read_cb, 0, NULL},
	{N_("/_Message/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Message/Re-_edit"),		NULL, reedit_cb, 0, NULL},

	{N_("/_Tools"),				NULL, NULL, 0, "<Branch>"},
	{N_("/_Tools/_Address book..."),	"<shift><control>A", addressbook_open_cb, 0, NULL},
	{N_("/_Tools/Add sender to address boo_k"),
						NULL, add_address_cb, 0, NULL},
	{N_("/_Tools/_Harvest addresses"),	NULL, NULL, 0, "<Branch>"},
	{N_("/_Tools/_Harvest addresses/from _Folder..."),
						NULL, addr_harvest_cb, 0, NULL},
	{N_("/_Tools/_Harvest addresses/from _Messages..."),
						NULL, addr_harvest_msg_cb, 0, NULL},
	{N_("/_Tools/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Tools/_Filter all messages in folder"),
						NULL, filter_cb, 0, NULL},
	{N_("/_Tools/Filter _selected messages"),
						NULL, filter_cb, 1, NULL},
	{N_("/_Tools/_Create filter rule"),	NULL, NULL, 0, "<Branch>"},
	{N_("/_Tools/_Create filter rule/_Automatically"),
						NULL, create_filter_cb, FILTER_BY_AUTO, NULL},
	{N_("/_Tools/_Create filter rule/by _From"),
						NULL, create_filter_cb, FILTER_BY_FROM, NULL},
	{N_("/_Tools/_Create filter rule/by _To"),
						NULL, create_filter_cb, FILTER_BY_TO, NULL},
	{N_("/_Tools/_Create filter rule/by _Subject"),
						NULL, create_filter_cb, FILTER_BY_SUBJECT, NULL},
	{N_("/_Tools/C_reate processing rule"),	NULL, NULL, 0, "<Branch>"},
	{N_("/_Tools/C_reate processing rule/_Automatically"),
						NULL, create_processing_cb, FILTER_BY_AUTO, NULL},
	{N_("/_Tools/C_reate processing rule/by _From"),
						NULL, create_processing_cb, FILTER_BY_FROM, NULL},
	{N_("/_Tools/C_reate processing rule/by _To"),
						NULL, create_processing_cb, FILTER_BY_TO, NULL},
	{N_("/_Tools/C_reate processing rule/by _Subject"),
						NULL, create_processing_cb, FILTER_BY_SUBJECT, NULL},
	{N_("/_Tools/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Tools/Actio_ns"),		NULL, NULL, 0, "<Branch>"},
	{N_("/_Tools/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Tools/Ch_eck for new messages in all folders"),
						NULL, update_folderview_cb, 0, NULL},
	{N_("/_Tools/Delete du_plicated messages"),
						NULL, NULL, 0, "<Branch>"},
	{N_("/_Tools/Delete du_plicated messages/In selected folder"),
						NULL, delete_duplicated_cb,   0, NULL},
	{N_("/_Tools/Delete du_plicated messages/In all folders"),
						NULL, delete_duplicated_all_cb,   0, NULL},
	{N_("/_Tools/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Tools/E_xecute"),		"X", execute_summary_cb, 0, NULL},
#ifdef USE_OPENSSL
	{N_("/_Tools/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Tools/SSL cer_tificates..."),	
						NULL, ssl_manager_open_cb, 0, NULL},
#endif
	{N_("/_Tools/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Tools/_Log window"),		"<shift><control>L", log_window_show_cb, 0, NULL},

	{N_("/_Configuration"),			NULL, NULL, 0, "<Branch>"},
	{N_("/_Configuration/C_hange current account"),
						NULL, NULL, 0, "<Branch>"},
	{N_("/_Configuration/_Preferences for current account..."),
						NULL, prefs_account_open_cb, 0, NULL},
	{N_("/_Configuration/Create _new account..."),
						NULL, new_account_cb, 0, NULL},
	{N_("/_Configuration/_Edit accounts..."),
						NULL, account_edit_open, 0, NULL},
	{N_("/_Configuration/---"),		NULL, NULL, 0, "<Separator>"},
	{N_("/_Configuration/P_references..."),
						NULL, prefs_open_cb, 0, NULL},
	{N_("/_Configuration/Pre-pr_ocessing..."),
						NULL, prefs_pre_processing_open_cb, 0, NULL},
	{N_("/_Configuration/Post-pro_cessing..."),
						NULL, prefs_post_processing_open_cb, 0, NULL},
	{N_("/_Configuration/_Filtering..."),
						NULL, prefs_filtering_open_cb, 0, NULL},
	{N_("/_Configuration/_Templates..."),	NULL, prefs_template_open_cb, 0, NULL},
	{N_("/_Configuration/_Actions..."),	NULL, prefs_actions_open_cb, 0, NULL},
	{N_("/_Configuration/Plu_gins..."),  	NULL, plugins_open_cb, 0, NULL},

	{N_("/_Help"),				NULL, NULL, 0, "<Branch>"},
	{N_("/_Help/_Manual"),			NULL, manual_open_cb, MANUAL_MANUAL_LOCAL, NULL},
	{N_("/_Help/_FAQ"),			NULL, manual_open_cb, MANUAL_FAQ_LOCAL, NULL},
	{N_("/_Help/_Online User-contributed FAQ"),	
						NULL, manual_open_cb, MANUAL_FAQ_CLAWS, NULL},
	{N_("/_Help/---"),			NULL, NULL, 0, "<Separator>"},
	{N_("/_Help/_About"),			NULL, about_show, 0, NULL}
};

static gboolean offline_ask_sync = TRUE;

static gboolean main_window_accel_activate (GtkAccelGroup *accelgroup,
                                            GObject *arg1,
                                            guint value,
                                            GdkModifierType mod,
                                            gpointer user_data) 
{
	MainWindow *mainwin = (MainWindow *)user_data;

	if (mainwin->summaryview &&
	    mainwin->summaryview->quicksearch &&
	    quicksearch_has_focus(mainwin->summaryview->quicksearch) &&
	    (mod == 0 || mod == GDK_SHIFT_MASK)) {
		quicksearch_pass_key(mainwin->summaryview->quicksearch, value, mod);
		return TRUE;
	}
	return FALSE;
}

MainWindow *main_window_create(SeparateType type)
{
	MainWindow *mainwin;
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkWidget *handlebox;
	GtkWidget *vbox_body;
	GtkWidget *hbox_stat;
	GtkWidget *statusbar;
	GtkWidget *progressbar;
	GtkWidget *statuslabel;
	GtkWidget *ac_button;
	GtkWidget *ac_label;
 	GtkWidget *online_pixmap;
	GtkWidget *offline_pixmap;
	GtkWidget *online_switch;
	GtkWidget *offline_switch;
	GtkTooltips *offline_tip;
	GtkTooltips *online_tip;
	GtkTooltips *sel_ac_tip;

	FolderView *folderview;
	SummaryView *summaryview;
	MessageView *messageview;
	GdkColormap *colormap;
	GdkColor color[4];
	gboolean success[4];
	GtkItemFactory *ifactory;
	GtkWidget *ac_menu;
	GtkWidget *menuitem;
	gint i;
	guint n_menu_entries;

	static GdkGeometry geometry;

	debug_print("Creating main window...\n");
	mainwin = g_new0(MainWindow, 1);

	/* main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), PROG_VERSION);
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

	if (!geometry.min_height) {
		geometry.min_width = 320;
		geometry.min_height = 200;
	}
	gtk_window_set_geometry_hints(GTK_WINDOW(window), NULL, &geometry,
				      GDK_HINT_MIN_SIZE);

	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(main_window_close_cb), mainwin);
	MANAGE_WINDOW_SIGNALS_CONNECT(window);
	g_signal_connect(G_OBJECT(window), "focus_in_event",
			 G_CALLBACK(mainwindow_focus_in_event),
			 mainwin);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(mainwindow_key_pressed), mainwin);

	gtk_widget_realize(window);
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK|GDK_KEY_RELEASE_MASK);
	

	gtkut_widget_set_app_icon(window);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* menu bar */
	n_menu_entries = sizeof(mainwin_entries) / sizeof(mainwin_entries[0]);
	menubar = menubar_create(window, mainwin_entries, 
				 n_menu_entries, "<Main>", mainwin);
	gtk_widget_show(menubar);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);
	ifactory = gtk_item_factory_from_widget(menubar);

	menu_set_sensitive(ifactory, "/Help/Manual", manual_available(MANUAL_MANUAL_LOCAL));
	menu_set_sensitive(ifactory, "/Help/FAQ", manual_available(MANUAL_FAQ_LOCAL));

	if (prefs_common.toolbar_detachable) {
		handlebox = gtk_handle_box_new();
		gtk_widget_show(handlebox);
		gtk_box_pack_start(GTK_BOX(vbox), handlebox, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(handlebox), "child_attached",
				 G_CALLBACK(toolbar_child_attached), mainwin);
		g_signal_connect(G_OBJECT(handlebox), "child_detached",
				 G_CALLBACK(toolbar_child_detached), mainwin);
	} else {
		handlebox = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(handlebox);
		gtk_box_pack_start(GTK_BOX(vbox), handlebox, FALSE, FALSE, 0);
	}
	/* link window to mainwin->window to avoid gdk warnings */
	mainwin->window       = window;
	
	/* create toolbar */
	mainwin->toolbar = toolbar_create(TOOLBAR_MAIN, 
					  handlebox, 
					  (gpointer)mainwin);

	/* vbox that contains body */
	vbox_body = gtk_vbox_new(FALSE, BORDER_WIDTH);
	gtk_widget_show(vbox_body);
	gtk_container_set_border_width(GTK_CONTAINER(vbox_body), BORDER_WIDTH);
	gtk_box_pack_start(GTK_BOX(vbox), vbox_body, TRUE, TRUE, 0);

	hbox_stat = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_end(GTK_BOX(vbox_body), hbox_stat, FALSE, FALSE, 0);

	statusbar = statusbar_create();
	gtk_box_pack_start(GTK_BOX(hbox_stat), statusbar, TRUE, TRUE, 0);

	progressbar = gtk_progress_bar_new();
	gtk_widget_set_size_request(progressbar, 120, 1);
	gtk_box_pack_start(GTK_BOX(hbox_stat), progressbar, FALSE, FALSE, 0);

	online_pixmap = stock_pixmap_widget(hbox_stat, STOCK_PIXMAP_ONLINE);
	offline_pixmap = stock_pixmap_widget(hbox_stat, STOCK_PIXMAP_OFFLINE);
	online_tip = gtk_tooltips_new();
	online_switch = gtk_button_new ();
	gtk_tooltips_set_tip(GTK_TOOLTIPS(online_tip),online_switch, 
			     _("You are online. Click the icon to go offline"), NULL);
	offline_tip = gtk_tooltips_new();
	offline_switch = gtk_button_new ();
	gtk_tooltips_set_tip(GTK_TOOLTIPS(offline_tip),offline_switch, 
			     _("You are offline. Click the icon to go online"),
			     NULL);
	gtk_container_add (GTK_CONTAINER(online_switch), online_pixmap);
	gtk_button_set_relief (GTK_BUTTON(online_switch), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(online_switch), "clicked", G_CALLBACK(online_switch_clicked), mainwin);
	gtk_box_pack_start (GTK_BOX(hbox_stat), online_switch, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER(offline_switch), offline_pixmap);
	gtk_button_set_relief (GTK_BUTTON(offline_switch), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(offline_switch), "clicked", G_CALLBACK(online_switch_clicked), mainwin);
	gtk_box_pack_start (GTK_BOX(hbox_stat), offline_switch, FALSE, FALSE, 0);
	
	statuslabel = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox_stat), statuslabel, FALSE, FALSE, 0);

	sel_ac_tip = gtk_tooltips_new();
	ac_button = gtk_button_new();
	gtk_tooltips_set_tip(GTK_TOOLTIPS(sel_ac_tip),
			     ac_button, _("Select account"), NULL);
	GTK_WIDGET_UNSET_FLAGS(ac_button, GTK_CAN_FOCUS);
	gtk_widget_set_size_request(ac_button, -1, 0);
	gtk_box_pack_end(GTK_BOX(hbox_stat), ac_button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(ac_button), "button_press_event",
			 G_CALLBACK(ac_label_button_pressed), mainwin);

	ac_label = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(ac_button), ac_label);

	gtk_widget_show_all(hbox_stat);

	gtk_widget_hide(offline_switch);
	/* create views */
	mainwin->folderview  = folderview  = folderview_create();
	mainwin->summaryview = summaryview = summary_create();
	mainwin->messageview = messageview = messageview_create(mainwin);
	mainwin->logwin      = log_window_create();

	folderview->mainwin      = mainwin;
	folderview->summaryview  = summaryview;

	summaryview->mainwin     = mainwin;
	summaryview->folderview  = folderview;
	summaryview->messageview = messageview;
	summaryview->window      = window;

	messageview->statusbar   = statusbar;
	mainwin->vbox           = vbox;
	mainwin->menubar        = menubar;
	mainwin->menu_factory   = ifactory;
	mainwin->handlebox      = handlebox;
	mainwin->vbox_body      = vbox_body;
	mainwin->hbox_stat      = hbox_stat;
	mainwin->statusbar      = statusbar;
	mainwin->progressbar    = progressbar;
	mainwin->statuslabel    = statuslabel;
	mainwin->online_switch  = online_switch;
	mainwin->online_pixmap  = online_pixmap;
	mainwin->offline_pixmap = offline_pixmap;
	mainwin->ac_button      = ac_button;
	mainwin->ac_label       = ac_label;
	mainwin->offline_switch    = offline_switch;
	
	/* set context IDs for status bar */
	mainwin->mainwin_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR(statusbar), "Main Window");
	mainwin->folderview_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR(statusbar), "Folder View");
	mainwin->summaryview_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR(statusbar), "Summary View");
	mainwin->messageview_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR(statusbar), "Message View");

	messageview->statusbar_cid = mainwin->messageview_cid;

	/* allocate colors for summary view and folder view */
	summaryview->color_marked.red = summaryview->color_marked.green = 0;
	summaryview->color_marked.blue = (guint16)65535;

	summaryview->color_dim.red = summaryview->color_dim.green =
		summaryview->color_dim.blue = COLOR_DIM;

	gtkut_convert_int_to_gdk_color(prefs_common.color_new,
				       &folderview->color_new);

	gtkut_convert_int_to_gdk_color(prefs_common.tgt_folder_col,
				       &folderview->color_op);

	summaryview->color_important.red = 0;
	summaryview->color_important.green = 0;
	summaryview->color_important.blue = (guint16)65535;

	color[0] = summaryview->color_marked;
	color[1] = summaryview->color_dim;
	color[2] = folderview->color_new;
	color[3] = folderview->color_op;

	colormap = gdk_drawable_get_colormap(window->window);
	gdk_colormap_alloc_colors(colormap, color, 4, FALSE, TRUE, success);
	for (i = 0; i < 4; i++) {
		if (success[i] == FALSE)
			g_warning("MainWindow: color allocation %d failed\n", i);
	}

	debug_print("done.\n");

	messageview->visible = prefs_common.msgview_visible;

	main_window_set_widgets(mainwin, type);

	g_signal_connect(G_OBJECT(window), "size_allocate",
			 G_CALLBACK(main_window_size_allocate_cb),
			 mainwin);

	/* set menu items */
	menuitem = gtk_item_factory_get_item
		(ifactory, "/View/Character encoding/Auto detect");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);

	switch (prefs_common.toolbar_style) {
	case TOOLBAR_NONE:
		menuitem = gtk_item_factory_get_item
			(ifactory, "/View/Show or hide/Toolbar/None");
		break;
	case TOOLBAR_ICON:
		menuitem = gtk_item_factory_get_item
			(ifactory, "/View/Show or hide/Toolbar/Icon");
		break;
	case TOOLBAR_TEXT:
		menuitem = gtk_item_factory_get_item
			(ifactory, "/View/Show or hide/Toolbar/Text");
		break;
	case TOOLBAR_BOTH:
		menuitem = gtk_item_factory_get_item
			(ifactory, "/View/Show or hide/Toolbar/Icon and text");
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);

	gtk_widget_hide(mainwin->hbox_stat);
	menuitem = gtk_item_factory_get_item
		(ifactory, "/View/Show or hide/Status bar");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
				       prefs_common.show_statusbar);
	
	/* set account selection menu */
	ac_menu = gtk_item_factory_get_widget
		(ifactory, "/Configuration/Change current account");
	mainwin->ac_menu = ac_menu;

	toolbar_main_set_sensitive(mainwin);

	/* create actions menu */
	main_window_update_actions_menu(mainwin);

	/* attach accel groups to main window */
#define	ADD_MENU_ACCEL_GROUP_TO_WINDOW(menu,win)			\
	gtk_window_add_accel_group					\
		(GTK_WINDOW(win), 					\
		 gtk_item_factory_from_widget(menu)->accel_group); 	\
	g_signal_connect(G_OBJECT(gtk_item_factory_from_widget(menu)->accel_group), \
			"accel_activate", 				\
		       	G_CALLBACK(main_window_accel_activate), mainwin);
			 
	
	ADD_MENU_ACCEL_GROUP_TO_WINDOW(summaryview->popupmenu, mainwin->window);
	
	/* connect the accelerators for equivalent 
	   menu items in different menus             */
	menu_connect_identical_items();


	gtk_window_iconify(mainwin->window);

	gtk_widget_show(mainwin->window);

	/* initialize views */
	folderview_init(folderview);
	summary_init(summaryview);
	messageview_init(messageview);
	log_window_init(mainwin->logwin);
	log_window_set_clipping(mainwin->logwin, prefs_common.cliplog,
				prefs_common.loglength);
#ifdef USE_OPENSSL
	sslcertwindow_register_hook();
#endif
	mainwin->lock_count = 0;
	mainwin->menu_lock_count = 0;
	mainwin->cursor_count = 0;

	mainwin->progressindicator_hook =
		hooks_register_hook(PROGRESSINDICATOR_HOOKLIST, mainwindow_progressindicator_hook, mainwin);

	if (!watch_cursor)
		watch_cursor = gdk_cursor_new(GDK_WATCH);

	mainwin_list = g_list_append(mainwin_list, mainwin);

	/* init work_offline */
	if (prefs_common.work_offline)
		online_switch_clicked (GTK_BUTTON(online_switch), mainwin);

	return mainwin;
}

void main_window_destroy(MainWindow *mainwin)
{
	/* TODO : destroy other component */
	messageview_destroy(mainwin->messageview);
	mainwin->messageview = NULL;	
}

void main_window_update_actions_menu(MainWindow *mainwin)
{
	GtkItemFactory *ifactory;

	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	action_update_mainwin_menu(ifactory, "/Tools/Actions", mainwin);
}

void main_window_cursor_wait(MainWindow *mainwin)
{

	if (mainwin->cursor_count == 0)
		gdk_window_set_cursor(mainwin->window->window, watch_cursor);

	mainwin->cursor_count++;

	gdk_flush();
}

void main_window_cursor_normal(MainWindow *mainwin)
{
	if (mainwin->cursor_count)
		mainwin->cursor_count--;

	if (mainwin->cursor_count == 0)
		gdk_window_set_cursor(mainwin->window->window, NULL);

	gdk_flush();
}

/* lock / unlock the user-interface */
void main_window_lock(MainWindow *mainwin)
{
	if (mainwin->lock_count == 0)
		gtk_widget_set_sensitive(mainwin->ac_button, FALSE);

	mainwin->lock_count++;

	main_window_set_menu_sensitive(mainwin);
	toolbar_main_set_sensitive(mainwin);
}

void main_window_unlock(MainWindow *mainwin)
{
	if (mainwin->lock_count)
		mainwin->lock_count--;

	main_window_set_menu_sensitive(mainwin);
	toolbar_main_set_sensitive(mainwin);

	if (mainwin->lock_count == 0)
		gtk_widget_set_sensitive(mainwin->ac_button, TRUE);
}

static void main_window_menu_callback_block(MainWindow *mainwin)
{
	mainwin->menu_lock_count++;
}

static void main_window_menu_callback_unblock(MainWindow *mainwin)
{
	if (mainwin->menu_lock_count)
		mainwin->menu_lock_count--;
}

static guint prefs_tag = 0;

void main_window_reflect_prefs_all(void)
{
	main_window_reflect_prefs_all_real(FALSE);
}

static gboolean reflect_prefs_timeout_cb(gpointer data) 
{
	gboolean pixmap_theme_changed = GPOINTER_TO_INT(data);
	GList *cur;
	MainWindow *mainwin;
	GtkWidget *pixmap;

	for (cur = mainwin_list; cur != NULL; cur = cur->next) {
		mainwin = (MainWindow *)cur->data;

		main_window_show_cur_account(mainwin);
		main_window_set_menu_sensitive(mainwin);
		toolbar_main_set_sensitive(mainwin);

		/* pixmap themes */
		if (pixmap_theme_changed) {
			toolbar_update(TOOLBAR_MAIN, mainwin);
			messageview_reflect_prefs_pixmap_theme();
			compose_reflect_prefs_pixmap_theme();
			folderview_reflect_prefs_pixmap_theme(mainwin->folderview);
			summary_reflect_prefs_pixmap_theme(mainwin->summaryview);

			pixmap = stock_pixmap_widget(mainwin->hbox_stat, STOCK_PIXMAP_ONLINE);
			gtk_container_remove(GTK_CONTAINER(mainwin->online_switch), 
					     mainwin->online_pixmap);
			gtk_container_add (GTK_CONTAINER(mainwin->online_switch), pixmap);
			gtk_widget_show(pixmap);
			mainwin->online_pixmap = pixmap;
			pixmap = stock_pixmap_widget(mainwin->hbox_stat, STOCK_PIXMAP_OFFLINE);
			gtk_container_remove(GTK_CONTAINER(mainwin->offline_switch), 
					     mainwin->offline_pixmap);
			gtk_container_add (GTK_CONTAINER(mainwin->offline_switch), pixmap);
			gtk_widget_show(pixmap);
			mainwin->offline_pixmap = pixmap;
		}
		
		headerview_set_font(mainwin->messageview->headerview);
		headerview_set_visibility(mainwin->messageview->headerview,
					  prefs_common.display_header_pane);
		textview_reflect_prefs(mainwin->messageview->mimeview->textview);
		folderview_reflect_prefs();
		summary_reflect_prefs();
		summary_redisplay_msg(mainwin->summaryview);
	}
	prefs_tag = 0;
	return FALSE;
}

void main_window_reflect_prefs_all_real(gboolean pixmap_theme_changed)
{
	if (prefs_tag == 0 || pixmap_theme_changed) {
		prefs_tag = g_timeout_add(500, reflect_prefs_timeout_cb, 
						GINT_TO_POINTER(pixmap_theme_changed));
	}
}

void main_window_set_summary_column(void)
{
	GList *cur;
	MainWindow *mainwin;

	for (cur = mainwin_list; cur != NULL; cur = cur->next) {
		mainwin = (MainWindow *)cur->data;
		summary_set_column_order(mainwin->summaryview);
	}
}

void main_window_set_folder_column(void)
{
	GList *cur;
	MainWindow *mainwin;

	for (cur = mainwin_list; cur != NULL; cur = cur->next) {
		mainwin = (MainWindow *)cur->data;
		folderview_set_column_order(mainwin->folderview);
	}
}

static void main_window_set_account_selector_menu(MainWindow *mainwin,
						  GList *account_list)
{
	GList *cur_ac, *cur_item;
	GtkWidget *menuitem;
	PrefsAccount *ac_prefs;

	/* destroy all previous menu item */
	cur_item = GTK_MENU_SHELL(mainwin->ac_menu)->children;
	while (cur_item != NULL) {
		GList *next = cur_item->next;
		gtk_widget_destroy(GTK_WIDGET(cur_item->data));
		cur_item = next;
	}

	for (cur_ac = account_list; cur_ac != NULL; cur_ac = cur_ac->next) {
		ac_prefs = (PrefsAccount *)cur_ac->data;

		menuitem = gtk_menu_item_new_with_label
			(ac_prefs->account_name
			 ? ac_prefs->account_name : _("Untitled"));
		gtk_widget_show(menuitem);
		gtk_menu_append(GTK_MENU(mainwin->ac_menu), menuitem);
		g_signal_connect(G_OBJECT(menuitem), "activate",
				 G_CALLBACK(account_selector_menu_cb),
				 ac_prefs);
	}
}

static void main_window_set_account_receive_menu(MainWindow *mainwin,
						 GList *account_list)
{
	GList *cur_ac, *cur_item;
	GtkWidget *menu;
	GtkWidget *menuitem;
	PrefsAccount *ac_prefs;

	menu = gtk_item_factory_get_widget(mainwin->menu_factory,
					   "/Message/Receive");

	/* search for separator */
	for (cur_item = GTK_MENU_SHELL(menu)->children; cur_item != NULL;
	     cur_item = cur_item->next) {
		if (GTK_BIN(cur_item->data)->child == NULL) {
			cur_item = cur_item->next;
			break;
		}
	}

	/* destroy all previous menu item */
	while (cur_item != NULL) {
		GList *next = cur_item->next;
		gtk_widget_destroy(GTK_WIDGET(cur_item->data));
		cur_item = next;
	}

	for (cur_ac = account_list; cur_ac != NULL; cur_ac = cur_ac->next) {
		ac_prefs = (PrefsAccount *)cur_ac->data;

		menuitem = gtk_menu_item_new_with_label
			(ac_prefs->account_name ? ac_prefs->account_name
			 : _("Untitled"));
		gtk_widget_show(menuitem);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		g_signal_connect(G_OBJECT(menuitem), "activate",
				 G_CALLBACK(account_receive_menu_cb),
				 ac_prefs);
	}
}

void main_window_set_account_menu(GList *account_list)
{
	GList *cur;
	MainWindow *mainwin;

	for (cur = mainwin_list; cur != NULL; cur = cur->next) {
		mainwin = (MainWindow *)cur->data;
		main_window_set_account_selector_menu(mainwin, account_list);
		main_window_set_account_receive_menu(mainwin, account_list);
	}
}

static void main_window_show_cur_account(MainWindow *mainwin)
{
	gchar *buf;
	gchar *ac_name;

	ac_name = g_strdup(cur_account
			   ? (cur_account->account_name
			      ? cur_account->account_name : _("Untitled"))
			   : _("none"));

	if (cur_account)
		buf = g_strdup_printf("%s - %s", ac_name, PROG_VERSION);
	else
		buf = g_strdup(PROG_VERSION);
	gtk_window_set_title(GTK_WINDOW(mainwin->window), buf);
	g_free(buf);

	gtk_label_set_text(GTK_LABEL(mainwin->ac_label), ac_name);
	gtk_widget_queue_resize(mainwin->ac_button);

	g_free(ac_name);
}

GtkWidget *main_window_get_folder_window(MainWindow *mainwin)
{
	switch (mainwin->type) {
	case SEPARATE_FOLDER:
		return mainwin->win.sep_folder.folderwin;
	case SEPARATE_BOTH:
		return mainwin->win.sep_both.folderwin;
	default:
		return NULL;
	}
}

GtkWidget *main_window_get_message_window(MainWindow *mainwin)
{
	switch (mainwin->type) {
	case SEPARATE_MESSAGE:
		return mainwin->win.sep_message.messagewin;
	case SEPARATE_BOTH:
		return mainwin->win.sep_both.messagewin;
	default:
		return NULL;
	}
}

void main_window_separation_change(MainWindow *mainwin, SeparateType type)
{
	GtkWidget *folder_wid  = GTK_WIDGET_PTR(mainwin->folderview);
	GtkWidget *summary_wid = GTK_WIDGET_PTR(mainwin->summaryview);
	/* GtkWidget *message_wid = GTK_WIDGET_PTR(mainwin->messageview); */
	GtkWidget *message_wid = mainwin->messageview->vbox;

	debug_print("Changing window separation type from %d to %d\n",
		    mainwin->type, type);

	if (mainwin->type == type) return;

	/* remove widgets from those containers */
	gtk_widget_ref(folder_wid);
	gtk_widget_ref(summary_wid);
	gtk_widget_ref(message_wid);
	gtkut_container_remove
		(GTK_CONTAINER(folder_wid->parent), folder_wid);
	gtkut_container_remove
		(GTK_CONTAINER(summary_wid->parent), summary_wid);
	gtkut_container_remove
		(GTK_CONTAINER(message_wid->parent), message_wid);

	/* clean containers */
	switch (mainwin->type) {
	case SEPARATE_NONE:
		gtk_widget_destroy(mainwin->win.sep_none.hpaned);
		break;
	case SEPARATE_FOLDER:
		gtk_widget_destroy(mainwin->win.sep_folder.vpaned);
		gtk_widget_destroy(mainwin->win.sep_folder.folderwin);
		break;
	case SEPARATE_MESSAGE:
		gtk_widget_destroy(mainwin->win.sep_message.hpaned);
		gtk_widget_destroy(mainwin->win.sep_message.messagewin);
		break;
	case SEPARATE_BOTH:
		gtk_widget_destroy(mainwin->win.sep_both.messagewin);
		gtk_widget_destroy(mainwin->win.sep_both.folderwin);
		break;
	}

	gtk_widget_hide(mainwin->window);
	main_window_set_widgets(mainwin, type);
	gtk_widget_show(mainwin->window);

	gtk_widget_unref(folder_wid);
	gtk_widget_unref(summary_wid);
	gtk_widget_unref(message_wid);
}

void main_window_toggle_message_view(MainWindow *mainwin)
{
	SummaryView *summaryview = mainwin->summaryview;
	union CompositeWin *cwin = &mainwin->win;
	GtkWidget *vpaned = NULL;
	GtkWidget *container = NULL;
	GtkWidget *msgwin = NULL;

	switch (mainwin->type) {
	case SEPARATE_NONE:
		vpaned = cwin->sep_none.vpaned;
		container = cwin->sep_none.hpaned;
		break;
	case SEPARATE_FOLDER:
		vpaned = cwin->sep_folder.vpaned;
		container = mainwin->vbox_body;
		break;
	case SEPARATE_MESSAGE:
		msgwin = mainwin->win.sep_message.messagewin;
		break;
	case SEPARATE_BOTH:
		msgwin = mainwin->win.sep_both.messagewin;
		break;
	}

	if (msgwin) {
		if (GTK_WIDGET_VISIBLE(msgwin)) {
			gtk_widget_hide(msgwin);
			mainwin->messageview->visible = FALSE;
			summaryview->displayed = NULL;
		} else {
			gtk_widget_show(msgwin);
			mainwin->messageview->visible = TRUE;
		}
	} else if (vpaned->parent != NULL) {
		mainwin->messageview->visible = FALSE;
		summaryview->displayed = NULL;
		gtk_widget_ref(vpaned);
		gtkut_container_remove(GTK_CONTAINER(container), vpaned);
		gtk_widget_reparent(GTK_WIDGET_PTR(summaryview), container);
	} else {
		mainwin->messageview->visible = TRUE;
		gtk_widget_reparent(GTK_WIDGET_PTR(summaryview), vpaned);
		gtk_container_add(GTK_CONTAINER(container), vpaned);
		gtk_widget_unref(vpaned);
	}

	if (messageview_is_visible(mainwin->messageview))
		gtk_arrow_set(GTK_ARROW(mainwin->summaryview->toggle_arrow),
			      GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	else
		gtk_arrow_set(GTK_ARROW(mainwin->summaryview->toggle_arrow),
			      GTK_ARROW_UP, GTK_SHADOW_OUT);

	if (mainwin->messageview->visible == FALSE)
		messageview_clear(mainwin->messageview);

	main_window_set_menu_sensitive(mainwin);

	prefs_common.msgview_visible = mainwin->messageview->visible;

	gtk_widget_grab_focus(summaryview->ctree);
}

void main_window_get_size(MainWindow *mainwin)
{
	GtkAllocation *allocation;

	if (mainwin->messageview == NULL) {
		debug_print("called after messageview "
			    "has been deallocated!\n");
		return;
	}

	allocation = &(GTK_WIDGET_PTR(mainwin->summaryview)->allocation);

	if (allocation->width > 1 && allocation->height > 1) {
		prefs_common.summaryview_width = allocation->width;

		if ((mainwin->type == SEPARATE_NONE ||
		     mainwin->type == SEPARATE_FOLDER) &&
		    messageview_is_visible(mainwin->messageview))
			prefs_common.summaryview_height = allocation->height;

		prefs_common.mainview_width = allocation->width;
	}

	allocation = &mainwin->window->allocation;
	if (allocation->width > 1 && allocation->height > 1) {
		prefs_common.mainview_height = allocation->height;
		prefs_common.mainwin_width   = allocation->width;
		prefs_common.mainwin_height  = allocation->height;
	}

	allocation = &(GTK_WIDGET_PTR(mainwin->folderview)->allocation);
	if (allocation->width > 1 && allocation->height > 1) {
		prefs_common.folderview_width  = allocation->width;
		prefs_common.folderview_height = allocation->height;
	}

	allocation = &(GTK_WIDGET_PTR(mainwin->messageview)->allocation);
	if (allocation->width > 1 && allocation->height > 1) {
		prefs_common.msgview_width = allocation->width;
		prefs_common.msgview_height = allocation->height;
	}

/*	debug_print("summaryview size: %d x %d\n",
		    prefs_common.summaryview_width,
		    prefs_common.summaryview_height);
	debug_print("folderview size: %d x %d\n",
		    prefs_common.folderview_width,
		    prefs_common.folderview_height);
	debug_print("messageview size: %d x %d\n",
		    prefs_common.msgview_width,
		    prefs_common.msgview_height); */
}

void main_window_get_position(MainWindow *mainwin)
{
	gint x, y;
	GtkWidget *window;

	gtkut_widget_get_uposition(mainwin->window, &x, &y);

	prefs_common.mainview_x = x;
	prefs_common.mainview_y = y;
	prefs_common.mainwin_x = x;
	prefs_common.mainwin_y = y;

	debug_print("main window position: %d, %d\n", x, y);

	window = main_window_get_folder_window(mainwin);
	if (window) {
		gtkut_widget_get_uposition(window, &x, &y);
		prefs_common.folderwin_x = x;
		prefs_common.folderwin_y = y;
		debug_print("folder window position: %d, %d\n", x, y);
	}
	window = main_window_get_message_window(mainwin);
	if (window) {
		gtkut_widget_get_uposition(window, &x, &y);
		prefs_common.main_msgwin_x = x;
		prefs_common.main_msgwin_y = y;
		debug_print("message window position: %d, %d\n", x, y);
	}
}

void main_window_progress_on(MainWindow *mainwin)
{
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mainwin->progressbar), "");
}

void main_window_progress_off(MainWindow *mainwin)
{
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mainwin->progressbar), 0.0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mainwin->progressbar), "");
}

void main_window_progress_set(MainWindow *mainwin, gint cur, gint total)
{
	gchar buf[32];

	g_snprintf(buf, sizeof(buf), "%d / %d", cur, total);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mainwin->progressbar), buf);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mainwin->progressbar),
				(cur == 0 && total == 0) ? 0 :
				(gfloat)cur / (gfloat)total);
}

void main_window_empty_trash(MainWindow *mainwin, gboolean confirm)
{
	if (confirm) {
		if (alertpanel(_("Empty trash"),
			       _("Delete all messages in trash folders?"),
			       GTK_STOCK_YES, GTK_STOCK_NO, NULL)
		    != G_ALERTDEFAULT)
			return;
		manage_window_focus_in(mainwin->window, NULL, NULL);
	}

	procmsg_empty_all_trash();

	if (mainwin->summaryview->folder_item &&
	    mainwin->summaryview->folder_item->stype == F_TRASH)
		gtk_widget_grab_focus(mainwin->folderview->ctree);
}

void main_window_add_mailbox(MainWindow *mainwin)
{
	gchar *path;
	Folder *folder;

	path = input_dialog(_("Add mailbox"),
			    _("Input the location of mailbox.\n"
			      "If the existing mailbox is specified, it will be\n"
			      "scanned automatically."),
			    "Mail");
	if (!path) return;
	if (folder_find_from_path(path)) {
		alertpanel_error(_("The mailbox '%s' already exists."), path);
		g_free(path);
		return;
	}
	folder = folder_new(folder_get_class_from_string("mh"), 
			    !strcmp(path, "Mail") ? _("Mailbox") : 
			    g_path_get_basename(path), path);
	g_free(path);

	if (folder->klass->create_tree(folder) < 0) {
		alertpanel_error(_("Creation of the mailbox failed.\n"
				   "Maybe some files already exist, or you don't have the permission to write there."));
		folder_destroy(folder);
		return;
	}

	folder_add(folder);
	folder_set_ui_func(folder, scan_tree_func, mainwin);
	folder_scan_tree(folder, TRUE);
	folder_set_ui_func(folder, NULL, NULL);
}

SensitiveCond main_window_get_current_state(MainWindow *mainwin)
{
	SensitiveCond state = 0;
	SummarySelection selection;
	FolderItem *item = mainwin->summaryview->folder_item;
	GList *account_list = account_get_list();
	
	selection = summary_get_selection_type(mainwin->summaryview);

	if (mainwin->lock_count == 0)
		state |= M_UNLOCKED;
	if (selection != SUMMARY_NONE)
		state |= M_MSG_EXIST;
	if (item && item->path && folder_item_parent(item) && !item->no_select) {
		state |= M_EXEC;
		/*		if (item->folder->type != F_NEWS) */
		state |= M_ALLOW_DELETE;

		if (prefs_common.immediate_exec == 0
		    && mainwin->lock_count == 0)
			state |= M_DELAY_EXEC;

		if ((selection == SUMMARY_NONE && item->hide_read_msgs)
		    || selection != SUMMARY_NONE)
			state |= M_HIDE_READ_MSG;	
	}		
	if (mainwin->summaryview->threaded)
		state |= M_THREADED;
	else
		state |= M_UNTHREADED;	
	if (selection == SUMMARY_SELECTED_SINGLE ||
	    selection == SUMMARY_SELECTED_MULTIPLE)
		state |= M_TARGET_EXIST;
	if (selection == SUMMARY_SELECTED_SINGLE)
		state |= M_SINGLE_TARGET_EXIST;
	if (mainwin->summaryview->folder_item &&
	    mainwin->summaryview->folder_item->folder->klass->type == F_NEWS)
		state |= M_NEWS;
	else
		state |= M_NOT_NEWS;
	if (selection == SUMMARY_SELECTED_SINGLE &&
	    (item &&
	     (folder_has_parent_of_type(item, F_DRAFT) ||
	      folder_has_parent_of_type(item, F_OUTBOX) ||
	      folder_has_parent_of_type(item, F_QUEUE))))
		state |= M_ALLOW_REEDIT;
	if (cur_account)
		state |= M_HAVE_ACCOUNT;
	
	for ( ; account_list != NULL; account_list = account_list->next) {
		if (((PrefsAccount*)account_list->data)->protocol == A_NNTP) {
			state |= M_HAVE_NEWS_ACCOUNT;
			break;
		}
	}

	if (inc_is_active())
		state |= M_INC_ACTIVE;

	if (mainwin->summaryview->deleted > 0 ||
	    mainwin->summaryview->moved > 0 ||
	    mainwin->summaryview->copied > 0)
		state |= M_DELAY_EXEC;

	return state;
}



void main_window_set_menu_sensitive(MainWindow *mainwin)
{
	GtkItemFactory *ifactory = mainwin->menu_factory;
	SensitiveCond state;
	gboolean sensitive;
	GtkWidget *menu;
	GtkWidget *menuitem;
	SummaryView *summaryview;
	gchar *menu_path;
	gint i;
	GList *cur_item;

	static const struct {
		gchar *const entry;
		SensitiveCond cond;
	} entry[] = {
		{"/File/Save as...", M_TARGET_EXIST},
		{"/File/Print..."  , M_TARGET_EXIST},
		{"/File/Exit"      , M_UNLOCKED},

		{"/Edit/Select thread"		   , M_SINGLE_TARGET_EXIST},

		{"/View/Sort"                      , M_EXEC},
		{"/View/Thread view"               , M_EXEC},
		{"/View/Expand all threads"        , M_MSG_EXIST},
		{"/View/Collapse all threads"      , M_MSG_EXIST},
		{"/View/Hide read messages"	   , M_HIDE_READ_MSG},
		{"/View/Go to/Prev message"        , M_MSG_EXIST},
		{"/View/Go to/Next message"        , M_MSG_EXIST},
		{"/View/Go to/Prev unread message" , M_MSG_EXIST},
		{"/View/Go to/Prev new message"    , M_MSG_EXIST},
		{"/View/Go to/Prev marked message" , M_MSG_EXIST},
		{"/View/Go to/Next marked message" , M_MSG_EXIST},
		{"/View/Go to/Prev labeled message", M_MSG_EXIST},
		{"/View/Go to/Next labeled message", M_MSG_EXIST},
		{"/View/Open in new window"        , M_SINGLE_TARGET_EXIST},
		{"/View/Show all headers"          , M_SINGLE_TARGET_EXIST},
		{"/View/Message source"            , M_SINGLE_TARGET_EXIST},

		{"/Message/Receive/Get from current account"
						 , M_HAVE_ACCOUNT|M_UNLOCKED},
		{"/Message/Receive/Get from all accounts"
						 , M_HAVE_ACCOUNT|M_UNLOCKED},
		{"/Message/Receive/Cancel receiving"
						 , M_INC_ACTIVE},

		{"/Message/Compose a news message", M_HAVE_NEWS_ACCOUNT},
		{"/Message/Reply"                 , M_HAVE_ACCOUNT|M_SINGLE_TARGET_EXIST},
		{"/Message/Reply to"              , M_HAVE_ACCOUNT|M_SINGLE_TARGET_EXIST},
		{"/Message/Follow-up and reply to", M_HAVE_ACCOUNT|M_SINGLE_TARGET_EXIST|M_NEWS},
		{"/Message/Forward"               , M_HAVE_ACCOUNT|M_TARGET_EXIST},
		{"/Message/Forward as attachment" , M_HAVE_ACCOUNT|M_TARGET_EXIST},
        	{"/Message/Redirect"		  , M_HAVE_ACCOUNT|M_SINGLE_TARGET_EXIST},
		{"/Message/Move..."		  , M_TARGET_EXIST|M_ALLOW_DELETE},
		{"/Message/Copy..."		  , M_TARGET_EXIST|M_EXEC},
		{"/Message/Move to trash"	  , M_TARGET_EXIST|M_ALLOW_DELETE|M_NOT_NEWS},
		{"/Message/Delete..." 		  , M_TARGET_EXIST|M_ALLOW_DELETE},
		{"/Message/Cancel a news message" , M_TARGET_EXIST|M_ALLOW_DELETE|M_NEWS},
		{"/Message/Mark"   		  , M_TARGET_EXIST},
		{"/Message/Re-edit"               , M_HAVE_ACCOUNT|M_ALLOW_REEDIT},

		{"/Tools/Add sender to address book"   , M_SINGLE_TARGET_EXIST},
		{"/Tools/Harvest addresses/from Folder..."       
						       , M_MSG_EXIST},
		{"/Tools/Harvest addresses/from Messages..."
						       , M_MSG_EXIST|M_TARGET_EXIST},
		{"/Tools/Filter all messages in folder", M_MSG_EXIST|M_EXEC},
		{"/Tools/Filter selected messages"     , M_TARGET_EXIST|M_EXEC},
		{"/Tools/Create filter rule"           , M_SINGLE_TARGET_EXIST|M_UNLOCKED},
		{"/Tools/Create processing rule"       , M_SINGLE_TARGET_EXIST|M_UNLOCKED},
		{"/Tools/Actions"                      , M_TARGET_EXIST},
		{"/Tools/Execute"                      , M_DELAY_EXEC},
		{"/Tools/Delete duplicated messages/In selected folder"   , M_MSG_EXIST|M_ALLOW_DELETE},

		{"/Configuration", M_UNLOCKED},

		{NULL, 0}
	};

	state = main_window_get_current_state(mainwin);

	for (i = 0; entry[i].entry != NULL; i++) {
		sensitive = ((entry[i].cond & state) == entry[i].cond);
		menu_set_sensitive(ifactory, entry[i].entry, sensitive);
	}

	menu = gtk_item_factory_get_widget(ifactory, "/Message/Receive");

	/* search for separator */
	for (cur_item = GTK_MENU_SHELL(menu)->children; cur_item != NULL;
	     cur_item = cur_item->next) {
		if (GTK_BIN(cur_item->data)->child == NULL) {
			cur_item = cur_item->next;
			break;
		}
	}

	for (; cur_item != NULL; cur_item = cur_item->next) {
		gtk_widget_set_sensitive(GTK_WIDGET(cur_item->data),
					 (M_UNLOCKED & state) != 0);
	}

	main_window_menu_callback_block(mainwin);

#define SET_CHECK_MENU_ACTIVE(path, active) \
{ \
	menuitem = gtk_item_factory_get_widget(ifactory, path); \
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), active); \
}

	SET_CHECK_MENU_ACTIVE("/View/Show or hide/Message view",
			      messageview_is_visible(mainwin->messageview));

	summaryview = mainwin->summaryview;
	menu_path = "/View/Sort/Don't sort";

	switch (summaryview->sort_key) {
	case SORT_BY_NUMBER:
		menu_path = "/View/Sort/by number"; break;
	case SORT_BY_SIZE:
		menu_path = "/View/Sort/by Size"; break;
	case SORT_BY_DATE:
		menu_path = "/View/Sort/by Date"; break;
	case SORT_BY_FROM:
		menu_path = "/View/Sort/by From"; break;
	case SORT_BY_TO:
		menu_path = "/View/Sort/by To"; break;
	case SORT_BY_SUBJECT:
		menu_path = "/View/Sort/by Subject"; break;
	case SORT_BY_LABEL:
		menu_path = "/View/Sort/by color label"; break;
	case SORT_BY_MARK:
		menu_path = "/View/Sort/by mark"; break;
	case SORT_BY_STATUS:
		menu_path = "/View/Sort/by status"; break;
	case SORT_BY_MIME:
		menu_path = "/View/Sort/by attachment"; break;
	case SORT_BY_SCORE:
		menu_path = "/View/Sort/by score"; break;
	case SORT_BY_LOCKED:
		menu_path = "/View/Sort/by locked"; break;
	case SORT_BY_NONE:
	default:
		menu_path = "/View/Sort/Don't sort"; break;
	}
	SET_CHECK_MENU_ACTIVE(menu_path, TRUE);

	if (summaryview->sort_type == SORT_ASCENDING) {
		SET_CHECK_MENU_ACTIVE("/View/Sort/Ascending", TRUE);
	} else {
		SET_CHECK_MENU_ACTIVE("/View/Sort/Descending", TRUE);
	}

	if (summaryview->sort_key != SORT_BY_NONE) {
		menu_set_sensitive(ifactory, "/View/Sort/Ascending", TRUE);
		menu_set_sensitive(ifactory, "/View/Sort/Descending", TRUE);
	} else {
		menu_set_sensitive(ifactory, "/View/Sort/Ascending", FALSE);
		menu_set_sensitive(ifactory, "/View/Sort/Descending", FALSE);
	}

	SET_CHECK_MENU_ACTIVE("/View/Show all headers",
			      mainwin->messageview->mimeview->textview->show_all_headers);
	SET_CHECK_MENU_ACTIVE("/View/Thread view", (state & M_THREADED) != 0);

#undef SET_CHECK_MENU_ACTIVE

	main_window_menu_callback_unblock(mainwin);
}

void main_window_popup(MainWindow *mainwin)
{
	gtkut_window_popup(mainwin->window);

	switch (mainwin->type) {
	case SEPARATE_FOLDER:
		gtkut_window_popup(mainwin->win.sep_folder.folderwin);
		break;
	case SEPARATE_MESSAGE:
		gtkut_window_popup(mainwin->win.sep_message.messagewin);
		break;
	case SEPARATE_BOTH:
		gtkut_window_popup(mainwin->win.sep_both.folderwin);
		gtkut_window_popup(mainwin->win.sep_both.messagewin);
		break;
	default:
		break;
	}
}

void main_window_show(MainWindow *mainwin)
{
	gtk_widget_show(mainwin->window);
	gtk_widget_show(mainwin->vbox_body);

        gtk_widget_set_uposition(mainwin->window,
                                 prefs_common.mainwin_x,
                                 prefs_common.mainwin_y);

	gtk_widget_set_size_request(GTK_WIDGET_PTR(mainwin->folderview),
			     prefs_common.folderview_width,
			     prefs_common.folderview_height);
	gtk_widget_set_size_request(GTK_WIDGET_PTR(mainwin->summaryview),
			     prefs_common.summaryview_width,
			     prefs_common.summaryview_height);
	gtk_widget_set_size_request(GTK_WIDGET_PTR(mainwin->messageview),
			     prefs_common.msgview_width,
			     prefs_common.msgview_height);

	if (mainwin->type & SEPARATE_FOLDER) {
		GtkWidget *folderwin;

		folderwin = main_window_get_folder_window(mainwin);
		gtk_widget_show(folderwin);
		gtk_widget_set_uposition(folderwin, prefs_common.folderwin_x,
                                         prefs_common.folderwin_y);
	}

	if (mainwin->type & SEPARATE_MESSAGE) {
		GtkWidget *messagewin;

		messagewin = main_window_get_message_window(mainwin);
		gtk_widget_show(messagewin);
		gtk_widget_set_uposition(messagewin, prefs_common.main_msgwin_x,
                                         prefs_common.main_msgwin_y);
	}
}

void main_window_hide(MainWindow *mainwin)
{
	main_window_get_size(mainwin);
	main_window_get_position(mainwin);

	gtk_widget_hide(mainwin->window);
	gtk_widget_hide(mainwin->vbox_body);

	if (mainwin->type & SEPARATE_FOLDER) {
		gtk_widget_hide(mainwin->win.sep_folder.folderwin);
	}

	if (mainwin->type & SEPARATE_MESSAGE) {
		gtk_widget_hide(mainwin->win.sep_message.messagewin);
	}
}

static void main_window_set_widgets(MainWindow *mainwin, SeparateType type)
{
	GtkWidget *folderwin = NULL;
	GtkWidget *messagewin = NULL;
	GtkWidget *hpaned;
	GtkWidget *vpaned;
	GtkWidget *vbox_body = mainwin->vbox_body;
	GtkItemFactory *ifactory = mainwin->menu_factory;
	GtkWidget *menuitem;
	GtkItemFactory *msgview_ifactory;

	debug_print("Setting widgets... ");

	gtk_widget_set_size_request(GTK_WIDGET_PTR(mainwin->folderview),
				    prefs_common.folderview_width,
				    prefs_common.folderview_height);
	gtk_widget_set_size_request(GTK_WIDGET_PTR(mainwin->summaryview),
				    prefs_common.summaryview_width,
				    prefs_common.summaryview_height);
	gtk_widget_set_size_request(GTK_WIDGET_PTR(mainwin->messageview),
				    prefs_common.msgview_width,
				    prefs_common.msgview_height);

	/* create separated window(s) if needed */
	if (type & SEPARATE_FOLDER) {
		static GdkGeometry folderwin_geometry;
		
		folderwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(folderwin),
				     _("Sylpheed-Claws - Folder View"));

		gtk_window_move(GTK_WINDOW(folderwin),
				prefs_common.folderwin_x,
				prefs_common.folderwin_y);

		if (!folderwin_geometry.min_height) {
			folderwin_geometry.min_width = 320;
			folderwin_geometry.min_height = 200;
		}
		gtk_window_set_geometry_hints(GTK_WINDOW(folderwin), NULL,
					      &folderwin_geometry, GDK_HINT_MIN_SIZE);
				
		gtk_widget_set_size_request(folderwin,
					    prefs_common.folderview_width,
					    prefs_common.folderview_height);

		gtk_container_set_border_width(GTK_CONTAINER(folderwin),
					       BORDER_WIDTH);

		g_signal_connect(G_OBJECT(folderwin), "delete_event",
				 G_CALLBACK(folder_window_close_cb),
				   mainwin);
		gtk_container_add(GTK_CONTAINER(folderwin),
				  GTK_WIDGET_PTR(mainwin->folderview));
		gtk_widget_realize(folderwin);

		if (prefs_common.folderview_visible)
			gtk_widget_show(folderwin);
	}

	if (type & SEPARATE_MESSAGE) {
		static GdkGeometry msgwin_geometry;
		
		messagewin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(messagewin),
				     _("Sylpheed-Claws - Message View"));
				     
		gtk_window_move(GTK_WINDOW(messagewin), 
				prefs_common.main_msgwin_x,
				prefs_common.main_msgwin_y);

		if (!msgwin_geometry.min_height) {
			msgwin_geometry.min_width = 320;
			msgwin_geometry.min_height = 200;
		}
		gtk_window_set_geometry_hints(GTK_WINDOW(messagewin), NULL,
					      &msgwin_geometry, GDK_HINT_MIN_SIZE);
		
		gtk_widget_set_size_request(messagewin, 
					    prefs_common.msgwin_width,
					    prefs_common.msgwin_height);

		gtk_container_set_border_width(GTK_CONTAINER(messagewin),
					       BORDER_WIDTH);
		g_signal_connect(G_OBJECT(messagewin), "delete_event",
				 G_CALLBACK(message_window_close_cb),
				 mainwin);
		if (messageview_is_visible(mainwin->messageview))
			gtk_widget_show(messagewin);
	} else {
		mainwin->messageview->statusbar = mainwin->statusbar;
		mainwin->messageview->statusbar_cid = mainwin->messageview_cid;
	}

	switch (type) {
	case SEPARATE_NONE:
		hpaned = gtk_hpaned_new();
		gtk_box_pack_start(GTK_BOX(vbox_body), hpaned, TRUE, TRUE, 0);
		gtk_paned_add1(GTK_PANED(hpaned),
			       GTK_WIDGET_PTR(mainwin->folderview));
		gtk_widget_show(hpaned);
		gtk_widget_queue_resize(hpaned);

		vpaned = gtk_vpaned_new();
		if (messageview_is_visible(mainwin->messageview)) {
			gtk_paned_add2(GTK_PANED(hpaned), vpaned);
			gtk_paned_add1(GTK_PANED(vpaned),
				       GTK_WIDGET_PTR(mainwin->summaryview));
		} else {
			gtk_paned_add2(GTK_PANED(hpaned),
				       GTK_WIDGET_PTR(mainwin->summaryview));
			gtk_widget_ref(vpaned);
		}
		gtk_paned_add2(GTK_PANED(vpaned),
			       GTK_WIDGET_PTR(mainwin->messageview));
		gtk_widget_show(vpaned);
		gtk_widget_queue_resize(vpaned);

		mainwin->win.sep_none.hpaned = hpaned;
		mainwin->win.sep_none.vpaned = vpaned;
		
		/* remove headerview if not in prefs */
		headerview_set_visibility(mainwin->messageview->headerview,
					  prefs_common.display_header_pane);
		break;
	case SEPARATE_FOLDER:
		vpaned = gtk_vpaned_new();
		if (messageview_is_visible(mainwin->messageview)) {
			gtk_box_pack_start(GTK_BOX(vbox_body), vpaned,
					   TRUE, TRUE, 0);
			gtk_paned_add1(GTK_PANED(vpaned),
				       GTK_WIDGET_PTR(mainwin->summaryview));
		} else {
			gtk_box_pack_start(GTK_BOX(vbox_body),
					   GTK_WIDGET_PTR(mainwin->summaryview),
					   TRUE, TRUE, 0);
			gtk_widget_ref(vpaned);
		}
		gtk_paned_add2(GTK_PANED(vpaned),
			       GTK_WIDGET_PTR(mainwin->messageview));
		gtk_widget_show(vpaned);
		gtk_widget_queue_resize(vpaned);

		mainwin->win.sep_folder.folderwin = folderwin;
		mainwin->win.sep_folder.vpaned    = vpaned;

		/* remove headerview if not in prefs */
		headerview_set_visibility(mainwin->messageview->headerview,
					  prefs_common.display_header_pane);
		
		break;
	case SEPARATE_MESSAGE:
		hpaned = gtk_hpaned_new();
		gtk_box_pack_start(GTK_BOX(vbox_body), hpaned, TRUE, TRUE, 0);
		gtk_paned_add1(GTK_PANED(hpaned),
			       GTK_WIDGET_PTR(mainwin->folderview));
		gtk_paned_add2(GTK_PANED(hpaned),
			       GTK_WIDGET_PTR(mainwin->summaryview));
		gtk_widget_show(hpaned);
		gtk_widget_queue_resize(hpaned);
		messageview_add_toolbar(mainwin->messageview, messagewin);
		msgview_ifactory = gtk_item_factory_from_widget(mainwin->messageview->menubar);
		menu_set_sensitive(msgview_ifactory, "/File/Close", FALSE);

		mainwin->win.sep_message.messagewin = messagewin;
		mainwin->win.sep_message.hpaned     = hpaned;

		gtk_widget_realize(messagewin);
		gtk_widget_show_all(GTK_WIDGET_PTR(mainwin->messageview));
		gtk_widget_show_all(messagewin);
		toolbar_set_style(mainwin->messageview->toolbar->toolbar, 
				  mainwin->messageview->handlebox, 
				  prefs_common.toolbar_style);

		break;
	case SEPARATE_BOTH:
		messageview_add_toolbar(mainwin->messageview, messagewin);
		msgview_ifactory = gtk_item_factory_from_widget(mainwin->messageview->menubar);
		menu_set_sensitive(msgview_ifactory, "/File/Close", FALSE);

		gtk_box_pack_start(GTK_BOX(vbox_body),
				   GTK_WIDGET_PTR(mainwin->summaryview),
				   TRUE, TRUE, 0);
		
		mainwin->win.sep_both.folderwin = folderwin;
		mainwin->win.sep_both.messagewin = messagewin;
		
		gtk_widget_realize(messagewin);
		gtk_widget_show_all(GTK_WIDGET_PTR(mainwin->messageview));
		gtk_widget_show_all(messagewin);
		toolbar_set_style(mainwin->messageview->toolbar->toolbar, 
				  mainwin->messageview->handlebox, 
				  prefs_common.toolbar_style);		

		break;
	}

	if (messageview_is_visible(mainwin->messageview))
		gtk_arrow_set(GTK_ARROW(mainwin->summaryview->toggle_arrow),
			      GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	else
		gtk_arrow_set(GTK_ARROW(mainwin->summaryview->toggle_arrow),
			      GTK_ARROW_UP, GTK_SHADOW_OUT);

	gtk_window_move(GTK_WINDOW(mainwin->window),
			prefs_common.mainwin_x,
			prefs_common.mainwin_y);

	gtk_widget_queue_resize(vbox_body);
	gtk_widget_queue_resize(mainwin->vbox);
	gtk_widget_queue_resize(mainwin->window);
	/* CLAWS: previous "gtk_widget_show_all" makes noticeview
	 * and mimeview icon list/ctree lose track of their visibility states */
	if (!noticeview_is_visible(mainwin->messageview->noticeview)) 
		gtk_widget_hide(GTK_WIDGET_PTR(mainwin->messageview->noticeview));
	if (!noticeview_is_visible(mainwin->messageview->mimeview->siginfoview)) 
		gtk_widget_hide(GTK_WIDGET_PTR(mainwin->messageview->mimeview->siginfoview));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mainwin->messageview->mimeview->mime_toggle)))
		gtk_widget_hide(mainwin->messageview->mimeview->icon_mainbox);
	else 
		gtk_widget_hide(mainwin->messageview->mimeview->ctree_mainbox);

	mainwin->type = type;


	/* toggle menu state */
	menuitem = gtk_item_factory_get_item
		(ifactory, "/View/Show or hide/Folder tree");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
				       (type & SEPARATE_FOLDER) == 0 ? TRUE :
				       prefs_common.folderview_visible);
	gtk_widget_set_sensitive(menuitem, ((type & SEPARATE_FOLDER) != 0));
	menuitem = gtk_item_factory_get_item
		(ifactory, "/View/Show or hide/Message view");
	gtk_check_menu_item_set_active
		(GTK_CHECK_MENU_ITEM(menuitem),
		 messageview_is_visible(mainwin->messageview));

	menuitem = gtk_item_factory_get_item
		(ifactory, "/View/Separate folder tree");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
				       ((type & SEPARATE_FOLDER) != 0));
	menuitem = gtk_item_factory_get_item
		(ifactory, "/View/Separate message view");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
				       ((type & SEPARATE_MESSAGE) != 0));

	if (folderwin) {
		g_signal_connect
			(G_OBJECT(folderwin), "size_allocate",
			 G_CALLBACK(folder_window_size_allocate_cb),
			 mainwin);
	}
	if (messagewin) {
		g_signal_connect
			(G_OBJECT(messagewin), "size_allocate",
			 G_CALLBACK(message_window_size_allocate_cb),
			 mainwin);
	}

	debug_print("done.\n");
}

void main_window_destroy_all(void)
{
	while (mainwin_list != NULL) {
		MainWindow *mainwin = (MainWindow*)mainwin_list->data;
		
		/* free toolbar stuff */
		toolbar_clear_list(TOOLBAR_MAIN);
		TOOLBAR_DESTROY_ACTIONS(mainwin->toolbar->action_list);
		TOOLBAR_DESTROY_ITEMS(mainwin->toolbar->item_list);

		g_free(mainwin->toolbar);
		g_free(mainwin);
		
		mainwin_list = g_list_remove(mainwin_list, mainwin);
	}
	g_list_free(mainwin_list);
}

#if 0
static gboolean toolbar_account_button_pressed(GtkWidget *widget,
					       GdkEventButton *event,
					       gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;

	if (!event) return FALSE;
	if (event->button != 3) return FALSE;

	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NORMAL);
	g_object_set_data(G_OBJECT(mainwin->ac_menu), "menu_button",
			  widget);

	gtk_menu_popup(GTK_MENU(mainwin->ac_menu), NULL, NULL,
		       menu_button_position, widget,
		       event->button, event->time);

	return FALSE;
}
#endif

static void toolbar_child_attached(GtkWidget *widget, GtkWidget *child,
				   gpointer data)
{
	gtk_widget_set_size_request(child, 1, -1);
}

static void toolbar_child_detached(GtkWidget *widget, GtkWidget *child,
				   gpointer data)
{
	gtk_widget_set_size_request(child, -1, -1);
}

static gboolean ac_label_button_pressed(GtkWidget *widget, GdkEventButton *event,
				    gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;

	if (!event) return FALSE;

	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NORMAL);
	g_object_set_data(G_OBJECT(mainwin->ac_menu), "menu_button",
			  widget);

	gtk_menu_popup(GTK_MENU(mainwin->ac_menu), NULL, NULL,
		       menu_button_position, widget,
		       event->button, event->time);

	return TRUE;
}

static gint main_window_close_cb(GtkWidget *widget, GdkEventAny *event,
				 gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;

	if (mainwin->lock_count == 0)
		app_exit_cb(data, 0, widget);

	return TRUE;
}

static gint folder_window_close_cb(GtkWidget *widget, GdkEventAny *event,
				   gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;
	GtkWidget *menuitem;

	menuitem = gtk_item_factory_get_item
		(mainwin->menu_factory, "/View/Show or hide/Folder tree");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), FALSE);

	return TRUE;
}

static gint message_window_close_cb(GtkWidget *widget, GdkEventAny *event,
				    gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;
	GtkWidget *menuitem;

	menuitem = gtk_item_factory_get_item
		(mainwin->menu_factory, "/View/Show or hide/Message view");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), FALSE);

	mainwin->messageview->statusbar = mainwin->statusbar;
	mainwin->messageview->statusbar_cid = mainwin->messageview_cid;

	return TRUE;
}

static void main_window_size_allocate_cb(GtkWidget *widget,
					 GtkAllocation *allocation,
					 gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;

	main_window_get_size(mainwin);
}

static void folder_window_size_allocate_cb(GtkWidget *widget,
					   GtkAllocation *allocation,
					   gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;

	main_window_get_size(mainwin);
}

static void message_window_size_allocate_cb(GtkWidget *widget,
					    GtkAllocation *allocation,
					    gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;

	main_window_get_size(mainwin);
}

static void add_mailbox_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	main_window_add_mailbox(mainwin);
}

static void update_folderview_cb(MainWindow *mainwin, guint action,
				 GtkWidget *widget)
{
	summary_show(mainwin->summaryview, NULL);
	folderview_check_new_all();
}

static void foldersort_cb(MainWindow *mainwin, guint action,
                           GtkWidget *widget)
{
	foldersort_open();
}

static void import_mbox_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	import_mbox(mainwin->summaryview->folder_item);
}

static void export_mbox_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	export_mbox(mainwin->summaryview->folder_item);
}

static void export_list_mbox_cb(MainWindow *mainwin, guint action,
				GtkWidget *widget)
{
	summaryview_export_mbox_list(mainwin->summaryview);
}

static void empty_trash_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	main_window_empty_trash(mainwin, TRUE);
}

static void save_as_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_save_as(mainwin->summaryview);
}

static void print_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_print(mainwin->summaryview);
}

static void app_exit_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	if (prefs_common.confirm_on_exit) {
		if (alertpanel(_("Exit"), _("Exit Sylpheed-Claws?"),
			       GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL)
		    != G_ALERTDEFAULT)
			return;
		manage_window_focus_in(mainwin->window, NULL, NULL);
	}

	app_will_exit(widget, mainwin);
}

static void search_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	if (action == 1)
		summary_search(mainwin->summaryview);
	else
		message_search(mainwin->messageview);
}

static void mainwindow_quicksearch(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	prefs_common.show_searchbar = TRUE;
	summaryview_activate_quicksearch(mainwin->summaryview);
}

static void toggle_folder_cb(MainWindow *mainwin, guint action,
			     GtkWidget *widget)
{
	gboolean active;

	active = GTK_CHECK_MENU_ITEM(widget)->active;

	switch (mainwin->type) {
	case SEPARATE_NONE:
	case SEPARATE_MESSAGE:
#if 0
		if (active)
			gtk_widget_show(GTK_WIDGET_PTR(mainwin->folderview));
		else
			gtk_widget_hide(GTK_WIDGET_PTR(mainwin->folderview));
#endif
		break;
	case SEPARATE_FOLDER:
		debug_print("separate folder\n");
		if (active)
			gtk_widget_show(mainwin->win.sep_folder.folderwin);
		else
			gtk_widget_hide(mainwin->win.sep_folder.folderwin);
		break;
	case SEPARATE_BOTH:
		if (active)
			gtk_widget_show(mainwin->win.sep_both.folderwin);
		else
			gtk_widget_hide(mainwin->win.sep_both.folderwin);
		break;
	}

	prefs_common.folderview_visible = active;
}

static void toggle_message_cb(MainWindow *mainwin, guint action,
			      GtkWidget *widget)
{
	gboolean active;

	active = GTK_CHECK_MENU_ITEM(widget)->active;

	if (active != messageview_is_visible(mainwin->messageview))
		summary_toggle_view(mainwin->summaryview);
}

static void toggle_toolbar_cb(MainWindow *mainwin, guint action,
			      GtkWidget *widget)
{
	toolbar_toggle(action, mainwin);
}

void main_window_reply_cb(MainWindow *mainwin, guint action,
			  GtkWidget *widget)
{
	MessageView *msgview = (MessageView*)mainwin->messageview;
	GSList *msginfo_list = NULL;

	g_return_if_fail(msgview != NULL);

	msginfo_list = summary_get_selection(mainwin->summaryview);
	g_return_if_fail(msginfo_list != NULL);
	compose_reply_from_messageview(msgview, msginfo_list, action);
	g_slist_free(msginfo_list);
}


static void toggle_statusbar_cb(MainWindow *mainwin, guint action,
				GtkWidget *widget)
{
	if (GTK_CHECK_MENU_ITEM(widget)->active) {
		gtk_widget_show(mainwin->hbox_stat);
		prefs_common.show_statusbar = TRUE;
	} else {
		gtk_widget_hide(mainwin->hbox_stat);
		prefs_common.show_statusbar = FALSE;
	}
}

static void separate_widget_cb(MainWindow *mainwin, guint action,
			       GtkWidget *widget)
{
	SeparateType type;

	if (GTK_CHECK_MENU_ITEM(widget)->active)
		type = mainwin->type | action;
	else
		type = mainwin->type & ~action;

	main_window_separation_change(mainwin, type);

	prefs_common.sep_folder = (type & SEPARATE_FOLDER)  != 0;
	prefs_common.sep_msg    = (type & SEPARATE_MESSAGE) != 0;
}

void main_window_toggle_work_offline (MainWindow *mainwin, gboolean offline,
					gboolean ask_sync)
{
	offline_ask_sync = ask_sync;
	if (offline)
		online_switch_clicked (GTK_BUTTON(mainwin->online_switch), mainwin);
	else
		online_switch_clicked (GTK_BUTTON(mainwin->offline_switch), mainwin);
	offline_ask_sync = TRUE;
}

static void toggle_work_offline_cb (MainWindow *mainwin, guint action, GtkWidget *widget)
{
	main_window_toggle_work_offline(mainwin, GTK_CHECK_MENU_ITEM(widget)->active, TRUE);
}

static void mainwindow_check_synchronise(MainWindow *mainwin, gboolean ask)
{
	GList *folderlist = folder_get_list();
	gboolean found = FALSE;

	/* see if there are synchronised folders */
	for (; folderlist; folderlist = folderlist->next) {
		Folder *folder = (Folder *)folderlist->data;
		if (folder_want_synchronise(folder)) {
			found = TRUE;
			break;
		}
	}
	
	if (!found)
		return;
		
	if (offline_ask_sync && ask && alertpanel(_("Folder synchronisation"),
			_("Do you want to synchronise your folders now?"),
			GTK_STOCK_YES, GTK_STOCK_NO, NULL) != G_ALERTDEFAULT)
		return;

	folder_synchronise(NULL);
}

static void online_switch_clicked (GtkButton *btn, gpointer data) 
{
	MainWindow *mainwin;
	GtkItemFactory *ifactory;
	GtkCheckMenuItem *menuitem;

	mainwin = (MainWindow *) data;
	
	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	menuitem = GTK_CHECK_MENU_ITEM (gtk_item_factory_get_widget(ifactory, "/File/Work offline"));
	
	g_return_if_fail(mainwin != NULL);
	g_return_if_fail(menuitem != NULL);
	
	if (btn == GTK_BUTTON(mainwin->online_switch)) {
		/* go offline */
		mainwindow_check_synchronise(mainwin, TRUE);
		gtk_widget_hide (mainwin->online_switch);
		gtk_widget_show (mainwin->offline_switch);
		menuitem->active = TRUE;
		prefs_common.work_offline = TRUE;
		inc_autocheck_timer_remove();		
	} else {
		/*go online */
		gtk_widget_hide (mainwin->offline_switch);
		gtk_widget_show (mainwin->online_switch);
		menuitem->active = FALSE;
		prefs_common.work_offline = FALSE;
		inc_autocheck_timer_set();
	}
}

static void addressbook_open_cb(MainWindow *mainwin, guint action,
				GtkWidget *widget)
{
	addressbook_open(NULL);
}

static void log_window_show_cb(MainWindow *mainwin, guint action,
			       GtkWidget *widget)
{
	log_window_show(mainwin->logwin);
}

static void inc_cancel_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	inc_cancel_all();
}

static void move_to_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_move_to(mainwin->summaryview);
}

static void copy_to_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_copy_to(mainwin->summaryview);
}

static void delete_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_delete(mainwin->summaryview);
}

static void delete_trash_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_delete_trash(mainwin->summaryview);
}

static void cancel_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_cancel(mainwin->summaryview);
}

static void open_msg_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_open_msg(mainwin->summaryview);
}

static void view_source_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	summary_view_source(mainwin->summaryview);
}

static void show_all_header_cb(MainWindow *mainwin, guint action,
			       GtkWidget *widget)
{
	if (mainwin->menu_lock_count) return;
	mainwin->summaryview->messageview->all_headers = 
			GTK_CHECK_MENU_ITEM(widget)->active;
	summary_display_msg_selected(mainwin->summaryview,
				     GTK_CHECK_MENU_ITEM(widget)->active);
}

static void mark_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_mark(mainwin->summaryview);
}

static void unmark_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_unmark(mainwin->summaryview);
}

static void mark_as_unread_cb(MainWindow *mainwin, guint action,
			      GtkWidget *widget)
{
	summary_mark_as_unread(mainwin->summaryview);
}

static void mark_as_read_cb(MainWindow *mainwin, guint action,
			    GtkWidget *widget)
{
	summary_mark_as_read(mainwin->summaryview);
}

static void mark_all_read_cb(MainWindow *mainwin, guint action,
			     GtkWidget *widget)
{
	summary_mark_all_read(mainwin->summaryview);
}

static void reedit_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_reedit(mainwin->summaryview);
}

static void add_address_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	summary_add_address(mainwin->summaryview);
}

static void set_charset_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	const gchar *str;

	if (GTK_CHECK_MENU_ITEM(widget)->active) {
		str = conv_get_charset_str((CharSet)action);
		
		g_free(mainwin->messageview->forced_charset);
		mainwin->messageview->forced_charset = str ? g_strdup(str) : NULL;
		procmime_force_charset(str);
		
		summary_redisplay_msg(mainwin->summaryview);
		
		debug_print("forced charset: %s\n", str ? str : "Auto-Detect");
	}
}

static void set_decode_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	if (GTK_CHECK_MENU_ITEM(widget)->active) {
		mainwin->messageview->forced_encoding = (EncodingType)action;
		
		summary_redisplay_msg(mainwin->summaryview);
		
		debug_print("forced encoding: %d\n", action);
	}
}

static void hide_read_messages (MainWindow *mainwin, guint action,
				GtkWidget *widget)
{
	if (!mainwin->summaryview->folder_item
	    || g_object_get_data(G_OBJECT(widget), "dont_toggle"))
		return;
	summary_toggle_show_read_messages(mainwin->summaryview);
}

static void thread_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	if (mainwin->menu_lock_count) return;
	if (!mainwin->summaryview->folder_item) return;

	if (GTK_CHECK_MENU_ITEM(widget)->active) {
		summary_thread_build(mainwin->summaryview);
/*		mainwin->summaryview->folder_item->threaded = TRUE; */
	} else {
		summary_unthread(mainwin->summaryview);
/*		mainwin->summaryview->folder_item->threaded = FALSE; */
	}
}

static void expand_threads_cb(MainWindow *mainwin, guint action,
			      GtkWidget *widget)
{
	summary_expand_threads(mainwin->summaryview);
}

static void collapse_threads_cb(MainWindow *mainwin, guint action,
				GtkWidget *widget)
{
	summary_collapse_threads(mainwin->summaryview);
}

static void set_summary_display_item_cb(MainWindow *mainwin, guint action,
				GtkWidget *widget)
{
	prefs_summary_column_open();
}

static void set_folder_display_item_cb(MainWindow *mainwin, guint action,
				GtkWidget *widget)
{
	prefs_folder_column_open();
}

static void sort_summary_cb(MainWindow *mainwin, guint action,
			    GtkWidget *widget)
{
	FolderItem *item = mainwin->summaryview->folder_item;
	GtkWidget *menuitem;

	if (mainwin->menu_lock_count) return;

	if (GTK_CHECK_MENU_ITEM(widget)->active && item) {
		menuitem = gtk_item_factory_get_item
			(mainwin->menu_factory, "/View/Sort/Ascending");
		summary_sort(mainwin->summaryview, (FolderSortKey)action,
			     GTK_CHECK_MENU_ITEM(menuitem)->active
			     ? SORT_ASCENDING : SORT_DESCENDING);
		item->sort_key = action;
	}
}

static void sort_summary_type_cb(MainWindow *mainwin, guint action,
				 GtkWidget *widget)
{
	FolderItem *item = mainwin->summaryview->folder_item;

	if (mainwin->menu_lock_count) return;

	if (GTK_CHECK_MENU_ITEM(widget)->active && item)
		summary_sort(mainwin->summaryview,
			     item->sort_key, (FolderSortType)action);
}

static void attract_by_subject_cb(MainWindow *mainwin, guint action,
				  GtkWidget *widget)
{
	summary_attract_by_subject(mainwin->summaryview);
}

static void delete_duplicated_cb(MainWindow *mainwin, guint action,
				 GtkWidget *widget)
{
	FolderItem *item;

	item = folderview_get_selected_item(mainwin->folderview);
	if (item) {
		main_window_cursor_wait(mainwin);
		STATUSBAR_PUSH(mainwin, _("Deleting duplicated messages..."));

		folderutils_delete_duplicates(item, prefs_common.immediate_exec ?
					      DELETE_DUPLICATES_REMOVE : DELETE_DUPLICATES_SETFLAG);

		STATUSBAR_POP(mainwin);
		main_window_cursor_normal(mainwin);
	}
}

struct DelDupsData
{
	guint	dups;
	guint	folders;
};

static void deldup_all(FolderItem *item, gpointer _data)
{
	struct DelDupsData *data = _data;
	gint result;
	
	result = folderutils_delete_duplicates(item, DELETE_DUPLICATES_REMOVE);
	if (result >= 0) {
		data->dups += result;
		data->folders += 1;
	}
}

static void delete_duplicated_all_cb(MainWindow *mainwin, guint action,
				 GtkWidget *widget)
{
	struct DelDupsData data = {0, 0};

	folder_func_to_all_folders(deldup_all, &data);
	alertpanel_notice(ngettext("Deleted %d duplicate message in %d folders.\n",
				   "Deleted %d duplicate messages in %d folders.\n",
				   data.dups),
			  data.dups, data.folders);
}

static void filter_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_filter(mainwin->summaryview, (gboolean)action);
}

static void execute_summary_cb(MainWindow *mainwin, guint action,
			       GtkWidget *widget)
{
	summary_execute(mainwin->summaryview);
}

static void update_summary_cb(MainWindow *mainwin, guint action,
			      GtkWidget *widget)
{
	FolderItem *fitem;
	FolderView *folderview = mainwin->folderview;

	if (!mainwin->summaryview->folder_item) return;
	if (!folderview->opened) return;

	folder_update_op_count();

	fitem = gtk_ctree_node_get_row_data(GTK_CTREE(folderview->ctree),
					    folderview->opened);
	if (!fitem) return;

	folder_item_scan(fitem);
	summary_show(mainwin->summaryview, fitem);
}

static void prev_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_step(mainwin->summaryview, GTK_SCROLL_STEP_BACKWARD);
}

static void next_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_step(mainwin->summaryview, GTK_SCROLL_STEP_FORWARD);
}

static void prev_unread_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	summary_select_prev_unread(mainwin->summaryview);
}

static void next_unread_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	summary_select_next_unread(mainwin->summaryview);
}

static void prev_new_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_select_prev_new(mainwin->summaryview);
}

static void next_new_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	summary_select_next_new(mainwin->summaryview);
}

static void prev_marked_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	summary_select_prev_marked(mainwin->summaryview);
}

static void next_marked_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	summary_select_next_marked(mainwin->summaryview);
}

static void prev_labeled_cb(MainWindow *mainwin, guint action,
			    GtkWidget *widget)
{
	summary_select_prev_labeled(mainwin->summaryview);
}

static void next_labeled_cb(MainWindow *mainwin, guint action,
			    GtkWidget *widget)
{
	summary_select_next_labeled(mainwin->summaryview);
}

static void goto_folder_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	FolderItem *to_folder;

	to_folder = foldersel_folder_sel(NULL, FOLDER_SEL_ALL, NULL);

	if (to_folder)
		folderview_select(mainwin->folderview, to_folder);
}

static void copy_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	messageview_copy_clipboard(mainwin->messageview);
}

static void allsel_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	MessageView *msgview = mainwin->messageview;

	if (GTK_WIDGET_HAS_FOCUS(mainwin->summaryview->ctree))
		summary_select_all(mainwin->summaryview);
	else if (messageview_is_visible(msgview) &&
		 (GTK_WIDGET_HAS_FOCUS(msgview->mimeview->textview->text)))
		messageview_select_all(mainwin->messageview);
}

static void select_thread_cb(MainWindow *mainwin, guint action,
			     GtkWidget *widget)
{
	summary_select_thread(mainwin->summaryview);
}

static void create_filter_cb(MainWindow *mainwin, guint action,
			     GtkWidget *widget)
{
	summary_filter_open(mainwin->summaryview, (PrefsFilterType)action, 0);
}

static void create_processing_cb(MainWindow *mainwin, guint action,
			     GtkWidget *widget)
{
	summary_filter_open(mainwin->summaryview, (PrefsFilterType)action, 1);
}

static void prefs_pre_processing_open_cb(MainWindow *mainwin, guint action,
				         GtkWidget *widget)
{
	prefs_filtering_open(&pre_global_processing,
			     _("Processing rules to apply before folder rules"),
			     NULL, NULL);
}

static void prefs_post_processing_open_cb(MainWindow *mainwin, guint action,
				          GtkWidget *widget)
{
	prefs_filtering_open(&post_global_processing,
			     _("Processing rules to apply after folder rules"),
			     NULL, NULL);
}

static void prefs_filtering_open_cb(MainWindow *mainwin, guint action,
				    GtkWidget *widget)
{
	prefs_filtering_open(&filtering_rules,
			     _("Filtering configuration"),
			     NULL, NULL);
}

static void prefs_template_open_cb(MainWindow *mainwin, guint action,
				   GtkWidget *widget)
{
	prefs_template_open();
}

static void prefs_actions_open_cb(MainWindow *mainwin, guint action,
				  GtkWidget *widget)
{
	prefs_actions_open(mainwin);
}
#ifdef USE_OPENSSL
static void ssl_manager_open_cb(MainWindow *mainwin, guint action,
				  GtkWidget *widget)
{
	ssl_manager_open(mainwin);
}
#endif
static void prefs_account_open_cb(MainWindow *mainwin, guint action,
				  GtkWidget *widget)
{
	if (!cur_account) {
		new_account_cb(mainwin, 0, widget);
	} else {
		account_open(cur_account);
	}
}

static void new_account_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	account_edit_open();
	if (!compose_get_compose_list()) account_add();
}

static void account_selector_menu_cb(GtkMenuItem *menuitem, gpointer data)
{
	FolderItem *item = NULL;
	cur_account = (PrefsAccount *)data;
	
	if (!mainwindow_get_mainwindow())
		return;
	main_window_show_cur_account(mainwindow_get_mainwindow());
	toolbar_update(TOOLBAR_MAIN, mainwindow_get_mainwindow());
	main_window_set_menu_sensitive(mainwindow_get_mainwindow());
	toolbar_main_set_sensitive(mainwindow_get_mainwindow());
	item = folderview_get_selected_item(
			mainwindow_get_mainwindow()->folderview);
	if (item) {
		toolbar_set_compose_button
			(mainwindow_get_mainwindow()->toolbar,
			 FOLDER_TYPE(item->folder) == F_NEWS ? 
			 COMPOSEBUTTON_NEWS : COMPOSEBUTTON_MAIL);
	}
}

static void account_receive_menu_cb(GtkMenuItem *menuitem, gpointer data)
{
	MainWindow *mainwin = (MainWindow *)mainwin_list->data;
	PrefsAccount *account = (PrefsAccount *)data;

	inc_account_mail(mainwin, account);
}

static void prefs_open_cb(GtkMenuItem *menuitem, gpointer data)
{
	prefs_gtk_open();
}

static void plugins_open_cb(GtkMenuItem *menuitem, gpointer data)
{
	pluginwindow_create();
}

static void manual_open_cb(MainWindow *mainwin, guint action,
			   GtkWidget *widget)
{
	manual_open((ManualType)action);
}

static void scan_tree_func(Folder *folder, FolderItem *item, gpointer data)
{
	MainWindow *mainwin = (MainWindow *)data;
	gchar *str;

	if (item->path)
		str = g_strdup_printf(_("Scanning folder %s%c%s ..."),
				      LOCAL_FOLDER(folder)->rootpath,
				      G_DIR_SEPARATOR,
				      item->path);
	else
		str = g_strdup_printf(_("Scanning folder %s ..."),
				      LOCAL_FOLDER(folder)->rootpath);

	STATUSBAR_PUSH(mainwin, str);
	STATUSBAR_POP(mainwin);
	g_free(str);
}

static gboolean mainwindow_focus_in_event(GtkWidget *widget, GdkEventFocus *focus,
					  gpointer data)
{
	SummaryView *summary;

	g_return_val_if_fail(data, FALSE);
	summary = ((MainWindow *)data)->summaryview;
	g_return_val_if_fail(summary, FALSE);

	if (GTK_CLIST(summary->ctree)->selection && 
	    g_list_length(GTK_CLIST(summary->ctree)->selection) > 1)
		return FALSE;

	if (summary->selected != summary->displayed)
		summary_select_node(summary, summary->displayed, FALSE, TRUE);
	return FALSE;
}

#define BREAK_ON_MODIFIER_KEY() \
	if ((event->state & (GDK_MOD1_MASK|GDK_CONTROL_MASK)) != 0) break

gboolean mainwindow_key_pressed (GtkWidget *widget, GdkEventKey *event,
				    gpointer data)
{
	MainWindow *mainwin = (MainWindow*) data;
	
	if (!mainwin || !event) 
		return FALSE;

	if (quicksearch_has_focus(mainwin->summaryview->quicksearch))
		return FALSE;

	switch (event->keyval) {
	case GDK_Q:             /* Quit */
		BREAK_ON_MODIFIER_KEY();

		app_exit_cb(mainwin, 0, NULL);
		return FALSE;
	case GDK_space:
		if (mainwin->folderview && mainwin->summaryview
		    && !mainwin->summaryview->displayed) {
			summary_lock(mainwin->summaryview);
			folderview_select_next_unread(mainwin->folderview);
			summary_unlock(mainwin->summaryview);
		}
		break;
	default:
		break;
	}
	return FALSE;
}

#undef BREAK_ON_MODIFIER_KEY

/*
 * Harvest addresses for selected folder.
 */
static void addr_harvest_cb( MainWindow *mainwin,
			    guint action,
			    GtkWidget *widget )
{
	addressbook_harvest( mainwin->summaryview->folder_item, FALSE, NULL );
}

/*
 * Harvest addresses for selected messages in summary view.
 */
static void addr_harvest_msg_cb( MainWindow *mainwin,
			    guint action,
			    GtkWidget *widget )
{
	summary_harvest_address( mainwin->summaryview );
}

/*!
 *\brief	get a MainWindow
 *
 *\return	MainWindow * The first mainwindow in the mainwin_list
 */
MainWindow *mainwindow_get_mainwindow(void)
{
	if (mainwin_list && mainwin_list->data)
		return (MainWindow *)(mainwin_list->data);
	else
		return NULL;
}

gboolean mainwindow_progressindicator_hook(gpointer source, gpointer userdata)
{
	ProgressData *data = (ProgressData *) source;
	MainWindow *mainwin = (MainWindow *) userdata;

	switch (data->cmd) {
	case PROGRESS_COMMAND_START:
	case PROGRESS_COMMAND_STOP:
		gtk_progress_bar_set_fraction
			(GTK_PROGRESS_BAR(mainwin->progressbar), 0.0);
		break;
	case PROGRESS_COMMAND_SET_PERCENTAGE:
		gtk_progress_bar_set_fraction
			(GTK_PROGRESS_BAR(mainwin->progressbar), data->value);
		break;		
	}
	while (gtk_events_pending()) gtk_main_iteration ();

	return FALSE;
}

static void sync_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	mainwindow_check_synchronise(mainwin, FALSE);
}

/*
* End of Source.
*/

