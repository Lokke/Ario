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

#ifndef __SHELL_H
#define __SHELL_H

G_BEGIN_DECLS

#define TYPE_SHELL         (shell_get_type ())
#define SHELL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_SHELL, Shell))
#define SHELL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_SHELL, ShellClass))
#define IS_SHELL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_SHELL))
#define IS_SHELL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_SHELL))
#define SHELL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_SHELL, ShellClass))

typedef struct ShellPrivate ShellPrivate;

typedef struct
{
        GObject parent;

        ShellPrivate *priv;
} Shell;

typedef struct
{
        GObjectClass parent_class;
} ShellClass;

GType                shell_get_type                (void);

Shell *                shell_new                (void);

void                shell_construct                (Shell *shell);

G_END_DECLS

#endif /* __SHELL_H */
