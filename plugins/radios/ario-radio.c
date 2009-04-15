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

#include "ario-radio.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>
#include <libxml/parser.h>
#include "ario-util.h"
#include "ario-debug.h"
#include "servers/ario-server.h"
#include "plugins/ario-plugin.h"
#include "widgets/ario-dnd-tree.h"

#define XML_ROOT_NAME (const unsigned char *)"ario-radios"
#define XML_VERSION (const unsigned char *)"1.0"

typedef struct ArioInternetRadio
{
        gchar *name;
        gchar *url;
} ArioInternetRadio;

static void ario_radio_class_init (ArioRadioClass *klass);
static void ario_radio_init (ArioRadio *radio);
static void ario_radio_finalize (GObject *object);
static void ario_radio_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void ario_radio_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec);
static void ario_radio_state_changed_cb (ArioServer *server,
                                         ArioRadio *radio);
static void ario_radio_add_in_playlist (ArioRadio *radio,
                                        gboolean play);
static void ario_radio_cmd_add_radios (GtkAction *action,
                                       ArioRadio *radio);
static void ario_radio_cmd_add_play_radios (GtkAction *action,
                                            ArioRadio *radio);
static void ario_radio_cmd_clear_add_play_radios (GtkAction *action,
                                                  ArioRadio *radio);
static void ario_radio_cmd_new_radio (GtkAction *action,
                                      ArioRadio *radio);
static void ario_radio_cmd_delete_radios (GtkAction *action,
                                          ArioRadio *radio);
static void ario_radio_cmd_radio_properties (GtkAction *action,
                                             ArioRadio *radio);
static void ario_radio_radios_selection_drag_foreach (GtkTreeModel *model,
                                                      GtkTreePath *path,
                                                      GtkTreeIter *iter,
                                                      gpointer userdata);

static void ario_radio_drag_data_get_cb (GtkWidget * widget,
                                         GdkDragContext * context,
                                         GtkSelectionData * selection_data,
                                         guint info, guint time, gpointer data);
static void ario_radio_free_internet_radio (ArioInternetRadio *internet_radio);
static void ario_radio_create_xml_file (char *xml_filename);
static char* ario_radio_get_xml_filename (void);
static void ario_radio_fill_radios (ArioRadio *radio);
static void ario_radio_popup_menu_cb (ArioDndTree* tree,
                                      ArioRadio *radio);
static void ario_radio_activate_cb (ArioDndTree* tree,
                                    ArioRadio *radio);

struct ArioRadioPrivate
{
        GtkWidget *tree;
        GtkListStore *model;
        GtkTreeSelection *selection;

        gboolean connected;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        xmlDocPtr doc;
};

static GtkActionEntry ario_radio_actions [] =
{
        { "RadioAddRadios", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_radio_cmd_add_radios) },
        { "RadioAddPlayRadios", GTK_STOCK_MEDIA_PLAY, N_("Add and _play"), NULL,
                NULL,
                G_CALLBACK (ario_radio_cmd_add_play_radios) },
        { "RadioClearAddPlayRadios", GTK_STOCK_REFRESH, N_("_Replace in playlist"), NULL,
                NULL,
                G_CALLBACK (ario_radio_cmd_clear_add_play_radios) },
        { "RadioNewRadio", GTK_STOCK_ADD, N_("Add a _new radio"), NULL,
                NULL,
                G_CALLBACK (ario_radio_cmd_new_radio) },
        { "RadioDeleteRadios", GTK_STOCK_DELETE, N_("_Delete this radios"), NULL,
                NULL,
                G_CALLBACK (ario_radio_cmd_delete_radios) },
        { "RadioProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_radio_cmd_radio_properties) }
};
static guint ario_radio_n_actions = G_N_ELEMENTS (ario_radio_actions);

enum
{
        PROP_0,
        PROP_UI_MANAGER
};

enum
{
        RADIO_NAME_COLUMN,
        RADIO_URL_COLUMN,
        N_COLUMN
};

static const GtkTargetEntry radios_targets  [] = {
        { "text/radios-list", 0, 0 },
};

#define ARIO_RADIO_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_RADIO, ArioRadioPrivate))
G_DEFINE_TYPE (ArioRadio, ario_radio, ARIO_TYPE_SOURCE)

static gchar *
ario_radio_get_id (ArioSource *source)
{
        return "radios";
}

