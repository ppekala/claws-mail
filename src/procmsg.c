/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto
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

#include "defs.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "intl.h"
#include "main.h"
#include "utils.h"
#include "procmsg.h"
#include "procheader.h"
#include "send_message.h"
#include "procmime.h"
#include "statusbar.h"
#include "prefs_filtering.h"
#include "filtering.h"
#include "folder.h"
#include "prefs_common.h"
#include "account.h"
#if USE_GPGME
#  include "rfc2015.h"
#endif
#include "alertpanel.h"
#include "news.h"
#include "hooks.h"
#include "msgcache.h"

GHashTable *procmsg_msg_hash_table_create(GSList *mlist)
{
	GHashTable *msg_table;

	if (mlist == NULL) return NULL;

	msg_table = g_hash_table_new(NULL, g_direct_equal);
	procmsg_msg_hash_table_append(msg_table, mlist);

	return msg_table;
}

void procmsg_msg_hash_table_append(GHashTable *msg_table, GSList *mlist)
{
	GSList *cur;
	MsgInfo *msginfo;

	if (msg_table == NULL || mlist == NULL) return;

	for (cur = mlist; cur != NULL; cur = cur->next) {
		msginfo = (MsgInfo *)cur->data;

		g_hash_table_insert(msg_table,
				    GUINT_TO_POINTER(msginfo->msgnum),
				    msginfo);
	}
}

GHashTable *procmsg_to_folder_hash_table_create(GSList *mlist)
{
	GHashTable *msg_table;
	GSList *cur;
	MsgInfo *msginfo;

	if (mlist == NULL) return NULL;

	msg_table = g_hash_table_new(NULL, g_direct_equal);

	for (cur = mlist; cur != NULL; cur = cur->next) {
		msginfo = (MsgInfo *)cur->data;
		g_hash_table_insert(msg_table, msginfo->to_folder, msginfo);
	}

	return msg_table;
}

gint procmsg_get_last_num_in_msg_list(GSList *mlist)
{
	GSList *cur;
	MsgInfo *msginfo;
	gint last = 0;

	for (cur = mlist; cur != NULL; cur = cur->next) {
		msginfo = (MsgInfo *)cur->data;
		if (msginfo && msginfo->msgnum > last)
			last = msginfo->msgnum;
	}

	return last;
}

void procmsg_msg_list_free(GSList *mlist)
{
	GSList *cur;
	MsgInfo *msginfo;

	for (cur = mlist; cur != NULL; cur = cur->next) {
		msginfo = (MsgInfo *)cur->data;
		procmsg_msginfo_free(msginfo);
	}
	g_slist_free(mlist);
}

struct MarkSum {
	gint *new_msgs;
	gint *unread_msgs;
	gint *total_msgs;
	gint *min;
	gint *max;
	gint first;
};

static gboolean procmsg_ignore_node(GNode *node, gpointer data)
{
	MsgInfo *msginfo = (MsgInfo *)node->data;
	
	procmsg_msginfo_unset_flags(msginfo, MSG_NEW | MSG_UNREAD, 0);
	procmsg_msginfo_set_flags(msginfo, MSG_IGNORE_THREAD, 0);

	return FALSE;
}

/* return the reversed thread tree */
GNode *procmsg_get_thread_tree(GSList *mlist)
{
	GNode *root, *parent, *node, *next, *last;
	GHashTable *msgid_table;
	GHashTable *subject_table;
	MsgInfo *msginfo;
	const gchar *msgid;
	const gchar *subject;
	GNode *found_subject;

	root = g_node_new(NULL);
	msgid_table = g_hash_table_new(g_str_hash, g_str_equal);
	subject_table = g_hash_table_new(g_str_hash, g_str_equal);

	for (; mlist != NULL; mlist = mlist->next) {
		msginfo = (MsgInfo *)mlist->data;
		parent = root;

		if (msginfo->inreplyto) {
			parent = g_hash_table_lookup(msgid_table, msginfo->inreplyto);
			if (parent == NULL) {
				parent = root;
			} else {
				if (MSG_IS_IGNORE_THREAD(((MsgInfo *)parent->data)->flags) && !MSG_IS_IGNORE_THREAD(msginfo->flags)) {
					procmsg_msginfo_unset_flags(msginfo, MSG_NEW | MSG_UNREAD, 0);
					procmsg_msginfo_set_flags(msginfo, MSG_IGNORE_THREAD, 0);
				}
			}
		}
		node = g_node_insert_data_before
			(parent, parent == root ? parent->children : NULL,
			 msginfo);
		if ((msgid = msginfo->msgid) &&
		    g_hash_table_lookup(msgid_table, msgid) == NULL)
			g_hash_table_insert(msgid_table, (gchar *)msgid, node);

		if (prefs_common.thread_by_subject) {
			subject  = msginfo->subject;
			subject += subject_get_reply_prefix_length(subject);
			found_subject = subject_table_lookup_clean(subject_table,
							           (gchar *) subject);
			if (found_subject == NULL)
				subject_table_insert_clean(subject_table, (gchar *) subject,
						           node);
			else {
				/* replace if msg in table is older than current one 
				 * can add here more stuff. */
                                if ( ((MsgInfo*)(found_subject->data))->date_t > 
                                     ((MsgInfo*)(node->data))->date_t )  {
					subject_table_remove_clean(subject_table, (gchar *) subject);
					subject_table_insert_clean(subject_table, (gchar *) subject, node);
				} 
			}
		}
	}

	/* complete the unfinished threads */
	for (node = root->children; node != NULL; ) {
		next = node->next;
		msginfo = (MsgInfo *)node->data;
		parent = NULL;
		if (msginfo->inreplyto) 
			parent = g_hash_table_lookup(msgid_table, msginfo->inreplyto);
		/* node should not be the parent, and node should not be an ancestor
		 * of parent (circular reference) */
		if (parent && parent != node 
		&& !g_node_is_ancestor(node, parent)) {
			g_node_unlink(node);
			g_node_insert_before
				(parent, parent->children, node);
			/* CLAWS: ignore thread */
			if (MSG_IS_IGNORE_THREAD(((MsgInfo *)parent->data)->flags) && !MSG_IS_IGNORE_THREAD(msginfo->flags)) {
				g_node_traverse(node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, procmsg_ignore_node, NULL);
			}
		}
		last = node; /* CLAWS: need to have the last one for subject threading */
		node = next;
	}

	/* CLAWS: now see if the first level (below root) still has some nodes that can be
	 * threaded by subject line. we need to handle this in a special way to prevent
	 * circular reference from a node that has already been threaded by IN-REPLY-TO
	 * but is also in the subject line hash table */
	if (prefs_common.thread_by_subject) {
		for (node = last; node && node != NULL;) {
			next = node->prev;
			msginfo = (MsgInfo *) node->data;
			subject = msginfo->subject + subject_get_reply_prefix_length(msginfo->subject);
			parent = subject_table_lookup_clean(subject_table, (gchar *) subject);
			
			/* the node may already be threaded by IN-REPLY-TO,
			   so go up in the tree to find the parent node */
			if (parent != NULL) {
				if (g_node_is_ancestor(node, parent))
					parent = NULL;
				if (parent == node)
					parent = NULL;
				/* Make new thread parent if too old compared to previous one; probably
				 * breaks ignoring threads for subject threading. This still isn't
				 * accurate because the tree isn't sorted by date. */	
				if (parent && abs(difftime(msginfo->date_t, ((MsgInfo *)parent->data)->date_t)) >
						prefs_common.thread_by_subject_max_age * 3600 * 24) {
					subject_table_remove_clean(subject_table, (gchar *) subject);
					subject_table_insert_clean(subject_table, (gchar *) subject, node);
					parent = NULL;
				}
			}

			if (parent) {
				g_node_unlink(node);
				g_node_append(parent, node);
				/* CLAWS: ignore thread */
				if (MSG_IS_IGNORE_THREAD(((MsgInfo *)parent->data)->flags) && !MSG_IS_IGNORE_THREAD(msginfo->flags)) {
					g_node_traverse(node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, procmsg_ignore_node, NULL);
				}
			}
			node = next;
		}	
	}
	
	g_hash_table_destroy(subject_table);
	g_hash_table_destroy(msgid_table);

	return root;
}

