/** \file   resourcebrowser.c
 * \brief   Text entry with label and browse button connected to a resource
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This class presents a text entry box and a "Browse ..." button to update
 * a resource, optionally providing a label before the text entry. It is meant
 * as a widget to set a resource that represents a file, such as a kernal image.
 *
 * Internally this widget is a GtkGrid, so when a dialog/widget needs multiple
 * instances of this class, the best thing to do is to set the label to `NULL`
 * and add the labels manually in another GtkGrid or other container to keep
 * things aligned.
 *
 * The constructor is slightly convoluted, but flexible, see
 * #vice_gtk3_resource_browser_new
 * (todo: figure out how to get Doxygen to print the function prototype here)
 *
 * The first argument is required, it's the VICE resource we wish to change, for
 * example "Kernal".
 * The second argument is a list of strings representing file name globbing
 * patterns, for example <tt>{"*.bin", "*.rom", NULL}</tt>
 *
 *
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

#include "archdep.h"
#include "debug_gtk3.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "openfiledialog.h"
#include "savefiledialog.h"
#include "widgethelpers.h"
#include "resourcehelpers.h"
#include "resourceentry.h"
#include "ui.h"
#include "util.h"

#include "resourcebrowser.h"


/** \brief  Object keeping track of the state of the widget
 */
typedef struct resource_browser_state_s {
    char *res_name;             /**< resource name */
    char *res_orig;             /**< resource value at widget creation */
    char **patterns;            /**< file matching patterns */
    char *pattern_name;         /**< name to display for the file patterns */
    char *browser_title;        /**< title to display for the file browser */
    char *append_dir;           /**< directory to use when the resource only
                                     contains a filename and not a path */
    void (*callback)(GtkWidget *,
                     gpointer); /**< optional callback */
    GtkWidget *entry;           /**< GtkEntry reference */
    GtkWidget *button;          /**< GtkButton reference */
} resource_browser_state_t;


/*
 * Forward declarations of functions
 */
static void free_patterns(char **patterns);


/** \brief  Clean up memory used by the main widget
 *
 * \param[in,out]   widget  resource browser widget
 * \param[in]       data    unused
 */
static void on_resource_browser_destroy(GtkWidget *widget, gpointer data)
{
    resource_browser_state_t *state;

    state = (resource_browser_state_t *)(g_object_get_data(G_OBJECT(widget),
                "ViceState"));
    lib_free(state->res_name);
    if (state->res_orig != NULL) {
        lib_free(state->res_orig);
    }
    if (state->patterns != NULL) {
        free_patterns(state->patterns);
    }
    if (state->pattern_name != NULL) {
        lib_free(state->pattern_name);
    }
    if (state->browser_title != NULL) {
        lib_free(state->browser_title);
    }
    if (state->append_dir != NULL) {
        lib_free(state->append_dir);
    }
    lib_free(state);
    resource_widget_free_resource_name(widget);
}


/** \brief  Callback for the dialog
 *
 * \param[in,out]   dialog      Open file dialog reference
 * \param[in]       filename    filename or NULL on cancel
 * \param[in,out]   data        browser state
 */