static gchar *
ario_radio_get_name (ArioSource *source)
{
        return _("Web Radios");
}

static gchar *
ario_radio_get_icon (ArioSource *source)
{
        return GTK_STOCK_NETWORK;
}

static void
ario_radio_class_init (ArioRadioClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        object_class->finalize = ario_radio_finalize;

        object_class->set_property = ario_radio_set_property;
        object_class->get_property = ario_radio_get_property;

        source_class->get_id = ario_radio_get_id;
        source_class->get_name = ario_radio_get_name;
        source_class->get_icon = ario_radio_get_icon;

        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        g_type_class_add_private (klass, sizeof (ArioRadioPrivate));
}

static void
ario_radio_init (ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        GtkWidget *scrolledwindow_radios;

        radio->priv = ARIO_RADIO_GET_PRIVATE (radio);

        /* Radios list */
        scrolledwindow_radios = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_radios);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_radios), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_radios), GTK_SHADOW_IN);

        radio->priv->tree = ario_dnd_tree_new (radios_targets,
                                               G_N_ELEMENTS (radios_targets),
                                               FALSE);

        gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (radio->priv->tree), TRUE);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Internet Radios"),
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (radio->priv->tree), column);
        radio->priv->model = gtk_list_store_new (N_COLUMN,
                                                 G_TYPE_STRING,
                                                 G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (radio->priv->model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (radio->priv->tree),
                                 GTK_TREE_MODEL (radio->priv->model));
        radio->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (radio->priv->tree));
        gtk_tree_selection_set_mode (radio->priv->selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_radios), radio->priv->tree);

        g_signal_connect (GTK_TREE_VIEW (radio->priv->tree),
                          "drag_data_get",
                          G_CALLBACK (ario_radio_drag_data_get_cb), radio);
        g_signal_connect (GTK_TREE_VIEW (radio->priv->tree),
                          "popup",
                          G_CALLBACK (ario_radio_popup_menu_cb), radio);
        g_signal_connect (GTK_TREE_VIEW (radio->priv->tree),
                          "activate",
                          G_CALLBACK (ario_radio_activate_cb), radio);

        /* Hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (radio), TRUE);
        gtk_box_set_spacing (GTK_BOX (radio), 4);

        gtk_box_pack_start (GTK_BOX (radio), scrolledwindow_radios, TRUE, TRUE, 0);
}

static void
ario_radio_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio;
        int i;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_RADIO (object));

        radio = ARIO_RADIO (object);

        g_return_if_fail (radio->priv != NULL);
        if (radio->priv->doc)
                xmlFreeDoc (radio->priv->doc);
        radio->priv->doc = NULL;

        for (i = 0; i < ario_radio_n_actions; ++i) {
                gtk_action_group_remove_action (radio->priv->actiongroup,
                                                gtk_action_group_get_action (radio->priv->actiongroup, ario_radio_actions[i].name));
        }

        G_OBJECT_CLASS (ario_radio_parent_class)->finalize (object);
}

static void
ario_radio_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio = ARIO_RADIO (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                radio->priv->ui_manager = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_radio_get_property (GObject *object,
                         guint prop_id,
                         GValue *value,
                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio = ARIO_RADIO (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                g_value_set_object (value, radio->priv->ui_manager);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_radio_new (GtkUIManager *mgr,
                GtkActionGroup *group)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio;

        radio = g_object_new (TYPE_ARIO_RADIO,
                              "ui-manager", mgr,
                              NULL);

        g_return_val_if_fail (radio->priv != NULL, NULL);
        radio->priv->actiongroup = group;

        /* Signals to synchronize the radio with server */
        g_signal_connect_object (ario_server_get_instance (),
                                 "state_changed",
                                 G_CALLBACK (ario_radio_state_changed_cb),
                                 radio, 0);
        radio->priv->connected = ario_server_is_connected ();

        gtk_action_group_add_actions (group,
                                      ario_radio_actions,
                                      ario_radio_n_actions, radio);

        ario_radio_fill_radios (radio);

        return GTK_WIDGET (radio);
}

static void
ario_radio_free_internet_radio (ArioInternetRadio *internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        if (internet_radio) {
                g_free (internet_radio->name);
                g_free (internet_radio->url);
                g_free (internet_radio);
        }
}