void procmsg_move_messages(GSList *mlist)
{
	GSList *cur, *movelist = NULL;
	MsgInfo *msginfo;
	FolderItem *dest = NULL;

	if (!mlist) return;

	folder_item_update_freeze();

	for (cur = mlist; cur != NULL; cur = cur->next) {
		msginfo = (MsgInfo *)cur->data;
		if (!dest) {
			dest = msginfo->to_folder;
			movelist = g_slist_append(movelist, msginfo);
		} else if (dest == msginfo->to_folder) {
			movelist = g_slist_append(movelist, msginfo);
		} else {
			folder_item_move_msgs_with_dest(dest, movelist);
			g_slist_free(movelist);
			movelist = NULL;
			dest = msginfo->to_folder;
			movelist = g_slist_append(movelist, msginfo);
		}
		procmsg_msginfo_set_to_folder(msginfo, NULL);
	}

	if (movelist) {
		folder_item_move_msgs_with_dest(dest, movelist);
		g_slist_free(movelist);
	}

	folder_item_update_thaw();
}

void procmsg_copy_messages(GSList *mlist)
{
	GSList *cur, *copylist = NULL;
	MsgInfo *msginfo;
	FolderItem *dest = NULL;

	if (!mlist) return;

	folder_item_update_freeze();

	for (cur = mlist; cur != NULL; cur = cur->next) {
		msginfo = (MsgInfo *)cur->data;
		if (!dest) {
			dest = msginfo->to_folder;
			copylist = g_slist_append(copylist, msginfo);
		} else if (dest == msginfo->to_folder) {
			copylist = g_slist_append(copylist, msginfo);
		} else {
			folder_item_copy_msgs_with_dest(dest, copylist);
			g_slist_free(copylist);
			copylist = NULL;
			dest = msginfo->to_folder;
			copylist = g_slist_append(copylist, msginfo);
		}
		procmsg_msginfo_set_to_folder(msginfo, NULL);
	}

	if (copylist) {
		folder_item_copy_msgs_with_dest(dest, copylist);
		g_slist_free(copylist);
	}

	folder_item_update_thaw();
}

gchar *procmsg_get_message_file_path(MsgInfo *msginfo)
{
	gchar *file;

	g_return_val_if_fail(msginfo != NULL, NULL);

	if (msginfo->plaintext_file)
		file = g_strdup(msginfo->plaintext_file);
	else {
		file = folder_item_fetch_msg(msginfo->folder, msginfo->msgnum);
	}

	return file;
}

gchar *procmsg_get_message_file(MsgInfo *msginfo)
{
	gchar *filename = NULL;

	g_return_val_if_fail(msginfo != NULL, NULL);

	filename = folder_item_fetch_msg(msginfo->folder, msginfo->msgnum);
	if (!filename)
		g_warning("can't fetch message %d\n", msginfo->msgnum);

	return filename;
}

FILE *procmsg_open_message(MsgInfo *msginfo)
{
	FILE *fp;
	gchar *file;

	g_return_val_if_fail(msginfo != NULL, NULL);

	file = procmsg_get_message_file_path(msginfo);
	g_return_val_if_fail(file != NULL, NULL);

	if (!is_file_exist(file)) {
		g_free(file);
		file = procmsg_get_message_file(msginfo);
		g_return_val_if_fail(file != NULL, NULL);
	}

	if ((fp = fopen(file, "rb")) == NULL) {
		FILE_OP_ERROR(file, "fopen");
		g_free(file);
		return NULL;
	}

	g_free(file);

	if (MSG_IS_QUEUED(msginfo->flags) || MSG_IS_DRAFT(msginfo->flags)) {
		gchar buf[BUFFSIZE];

		while (fgets(buf, sizeof(buf), fp) != NULL)
			if (buf[0] == '\r' || buf[0] == '\n') break;
	}

	return fp;
}

