/*
 *  arch-tag: Header for Rhythmbox Glade XML utility functions
 *
 *  Copyright (C) 2002 Jorn Baayen
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

#include <glade/glade.h>

#ifndef __RB_GLADE_HELPERS_H
#define __RB_GLADE_HELPERS_H

G_BEGIN_DECLS

GladeXML *rb_glade_xml_new (const char *file,
                            const char *root,
                            gpointer user_data);

void rb_glade_boldify_label (GladeXML *xml,
			     const char *name);

G_END_DECLS

#endif /* __RB_GLADE_HELPERS_H */
