#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include "intl.h"
#include "utils.h"
#include "defs.h"
#include "procheader.h"
#include "matcher.h"
#include "filtering.h"
#include "prefs.h"
#include "compose.h"
#include "matcher_parser.h"

#define PREFSBUFSIZE		1024

/* GSList * prefs_filtering = NULL; */
GSList * global_filtering = NULL;

FilteringAction * filteringaction_new(int type, int account_id,
				      gchar * destination)
{
	FilteringAction * action;

	action = g_new0(FilteringAction, 1);

	action->type = type;
	action->account_id = account_id;
	if (destination)
		action->destination = g_strdup(destination);

	return action;
}

void filteringaction_free(FilteringAction * action)
{
	if (action->destination)
		g_free(action->destination);
	g_free(action);
}

/*
FilteringAction * filteringaction_parse(gchar ** str)
{
	FilteringAction * action;
	gchar * tmp;
	gchar * destination = NULL;
	gint account_id = 0;
	gint key;

	tmp = * str;

	key = matcher_parse_keyword(&tmp);

	switch (key) {
	case MATCHING_ACTION_MOVE:
	case MATCHING_ACTION_COPY:
	case MATCHING_EXECUTE:
		destination = matcher_parse_str(&tmp);
		if (tmp == NULL) {
			* str = NULL;
			return NULL;
		}
		break;
	case MATCHING_ACTION_DELETE:
		break;
	case MATCHING_ACTION_MARK:
		break;
	case MATCHING_ACTION_MARK_AS_READ:
		break;
	case MATCHING_ACTION_UNMARK:
		break;
	case MATCHING_ACTION_MARK_AS_UNREAD:
		break;
	case MATCHING_ACTION_FORWARD:
	case MATCHING_ACTION_FORWARD_AS_ATTACHMENT:
		account_id = matcher_parse_number(&tmp);
		if (tmp == NULL) {
			* str = NULL;
			return NULL;
		}

		destination = matcher_parse_str(&tmp);
		if (tmp == NULL) {
			* str = NULL;
			return NULL;
		}

		break;
	default:
		* str = NULL;
		return NULL;
	}

	* str = tmp;
	action = filteringaction_new(key, account_id, destination);

	return action;
}

FilteringProp * filteringprop_parse(gchar ** str)
{
	gchar * tmp;
	gint key;
	FilteringProp * filtering;
	MatcherList * matchers;
	FilteringAction * action;
	
	tmp = * str;

	matchers = matcherlist_parse(&tmp);
	if (tmp == NULL) {
		* str = NULL;
		return NULL;
	}

	if (tmp == NULL) {
		matcherlist_free(matchers);
		* str = NULL;
		return NULL;
	}

	action = filteringaction_parse(&tmp);
	if (tmp == NULL) {
		matcherlist_free(matchers);
		* str = NULL;
		return NULL;
	}

	filtering = filteringprop_new(matchers, action);

	* str = tmp;
	return filtering;
}
*/

FilteringProp * filteringprop_new(MatcherList * matchers,
				  FilteringAction * action)
{
	FilteringProp * filtering;

	filtering = g_new0(FilteringProp, 1);
	filtering->matchers = matchers;
	filtering->action = action;

	return filtering;
}

void filteringprop_free(FilteringProp * prop)
{
	matcherlist_free(prop->matchers);
	filteringaction_free(prop->action);
	g_free(prop);
}

static gboolean filteringaction_update_mark(MsgInfo * info)
{
	gchar * dest_path;
	FILE * fp;

	if (info->folder->folder->type == F_MH) {
		dest_path = folder_item_get_path(info->folder);
		if (!is_dir_exist(dest_path))
			make_dir_hier(dest_path);
		
		if (dest_path == NULL) {
			g_warning(_("Can't open mark file.\n"));
			return FALSE;
		}
		
		if ((fp = procmsg_open_mark_file(dest_path, TRUE))
		    == NULL) {
			g_warning(_("Can't open mark file.\n"));
			return FALSE;
		}
		
		procmsg_write_flags(info, fp);
		fclose(fp);
		return TRUE;
	}
	return FALSE;
}