#if USE_GPGME
FILE *procmsg_open_message_decrypted(MsgInfo *msginfo, MimeInfo **mimeinfo)
{
	FILE *fp;
	MimeInfo *mimeinfo_;
	glong fpos;

	g_return_val_if_fail(msginfo != NULL, NULL);

	if (mimeinfo) *mimeinfo = NULL;

	if ((fp = procmsg_open_message(msginfo)) == NULL) return NULL;

	mimeinfo_ = procmime_scan_mime_header(fp);
	if (!mimeinfo_) {
		fclose(fp);
		return NULL;
	}

	if (!MSG_IS_ENCRYPTED(msginfo->flags) &&
	    rfc2015_is_encrypted(mimeinfo_)) {
		MSG_SET_TMP_FLAGS(msginfo->flags, MSG_ENCRYPTED);
	}

	if (MSG_IS_ENCRYPTED(msginfo->flags) &&
	    !msginfo->plaintext_file &&
	    !msginfo->decryption_failed) {
		fpos = ftell(fp);
		rfc2015_decrypt_message(msginfo, mimeinfo_, fp);
		if (msginfo->plaintext_file &&
		    !msginfo->decryption_failed) {
			fclose(fp);
			procmime_mimeinfo_free_all(mimeinfo_);
			if ((fp = procmsg_open_message(msginfo)) == NULL)
				return NULL;
			mimeinfo_ = procmime_scan_mime_header(fp);
			if (!mimeinfo_) {
				fclose(fp);
				return NULL;
			}
		} else {
			if (fseek(fp, fpos, SEEK_SET) < 0)
				perror("fseek");
		}
	}

	if (mimeinfo) *mimeinfo = mimeinfo_;
	return fp;
}
#endif

gboolean procmsg_msg_exist(MsgInfo *msginfo)
{
	gchar *path;
	gboolean ret;

	if (!msginfo) return FALSE;

	path = folder_item_get_path(msginfo->folder);
	change_dir(path);
	ret = !folder_item_is_msg_changed(msginfo->folder, msginfo);
	g_free(path);

	return ret;
}

void procmsg_get_filter_keyword(MsgInfo *msginfo, gchar **header, gchar **key,
				PrefsFilterType type)
{
	static HeaderEntry hentry[] = {{"X-BeenThere:",    NULL, TRUE},
				       {"X-ML-Name:",      NULL, TRUE},
				       {"X-List:",         NULL, TRUE},
				       {"X-Mailing-list:", NULL, TRUE},
				       {"List-Id:",        NULL, TRUE},
				       {"X-Sequence:",	   NULL, TRUE},
				       {NULL,		   NULL, FALSE}};
	enum
	{
		H_X_BEENTHERE	 = 0,
		H_X_ML_NAME      = 1,
		H_X_LIST         = 2,
		H_X_MAILING_LIST = 3,
		H_LIST_ID	 = 4,
		H_X_SEQUENCE	 = 5
	};

	FILE *fp;

	g_return_if_fail(msginfo != NULL);
	g_return_if_fail(header != NULL);
	g_return_if_fail(key != NULL);

	*header = NULL;
	*key = NULL;

	switch (type) {
	case FILTER_BY_NONE:
		return;
	case FILTER_BY_AUTO:
		if ((fp = procmsg_open_message(msginfo)) == NULL)
			return;
		procheader_get_header_fields(fp, hentry);
		fclose(fp);

#define SET_FILTER_KEY(hstr, idx)	\
{					\
	*header = g_strdup(hstr);	\
	*key = hentry[idx].body;	\
	hentry[idx].body = NULL;	\
}

		if (hentry[H_X_BEENTHERE].body != NULL) {
			SET_FILTER_KEY("header \"X-BeenThere\"", H_X_BEENTHERE);
		} else if (hentry[H_X_ML_NAME].body != NULL) {
			SET_FILTER_KEY("header \"X-ML-Name\"", H_X_ML_NAME);
		} else if (hentry[H_X_LIST].body != NULL) {
			SET_FILTER_KEY("header \"X-List\"", H_X_LIST);
		} else if (hentry[H_X_MAILING_LIST].body != NULL) {
			SET_FILTER_KEY("header \"X-Mailing-List\"", H_X_MAILING_LIST);
		} else if (hentry[H_LIST_ID].body != NULL) {
			SET_FILTER_KEY("header \"List-Id\"", H_LIST_ID);
			extract_list_id_str(*key);
		} else if (hentry[H_X_SEQUENCE].body != NULL) {
			gchar *p;

			SET_FILTER_KEY("X-Sequence", H_X_SEQUENCE);
			p = *key;
			while (*p != '\0') {
				while (*p != '\0' && !isspace(*p)) p++;
				while (isspace(*p)) p++;
				if (isdigit(*p)) {
					*p = '\0';
					break;
				}
			}
			g_strstrip(*key);
		} else if (msginfo->subject) {
			*header = g_strdup("subject");
			*key = g_strdup(msginfo->subject);
		}

#undef SET_FILTER_KEY

		g_free(hentry[H_X_BEENTHERE].body);
		hentry[H_X_BEENTHERE].body = NULL;
		g_free(hentry[H_X_ML_NAME].body);
		hentry[H_X_ML_NAME].body = NULL;
		g_free(hentry[H_X_LIST].body);
		hentry[H_X_LIST].body = NULL;
		g_free(hentry[H_X_MAILING_LIST].body);
		hentry[H_X_MAILING_LIST].body = NULL;
		g_free(hentry[H_LIST_ID].body);
		hentry[H_LIST_ID].body = NULL;

		break;
	case FILTER_BY_FROM:
		*header = g_strdup("from");
		*key = g_strdup(msginfo->from);
		break;
	case FILTER_BY_TO:
		*header = g_strdup("to");
		*key = g_strdup(msginfo->to);
		break;
	case FILTER_BY_SUBJECT:
		*header = g_strdup("subject");
		*key = g_strdup(msginfo->subject);
		break;
	default:
		break;
	}
}

