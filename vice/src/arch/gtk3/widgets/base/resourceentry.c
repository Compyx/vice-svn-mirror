/** \file   resourceentry.c
 * \brief   Text entry connected to a resource
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <string.h>
#include <stdarg.h>

#include "debug_gtk3.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "resourcehelpers.h"

#include "resourceentry.h"


/** \brief  Handler for the 'destroy' event of the entry
 *
 * Frees the heap-allocated copy of the resource name and the value of the
 * resource on instanciation of the widget.
 *
 * \param[in,out]   entry       entry
 * \param[in]       user_data   extra event data (unused)
 */
static void on_entry_destroy(GtkWidget *entry, gpointer user_data)
{
    resource_widget_free_resource_name(entry);
    resource_widget_free_string(entry, "ResourceOrig");
}


/** \brief  Handler for the 'changed' event of the check button
 *
 * \param[in]   entry       entry
 * \param[in]   user_data   unused
 */
static void on_entry_changed(GtkWidget *entry, gpointer user_data)
{
    const char *resource;
    const char *text;

    resource = resource_widget_get_resource_name(entry);
    text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (resources_set_string(resource, text) < 0) {
        log_error(LOG_ERR, "failed to set resource '%s' to '%s'\n",
                resource, text);
    }
}


/** \brief  Create entry to control a string resource
 *
 * Creates a text entry to update \a resource. Makes a heap-allocated copy
 * of the resource name so initializing this widget with a constructed/temporary
 * resource name works as well.
 *
 * \param[in]   resource    resource name
 *
 * \note    The resource name is stored in the "ResourceName" property.
 *
 * \return  GtkEntry
 */
GtkWidget *vice_gtk3_resource_entry_new(const char *resource)
{
    GtkWidget *entry;
    const char *current;

    /* get current resource value */
    if (resources_get_string(resource, &current) < 0) {
        /* invalid resource, set text to NULL */
        log_error(LOG_ERR, "failed to get resource '%s'\n", resource);
        current = NULL;
    }

    entry = gtk_entry_new();
    if (current != NULL) {
        gtk_entry_set_text(GTK_ENTRY(entry), current);
    }

    /* make a copy of the resource name and store the pointer in the propery
     * "ResourceName" */
    resource_widget_set_resource_name(entry, resource);

    /* make a deep copy of the current resource value */
    resource_widget_set_string(entry, "ResourceOrig", current);

    /* register methods to be used by the resource widget manager */
    resource_widget_register_methods(
            entry,
            vice_gtk3_resource_entry_reset,
            vice_gtk3_resource_entry_factory,
            vice_gtk3_resource_entry_sync);

    g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed), NULL);
    g_signal_connect_unlocked(entry, "destroy", G_CALLBACK(on_entry_destroy), NULL);

    gtk_widget_show(entry);
    return entry;
}


/** \brief  Set new \a value for \a entry
 *
 * \param[in,out]   entry   entry
 * \param[in]       new     new text for \a entry
 *
 * \return  TRUE
 */