static void
ario_radio_create_xml_file (char *xml_filename)
{
        gchar *radios_dir;
        gchar *file;

        /* if the file exists, we do nothing */
        if (ario_util_uri_exists (xml_filename))
                return;

        radios_dir = g_build_filename (ario_util_config_dir (), "radios", NULL);

        /* If the radios directory doesn't exist, we create it */
        if (!ario_util_uri_exists (radios_dir))
                ario_util_mkdir (radios_dir);

        file = ario_plugin_find_file ("radios.xml.default");
        if (file) {
                ario_util_copy_file (file, xml_filename);
                g_free (file);
        }
}

static char*
ario_radio_get_xml_filename (void)
{
        static char *xml_filename = NULL;

        if (!xml_filename)
                xml_filename = g_build_filename (ario_util_config_dir (), "radios" , "radios.xml", NULL);

        return xml_filename;
}

// Return TRUE on success
static gboolean
ario_radio_fill_doc (ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        char *xml_filename;
        xmlNodePtr cur;

        if (!radio->priv->doc) {
                xml_filename = ario_radio_get_xml_filename();

                /* If radios.xml file doesn't exist, we create it */
                ario_radio_create_xml_file (xml_filename);

                /* This option is necessary to save a well formated xml file */
                xmlKeepBlanksDefault (0);

                radio->priv->doc = xmlParseFile (xml_filename);
                if (radio->priv->doc == NULL )
                        return FALSE;

                /* We check that the root node has the right name */
                cur = xmlDocGetRootElement (radio->priv->doc);
                if (cur == NULL) {
                        xmlFreeDoc (radio->priv->doc);
                        radio->priv->doc = NULL;
                        return FALSE;
                }
                if (xmlStrcmp(cur->name, (const xmlChar *) XML_ROOT_NAME)) {
                        xmlFreeDoc (radio->priv->doc);
                        radio->priv->doc = NULL;
                        return FALSE;
                }
        }
        return TRUE;
}