void procmsg_empty_trash(void)
{
	FolderItem *trash;
	GList *cur;

	for (cur = folder_get_list(); cur != NULL; cur = cur->next) {
		trash = FOLDER(cur->data)->trash;
		if (trash && trash->total_msgs > 0)
			folder_item_remove_all_msg(trash);
	}
}

/*!
 *\brief	Send messages in queue
 *
 *\param	queue Queue folder to process
 *\param	save_msgs Unused
 *
 *\return	Number of messages sent, negative if an error occurred
 *		positive if no error occurred
 */
gint procmsg_send_queue(FolderItem *queue, gboolean save_msgs)
{
	gint ret = 1, count = 0;
	GSList *list, *elem;

	if (!queue)
		queue = folder_get_default_queue();
	g_return_val_if_fail(queue != NULL, -1);

	folder_item_scan(queue);
	list = folder_item_get_msg_list(queue);

	for (elem = list; elem != NULL; elem = elem->next) {
		gchar *file;
		MsgInfo *msginfo;
		
		msginfo = (MsgInfo *)(elem->data);
		if (!MSG_IS_LOCKED(msginfo->flags)) {
			file = folder_item_fetch_msg(queue, msginfo->msgnum);
			if (file) {
				if (procmsg_send_message_queue(file) < 0) {
					g_warning("Sending queued message %d failed.\n", 
						  msginfo->msgnum);
					ret = -1;
				} else {
					/* CLAWS: 
					 * We save in procmsg_send_message_queue because
					 * we need the destination folder from the queue
					 * header
							
					if (save_msgs)
						procmsg_save_to_outbox
							(queue->folder->outbox,
							 file, TRUE);
					 */
					count++; 
					folder_item_remove_msg(queue, msginfo->msgnum);
				}
				g_free(file);
			}
		}
		/* FIXME: supposedly if only one message is locked, and queue
		 * is being flushed, the following free says something like 
		 * "freeing msg ## in folder (nil)". */
		procmsg_msginfo_free(msginfo);
	}

	return ret * count;
}

gint procmsg_remove_special_headers(const gchar *in, const gchar *out)
{
	FILE *fp, *outfp;
	gchar buf[BUFFSIZE];
	
	if ((fp = fopen(in, "rb")) == NULL) {
		FILE_OP_ERROR(in, "fopen");
		return -1;
	}
	if ((outfp = fopen(out, "wb")) == NULL) {
		FILE_OP_ERROR(out, "fopen");
		fclose(fp);
		return -1;
	}
	while (fgets(buf, sizeof(buf), fp) != NULL)
		if (buf[0] == '\r' || buf[0] == '\n') break;
	while (fgets(buf, sizeof(buf), fp) != NULL)
		fputs(buf, outfp);
	fclose(outfp);
	fclose(fp);
	return 0;

}
gint procmsg_save_to_outbox(FolderItem *outbox, const gchar *file,
			    gboolean is_queued)
{
	gint num;
	MsgInfo *msginfo;

	debug_print("saving sent message...\n");

	if (!outbox)
		outbox = folder_get_default_outbox();
	g_return_val_if_fail(outbox != NULL, -1);

	/* remove queueing headers */
	if (is_queued) {
		gchar tmp[MAXPATHLEN + 1];

		g_snprintf(tmp, sizeof(tmp), "%s%ctmpmsg.out.%08x",
			   get_rc_dir(), G_DIR_SEPARATOR, (guint)random());
		
		if (procmsg_remove_special_headers(file, tmp) !=0)
			return -1;

		folder_item_scan(outbox);
		if ((num = folder_item_add_msg(outbox, tmp, TRUE)) < 0) {
			g_warning("can't save message\n");
			unlink(tmp);
			return -1;
		}
	} else {
		folder_item_scan(outbox);
		if ((num = folder_item_add_msg(outbox, file, FALSE)) < 0) {
			g_warning("can't save message\n");
			return -1;
		}
		return -1;
	}
	msginfo = folder_item_get_msginfo(outbox, num);
	if (msginfo != NULL) {
	    procmsg_msginfo_unset_flags(msginfo, ~0, 0);
	    procmsg_msginfo_free(msginfo);
	}
	folder_item_update(outbox, TRUE);

	return 0;
}

void procmsg_print_message(MsgInfo *msginfo, const gchar *cmdline)
{
#ifdef WIN32
	static const gchar *def_cmd = "notepad /p \"%s\"";
#else
	static const gchar *def_cmd = "lpr %s";
#endif
	static guint id = 0;
	gchar *prtmp;
	FILE *tmpfp, *prfp;
	gchar buf[1024];
	gchar *p;

	g_return_if_fail(msginfo);

	if ((tmpfp = procmime_get_first_text_content(msginfo)) == NULL) {
		g_warning("Can't get text part\n");
		return;
	}

	prtmp = g_strdup_printf("%s%cprinttmp.%08x",
				get_mime_tmp_dir(), G_DIR_SEPARATOR, id++);

	if ((prfp = fopen(prtmp, "w")) == NULL) {	/* translate crlf on dos based systems */
		FILE_OP_ERROR(prtmp, "fopen");
		g_free(prtmp);
		fclose(tmpfp);
		return;
	}

	if (msginfo->date) fprintf(prfp, "Date: %s\n", msginfo->date);
	if (msginfo->from) fprintf(prfp, "From: %s\n", msginfo->from);
	if (msginfo->to)   fprintf(prfp, "To: %s\n", msginfo->to);
	if (msginfo->cc)   fprintf(prfp, "Cc: %s\n", msginfo->cc);
	if (msginfo->newsgroups)
		fprintf(prfp, "Newsgroups: %s\n", msginfo->newsgroups);
	if (msginfo->subject) fprintf(prfp, "Subject: %s\n", msginfo->subject);
	fputc('\n', prfp);

	while (fgets(buf, sizeof(buf), tmpfp) != NULL)
		fputs(buf, prfp);

	fclose(prfp);
	fclose(tmpfp);

	if (cmdline && (p = strchr(cmdline, '%')) && *(p + 1) == 's' &&
	    !strchr(p + 2, '%'))
		g_snprintf(buf, sizeof(buf) - 1, cmdline, prtmp);
	else {
		if (cmdline)
			g_warning("Print command line is invalid: `%s'\n",
				  cmdline);
		g_snprintf(buf, sizeof(buf) - 1, def_cmd, prtmp);
	}

	g_free(prtmp);

	g_strchomp(buf);
#ifndef WIN32
	if (buf[strlen(buf) - 1] != '&') strcat(buf, "&");
#endif
	system(buf);
}