gboolean vice_gtk3_resource_entry_set(GtkWidget *entry, const char *new)
{
    if (new != NULL) {
        gtk_entry_set_text(GTK_ENTRY(entry), new);
    } else {
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
    return TRUE;
}


/** \brief  Get current resource value of \a widget
 *
 * Retrieves the current resource value and stores it in \a *dest, if retrieving
 * the resource value fails, `FALSE` is returned and \a *dest is set to `NULL`.
 *
 * \param[in]   widget  resource entry
 * \param[out]  dest    object to store value
 *
 * \return  TRUE if the value was retrieved
 */
gboolean vice_gtk3_resource_entry_get(GtkWidget *widget, const char **dest)
{
    const char *resource = resource_widget_get_resource_name(widget);

    if (resources_get_string(resource, dest) < 0) {
        log_error(LOG_ERR, "failed to retrieve value for resource '%s'.", resource);
        *dest = NULL;
        return FALSE;
    }
    return TRUE;
}


/** \brief  Reset \a entry to its resource factory value
 *
 * \param[in]   entry   entry
 *
 * \return  TRUE when the entry was reset to factory
 */
gboolean vice_gtk3_resource_entry_factory(GtkWidget *entry)
{
    const char *resource;
    const char *factory;

    resource = resource_widget_get_resource_name(entry);
    if (resources_get_default_value(resource, &factory) < 0) {
        log_error(LOG_ERR,
                "failed to retrieve factory value for resource '%s'.",
                resource);
        return FALSE;
    }
    return vice_gtk3_resource_entry_set(entry, factory);
}


/** \brief  Reset \a entry to the resource value during instanciation.
 *
 * \param[in,out]   widget  resource entry widget
 *
 * \return  TRUE when the entry was reset to its original value
 */
gboolean vice_gtk3_resource_entry_reset(GtkWidget *widget)
{
    const char *resource;
    const char *orig;
    const char *current;

    resource = resource_widget_get_resource_name(widget);
    orig = resource_widget_get_string(widget, "ResourceOrig");
    if (resources_get_string(resource, &current) < 0) {
        log_error(LOG_ERR, "failed to get value of resource '%s'.", resource);
        return FALSE;
    }

    /* avoid setting the resource when not required */
    if (orig == NULL || current == NULL) {
        /* can't compare them */
        return FALSE;
    }
    if (strcmp(current, orig) != 0) {
        gtk_entry_set_text(GTK_ENTRY(widget), orig);
    }
    return TRUE;
}


/** \brief  Synchronize \a widget with its resource
 *
 * \param[in,out]   widget  resource entry
 *
 * \return  TRUE on when widget was synchronized
 */
gboolean vice_gtk3_resource_entry_sync(GtkWidget *widget)
{
    const char *resource_name;
    const char *resource_val;
    const char *widget_val;

    widget_val = gtk_entry_get_text(GTK_ENTRY(widget));
    resource_name = resource_widget_get_resource_name(widget);
    if (resources_get_string(resource_name, &resource_val) < 0) {
        log_error(LOG_ERR, "failed to get retrieve value for '%s'.", resource_name);
        return FALSE;
    }

    if (widget_val == NULL || resource_val == NULL) {
        /* give up */
        return FALSE;
    }

    if (strcmp(resource_val, widget_val) != 0) {
        /* differs, setting new widget value triggers resource update through
         * event handler */
        gtk_entry_set_text(GTK_ENTRY(widget), resource_val);
    }
    return TRUE;
}




/*
 * Resource entry box that only responds to 'full' changes
 *
 * This entry box only updates its resource when either Enter is pressed or
 * the widget looses focus (user pressing tab, clicking somewhere else). This
 * avoids setting the connected resource with every key pressed, resulting, for
 * example, in rom files 'a', 'ab', 'abc' and 'abcd' while entering 'abcd'.
 */

/** \brief  Handler for the "destroy" event of the full entry box
 *
 * \param[in,out]   entry   full resource entry box
 * \param[in]       data    ununsed
 */
static void on_resource_entry_full_destroy(GtkEntry *entry, gpointer data)
{
    char *tmp;

    tmp = g_object_get_data(G_OBJECT(entry), "ResourceName");
    if (tmp != NULL) {
        lib_free(tmp);
    }
    tmp = g_object_get_data(G_OBJECT(entry), "ResourceOrig");
    if (tmp != NULL) {
        lib_free(tmp);
    }
}


/** \brief  Update resource when it differs from the \a entry's value
 *
 * \param[in]   entry   full resource entry box
 *
 * \return  TRUE when the entry was updated
 */
static gboolean resource_entry_full_update_resource(GtkEntry *entry)
{
    const char *res_name;
    const char *res_val;
    const char *entry_text;

    res_name = resource_widget_get_resource_name(GTK_WIDGET(entry));
    if (resources_get_string(res_name, &res_val) < 0) {
        log_error(LOG_ERR, "failed to retrieve value for resource '%s'\n",
                res_name);
        return FALSE;
    }
    entry_text = gtk_entry_get_text(entry);
    if ((res_val == NULL) || (strcmp(entry_text, res_val) != 0)) {
        if (resources_set_string(res_name, entry_text) < 0) {
            log_error(LOG_ERR, "failed to set resource '%s' to '%s'\n",
                    res_name, entry_text);
            return FALSE;
        }
    }
    return TRUE;
}


/** \brief  Handler for the 'focus-out' event
 *
 * \param[in,out]   entry   entry box
 * \param[in]       event   event object (unused)
 * \param[in]       data    extra event data (unused)
 *
 * \return  TRUE
 */
static gboolean on_focus_out_event(
        GtkEntry *entry,
        GdkEvent *event,
        gpointer data)
{

    if (resource_widget_get_auto_update(GTK_WIDGET(entry))) {
        resource_entry_full_update_resource(entry);
    }
    return FALSE;
}


/** \brief  Handler for the 'on-key-press' event
 *
 * \param[in,out]   entry   entry box
 * \param[in]   event   event object
 * \param[in]   data    unused
 *
 * \return  TRUE if Enter was pushed, FALSE otherwise (makes the pushed key
 *          propagate to the entry)
 */
static gboolean on_key_press_event(
        GtkEntry *entry,
        GdkEvent *event,
        gpointer data)
{
    GdkEventKey *keyev = (GdkEventKey *)event;

    if (resource_widget_get_auto_update(GTK_WIDGET(entry))) {
        if (keyev->type == GDK_KEY_PRESS && keyev->keyval == GDK_KEY_Return) {
            /*
             * We handled Enter/Return for Gtk3/GLib, whether or not the
             * resource actually gets updated is another issue.
             */
            resource_entry_full_update_resource(entry);
            return TRUE;
        }
    }
    return FALSE;
}


/** \brief  Create resource entry box that only reacts to 'full' entries
 *
 * Creates a resource-connected entry box that only updates the resource when
 * the either the widget looses focus (due to Tab or mouse click somewhere else
 * in the UI) or when the user presses 'Enter'. This behaviour differs from the
 * other resource entry which updates its resource on every key press.
 *
 * \param[in]   resource    resource name
 *
 * \return  GtkEntry
 */
GtkWidget *vice_gtk3_resource_entry_full_new(const char *resource)
{
    GtkWidget *entry;
    const char *current;
    char *orig = NULL;

    entry = gtk_entry_new();
    /* make a copy of the resource name and store the pointer in the propery
     * "ResourceName" */
    resource_widget_set_resource_name(entry, resource);

    /* set current value */
    if (resources_get_string(resource, &current) < 0) {
        current = NULL;
    }

    /* make the widget update the resource on change */
    resource_widget_set_auto_update(entry, TRUE);

    /* store current resource value, so it can be restored via
     * resource_entry_full_reset() */
    if (current != NULL) {
        orig = lib_strdup(current);
    } else {
        orig = lib_strdup("");
    }
    g_object_set_data(G_OBJECT(entry), "ResourceOrig", orig);

    if (current != NULL) {
        gtk_entry_set_text(GTK_ENTRY(entry), current);
    }

    /* register methods to be used by the resource widget manager */
    resource_widget_register_methods(
            entry,
            vice_gtk3_resource_entry_full_reset,
            vice_gtk3_resource_entry_full_factory,
            vice_gtk3_resource_entry_full_sync);
    g_signal_connect_unlocked(entry, "destroy",
            G_CALLBACK(on_resource_entry_full_destroy), NULL);
    g_signal_connect(entry, "focus-out-event",
            G_CALLBACK(on_focus_out_event), NULL);
    g_signal_connect(entry, "key-press-event",
            G_CALLBACK(on_key_press_event), NULL);

    return entry;
}


/** \brief  Create resource entry box that only reacts to 'full' entries
 *
 * Creates a resource-connected entry box that only updates the resource when
 * the either the widget looses focus (due to Tab or mouse click somewhere else
 * in the UI) or when the user presses 'Enter'. This behaviour differs from the
 * other resource entry which updates its resource on every key press.
 *
 * This is a variant of vice_gtk3_resource_entry_full_new() that allows using
 * a printf format string to specify the resource name.
 *
 * \param[in]   fmt     resource name format string (printf-style)
 * \param[in]   ...     format string arguments
 *
 * \return  GtkEntry
 */
GtkWidget *vice_gtk3_resource_entry_full_new_sprintf(const char *fmt, ...)
{
    GtkWidget *entry;
    char *resource;
    va_list args;

    va_start(args, fmt);
    resource = lib_mvsprintf(fmt, args);
    va_end(args);
    entry = vice_gtk3_resource_entry_full_new(resource);
    lib_free(resource);
    return entry;
}



/** \brief  Disable the auto updating of the bound resource
 *
 * \param[in,out]   widget  resource entry widget
 */
void vice_gtk3_resource_entry_full_disable_auto_update(GtkWidget *widget)
{
    resource_widget_set_auto_update(widget, FALSE);
}



/** \brief  Reset the widget to the original resource value
 *
 * Resets the widget and the connect resource to the value the resource
 * contained when the widget was created.
 *
 * \param[in,out]   entry   resource entry box
 *
 * \return  TRUE when the entry was reset to its original value
 */
gboolean vice_gtk3_resource_entry_full_reset(GtkWidget *entry)
{
    const char *orig;

    orig = resource_widget_get_string(entry, "ResourceOrig");
    return vice_gtk3_resource_entry_full_set(entry, orig);
}


/** \brief  Update \a entry with text \a new
 *
 * Sets \a new as the new text for \a entry and also updates the connected
 * resource. Returns TRUE on success, FALSE on failure. It is assumed that a
 * failure to set a resource in only due to some registered resource handler
 * failing, not due to an invalid resource name.
 *
 * \param[in,out]   entry   entry box
 * \param[in]       new     new string for \a entry
 *
 * \return  TRUE on success
 */
gboolean vice_gtk3_resource_entry_full_set(GtkWidget *entry, const char *new)
{
    const char *res_name;

    if (new == NULL) {
        new = "";
    }

    res_name = resource_widget_get_resource_name(entry);
    if (resources_set_string(res_name, new) < 0) {
        log_error(LOG_ERR, "failed to set resource %s to '%s'\n", res_name, new);
        return FALSE;
    } else {
        gtk_entry_set_text(GTK_ENTRY(entry), new);
        return TRUE;
    }
}


/** \brief  Get current resource value of \a widget
 *
 * Retrieves the current resource value and stores it in \a *dest, if retrieving
 * the resource value fails, `FALSE` is returned and \a *dest is set to `NULL`.
 *
 * \param[in]   widget  resource entry
 * \param[out]  dest    object to store value
 *
 * \return  TRUE when the value was retrieved
 */
gboolean vice_gtk3_resource_entry_full_get(GtkWidget *widget, const char **dest)
{
    return vice_gtk3_resource_entry_get(widget, dest);
}


/** \brief  Synchronize \a widget with its resource
 *
 * \param[in,out]   widget  resource entry
 *
 * \return  TRUE on success
 */
gboolean vice_gtk3_resource_entry_full_sync(GtkWidget *widget)
{
    return vice_gtk3_resource_entry_sync(widget);
}


/** \brief  Reset \a entry to its resource factory value
 *
 * \param[in]   entry   entry
 *
 * \return  TRUE when the entry was restored to its factory value
 */
gboolean vice_gtk3_resource_entry_full_factory(GtkWidget *entry)
{
    const char *resource;
    const char *factory;

    resource = resource_widget_get_resource_name(entry);
    if (resources_get_default_value(resource, &factory) < 0) {
        log_error(LOG_ERR,
                "failed to retrieve factory value for resource '%s'.",
                resource);
        return FALSE;
    }
    return vice_gtk3_resource_entry_full_set(entry, factory);
}
