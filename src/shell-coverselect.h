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


#ifndef __SHELL_COVERSELECT_H
#define __SHELL_COVERSELECT_H

G_BEGIN_DECLS

#define TYPE_SHELL_COVERSELECT         (shell_coverselect_get_type ())
#define SHELL_COVERSELECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_SHELL_COVERSELECT, ShellCoverselect))
#define SHELL_COVERSELECT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_SHELL_COVERSELECT, ShellCoverselectClass))
#define IS_SHELL_COVERSELECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_SHELL_COVERSELECT))
#define IS_SHELL_COVERSELECT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_SHELL_COVERSELECT))
#define SHELL_COVERSELECT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_SHELL_COVERSELECT, ShellCoverselectClass))

typedef struct ShellCoverselectPrivate ShellCoverselectPrivate;

typedef struct
{
        GtkDialog parent;

        ShellCoverselectPrivate *priv;
} ShellCoverselect;

typedef struct
{
        GtkDialogClass parent_class;
} ShellCoverselectClass;

GType              shell_coverselect_get_type         (void);

GtkWidget *         shell_coverselect_new                (const char *artist,
                                                 const char *album);

G_END_DECLS

#endif /* __SHELL_COVERSELECT_H */