MsgInfo *procmsg_msginfo_new_ref(MsgInfo *msginfo)
{
	msginfo->refcnt++;
	
	return msginfo;
}

MsgInfo *procmsg_msginfo_new(void)
{
	MsgInfo *newmsginfo;

	newmsginfo = g_new0(MsgInfo, 1);
	newmsginfo->refcnt = 1;
	
	return newmsginfo;
}

MsgInfo *procmsg_msginfo_copy(MsgInfo *msginfo)
{
	MsgInfo *newmsginfo;

	if (msginfo == NULL) return NULL;

	newmsginfo = g_new0(MsgInfo, 1);

	newmsginfo->refcnt = 1;

#define MEMBCOPY(mmb)	newmsginfo->mmb = msginfo->mmb
#define MEMBDUP(mmb)	newmsginfo->mmb = msginfo->mmb ? \
			g_strdup(msginfo->mmb) : NULL

	MEMBCOPY(msgnum);
	MEMBCOPY(size);
	MEMBCOPY(mtime);
	MEMBCOPY(date_t);
	MEMBCOPY(flags);

	MEMBDUP(fromname);

	MEMBDUP(date);
	MEMBDUP(from);
	MEMBDUP(to);
	MEMBDUP(cc);
	MEMBDUP(newsgroups);
	MEMBDUP(subject);
	MEMBDUP(msgid);
	MEMBDUP(inreplyto);
	MEMBDUP(xref);

	MEMBCOPY(folder);
	MEMBCOPY(to_folder);

	MEMBDUP(xface);
	MEMBDUP(dispositionnotificationto);
	MEMBDUP(returnreceiptto);
	MEMBDUP(references);

	MEMBCOPY(score);
	MEMBCOPY(threadscore);

	return newmsginfo;
}

MsgInfo *procmsg_msginfo_get_full_info(MsgInfo *msginfo)
{
	MsgInfo *full_msginfo;
	gchar *file;

	if (msginfo == NULL) return NULL;

	file = procmsg_get_message_file(msginfo);
	if (!file) {
		g_warning("procmsg_msginfo_get_full_info(): can't get message file.\n");
		return NULL;
	}

	full_msginfo = procheader_parse_file(file, msginfo->flags, TRUE, FALSE);
	g_free(file);
	if (!full_msginfo) return NULL;

	full_msginfo->msgnum = msginfo->msgnum;
	full_msginfo->size = msginfo->size;
	full_msginfo->mtime = msginfo->mtime;
	full_msginfo->folder = msginfo->folder;
#if USE_GPGME
	full_msginfo->plaintext_file = g_strdup(msginfo->plaintext_file);
	full_msginfo->decryption_failed = msginfo->decryption_failed;
#endif
	procmsg_msginfo_set_to_folder(full_msginfo, msginfo->to_folder);

	return full_msginfo;
}

void procmsg_msginfo_free(MsgInfo *msginfo)
{
	if (msginfo == NULL) return;

	msginfo->refcnt--;
	if (msginfo->refcnt > 0)
		return;

	debug_print("freeing msginfo %d in %s\n", msginfo->msgnum, msginfo->folder ? msginfo->folder->path : "(nil)");

	if (msginfo->to_folder) {
		msginfo->to_folder->op_count--;
		folder_item_update(msginfo->to_folder, F_ITEM_UPDATE_MSGCNT);
	}

	g_free(msginfo->fromspace);
	g_free(msginfo->references);
	g_free(msginfo->returnreceiptto);
	g_free(msginfo->dispositionnotificationto);
	g_free(msginfo->xface);

	g_free(msginfo->fromname);

	g_free(msginfo->date);
	g_free(msginfo->from);
	g_free(msginfo->to);
	g_free(msginfo->cc);
	g_free(msginfo->newsgroups);
	g_free(msginfo->subject);
	g_free(msginfo->msgid);
	g_free(msginfo->inreplyto);
	g_free(msginfo->xref);

	g_free(msginfo);
}

guint procmsg_msginfo_memusage(MsgInfo *msginfo)
{
	guint memusage = 0;
	
	memusage += sizeof(MsgInfo);
	if (msginfo->fromname)
		memusage += strlen(msginfo->fromname);
	if (msginfo->date)
		memusage += strlen(msginfo->date);
	if (msginfo->from)
		memusage += strlen(msginfo->from);
	if (msginfo->to)
		memusage += strlen(msginfo->to);
	if (msginfo->cc)
		memusage += strlen(msginfo->cc);
	if (msginfo->newsgroups)
		memusage += strlen(msginfo->newsgroups);
	if (msginfo->subject)
		memusage += strlen(msginfo->subject);
	if (msginfo->msgid)
		memusage += strlen(msginfo->msgid);
	if (msginfo->inreplyto)
		memusage += strlen(msginfo->inreplyto);
	if (msginfo->xface)
		memusage += strlen(msginfo->xface);
	if (msginfo->dispositionnotificationto)
		memusage += strlen(msginfo->dispositionnotificationto);
	if (msginfo->returnreceiptto)
		memusage += strlen(msginfo->returnreceiptto);
	if (msginfo->references)
		memusage += strlen(msginfo->references);
	if (msginfo->fromspace)
		memusage += strlen(msginfo->fromspace);

	return memusage;
}

gint procmsg_cmp_msgnum_for_sort(gconstpointer a, gconstpointer b)
{
	const MsgInfo *msginfo1 = a;
	const MsgInfo *msginfo2 = b;

	if (!msginfo1)
		return -1;
	if (!msginfo2)
		return -1;

	return msginfo1->msgnum - msginfo2->msgnum;
}