static void browse_filename_callback(GtkDialog *dialog,
                                     char *filename,
                                     gpointer data)
{
    resource_browser_state_t *state = data;

    if (filename != NULL) {
        if (!vice_gtk3_resource_entry_full_set(state->entry, filename)){
            log_error(LOG_ERR,
                    "failed to set resource %s to '%s', reverting\n",
                    state->res_name, filename);
            /* restore resource to original state */
            resources_set_string(state->res_name, state->res_orig);
            gtk_entry_set_text(GTK_ENTRY(state->entry), state->res_orig);
        } else {
            if (state->callback != NULL) {
                state->callback(GTK_WIDGET(dialog), (gpointer)filename);
            }
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Callback for the save dialog
 *
 * \param[in,out]   dialog      Save file dialog reference
 * \param[in]       filename    filename or NULL on cancel
 * \param[in,out]   data        browser state
 */
static void save_filename_callback(GtkDialog *dialog,
                                     char *filename,
                                     gpointer data)
{
    resource_browser_state_t *state = data;

    if (filename != NULL) {
        if (!vice_gtk3_resource_entry_full_set(state->entry, filename)){
            log_error(LOG_ERR,
                    "failed to set resource %s to '%s', reverting\n",
                    state->res_name, filename);
            /* restore resource to original state */
            resources_set_string(state->res_name, state->res_orig);
            gtk_entry_set_text(GTK_ENTRY(state->entry), state->res_orig);
        } else {
            if (state->callback != NULL) {
                state->callback(GTK_WIDGET(dialog), (gpointer)filename);
            }
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Handler for the "clicked" event of the browse button
 *
 * Shows a file open dialog to select a file.
 *
 * If the connected resource value contains a valid filename/path, the dialog's
 * directory is set to that file's directory. If only a filename is given, such
 * as the default KERNAL, $datadir/$machine_name is used for the directory.
 * In the dialog's directory listing the current file is selected.
 *
 * \param[in]   widget  browse button
 * \param[in]   data    unused
 *
 */
static void on_resource_browser_browse_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *parent;
    resource_browser_state_t *state;
    const char *res_value = NULL;

    parent = gtk_widget_get_parent(widget);
    state = g_object_get_data(G_OBJECT(parent), "ViceState");
    /* get the filename/path in the resource */
    resources_get_string(state->res_name, &res_value);
    debug_gtk3("resource '%s' = '%s'", state->res_name, res_value);

    dialog = vice_gtk3_open_file_dialog(
            state->browser_title,
            state->pattern_name,
            (const char **)(state->patterns),
            NULL,
            browse_filename_callback,
            state);

    /* set browser directory to directory in resource if available */
    if (res_value != NULL) {
        /* get dirname and basename */
        gchar *dirname = g_path_get_dirname(res_value);
        gchar *basename = g_path_get_basename(res_value);

        debug_gtk3("dirname = '%s', basename = '%s'", dirname, basename);

        /* if no path is present in the resource value, set the directory to
         * the VICE datadir + machine and the file to the current filename */
        if (strcmp(dirname, ".") == 0 && state->append_dir != NULL) {
            char *fullpath = util_join_paths(state->append_dir, basename, NULL);

            debug_gtk3("fullpath = '%s'", fullpath);
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), fullpath);
            lib_free(fullpath);
        } else {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), res_value);
        }

        /* clean up */
        g_free(dirname);
        g_free(basename);
    }

}


/** \brief  Handler for the "clicked" event of the save button
 *
 * \param[in]   widget  save button
 * \param[in]   data    unused
 */
static void on_resource_browser_save_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *parent;
    resource_browser_state_t *state;

    parent = gtk_widget_get_parent(widget);
    state = g_object_get_data(G_OBJECT(parent), "ViceState");

    vice_gtk3_save_file_dialog(
            state->browser_title,
            NULL,
            FALSE,  /* FIXME: False results in a weird segfault */
            NULL,
            save_filename_callback,
            state);
}


/** \brief  Create a heap allocated copy of \a patterns
 *
 * \param[in]   patterns    file filter patterns
 *
 * \return  deep copy of \a patterns
 */
static char **copy_patterns(const char * const *patterns)
{
    size_t num = 0;
    char **arr;

    if (patterns == NULL || *patterns == NULL) {
        return NULL;
    }

    /* count number of patterns */
    while (patterns[num++] != NULL) {
        /* NOP */
    }

    arr = lib_malloc((num + 1) * sizeof *arr);
    num = 0;
    while (patterns[num] != NULL) {
        arr[num] = lib_strdup(patterns[num]);
        num++;
    }
    arr[num] = NULL;
    return arr;
}


/** \brief  Clean up copies of the file name patterns
 *
 * \param[in,out]   patterns    list of file name matching patterns
 */
static void free_patterns(char **patterns)
{
    size_t i = 0;

    while (patterns[i] != NULL) {
        lib_free(patterns[i]);
        i++;
    }
    lib_free(patterns);
}


/** \brief  Create file selection widget with browse button
 *
 * \param[in]   resource        resource name
 * \param[in]   patterns        file match patterns (optional)
 * \param[in]   pattern_name    name to use for \a patterns in the file dialog
 *                              (optional)
 * \param[in]   browser_title   title to display in the file dialog (optional,
 *                              defaults to "Select file")
 * \param[in]   label           label (optional)
 * \param[in]   callback        user callback (optional, not implemented yet)
 *
 * \note    both \a patterns and \a pattern_name need to be defined (ie not
 *          `NULL` for the \a patterns to work.
 *
 * \return  GtkGrid
 */
