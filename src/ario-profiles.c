/*
 *  Copyright (C) 2007 Marc Pavot <marc.pavot@gmail.com>
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

#include <string.h>
#include <config.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <glib/gi18n.h>
#include "ario-profiles.h"
#include "ario-util.h"
#include "ario-debug.h"

#define XML_ROOT_NAME (const unsigned char *)"ario-profiles"
#define XML_VERSION (const unsigned char *)"1.0"

static void ario_profiles_create_xml_file (char *xml_filename);
static char* ario_profiles_get_xml_filename (void);


static void
ario_profiles_create_xml_file (char *xml_filename)
{
        gchar *profiles_dir;

        /* if the file exists, we do nothing */
        if (ario_util_uri_exists (xml_filename))
                return;

        profiles_dir = g_build_filename (ario_util_config_dir (), "profiles", NULL);

        /* If the profiles directory doesn't exist, we create it */
        if (!ario_util_uri_exists (profiles_dir))
                ario_util_mkdir (profiles_dir);

        ario_util_copy_file (DATA_PATH "profiles.xml.default",
                             xml_filename);
}

static char*
ario_profiles_get_xml_filename (void)
{
        char *xml_filename;

        xml_filename = g_build_filename (ario_util_config_dir (), "profiles" , "profiles.xml", NULL);

        return xml_filename;
}

void
ario_profiles_free (ArioProfile* profile)
{
        ARIO_LOG_FUNCTION_START
        g_free (profile->name);
        g_free (profile->host);
        g_free (profile->password);
        g_free (profile);
}

GList*
ario_profiles_read (void)
{
        ARIO_LOG_FUNCTION_START
        GList* profiles = NULL;
        ArioProfile *profile;
        xmlDocPtr doc;
        xmlNodePtr cur;
        char *xml_filename;
        xmlChar *xml_name;
        xmlChar *xml_host;
        xmlChar *xml_port;
        xmlChar *xml_password;
        xmlChar *xml_current;

        xml_filename = ario_profiles_get_xml_filename();

        /* If profiles.xml file doesn't exist, we create it */
        ario_profiles_create_xml_file (xml_filename);

        /* This option is necessary to save a well formated xml file */
        xmlKeepBlanksDefault (0);

        doc = xmlParseFile (xml_filename);
        g_free (xml_filename);
        if (doc == NULL )
                return profiles;

        cur = xmlDocGetRootElement(doc);
        if (cur == NULL) {
                xmlFreeDoc (doc);
                return profiles;
        }

        /* We check that the root node has the right name */
        if (xmlStrcmp(cur->name, (const xmlChar *) XML_ROOT_NAME)) {
                xmlFreeDoc (doc);
                return profiles;
        }

        cur = cur->children;
        while (cur != NULL) {
                /* For each "profiles" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *)"profile")){
                        profile = (ArioProfile *) g_malloc (sizeof (ArioProfile));

                        xml_name = xmlNodeGetContent (cur);
                        profile->name = g_strdup ((char *) xml_name);
                        xmlFree(xml_name);

                        xml_host = xmlGetProp (cur, (const unsigned char *)"host");
                        profile->host = g_strdup ((char *) xml_host);
                        xmlFree(xml_host);

                        xml_port = xmlGetProp (cur, (const unsigned char *)"port");
                        profile->port = atoi ((char *) xml_port);
                        xmlFree(xml_port);

                        xml_password = xmlGetProp (cur, (const unsigned char *)"password");
                        if (xml_password) {
                                profile->password = g_strdup ((char *) xml_password);
                                xmlFree(xml_password);
                        } else {
                                profile->password = NULL;
                        }

                        xml_current = xmlGetProp (cur, (const unsigned char *)"current");
                        if (xml_current) {
                                profile->current = TRUE;
                                xmlFree(xml_current);
                        } else {
                                profile->current = FALSE;
                        }

                        profiles = g_list_append (profiles, profile);
                }
                cur = cur->next;
        }
        xmlFreeDoc (doc);

        return profiles;
}

void
ario_profiles_save (GList* profiles)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur, cur2;
        char *xml_filename;
        ArioProfile *profile;
        GList *tmp;
        gchar *port_char;

        xml_filename = ario_profiles_get_xml_filename();

        /* If profiles.xml file doesn't exist, we create it */
        ario_profiles_create_xml_file (xml_filename);

        /* This option is necessary to save a well formated xml file */
        xmlKeepBlanksDefault (0);

        doc = xmlNewDoc (XML_VERSION);
        if (doc == NULL ) {
                g_free (xml_filename);
                return;
        }

        /* Create the root node */
        cur = xmlNewNode (NULL, (const xmlChar *) XML_ROOT_NAME);
        if (cur == NULL) {
                g_free (xml_filename);
                xmlFreeDoc (doc);
                return;
        }
        xmlDocSetRootElement (doc, cur);

        for (tmp = profiles; tmp; tmp = g_list_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                port_char = g_strdup_printf ("%d",  profile->port);

                /* We add a new "profiles" entry */
                cur2 = xmlNewChild (cur, NULL, (const xmlChar *)"profile", NULL);
                xmlNodeAddContent (cur2, (const xmlChar *) profile->name);
                xmlSetProp (cur2, (const xmlChar *)"host", (const xmlChar *) profile->host);
                xmlSetProp (cur2, (const xmlChar *)"port", (const xmlChar *) port_char);
                if (profile->password) {
                        xmlSetProp (cur2, (const xmlChar *)"password", (const xmlChar *) profile->password);
                }
                if (profile->current) {
                        xmlSetProp (cur2, (const xmlChar *)"current", (const xmlChar *) "true");
                }
                g_free (port_char);
        }

        /* We save the xml file */
        xmlSaveFormatFile (xml_filename, doc, TRUE);
        g_free (xml_filename);
        xmlFreeDoc (doc);
}

ArioProfile*
ario_profiles_get_current (GList* profiles)
{
        ARIO_LOG_FUNCTION_START
        GList *tmp;
        ArioProfile *profile;

        for (tmp = profiles; tmp; tmp = g_list_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                if (profile->current)
                        return profile;
        }
        return NULL;
}

void
ario_profiles_set_current (GList* profiles,
                           ArioProfile* profile)
{
        ARIO_LOG_FUNCTION_START
        GList *tmp;
        ArioProfile *tmp_profile;

        if (g_list_find (profiles, profile)) {
                for (tmp = profiles; tmp; tmp = g_list_next (tmp)) {
                        tmp_profile = (ArioProfile *) tmp->data;
                        if (tmp_profile == profile) {
                                tmp_profile->current = TRUE;
                        } else {
                                tmp_profile->current = FALSE;
                        }
                }
        }
}