static GSList*
ario_radio_get_radios (ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GSList* radios = NULL;
        ArioInternetRadio *internet_radio;
        xmlNodePtr cur;
        xmlChar *xml_name;
        xmlChar *xml_url;

        if (!ario_radio_fill_doc (radio))
                return NULL;

        cur = xmlDocGetRootElement (radio->priv->doc);
        for (cur = cur->children; cur; cur = cur->next) {
                /* For each "radio" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *)"radio")){
                        internet_radio = (ArioInternetRadio *) g_malloc (sizeof (ArioInternetRadio));

                        xml_name = xmlNodeGetContent (cur);
                        internet_radio->name = g_strdup ((char *) xml_name);
                        xmlFree(xml_name);

                        xml_url = xmlGetProp (cur, (const unsigned char *)"url");
                        internet_radio->url = g_strdup ((char *) xml_url);
                        xmlFree(xml_url);

                        radios = g_slist_append (radios, internet_radio);
                }
        }

        return radios;
}

static void
ario_radio_append_radio (ArioRadio *radio,
                         ArioInternetRadio *internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter radio_iter;

        gtk_list_store_append (radio->priv->model, &radio_iter);
        gtk_list_store_set (radio->priv->model, &radio_iter,
                            RADIO_NAME_COLUMN, internet_radio->name,
                            RADIO_URL_COLUMN, internet_radio->url,
                            -1);
}

static void
ario_radio_fill_radios (ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GSList *radios;
        GSList *tmp;
        GtkTreeIter radio_iter;
        GList* paths;
        GtkTreePath *path;
        GtkTreeModel *models = GTK_TREE_MODEL (radio->priv->model);
        ArioInternetRadio *internet_radio;

        paths = gtk_tree_selection_get_selected_rows (radio->priv->selection, &models);

        gtk_list_store_clear (radio->priv->model);

        if (!radio->priv->connected)
                return;

        radios = ario_radio_get_radios (radio);

        for (tmp = radios; tmp; tmp = g_slist_next (tmp)) {
                internet_radio = (ArioInternetRadio *) tmp->data;
                ario_radio_append_radio (radio, internet_radio);
        }
        g_slist_foreach (radios, (GFunc) ario_radio_free_internet_radio, NULL);
        g_slist_free (radios);

        gtk_tree_selection_unselect_all (radio->priv->selection);

        if (paths) {
                path = paths->data;
                if (path) {
                        gtk_tree_selection_select_path (radio->priv->selection, path);
                }
        } else {
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (radio->priv->model), &radio_iter))
                        gtk_tree_selection_select_iter (radio->priv->selection, &radio_iter);
        }

        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);
}

static void
ario_radio_state_changed_cb (ArioServer *server,
                             ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;

        if (radio->priv->connected != ario_server_is_connected ()) {
                radio->priv->connected = ario_server_is_connected ();
                ario_radio_fill_radios (radio);
        }
}

static void
radios_foreach (GtkTreeModel *model,
                GtkTreePath *path,
                GtkTreeIter *iter,
                gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **radios = (GSList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, RADIO_URL_COLUMN, &val, -1);

        *radios = g_slist_append (*radios, val);
}

static void
radios_foreach2 (GtkTreeModel *model,
                 GtkTreePath *path,
                 GtkTreeIter *iter,
                 gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **internet_radios = (GSList **) userdata;
        ArioInternetRadio *internet_radio = (ArioInternetRadio *) g_malloc (sizeof (ArioInternetRadio));

        gtk_tree_model_get (model, iter, RADIO_NAME_COLUMN, &internet_radio->name, -1);
        gtk_tree_model_get (model, iter, RADIO_URL_COLUMN, &internet_radio->url, -1);

        *internet_radios = g_slist_append (*internet_radios, internet_radio);
}

static void
ario_radio_add_in_playlist (ArioRadio *radio,
                            gboolean play)
{
        ARIO_LOG_FUNCTION_START;
        GSList *radios = NULL;

        gtk_tree_selection_selected_foreach (radio->priv->selection,
                                             radios_foreach,
                                             &radios);
        ario_server_playlist_append_songs (radios, play);

        g_slist_foreach (radios, (GFunc) g_free, NULL);
        g_slist_free (radios);
}

static void
ario_radio_cmd_add_radios (GtkAction *action,
                           ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        ario_radio_add_in_playlist (radio, FALSE);
}

static void
ario_radio_cmd_add_play_radios (GtkAction *action,
                                ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        ario_radio_add_in_playlist (radio, TRUE);
}

static void
ario_radio_cmd_clear_add_play_radios (GtkAction *action,
                                      ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_clear ();
        ario_radio_add_in_playlist (radio, TRUE);
}

static void
ario_radio_popup_menu_cb (ArioDndTree* tree,
                          ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *menu;

        switch (gtk_tree_selection_count_selected_rows (radio->priv->selection)) {
        case 0:
                menu = gtk_ui_manager_get_widget (radio->priv->ui_manager, "/RadioPopupNone");
                break;
        case 1:
                menu = gtk_ui_manager_get_widget (radio->priv->ui_manager, "/RadioPopupSingle");
                break;
        default:
                menu = gtk_ui_manager_get_widget (radio->priv->ui_manager, "/RadioPopupMultiple");
                break;
        }

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3,
                        gtk_get_current_event_time ());
}

static void
ario_radio_activate_cb (ArioDndTree* tree,
                        ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        ario_radio_add_in_playlist (radio, FALSE);
}

static void
ario_radio_radios_selection_drag_foreach (GtkTreeModel *model,
                                          GtkTreePath *path,
                                          GtkTreeIter *iter,
                                          gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GString *radios = (GString *) userdata;
        g_return_if_fail (radios != NULL);

        gchar* val = NULL;

        gtk_tree_model_get (model, iter, RADIO_URL_COLUMN, &val, -1);
        g_string_append (radios, val);
        g_string_append (radios, "\n");
        g_free (val);
}

static void
ario_radio_drag_data_get_cb (GtkWidget * widget,
                             GdkDragContext * context,
                             GtkSelectionData * selection_data,
                             guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio;
        GString* radios = NULL;

        radio = ARIO_RADIO (data);

        g_return_if_fail (IS_ARIO_RADIO (radio));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        radios = g_string_new("");
        gtk_tree_selection_selected_foreach (radio->priv->selection,
                                             ario_radio_radios_selection_drag_foreach,
                                             radios);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) radios->str,
                                strlen (radios->str) * sizeof(guchar));

        g_string_free (radios, TRUE);
}

static void
ario_radio_add_new_radio (ArioRadio *radio,
                          ArioInternetRadio *internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        xmlNodePtr cur, cur2;

        if (!ario_radio_fill_doc (radio))
                return;

        cur = xmlDocGetRootElement (radio->priv->doc);

        /* We add a new "radio" entry */
        cur2 = xmlNewChild (cur, NULL, (const xmlChar *)"radio", NULL);
        xmlSetProp (cur2, (const xmlChar *)"url", (const xmlChar *) internet_radio->url);
        xmlNodeAddContent (cur2, (const xmlChar *) internet_radio->name);

        /* We save the xml file */
        xmlSaveFormatFile (ario_radio_get_xml_filename(), radio->priv->doc, TRUE);

        ario_radio_append_radio (radio, internet_radio);
}

static gboolean
ario_radio_launch_dialog (const gchar *title,
                          ArioInternetRadio *internet_radio,
                          ArioInternetRadio *new_internet_radio)
{
        GtkWidget *dialog, *error_dialog;
        GtkWidget *table;
        GtkWidget *label1, *label2;
        GtkWidget *entry1, *entry2;
        gint retval = GTK_RESPONSE_CANCEL;

        /* Create the widgets */
        dialog = gtk_dialog_new_with_buttons (title,
                                              NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_OK,
                                              NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_OK);
        label1 = gtk_label_new (_("Name :"));
        label2 = gtk_label_new (_("URL :"));

        entry1 = gtk_entry_new ();
        entry2 = gtk_entry_new ();

        if (internet_radio) {
                gtk_entry_set_text (GTK_ENTRY (entry1), internet_radio->name);
                gtk_entry_set_text (GTK_ENTRY (entry2), internet_radio->url);
        }
        gtk_entry_set_activates_default (GTK_ENTRY (entry1), TRUE);
        gtk_entry_set_activates_default (GTK_ENTRY (entry2), TRUE);
        table = gtk_table_new (2, 2 , FALSE);
        gtk_container_set_border_width (GTK_CONTAINER (table), 12);

        gtk_table_attach_defaults (GTK_TABLE(table),
                                   label1,
                                   0, 1,
                                   0, 1);

        gtk_table_attach_defaults (GTK_TABLE(table),
                                   label2,
                                   0, 1,
                                   1, 2);

        gtk_table_attach_defaults (GTK_TABLE(table),
                                   entry1,
                                   1, 2,
                                   0, 1);

        gtk_table_attach_defaults (GTK_TABLE(table),
                                   entry2,
                                   1, 2,
                                   1, 2);

        gtk_table_set_col_spacing (GTK_TABLE(table),
                                   0, 4);

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                           table);
        gtk_widget_show_all (dialog);

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        if (retval != GTK_RESPONSE_OK) {
                gtk_widget_destroy (dialog);
                return FALSE;
        }

        new_internet_radio->name = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry1)));
        new_internet_radio->url = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry2)));

        if (!new_internet_radio->name
            || !new_internet_radio->url
            || !strcmp(new_internet_radio->name, "")
            || !strcmp(new_internet_radio->url, "")) {
                error_dialog = gtk_message_dialog_new(NULL,
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_OK,
                                                      _("Bad parameters. You must specify a name and a URL for the radio."));
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
                gtk_widget_destroy(dialog);
                return FALSE;
        }

        gtk_widget_destroy (dialog);

        return TRUE;
}

