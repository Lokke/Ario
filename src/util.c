/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <i18n.h>
#include <libgnome/gnome-init.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-directory.h>
#include <curl/curl.h>
#include "util.h"
#include "debug.h"
#include "eel/eel-gconf-extensions.h"
#include "preferences.h"

static char *dot_dir = NULL;

char *
util_format_time (int time)
{
        LOG_FUNCTION_START
        int sec, min;

        min = (int)(time / 60);
        sec = (time % 60);

        return g_strdup_printf ("%02i:%02i", min, sec);
}

char *
util_format_total_time (int time)
{
        LOG_FUNCTION_START
        gchar *res;
        gchar *temp1, *temp2;
        int temp_time;
        int sec, min, hours, days;

        days = (int)(time / 86400);
        temp_time = (time % 86400);

        hours = (int)(temp_time / 3600);
        temp_time = (temp_time % 3600);

        min = (int)(temp_time / 60);
        sec = (temp_time % 60);

        res = g_strdup_printf (_("%d seconds"), sec);

        if (min != 0) {
                temp1 = g_strdup_printf (_("%d minutes, "), min);
                temp2 = g_strconcat (temp1, res, NULL);
                g_free (temp1);
                g_free (res);
                res = temp2;
        }

        if (hours != 0) {
                temp1 = g_strdup_printf (_("%d hours, "), hours);
                temp2 = g_strconcat (temp1, res, NULL);
                g_free (temp1);
                g_free (res);
                res = temp2;
        }

        if (days != 0) {
                temp1 = g_strdup_printf (_("%d days, "), days);
                temp2 = g_strconcat (temp1, res, NULL);
                g_free (temp1);
                g_free (res);
                res = temp2;
        }
                
        return res;
}

gchar *
util_format_track (gchar *track)
{
        LOG_FUNCTION_START
        gchar **splited_track;
        gchar *res;

        if (!track)
                return g_strdup ("00");

        /* Some tracks are x/y, we only want to display x */
        splited_track = g_strsplit (track, "/", 0);
        res = g_strdup_printf ("%02i", atoi (splited_track[0]));
        g_strfreev (splited_track);
        return res;
}

gchar *
util_format_title (gchar *title)
{
        LOG_FUNCTION_START
        gchar **splited_title;
        gchar **splited_filename;
        gchar *res;

        if (!title)
                return g_strdup ("Unknown");

        /* Original format is : "artist - album/filename.extension" 
         * We only want to display filename */
        splited_title = g_strsplit (title, "/", 2);

        if (splited_title[1]) {
                splited_filename = g_strsplit (splited_title[1], ".", 2);
                res = g_strdup (splited_filename[0]);
                g_strfreev (splited_filename);
        } else
                res = g_strdup ("Unknown");

        g_strfreev (splited_title);
        return res;
}

void
util_init_stock_icons (void)
{
        LOG_FUNCTION_START
        GtkIconFactory *factory;
        GdkPixbuf *pb;
        GtkIconSet *set;
        factory = gtk_icon_factory_new ();

        pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "volume-zero.png",
                                       NULL);
        set = gtk_icon_set_new_from_pixbuf (pb);
        gtk_icon_factory_add (factory, "volume-zero", set);
        g_object_unref (G_OBJECT (pb));


        pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "volume-min.png",
                                       NULL);
        set = gtk_icon_set_new_from_pixbuf (pb);
        gtk_icon_factory_add (factory, "volume-min", set);
        g_object_unref (G_OBJECT (pb));


        pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "volume-medium.png",
                                       NULL);
        set = gtk_icon_set_new_from_pixbuf (pb);
        gtk_icon_factory_add (factory, "volume-medium", set);
        g_object_unref (G_OBJECT (pb));


        pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "volume-max.png",
                                       NULL);
        set = gtk_icon_set_new_from_pixbuf (pb);
        gtk_icon_factory_add (factory, "volume-max", set);
        g_object_unref (G_OBJECT (pb));


        pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "repeat.png",
                                       NULL);
        set = gtk_icon_set_new_from_pixbuf (pb);
        gtk_icon_factory_add (factory, "repeat", set);
        g_object_unref (G_OBJECT (pb));


        pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "shuffle.png",
                                       NULL);
        set = gtk_icon_set_new_from_pixbuf (pb);
        gtk_icon_factory_add (factory, "random", set);
        g_object_unref (G_OBJECT (pb));

        gtk_icon_factory_add_default (factory);
}

gint
util_abs (gint a)
{
        LOG_FUNCTION_START
        if (a > 0)
                return a;
        else
                return -a;
}

const char *
util_dot_dir (void)
{
        LOG_FUNCTION_START
        if (dot_dir == NULL) {
                dot_dir = g_build_filename (g_get_home_dir (),
                                            GNOME_DOT_GNOME,
                                            "ario",
                                            NULL);
                if (!g_file_test (dot_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
                        mkdir (dot_dir, 0750);
        }
        
        return dot_dir;
}

gboolean
util_uri_exists (const char *uri)
{
        LOG_FUNCTION_START
        GnomeVFSURI *vuri;
        gboolean ret;
        
        g_return_val_if_fail (uri != NULL, FALSE);

        vuri = gnome_vfs_uri_new (uri);
        ret = gnome_vfs_uri_exists (vuri);
        gnome_vfs_uri_unref (vuri);

        return ret;
}

typedef struct _download_struct{
	char *data;
	int size;
}download_struct;

#define MAX_SIZE 5*1024*1024

static size_t
util_write_data(void *buffer,
                size_t size,
                size_t nmemb,
                download_struct *download_data)
{
	if(!size || !nmemb)
		return 0;
	if(download_data->data == NULL)
	{
		download_data->size = 0;
	}
	download_data->data = g_realloc(download_data->data,(gulong)(size*nmemb+download_data->size)+1);

	memset(&(download_data->data)[download_data->size], '\0', (size*nmemb)+1);
	memcpy(&(download_data->data)[download_data->size], buffer, size*nmemb);

	download_data->size += size*nmemb;
	if(download_data->size >= MAX_SIZE)
	{
		return 0;
	}
	return size*nmemb;
}

void
util_download_file (const char *uri,
                    int* size,
                    char** data)
{
        LOG_FUNCTION_START
        LOG_DBG("Download:%s\n", uri);
        download_struct download_data;
        gchar* address;
        int port;
        
        CURL* curl = curl_easy_init();
        if(!curl)
                return;
         
         *size = 0;
         *data = NULL;

	download_data.size = 0;
	download_data.data = NULL;
	     
	/* set uri */
	curl_easy_setopt(curl, CURLOPT_URL, uri);
	/* set callback data */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &download_data);
	/* set callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, util_write_data);
	/* set timeout */
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	/* set redirect */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION ,1);
	/* set NO SIGNAL */
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, TRUE);

	if(eel_gconf_get_boolean (CONF_USE_PROXY)) {
		address = eel_gconf_get_string (CONF_PROXY_ADDRESS);
		port =  eel_gconf_get_integer (CONF_PROXY_PORT);
		if(address) {
			curl_easy_setopt(curl, CURLOPT_PROXY, address);
			curl_easy_setopt(curl, CURLOPT_PROXYPORT, port);
		} else {
			LOG_DBG("Proxy enabled, but no proxy defined");
		}
	}

	curl_easy_perform(curl);
	
	*size = download_data.size;
	*data = download_data.data;
}