enum
{
	Q_SENDER           = 0,
	Q_SMTPSERVER       = 1,
	Q_RECIPIENTS       = 2,
	Q_NEWSGROUPS       = 3,
	Q_MAIL_ACCOUNT_ID  = 4,
	Q_NEWS_ACCOUNT_ID  = 5,
	Q_SAVE_COPY_FOLDER = 6,
	Q_REPLY_MESSAGE_ID = 7,
	Q_FWD_MESSAGE_ID   = 8
};

gint procmsg_send_message_queue(const gchar *file)
{
	static HeaderEntry qentry[] = {{"S:",    NULL, FALSE},
				       {"SSV:",  NULL, FALSE},
				       {"R:",    NULL, FALSE},
				       {"NG:",   NULL, FALSE},
				       {"MAID:", NULL, FALSE},
				       {"NAID:", NULL, FALSE},
				       {"SCF:",  NULL, FALSE},
				       {"RMID:", NULL, FALSE},
				       {"FMID:", NULL, FALSE},
				       {NULL,    NULL, FALSE}};
	FILE *fp;
	gint filepos;
	gint mailval = 0, newsval = 0;
	gchar *from = NULL;
	gchar *smtpserver = NULL;
	GSList *to_list = NULL;
	GSList *newsgroup_list = NULL;
	gchar *savecopyfolder = NULL;
	gchar *replymessageid = NULL;
	gchar *fwdmessageid = NULL;
	gchar buf[BUFFSIZE];
	gint hnum;
	PrefsAccount *mailac = NULL, *newsac = NULL;
	int local = 0;

	g_return_val_if_fail(file != NULL, -1);

	if ((fp = fopen(file, "rb")) == NULL) {
		FILE_OP_ERROR(file, "fopen");
		return -1;
	}

	while ((hnum = procheader_get_one_field(buf, sizeof(buf), fp, qentry))
	       != -1) {
		gchar *p = buf + strlen(qentry[hnum].name);

		switch (hnum) {
		case Q_SENDER:
			if (!from) from = g_strdup(p);
			break;
		case Q_SMTPSERVER:
			if (!smtpserver) smtpserver = g_strdup(p);
			break;
		case Q_RECIPIENTS:
			to_list = address_list_append(to_list, p);
			break;
		case Q_NEWSGROUPS:
			newsgroup_list = newsgroup_list_append(newsgroup_list, p);
			break;
		case Q_MAIL_ACCOUNT_ID:
			mailac = account_find_from_id(atoi(p));
			break;
		case Q_NEWS_ACCOUNT_ID:
			newsac = account_find_from_id(atoi(p));
			break;
		case Q_SAVE_COPY_FOLDER:
			if (!savecopyfolder) savecopyfolder = g_strdup(p);
			break;
		case Q_REPLY_MESSAGE_ID:
			if (!replymessageid) replymessageid = g_strdup(p);
			break;
		case Q_FWD_MESSAGE_ID:
			if (!fwdmessageid) fwdmessageid = g_strdup(p);
			break;
		}
	}
	filepos = ftell(fp);

	if (to_list) {
		debug_print("Sending message by mail\n");
		if (!from) {
			g_warning("Queued message header is broken.\n");
			mailval = -1;
		} else if (mailac && mailac->use_mail_command &&
			   mailac->mail_command && (* mailac->mail_command)) {
			mailval = send_message_local(mailac->mail_command, fp);
			local = 1;
		} else if (prefs_common.use_extsend && prefs_common.extsend_cmd) {
			mailval = send_message_local(prefs_common.extsend_cmd, fp);
			local = 1;
		} else {
			if (!mailac) {
				mailac = account_find_from_smtp_server(from, smtpserver);
				if (!mailac) {
					g_warning("Account not found. "
						    "Using current account...\n");
					mailac = cur_account;
				}
			}

			if (mailac)
				mailval = send_message_smtp(mailac, to_list, fp);
			else {
				PrefsAccount tmp_ac;

				g_warning("Account not found.\n");

				memset(&tmp_ac, 0, sizeof(PrefsAccount));
				tmp_ac.address = from;
				tmp_ac.smtp_server = smtpserver;
				tmp_ac.smtpport = SMTP_PORT;
				mailval = send_message_smtp(&tmp_ac, to_list, fp);
			}
		}
	}

	fseek(fp, filepos, SEEK_SET);
	if (newsgroup_list && (newsval == 0)) {
		Folder *folder;
		gchar *tmp = NULL;
		FILE *tmpfp;

    		/* write to temporary file */
    		tmp = g_strdup_printf("%s%ctmp%d", g_get_tmp_dir(),
                    	    G_DIR_SEPARATOR, (gint)file);
    		if ((tmpfp = fopen(tmp, "wb")) == NULL) {
            		FILE_OP_ERROR(tmp, "fopen");
            		newsval = -1;
			alertpanel_error(_("Could not create temporary file for news sending."));
    		} else {
    			if (change_file_mode_rw(tmpfp, tmp) < 0) {
            			FILE_OP_ERROR(tmp, "chmod");
            			g_warning("can't change file mode\n");
    			}

			while ((newsval == 0) && fgets(buf, sizeof(buf), fp) != NULL) {
				if (fputs(buf, tmpfp) == EOF) {
					FILE_OP_ERROR(tmp, "fputs");
					newsval = -1;
					alertpanel_error(_("Error when writing temporary file for news sending."));
				}
			}
			fclose(tmpfp);

			if (newsval == 0) {
				debug_print("Sending message by news\n");

				folder = FOLDER(newsac->folder);

    				newsval = news_post(folder, tmp);
    				if (newsval < 0) {
            				alertpanel_error(_("Error occurred while posting the message to %s ."),
                            			 newsac->nntp_server);
    				}
			}
			unlink(tmp);
		}
		g_free(tmp);
	}

	slist_free_strings(to_list);
	g_slist_free(to_list);
	slist_free_strings(newsgroup_list);
	g_slist_free(newsgroup_list);
	g_free(from);
	g_free(smtpserver);
	fclose(fp);

	/* save message to outbox */
	if (mailval == 0 && newsval == 0 && savecopyfolder) {
		FolderItem *outbox;

		debug_print("saving sent message...\n");

		outbox = folder_find_item_from_identifier(savecopyfolder);
		if (!outbox)
			outbox = folder_get_default_outbox();

		procmsg_save_to_outbox(outbox, file, TRUE);
	}

	if (replymessageid != NULL || fwdmessageid != NULL) {
		gchar **tokens;
		FolderItem *item;
		
		if (replymessageid != NULL)
			tokens = g_strsplit(replymessageid, "\x7f", 0);
		else
			tokens = g_strsplit(fwdmessageid, "\x7f", 0);
		item = folder_find_item_from_identifier(tokens[0]);

		/* check if queued message has valid folder and message id */
		if (item != NULL && tokens[2] != NULL) {
			MsgInfo *msginfo;
			
			msginfo = folder_item_get_msginfo(item, atoi(tokens[1]));
		
			/* check if referring message exists and has a message id */
			if ((msginfo != NULL) && 
			    (msginfo->msgid != NULL) &&
			    (strcmp(msginfo->msgid, tokens[2]) != 0)) {
				procmsg_msginfo_free(msginfo);
				msginfo = NULL;
			}
			
			if (msginfo == NULL) {
				msginfo = folder_item_get_msginfo_by_msgid(item, tokens[2]);
			}
			
			if (msginfo != NULL) {
				if (replymessageid != NULL) {
					procmsg_msginfo_unset_flags(msginfo, MSG_FORWARDED, 0);
					procmsg_msginfo_set_flags(msginfo, MSG_REPLIED, 0);
				} 
				else {
					procmsg_msginfo_unset_flags(msginfo, MSG_REPLIED, 0);
					procmsg_msginfo_set_flags(msginfo, MSG_FORWARDED, 0);
				}
				procmsg_msginfo_free(msginfo);
			}
		}
		g_strfreev(tokens);
	}

	g_free(savecopyfolder);
	g_free(replymessageid);
	g_free(fwdmessageid);
	
	return (newsval != 0 ? newsval : mailval);
}