static gchar * filteringaction_execute_command(gchar * cmd, MsgInfo * info)
{
	gchar * s = cmd;
	gchar * filename = NULL;
	gchar * processed_cmd;
	gchar * p;
	gint size;

	size = strlen(cmd) + 1;
	while (*s != '\0') {
		if (*s == '%') {
			s++;
			switch (*s) {
			case '%':
				size -= 1;
				break;
			case 's': /* subject */
				size += strlen(info->subject) - 2;
				break;
			case 'f': /* from */
				size += strlen(info->from) - 2;
				break;
			case 't': /* to */
				size += strlen(info->to) - 2;
				break;
			case 'c': /* cc */
				size += strlen(info->cc) - 2;
				break;
			case 'd': /* date */
				size += strlen(info->date) - 2;
				break;
			case 'i': /* message-id */
				size += strlen(info->msgid) - 2;
				break;
			case 'n': /* newsgroups */
				size += strlen(info->newsgroups) - 2;
				break;
			case 'r': /* references */
				size += strlen(info->references) - 2;
				break;
			case 'F': /* file */
				filename = folder_item_fetch_msg(info->folder,
								 info->msgnum);
				
				if (filename == NULL) {
					g_warning(_("filename is not set"));
					return NULL;
				}
				else
					size += strlen(filename) - 2;
				break;
			}
			s++;
		}
		else s++;
	}


	processed_cmd = g_new0(gchar, size);
	s = cmd;
	p = processed_cmd;

	while (*s != '\0') {
		if (*s == '%') {
			s++;
			switch (*s) {
			case '%':
				*p = '%';
				p++;
				break;
			case 's': /* subject */
				if (info->subject != NULL)
					strcpy(p, info->subject);
				else
					strcpy(p, "(none)");
				p += strlen(p);
				break;
			case 'f': /* from */
				if (info->from != NULL)
					strcpy(p, info->from);
				else
					strcpy(p, "(none)");
				p += strlen(p);
				break;
			case 't': /* to */
				if (info->to != NULL)
					strcpy(p, info->to);
				else
					strcpy(p, "(none)");
				p += strlen(p);
				break;
			case 'c': /* cc */
				if (info->cc != NULL)
					strcpy(p, info->cc);
				else
					strcpy(p, "(none)");
				p += strlen(p);
				break;
			case 'd': /* date */
				if (info->date != NULL)
					strcpy(p, info->date);
				else
					strcpy(p, "(none)");
				p += strlen(p);
				break;
			case 'i': /* message-id */
				if (info->msgid != NULL)
					strcpy(p, info->msgid);
				else
					strcpy(p, "(none)");
				p += strlen(p);
				break;
			case 'n': /* newsgroups */
				if (info->newsgroups != NULL)
					strcpy(p, info->newsgroups);
				else
					strcpy(p, "(none)");
				p += strlen(p);
				break;
			case 'r': /* references */
				if (info->references != NULL)
					strcpy(p, info->references);
				else
					strcpy(p, "(none)");
				p += strlen(p);
				break;
			case 'F': /* file */
				strcpy(p, filename);
				p += strlen(p);
				break;
			default:
				*p = '%';
				p++;
				*p = *s;
				p++;
				break;
			}
			s++;
		}
		else {
			*p = *s;
			p++;
			s++;
		}
	}
	return processed_cmd;
}

/*
  fitleringaction_apply
  runs the action on one MsgInfo
  return value : return TRUE if the action could be applied
*/

#define CHANGE_FLAGS(msginfo) \
{ \
if (msginfo->folder->folder->change_flags != NULL) \
msginfo->folder->folder->change_flags(msginfo->folder->folder, \
				      msginfo->folder, \
				      msginfo); \
}