static void
ario_radio_cmd_new_radio (GtkAction *action,
                          ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        ArioInternetRadio new_internet_radio;

        new_internet_radio.name = NULL;
        new_internet_radio.url = NULL;

        if (ario_radio_launch_dialog (_("Add a WebRadio"),
                                      NULL,
                                      &new_internet_radio)) {
                ario_radio_add_new_radio(radio,
                                         &new_internet_radio);
        }
        g_free (new_internet_radio.name);
        g_free (new_internet_radio.url);
}

static void
ario_radio_delete_radio (ArioInternetRadio *internet_radio,
                         ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        xmlNodePtr cur, next_cur;
        xmlChar *xml_name;
        xmlChar *xml_url;

        if (!ario_radio_fill_doc (radio))
                return;

        cur = xmlDocGetRootElement (radio->priv->doc);
        for (cur = cur->children; cur; cur = next_cur) {
                next_cur = cur->next;
                /* For each "radio" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *)"radio")){
                        xml_name = xmlNodeGetContent (cur);
                        xml_url = xmlGetProp (cur, (const unsigned char *)"url");
                        if (!xmlStrcmp (xml_name, (const xmlChar *)internet_radio->name)
                            && !xmlStrcmp (xml_url, (const xmlChar *)internet_radio->url)) {
                                xmlUnlinkNode(cur);
                                xmlFreeNode(cur);
                        }

                        xmlFree(xml_name);
                        xmlFree(xml_url);
                }
        }

        /* We save the xml file */
        xmlSaveFormatFile (ario_radio_get_xml_filename(), radio->priv->doc, TRUE);

        ario_radio_fill_radios (radio);
}

static void
ario_radio_cmd_delete_radios (GtkAction *action,
                              ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GSList *internet_radios = NULL;
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;

        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         _("Are you sure you want to delete all the selected radios?"));

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        gtk_tree_selection_selected_foreach (radio->priv->selection,
                                             radios_foreach2,
                                             &internet_radios);

        g_slist_foreach (internet_radios, (GFunc) ario_radio_delete_radio, radio);
        g_slist_foreach (internet_radios, (GFunc) ario_radio_free_internet_radio, NULL);
        g_slist_free (internet_radios);
}