static void update_folder_msg_counts(FolderItem *item, MsgInfo *msginfo, MsgPermFlags old_flags)
{
	MsgPermFlags new_flags = msginfo->flags.perm_flags;

	/* NEW flag */
	if (!(old_flags & MSG_NEW) && (new_flags & MSG_NEW)) {
		item->new_msgs++;
	}

	if ((old_flags & MSG_NEW) && !(new_flags & MSG_NEW)) {
		item->new_msgs--;
	}

	/* UNREAD flag */
	if (!(old_flags & MSG_UNREAD) && (new_flags & MSG_UNREAD)) {
		item->unread_msgs++;
		if (procmsg_msg_has_marked_parent(msginfo))
			item->unreadmarked_msgs++;
	}

	if ((old_flags & MSG_UNREAD) && !(new_flags & MSG_UNREAD)) {
		item->unread_msgs--;
		if (procmsg_msg_has_marked_parent(msginfo))
			item->unreadmarked_msgs--;
	}
	
	/* MARK flag */
	if (!(old_flags & MSG_MARKED) && (new_flags & MSG_MARKED)) {
		procmsg_update_unread_children(msginfo, TRUE);
	}

	if ((old_flags & MSG_MARKED) && !(new_flags & MSG_MARKED)) {
		procmsg_update_unread_children(msginfo, FALSE);
	}
}

void procmsg_msginfo_set_flags(MsgInfo *msginfo, MsgPermFlags perm_flags, MsgTmpFlags tmp_flags)
{
	FolderItem *item;
	MsgInfoUpdate msginfo_update;
	MsgPermFlags perm_flags_new, perm_flags_old;

	g_return_if_fail(msginfo != NULL);
	item = msginfo->folder;
	g_return_if_fail(item != NULL);
	
	debug_print("Setting flags for message %d in folder %s\n", msginfo->msgnum, item->path);

	/* Perm Flags handling */
	perm_flags_old = msginfo->flags.perm_flags;
	perm_flags_new = msginfo->flags.perm_flags | perm_flags;
	if ((perm_flags & MSG_IGNORE_THREAD) || (perm_flags_old & MSG_IGNORE_THREAD)) {
		perm_flags_new &= ~(MSG_NEW | MSG_UNREAD);
	}

	if (perm_flags_old != perm_flags_new) {
		folder_item_change_msg_flags(msginfo->folder, msginfo, perm_flags_new);

		update_folder_msg_counts(item, msginfo, perm_flags_old);

		msginfo_update.msginfo = msginfo;
		hooks_invoke(MSGINFO_UPDATE_HOOKLIST, &msginfo_update);
		folder_item_update(msginfo->folder, F_ITEM_UPDATE_MSGCNT);
	}

	/* Tmp flags hanlding */
	msginfo->flags.tmp_flags |= tmp_flags;
}

void procmsg_msginfo_unset_flags(MsgInfo *msginfo, MsgPermFlags perm_flags, MsgTmpFlags tmp_flags)
{
	FolderItem *item;
	MsgInfoUpdate msginfo_update;
	MsgPermFlags perm_flags_new, perm_flags_old;

	g_return_if_fail(msginfo != NULL);
	item = msginfo->folder;
	g_return_if_fail(item != NULL);
	
	debug_print("Unsetting flags for message %d in folder %s\n", msginfo->msgnum, item->path);

	/* Perm Flags handling */
	perm_flags_old = msginfo->flags.perm_flags;
	perm_flags_new = msginfo->flags.perm_flags & ~perm_flags;
	
	if (perm_flags_old != perm_flags_new) {
		folder_item_change_msg_flags(msginfo->folder, msginfo, perm_flags_new);

		update_folder_msg_counts(item, msginfo, perm_flags_old);

		msginfo_update.msginfo = msginfo;
		hooks_invoke(MSGINFO_UPDATE_HOOKLIST, &msginfo_update);
		folder_item_update(msginfo->folder, F_ITEM_UPDATE_MSGCNT);
	}

	/* Tmp flags hanlding */
	msginfo->flags.tmp_flags &= ~tmp_flags;
}