static gboolean filteringaction_apply(FilteringAction * action, MsgInfo * info,
				      GHashTable *folder_table)
{
	FolderItem * dest_folder;
	gint val;
	Compose * compose;
	PrefsAccount * account;
	gchar * cmd;

	switch(action->type) {
	case MATCHACTION_MOVE:
		dest_folder =
			folder_find_item_from_identifier(action->destination);
		if (!dest_folder)
			return FALSE;

		if (folder_item_move_msg(dest_folder, info) == -1)
			return FALSE;

		info->flags = 0;
		filteringaction_update_mark(info);
		
		if (folder_table) {
			val = GPOINTER_TO_INT(g_hash_table_lookup
					      (folder_table, dest_folder));
			if (val == 0) {
				folder_item_scan(dest_folder);
				g_hash_table_insert(folder_table, dest_folder,
						    GINT_TO_POINTER(1));
			}
			val = GPOINTER_TO_INT(g_hash_table_lookup
					      (folder_table, info->folder));
			if (val == 0) {
				folder_item_scan(info->folder);
				g_hash_table_insert(folder_table, info->folder,
						    GINT_TO_POINTER(1));
			}
		}

		return TRUE;

	case MATCHACTION_COPY:
		dest_folder =
			folder_find_item_from_identifier(action->destination);
		if (!dest_folder)
			return FALSE;

		if (folder_item_copy_msg(dest_folder, info) == -1)
			return FALSE;

		if (folder_table) {
			val = GPOINTER_TO_INT(g_hash_table_lookup
					      (folder_table, dest_folder));
			if (val == 0) {
				folder_item_scan(dest_folder);
				g_hash_table_insert(folder_table, dest_folder,
						    GINT_TO_POINTER(1));
			}
		}
			
		return TRUE;

	case MATCHACTION_DELETE:
		if (folder_item_remove_msg(info->folder, info->msgnum) == -1)
			return FALSE;

		info->flags = 0;
		filteringaction_update_mark(info);

		return TRUE;

	case MATCHACTION_MARK:
		MSG_SET_FLAGS(info->flags, MSG_MARKED);
		filteringaction_update_mark(info);

		CHANGE_FLAGS(info);

		return TRUE;

	case MATCHACTION_UNMARK:
		MSG_UNSET_FLAGS(info->flags, MSG_MARKED);
		filteringaction_update_mark(info);

		CHANGE_FLAGS(info);

		return TRUE;
		
	case MATCHACTION_MARK_AS_READ:
		MSG_UNSET_FLAGS(info->flags, MSG_UNREAD | MSG_NEW);
		filteringaction_update_mark(info);

		CHANGE_FLAGS(info);

		return TRUE;

	case MATCHACTION_MARK_AS_UNREAD:
		MSG_SET_FLAGS(info->flags, MSG_UNREAD | MSG_NEW);
		filteringaction_update_mark(info);

		CHANGE_FLAGS(info);
		
		return TRUE;

	case MATCHACTION_FORWARD:

		account = account_find_from_id(action->account_id);
		compose = compose_forward(account, info, FALSE);
		if (compose->account->protocol == A_NNTP)
			compose_entry_append(compose, action->destination,
					     COMPOSE_NEWSGROUPS);
		else
			compose_entry_append(compose, action->destination,
					     COMPOSE_TO);

		val = compose_send(compose);
		if (val == 0) {
			gtk_widget_destroy(compose->window);
			return TRUE;
		}

		gtk_widget_destroy(compose->window);
		return FALSE;

	case MATCHACTION_FORWARD_AS_ATTACHMENT:

		account = account_find_from_id(action->account_id);
		compose = compose_forward(account, info, TRUE);
		if (compose->account->protocol == A_NNTP)
			compose_entry_append(compose, action->destination,
					     COMPOSE_NEWSGROUPS);
		else
			compose_entry_append(compose, action->destination,
					     COMPOSE_TO);

		val = compose_send(compose);
		if (val == 0) {
			gtk_widget_destroy(compose->window);
			return TRUE;
		}

		gtk_widget_destroy(compose->window);
		return FALSE;

	case MATCHACTION_EXECUTE:

		cmd = matching_build_command(action->destination, info);
		if (cmd == NULL)
			return TRUE;
		else {
			system(cmd);
			g_free(cmd);
		}

		return TRUE;

	default:
		return FALSE;
	}
}

/*
  filteringprop_apply
  runs the action on one MsgInfo if it matches the criterium
  return value : return TRUE if the action doesn't allow more actions
*/

