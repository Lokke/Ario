/*
 *  Copyright (C) 2004,2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include "covers/ario-cover-amazon.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "covers/ario-cover.h"
#include "servers/ario-server.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define AMAZON_URI  "http://ecs.amazonaws.%s/onca/xml?Service=AWSECommerceService&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images&AWSAccessKeyId=%s&Keywords=%s"

#define COVER_SMALL "SmallImage"
#define COVER_MEDIUM "MediumImage"
#define COVER_LARGE "LargeImage"

static char* ario_cover_amazon_make_xml_uri (const char *artist, 
                                             const char *album);
static GSList* ario_cover_amazon_parse_xml_file (char *xmldata,
                                                 int size,
                                                 ArioCoverProviderOperation operation,
                                                 const char *cover_size);
gboolean ario_cover_amazon_get_covers (ArioCoverProvider *cover_provider,
                                       const char *artist,
                                       const char *album,
                                       const char *file,
                                       GArray **file_size,
                                       GSList **file_contents,
                                       ArioCoverProviderOperation operation);

G_DEFINE_TYPE (ArioCoverAmazon, ario_cover_amazon, ARIO_TYPE_COVER_PROVIDER)

static gchar *
ario_cover_amazon_get_id (ArioCoverProvider *cover_provider)
{
        return "amazon";
}

static gchar *
ario_cover_amazon_get_name (ArioCoverProvider *cover_provider)
{
        return "Amazon";
}

static void
ario_cover_amazon_class_init (ArioCoverAmazonClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioCoverProviderClass *cover_provider_class = ARIO_COVER_PROVIDER_CLASS (klass);

        cover_provider_class->get_id = ario_cover_amazon_get_id;
        cover_provider_class->get_name = ario_cover_amazon_get_name;
        cover_provider_class->get_covers = ario_cover_amazon_get_covers;
}

static void
ario_cover_amazon_init (ArioCoverAmazon *cover_amazon)
{
        ARIO_LOG_FUNCTION_START;
}

ArioCoverProvider*
ario_cover_amazon_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioCoverAmazon *amazon;

        amazon = g_object_new (TYPE_ARIO_COVER_AMAZON,
                               NULL);

        return ARIO_COVER_PROVIDER (amazon);
}

static GSList *
ario_cover_amazon_parse_xml_file (char *xmldata,
                                  int size,
                                  ArioCoverProviderOperation operation,
                                  const char *cover_size)
{
        ARIO_LOG_FUNCTION_START;
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        xmlNodePtr cur3;
        xmlChar *key;
        GSList *ario_cover_uris = NULL;

        doc = xmlParseMemory (xmldata, size);

        if (doc == NULL ) {
                return NULL;
        }

        cur = xmlDocGetRootElement(doc);

        if (!cur) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* We check that the root node name is "ItemSearchResponse" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "ItemSearchResponse")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (!xmlStrcmp (cur->name, (const xmlChar *) "Items"))
                        break;
        }

        if (!cur) {
                xmlFreeDoc (doc);
                return NULL;
        }

        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (!xmlStrcmp (cur->name, (const xmlChar *) "Item")){
                        for (cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next) {
                                if ((!xmlStrcmp (cur2->name, (const xmlChar *) cover_size))) {
                                        for (cur3 = cur2->xmlChildrenNode; cur3; cur3 = cur3->next) {
                                                if ((!xmlStrcmp (cur3->name, (const xmlChar *) "URL"))) {
                                                        /* A possible cover uri has been found, we add it to the list*/
                                                        key = xmlNodeListGetString (doc, cur3->xmlChildrenNode, 1);
                                                        ario_cover_uris = g_slist_append (ario_cover_uris, key);
                                                        if (operation == GET_FIRST_COVER) {
                                                                /* If we only want one cover, we now stop to parse the file */
                                                                xmlFreeDoc (doc);
                                                                return ario_cover_uris;
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }


        xmlFreeDoc (doc);

        return ario_cover_uris;
}

static char *
ario_cover_amazon_make_xml_uri (const char *artist,
                                const char *album)
{
        ARIO_LOG_FUNCTION_START;
        char *xml_uri;
        char *keywords;
        const char *ext;
        char *tmp;

        /* This is the key used to send requests on the amazon WebServices */
        const char *mykey = "14Z4RJ90F1ZE98DXRJG2";

        if (!album || !artist)
                return NULL;

        /* If the artist in unknown, we don't search for a cover */
        if (!strcmp (artist, ARIO_SERVER_UNKNOWN))
                return NULL;

        /* If the album is unknown, we only use the artist to search for the cover */
        if (!strcmp (album, ARIO_SERVER_UNKNOWN))
                keywords = g_strdup (artist);
        else
                keywords = g_strdup_printf ("%s %s", artist, album);

        tmp = ario_util_format_keyword (keywords);
        g_free (keywords);
        keywords = tmp;

        /* What is the amazon country choosen in the preferences? */
        ext = ario_conf_get_string (PREF_COVER_AMAZON_COUNTRY, PREF_COVER_AMAZON_COUNTRY_DEFAULT);

        /* We make the xml uri with all the parameters */
        xml_uri = g_strdup_printf (AMAZON_URI, ext, mykey, keywords);
        g_free (keywords);

        return xml_uri;
}

gboolean
ario_cover_amazon_get_covers (ArioCoverProvider *cover_provider,
                              const char *artist,
                              const char *album,
                              const char *file,
                              GArray **file_size,
                              GSList **file_contents,
                              ArioCoverProviderOperation operation)
{
        ARIO_LOG_FUNCTION_START;
        char *xml_uri;
        int xml_size;
        char *xml_data;
        GSList *temp;
        int temp_size;
        char *temp_contents;
        gboolean ret;
        GSList *ario_cover_uris;

        /* We construct the uri to make a request on the amazon WebServices */
        xml_uri = ario_cover_amazon_make_xml_uri (artist,
                                                  album);

        if (!xml_uri)
                return FALSE;

        /* We load the xml file in xml_data */
        ario_util_download_file (xml_uri,
                                 NULL, 0, NULL,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);

        if (xml_size == 0) {
                return FALSE;
        }

        if (g_strrstr (xml_data, "<ErrorMsg>") || g_strrstr (xml_data, "<html>")) {
                g_free (xml_data);
                return FALSE;
        }

        /* We parse the xml file to extract the cover uris */
        ario_cover_uris = ario_cover_amazon_parse_xml_file (xml_data,
                                                            xml_size,
                                                            operation,
                                                            COVER_MEDIUM);

        g_free (xml_data);

        /* By default, we return an error */
        ret = FALSE;

        for (temp = ario_cover_uris; temp; temp = temp->next) {
                if (temp->data) {
                        /* For each cover uri, we load the image data in temp_contents */
                        ario_util_download_file (temp->data,
                                                 NULL, 0, NULL,
                                                 &temp_size,
                                                 &temp_contents);
                        if (ario_cover_size_is_valid (temp_size)) {
                                /* If the cover is not too big and not too small (blank amazon image), we append it to file_contents */
                                g_array_append_val (*file_size, temp_size);
                                *file_contents = g_slist_append (*file_contents, temp_contents);
                                /* If at least one cover is found, we return OK */
                                ret = TRUE;
                        }
                }
        }

        g_slist_foreach (ario_cover_uris, (GFunc) g_free, NULL);
        g_slist_free (ario_cover_uris);

        return ret;
}