/*!
 *\brief	check for flags (e.g. mark) in prior msgs of current thread
 *
 *\param	info Current message
 *\param	perm_flags Flags to be checked
 *\param	parentmsgs Hash of prior msgs to avoid loops
 *
 *\return	gboolean TRUE if perm_flags are found
 */
gboolean procmsg_msg_has_flagged_parent_real(MsgInfo *info,
		MsgPermFlags perm_flags, GHashTable *parentmsgs)
{
	MsgInfo *tmp;

	g_return_val_if_fail(info != NULL, FALSE);

	if (info != NULL && info->folder != NULL && info->inreplyto != NULL) {
		tmp = folder_item_get_msginfo_by_msgid(info->folder,
				info->inreplyto);
		if (tmp && (tmp->flags.perm_flags & perm_flags)) {
			procmsg_msginfo_free(tmp);
			return TRUE;
		} else if (tmp != NULL) {
			gboolean result;

			if (g_hash_table_lookup(parentmsgs, info)) {
				debug_print("loop detected: %s%c%d\n",
					folder_item_get_path(info->folder),
					G_DIR_SEPARATOR, info->msgnum);
				result = FALSE;
			} else {
				g_hash_table_insert(parentmsgs, info, "1");
				result = procmsg_msg_has_flagged_parent_real(
				    tmp, perm_flags, parentmsgs);
			}
			procmsg_msginfo_free(tmp);
			return result;
		} else {
			return FALSE;
		}
	} else
		return FALSE;
}

/*!
 *\brief	Callback for cleaning up hash of parentmsgs
 */
gboolean parentmsgs_hash_remove(gpointer key,
                            gpointer value,
                            gpointer user_data)
{
	return TRUE;
}

/*!
 *\brief	Set up list of parentmsgs
 *		See procmsg_msg_has_flagged_parent_real()
 */
gboolean procmsg_msg_has_flagged_parent(MsgInfo *info, MsgPermFlags perm_flags)
{
	gboolean result;
	GHashTable *parentmsgs = g_hash_table_new(NULL, NULL); 

	result = procmsg_msg_has_flagged_parent_real(info, perm_flags, parentmsgs);
	g_hash_table_foreach_remove(parentmsgs, parentmsgs_hash_remove, NULL);
	g_hash_table_destroy(parentmsgs);
	return result;
}

/*!
 *\brief	Check if msgs prior in thread are marked
 *		See procmsg_msg_has_flagged_parent_real()
 */
gboolean procmsg_msg_has_marked_parent(MsgInfo *info)
{
	return procmsg_msg_has_flagged_parent(info, MSG_MARKED);
}


GSList *procmsg_find_children_func(MsgInfo *info, 
				   GSList *children, GSList *all)
{
	GSList *cur;

	g_return_val_if_fail(info!=NULL, children);
	if (info->msgid == NULL)
		return children;

	for (cur = all; cur != NULL; cur = g_slist_next(cur)) {
		MsgInfo *tmp = (MsgInfo *)cur->data;
		if (tmp->inreplyto && !strcmp(tmp->inreplyto, info->msgid)) {
			/* Check if message is already in the list */
			if ((children == NULL) || 
			    (g_slist_index(children, tmp) == -1)) {
				children = g_slist_prepend(children,
						procmsg_msginfo_new_ref(tmp));
				children = procmsg_find_children_func(tmp, 
							children, 
							all);
			}
		}
	}
	return children;
}

GSList *procmsg_find_children (MsgInfo *info)
{
	GSList *children;
	GSList *all, *cur;

	g_return_val_if_fail(info!=NULL, NULL);
	all = folder_item_get_msg_list(info->folder);
	children = procmsg_find_children_func(info, NULL, all);
	if (children != NULL) {
		for (cur = all; cur != NULL; cur = g_slist_next(cur)) {
			/* this will not free the used pointers
			   created with procmsg_msginfo_new_ref */
			procmsg_msginfo_free((MsgInfo *)cur->data);
		}
	}
	g_slist_free(all);

	return children;
}

void procmsg_update_unread_children(MsgInfo *info, gboolean newly_marked)
{
	GSList *children = procmsg_find_children(info);
	GSList *cur;
	for (cur = children; cur != NULL; cur = g_slist_next(cur)) {
		MsgInfo *tmp = (MsgInfo *)cur->data;
		if(MSG_IS_UNREAD(tmp->flags) && !MSG_IS_IGNORE_THREAD(tmp->flags)) {
			if(newly_marked) 
				info->folder->unreadmarked_msgs++;
			else
				info->folder->unreadmarked_msgs--;
			folder_item_update(info->folder, F_ITEM_UPDATE_MSGCNT);
		}
		procmsg_msginfo_free(tmp);
	}
	g_slist_free(children);
}

/**
 * Set the destination folder for a copy or move operation
 *
 * \param msginfo The message which's destination folder is changed
 * \param to_folder The destination folder for the operation
 */
void procmsg_msginfo_set_to_folder(MsgInfo *msginfo, FolderItem *to_folder)
{
	if(msginfo->to_folder != NULL) {
		msginfo->to_folder->op_count--;
		folder_item_update(msginfo->to_folder, F_ITEM_UPDATE_MSGCNT);
	}
	msginfo->to_folder = to_folder;
	if(to_folder != NULL) {
		to_folder->op_count++;
		folder_item_update(msginfo->to_folder, F_ITEM_UPDATE_MSGCNT);
	}
}

/**
 * Apply filtering actions to the msginfo
 *
 * \param msginfo The MsgInfo describing the message that should be filtered
 * \return TRUE if the message was moved and MsgInfo is now invalid,
 *         FALSE otherwise
 */
gboolean procmsg_msginfo_filter(MsgInfo *msginfo)
{
	MailFilteringData mail_filtering_data;
			
	mail_filtering_data.msginfo = msginfo;			
	if (hooks_invoke(MAIL_FILTERING_HOOKLIST, &mail_filtering_data))
		return TRUE;

	/* filter if enabled in prefs or move to inbox if not */
	if((global_processing != NULL) &&
	   filter_message_by_msginfo(global_processing, msginfo))
		return TRUE;

	return FALSE;
}