static gboolean filteringprop_apply(FilteringProp * filtering, MsgInfo * info,
				    GHashTable *folder_table)
{
	if (matcherlist_match(filtering->matchers, info)) {
		gint result;
		gchar * action_str;

		result = TRUE;

		result = filteringaction_apply(filtering->action, info,
					       folder_table);
		action_str =
			filteringaction_to_string(filtering->action);
		if (!result) {
			g_warning(_("action %s could not be applied"),
				  action_str);
		}
		else {
			debug_print(_("message %i %s..."),
				      info->msgnum, action_str);
		}

		g_free(action_str);

		switch(filtering->action->type) {
		case MATCHACTION_MOVE:
		case MATCHACTION_DELETE:
			return TRUE;
		case MATCHACTION_EXECUTE:
		case MATCHACTION_COPY:
		case MATCHACTION_MARK:
		case MATCHACTION_MARK_AS_READ:
		case MATCHACTION_UNMARK:
		case MATCHACTION_MARK_AS_UNREAD:
		case MATCHACTION_FORWARD:
		case MATCHACTION_FORWARD_AS_ATTACHMENT:
			return FALSE;
		default:
			return FALSE;
		}
	}
	else
		return FALSE;
}

void filter_msginfo(GSList * filtering_list, MsgInfo * info,
		    GHashTable *folder_table)
{
	GSList * l;

	if (info == NULL) {
		g_warning(_("msginfo is not set"));
		return;
	}
	
	for(l = filtering_list ; l != NULL ; l = g_slist_next(l)) {
		FilteringProp * filtering = (FilteringProp *) l->data;
		
		if (filteringprop_apply(filtering, info, folder_table))
			break;
	}
}

void filter_msginfo_move_or_delete(GSList * filtering_list, MsgInfo * info,
				   GHashTable *folder_table)
{
	GSList * l;

	if (info == NULL) {
		g_warning(_("msginfo is not set"));
		return;
	}
	
	for(l = filtering_list ; l != NULL ; l = g_slist_next(l)) {
		FilteringProp * filtering = (FilteringProp *) l->data;

		switch (filtering->action->type) {
		case MATCHACTION_MOVE:
		case MATCHACTION_DELETE:
			if (filteringprop_apply(filtering, info, folder_table))
				return;
		}
	}
}

void filter_message(GSList * filtering_list, FolderItem * item,
		    gint msgnum, GHashTable *folder_table)
{
	MsgInfo * msginfo;
	gchar * filename;

	if (item == NULL) {
		g_warning(_("folderitem not set"));
		return;
	}

	filename = folder_item_fetch_msg(item, msgnum);

	if (filename == NULL) {
		g_warning(_("filename is not set"));
		return;
	}

	msginfo = procheader_parse(filename, 0, TRUE);

	g_free(filename);

	if (msginfo == NULL) {
		g_warning(_("could not get info for %s"), filename);
		return;
	}

	msginfo->folder = item;
	msginfo->msgnum = msgnum;

	filter_msginfo(filtering_list, msginfo, folder_table);
}

void prefs_filtering_read_config(void)
{
	gchar *rcpath;
	FILE *fp;

	prefs_filtering_clear();

	debug_print(_("Reading filtering configuration...\n"));

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
			     FILTERING_RC, NULL);
	if ((fp = fopen(rcpath, "r")) == NULL) {
		if (ENOENT != errno) FILE_OP_ERROR(rcpath, "fopen");
		g_free(rcpath);
		return;
	}

	matcher_parserlineno = 1;

	matcher_parserrestart(fp);
	if (matcher_parserparse() != 0) {
		prefs_filtering_clear();
	}
	g_free(rcpath);
	fclose(fp);
}

/*
void prefs_filtering_read_config(void)
{
	gchar *rcpath;
	FILE *fp;
	gchar buf[PREFSBUFSIZE];

	debug_print(_("Reading filtering configuration...\n"));

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
			     FILTERING_RC, NULL);
	if ((fp = fopen(rcpath, "r")) == NULL) {
		if (ENOENT != errno) FILE_OP_ERROR(rcpath, "fopen");
		g_free(rcpath);
		prefs_filtering = NULL;
		return;
	}
	g_free(rcpath);
*/

 	/* remove all filtering */