GtkWidget *vice_gtk3_resource_browser_new(
        const char *resource,
        const char * const *patterns,
        const char *pattern_name,
        const char *browser_title,
        const char *label,
        void (*callback)(GtkWidget *, gpointer))
{
    GtkWidget *grid;
    GtkWidget *lbl = NULL;
    resource_browser_state_t *state;
    const char *orig;
    int column = 0;

    grid = vice_gtk3_grid_new_spaced(16, 0);

    /* allocate and init state object */
    state = lib_malloc(sizeof *state);

    /* copy resource name */
    state->res_name = lib_strdup(resource);
    resource_widget_set_resource_name(grid, resource);

    /* get current value of resource */
    if (resources_get_string(resource, &orig) < 0) {
        orig = "";
    } else if (orig == NULL) {
        orig = "";
    }
    state->res_orig = lib_strdup(orig);

    /* store optional callback */
    state->callback = callback;

    /* copy file matching patterns */
    state->patterns = copy_patterns(patterns);

    /* copy pattern name */
    if (pattern_name != NULL && *pattern_name != '\0') {
        state->pattern_name = lib_strdup(pattern_name);
    } else {
        state->pattern_name = NULL;
    }

    /* copy browser title */
    if (browser_title != NULL && *browser_title != '\0') {
        state->browser_title = lib_strdup("Select file");
    } else {
        state->browser_title = lib_strdup(browser_title);
    }

    /* set append dir to `NULL` */
    state->append_dir = NULL;

    /*
     * add widgets to the grid
     */

    /* label, if given */
    if (label != NULL && *label != '\0') {
        lbl = gtk_label_new(label);
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);
        column++;
    }

    /* text entry */
    state->entry = vice_gtk3_resource_entry_full_new(resource);
    gtk_widget_set_hexpand(state->entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), state->entry, column, 0, 1, 1);
    column++;

    /* browse button */
    state->button = gtk_button_new_with_label("Browse ...");
    gtk_grid_attach(GTK_GRID(grid), state->button, column, 0, 1,1);

    /* store the state object in the widget */
    g_object_set_data(G_OBJECT(grid), "ViceState", (gpointer)state);

    /* register methods to be used by the resource widget manager */
    resource_widget_register_methods(
            grid,
            vice_gtk3_resource_browser_reset,
            vice_gtk3_resource_browser_factory,
            vice_gtk3_resource_browser_sync);

    /* connect signal handlers */
    g_signal_connect(state->button, "clicked",
            G_CALLBACK(on_resource_browser_browse_clicked), NULL);
    g_signal_connect_unlocked(grid, "destroy", G_CALLBACK(on_resource_browser_destroy),
            NULL);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set \a widget value to \a new
 *
 * \param[in,out]   widget  resource browser widget
 * \param[in]       new     new value for \a widget
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_browser_set(GtkWidget *widget, const char *new)
{
    resource_browser_state_t *state;
    state = g_object_get_data(G_OBJECT(widget), "ViceState");

    if (resources_set_string(state->res_name, new) < 0) {
        /* restore to default */
        resources_set_string(state->res_name, state->res_orig);
        gtk_entry_set_text(GTK_ENTRY(state->entry), state->res_orig);
        return FALSE;
    } else {
        gtk_entry_set_text(GTK_ENTRY(state->entry), new != NULL ? new : "");
        return TRUE;
    }
}


/** \brief  Get the current value of \a widget
 *
 * Get the current resource value of \a widget and store it in \a dest. If
 * getting the resource value fails for some reason, `FALSE` is returned and
 * \a dest is set to `NULL`.
 *
 * \param[in]   widget  resource browser widget
 * \param[out]  dest    destination of value
 *
 * \return  TRUE when the resource value was retrieved
 */
gboolean vice_gtk3_resource_browser_get(GtkWidget *widget, const char **dest)
{
    resource_browser_state_t *state;
    state = g_object_get_data(G_OBJECT(widget), "ViceState");

    if (resources_get_string(state->res_name, dest) < 0) {
        *dest = NULL;
        return FALSE;
    }
    return TRUE;
}