static void
ario_radio_modify_radio (ArioRadio *radio,
                         ArioInternetRadio *old_internet_radio,
                         ArioInternetRadio *new_internet_radio)
{
        ARIO_LOG_FUNCTION_START;

        ARIO_LOG_FUNCTION_START;
        xmlNodePtr cur, next_cur;
        xmlChar *xml_name;
        xmlChar *xml_url;
        xmlChar *new_xml_name;

        if (!ario_radio_fill_doc (radio))
                return;

        cur = xmlDocGetRootElement (radio->priv->doc);
        for (cur = cur->children; cur; cur = next_cur) {
                next_cur = cur->next;
                /* For each "radio" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *)"radio")){
                        xml_name = xmlNodeGetContent (cur);
                        xml_url = xmlGetProp (cur, (const unsigned char *)"url");
                        if (!xmlStrcmp (xml_name, (const xmlChar *)old_internet_radio->name)
                            && !xmlStrcmp (xml_url, (const xmlChar *)old_internet_radio->url)) {
                                xmlSetProp (cur, (const xmlChar *)"url", (const xmlChar *) new_internet_radio->url);
                                new_xml_name = xmlEncodeEntitiesReentrant (radio->priv->doc, (const xmlChar *) new_internet_radio->name);
                                xmlNodeSetContent (cur, new_xml_name);
                                xmlFree(new_xml_name);
                        }

                        xmlFree(xml_name);
                        xmlFree(xml_url);
                }
        }

        /* We save the xml file */
        xmlSaveFormatFile (ario_radio_get_xml_filename(), radio->priv->doc, TRUE);

        ario_radio_fill_radios (radio);
}

static void
ario_radio_edit_radio_properties (ArioRadio *radio,
                                  ArioInternetRadio *internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        ArioInternetRadio new_internet_radio;

        new_internet_radio.name = NULL;
        new_internet_radio.url = NULL;

        if (ario_radio_launch_dialog (_("Edit a WebRadio"),
                                      internet_radio,
                                      &new_internet_radio)) {
                ario_radio_modify_radio (radio,
                                         internet_radio,
                                         &new_internet_radio);
        }
        g_free (new_internet_radio.name);
        g_free (new_internet_radio.url);
}

static void
ario_radio_cmd_radio_properties (GtkAction *action,
                                 ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GList* paths;
        GtkTreeIter iter;
        ArioInternetRadio *internet_radio;
        GtkTreeModel *tree_model = GTK_TREE_MODEL (radio->priv->model);
        GtkTreePath *path;

        paths = gtk_tree_selection_get_selected_rows (radio->priv->selection,
                                                      &tree_model);

        path = g_list_first(paths)->data;
        gtk_tree_model_get_iter (tree_model,
                                 &iter,
                                 path);
        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);

        internet_radio = (ArioInternetRadio *) g_malloc (sizeof (ArioInternetRadio));;
        gtk_tree_model_get (tree_model, &iter, RADIO_NAME_COLUMN, &internet_radio->name, -1);
        gtk_tree_model_get (tree_model, &iter, RADIO_URL_COLUMN, &internet_radio->url, -1);

        ario_radio_edit_radio_properties (radio, internet_radio);

        ario_radio_free_internet_radio (internet_radio);
}

