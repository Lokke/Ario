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



char*                   util_format_time                (int time);

char*                   util_format_total_time          (int time);

gchar*                  util_format_track               (gchar *track);

gchar*                  util_format_title               (gchar *title);
 
void                    util_init_stock_icons           (void);

gint                    util_abs                        (gint a);

const char*             util_dot_dir                    (void);

gboolean                util_uri_exists                 (const char *uri);

void                    util_download_file              (const char *uri,
                                                         int* size,
                                                         char** data);