/** \brief  Restore resource in \a widget to its original value
 *
 * \param[in,out]   widget  resource browser widget
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_browser_reset(GtkWidget *widget)
{
    resource_browser_state_t *state;

    state = g_object_get_data(G_OBJECT(widget), "ViceState");

    /* restore resource value */
    if (resources_set_string(state->res_name, state->res_orig) < 0) {
        return FALSE;
    }
    /* update text entry */
    gtk_entry_set_text(GTK_ENTRY(state->entry), state->res_orig);
    return TRUE;
}


/** \brief  Synchronize widget with current resource value
 *
 * Only needed to call if the resource's value is changed from code other than
 * this widget's code.
 *
 * \param[in,out]   widget  resource browser widget
 *
 * \return  TRUE if the resource value was retrieved
 */
gboolean vice_gtk3_resource_browser_sync(GtkWidget *widget)
{
    resource_browser_state_t *state;
    const char *value;

    /* get current resource value */
    state = g_object_get_data(G_OBJECT(widget), "ViceState");
    if (resources_get_string(state->res_name, &value) < 0) {
        return FALSE;
    }

    gtk_entry_set_text(GTK_ENTRY(state->entry), value);
    return TRUE;
}


/** \brief  Reset widget to the resource's factory value
 *
 * \param[in,out]   widget  resource browser widget
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_browser_factory(GtkWidget *widget)
{
    resource_browser_state_t *state;
    const char *value;

    /* get resource factory value */
    state = g_object_get_data(G_OBJECT(widget), "ViceState");
    if (resources_get_default_value(state->res_name, &value) < 0) {
        return FALSE;
    }
    return vice_gtk3_resource_browser_set(widget, value);
}


/** \brief  Resource browser widget to select a file to save
 *
 * \param[in]   resource        resource name
 * \param[in]   browser_title   dialog title
 * \param[in]   label           optional label before the text entry
 * \param[in]   suggested       suggested filename (unimplemented)
 * \param[in]   callback        callback
 *
 * \return  GtkGrid
 */
GtkWidget *vice_gtk3_resource_browser_save_new(
        const char *resource,
        char *browser_title,
        char *label,
        const char *suggested,
        void (*callback)(GtkWidget *, gpointer))
{
    GtkWidget *grid;
    GtkWidget *lbl;
    resource_browser_state_t *state;
    const char *orig = NULL;
    int column = 0;

    grid = vice_gtk3_grid_new_spaced(16, 0);

    /* alloc and init state object */
    state = lib_malloc(sizeof *state);
    state->res_name = lib_strdup(resource);
    resource_widget_set_resource_name(grid, resource);

    /* get current value, if any */
    if (resources_get_string(resource, &orig) < 0) {
        orig = "";
    } else if (orig == NULL) {
        orig = "";
    }
    state->res_orig = lib_strdup(orig);
    state->patterns = NULL;
    state->pattern_name = NULL;
    state->append_dir = NULL;

    /* copy browser title */
    if (browser_title != NULL) {
        state->browser_title = lib_strdup("Select file");
    } else {
        state->browser_title = lib_strdup(browser_title);
    }

    /*
     * Add widgets
     */

    /* label */
    if (label != NULL) {
        lbl = gtk_label_new(label);
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);
        column++;
    }

    /* text entry */
    state->entry = vice_gtk3_resource_entry_full_new(resource);
    gtk_widget_set_hexpand(state->entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), state->entry, column, 0, 1, 1);
    column++;

    /* browse button */
    state->button = gtk_button_new_with_label("Browse ...");
    gtk_grid_attach(GTK_GRID(grid), state->button, column, 0, 1,1);

    /* store the state object in the widget */
    g_object_set_data(G_OBJECT(grid), "ViceState", (gpointer)state);

    /* connect signal handlers */
    g_signal_connect(state->button, "clicked",
            G_CALLBACK(on_resource_browser_save_clicked), NULL);
    g_signal_connect_unlocked(grid, "destroy", G_CALLBACK(on_resource_browser_destroy),
            NULL);
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set the directory to use when the resource only contains a filename
 *
 * \param[in,out]   widget  resource browser widget
 * \param[in]       path    directory to use
 */
void vice_gtk3_resource_browser_set_append_dir(GtkWidget *widget,
                                               const char *path)
{
    resource_browser_state_t *state;

    state = g_object_get_data(G_OBJECT(widget), "ViceState");
    if (state->append_dir != NULL) {
        lib_free(state->append_dir);
        state->append_dir = NULL;
    }
    if (path != NULL && *path != '\0') {
        state->append_dir = lib_strdup(path);
    }
}