/*
 	while (prefs_filtering != NULL) {
 		FilteringProp * filtering =
			(FilteringProp *) prefs_filtering->data;
 		filteringprop_free(filtering);
 		prefs_filtering = g_slist_remove(prefs_filtering, filtering);
 	}

 	while (fgets(buf, sizeof(buf), fp) != NULL) {
 		FilteringProp * filtering;
		gchar * tmp;

 		g_strchomp(buf);

		if ((*buf != '#') && (*buf != '\0')) {
			matcher_parse(buf);

			tmp = buf;
			filtering = filteringprop_parse(&tmp);
			if (tmp != NULL) {
				prefs_filtering =
					g_slist_append(prefs_filtering,
						       filtering);
			}
			else {
*/
				/* debug */
/*
				g_warning(_("syntax error : %s\n"), buf);
			}
		}
 	}

 	fclose(fp);
}
*/

gchar * filteringaction_to_string(FilteringAction * action)
{
	gchar * command_str;
	gint i;
	gchar * account_id_str;

	command_str = NULL;
	command_str = get_matchaction_str(action->type);

	if (command_str == NULL)
		return NULL;

	switch(action->type) {
	case MATCHACTION_MOVE:
	case MATCHACTION_COPY:
	case MATCHACTION_EXECUTE:
		return g_strconcat(command_str, " \"", action->destination,
				   "\"", NULL);

	case MATCHACTION_DELETE:
	case MATCHACTION_MARK:
	case MATCHACTION_UNMARK:
	case MATCHACTION_MARK_AS_READ:
	case MATCHACTION_MARK_AS_UNREAD:
		return g_strdup(command_str);
		break;

	case MATCHACTION_FORWARD:
	case MATCHACTION_FORWARD_AS_ATTACHMENT:
		account_id_str = itos(action->account_id);
		return g_strconcat(command_str, " ", account_id_str,
				   " \"", action->destination, "\"", NULL);

	default:
		return NULL;
	}
}

gchar * filteringprop_to_string(FilteringProp * prop)
{
	gchar * list_str;
	gchar * action_str;
	gchar * filtering_str;

	action_str = filteringaction_to_string(prop->action);

	if (action_str == NULL)
		return NULL;

	list_str = matcherlist_to_string(prop->matchers);

	if (list_str == NULL) {
		g_free(action_str);
		return NULL;
	}

	filtering_str = g_strconcat(list_str, " ", action_str, "\n", NULL);
	g_free(list_str);
	g_free(action_str);

	return filtering_str;
}

void prefs_filtering_write_config(void)
{
	gchar *rcpath;
	PrefFile *pfile;
	GSList *cur;
	FilteringProp * prop;

	debug_print(_("Writing filtering configuration...\n"));

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, FILTERING_RC, NULL);

	if ((pfile = prefs_write_open(rcpath)) == NULL) {
		g_warning(_("failed to write configuration to file\n"));
		g_free(rcpath);
		return;
	}

	for (cur = global_filtering; cur != NULL; cur = cur->next) {
		gchar *filtering_str;

		prop = (FilteringProp *) cur->data;
		filtering_str = filteringprop_to_string(prop);
		if (fputs(filtering_str, pfile->fp) == EOF) {
			FILE_OP_ERROR(rcpath, "fputs");
			prefs_write_close_revert(pfile);
			g_free(rcpath);
			g_free(filtering_str);
			return;
		}
		g_free(filtering_str);
	}

	g_free(rcpath);

	if (prefs_write_close(pfile) < 0) {
		g_warning(_("failed to write configuration to file\n"));
		return;
	}
}

void prefs_filtering_free(GSList * prefs_filtering)
{
 	while (prefs_filtering != NULL) {
 		FilteringProp * filtering =
			(FilteringProp *) prefs_filtering->data;
 		filteringprop_free(filtering);
 		prefs_filtering = g_slist_remove(prefs_filtering, filtering);
 	}
}

static gboolean prefs_filtering_free_func(GNode *node, gpointer data)
{
	FolderItem *item = node->data;

	prefs_filtering_free(item->prefs->processing);
	item->prefs->processing = NULL;

	return FALSE;
}


void prefs_filtering_clear()
{
	GList * cur;

	for (cur = folder_get_list() ; cur != NULL ; cur = g_list_next(cur)) {
		Folder *folder;

		folder = (Folder *) cur->data;
		g_node_traverse(folder->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
				prefs_filtering_free_func, NULL);
	}

	prefs_filtering_free(global_filtering);
	global_filtering = NULL;
}
