/** \file   vsidcontrolwidget.c
 * \brief   GTK3 control widget for VSID
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * Icons used by this file:
 *
 * $VICEICON    actions/media-skip-backward
 * $VICEICON    actions/media-playback-start
 * $VICEICON    actions/media-playback-pause
 * $VICEICON    actions/media-playback-stop
 * $VICEICON    actions/media-seek-forward
 * $VICEICON    actions/media-skip-forward
 * $VICEICON    actions/media-eject
 * $VICEICON    actions/media-record
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

#include "vice_gtk3.h"
#include "c64mem.h"
#include "debug.h"
#include "machine.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "psid.h"
#include "uicommands.h"
#include "ui.h"
#include "uisidattach.h"
#include "vsidstate.h"
#include "vsidtuneinfowidget.h"
#include "vsync.h"

#include "vsidcontrolwidget.h"


/** \brief  Emulation speed during fast forward
 */
#define FFWD_SPEED  500



/** \brief  Object containing icon and callback
 */
typedef struct vsid_ctrl_button_s {
    const char *icon_name;                      /**< icon name */
    void (*callback)(GtkWidget *, gpointer);    /**< callback */
    const char *tooltip;                        /**< tool tip */
} vsid_ctrl_button_t;


#if 0
/** \brief  Number of subtunes in the SID */
static int tune_count;

/** \brief  Current subtune number */
static int tune_current;

/** \brief  Default subtune number */
static int tune_default;
#endif


/** \brief  Progress bar */
static GtkWidget *progress = NULL;

/** \brief  Repeat toggle button */
static GtkWidget *repeat = NULL;


/** \brief  Temporary callback for the media buttons
 *
 * \param[in]   widget  widget
 * \param[in]   data    icon name
 */
static void fake_callback(GtkWidget *widget, gpointer data)
{
    debug_gtk3("Unsupported callback for '%s'.", (const char *)data);
}


/** \brief  Trigger play of current tune */
static void play_current_tune(void)
{
    vsid_state_t *state = vsid_state_lock();
    int current = state->tune_current;
#ifdef HAVE_DEBUG_GTK3UI
    int count = state->tune_count;
    int def = state->tune_default;
#endif

    vsid_state_unlock();

    debug_gtk3("current: %d, total: %d, default: %d.",
                current, count, def);
    debug_gtk3("calling machine_trigger_reset(SOFT).");
    machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
    debug_gtk3("calling psid_init_driver().");
    psid_init_driver();
    debug_gtk3("calling machine_play_psid(%d).", current);
    machine_play_psid(current);
}


/** \brief  Callback for 'next subtune'
 *
 * Select next subtune, or wrap around to the first subtune
 *
 * \param[in]   widget  widget
 * \param[in]   data    icon name
 */
static void next_tune_callback(GtkWidget *widget, gpointer data)
{
    vsid_state_t *state = vsid_state_lock();

    debug_gtk3("called.");


    if (state->tune_current >= state->tune_count || state->tune_current < 1) {
        state->tune_current = 1;
    } else {
        state->tune_current++;
    }
    vsid_state_unlock();

    play_current_tune();
}


/** \brief  Callback for 'previous subtune'
 *
 * Select previous subtune, or wrap aroun to the last subtune
 *
 * \param[in]   widget  widget
 * \param[in]   data    icon name
 */
static void prev_tune_callback(GtkWidget *widget, gpointer data)
{
    vsid_state_t *state;

    state = vsid_state_lock();

    debug_gtk3("called.");
    if (state->tune_current <= 1) {
        state->tune_current = state->tune_count;
    } else {
        state->tune_current--;
    }
    vsid_state_unlock();

    play_current_tune();
}


/** \brief  Callback for 'fast forward'
 *
 * Fast forward using Speed resource (toggled)
 *
 * \param[in]   widget  widget
 * \param[in]   data    icon name
 */
static void ffwd_callback(GtkWidget *widget, gpointer data)
{
    int speed;

    if (resources_get_int("Speed", &speed) < 0) {
        /* error, shouldn't happen */
        return;
    }

    if (speed == 100) {
        resources_set_int("Speed", FFWD_SPEED);
    } else {
        resources_set_int("Speed", 100);
    }
}


/** \brief  Callback for 'play'
 *
 * Continue playback by using the emulator's pause feature.
 *
 * \param[in]   widget  widget
 * \param[in]   data    icon name
 */
static void play_callback(GtkWidget *widget, gpointer data)
{
    vsid_state_t *state;
    int current;

    state = vsid_state_lock();
    current = state->tune_current;

    if (current <= 0) {
        /* restart previous tune if stopped before */
        current = state->tune_current = state->tune_default;
        vsid_state_unlock();

        /* reload unloaded PSID file if loaded before */
        if (state->psid_filename != NULL) {
            psid_load_file(state->psid_filename);
        }

        psid_init_driver();
        machine_play_psid(current);
        machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
        ui_pause_disable();
    } else {
        /* return emulation speed back to 100% */
        vsid_state_unlock();
        resources_set_int("Speed", 100);
    }
}


/** \brief  Callback for 'pause'
 *
 * Pause playback by using the emulator's pause feature.
 *
 * \param[in]   widget  widget
 * \param[in]   data    icon name
 */
static void pause_callback(GtkWidget *widget, gpointer data)
{
    ui_pause_toggle();
}


static void stop_callback(GtkWidget *widget, gpointer data)
{
    vsid_state_t *state = vsid_state_lock();

    state->tune_current = -1;
    vsid_state_unlock();

    machine_play_psid(-1);
    machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
}


/** \brief  Wrapper for the attach callback
 *
 * \param[in,out]   widget  control button
 * \param[in]       data    icon name
 */
static void sid_attach_wrapper(GtkWidget *widget, gpointer data)
{
    uisidattach_show_dialog(widget, data);
}


/** \brief  List of media control buttons
 */
static const vsid_ctrl_button_t buttons[] = {
    { "media-skip-backward", prev_tune_callback,
        "Go to previous subtune" },
    { "media-playback-start", play_callback,
        "Play tune" },
    { "media-playback-pause", pause_callback,
        "Pause playback" },
    { "media-playback-stop", stop_callback,
        "Stop playback" },
    { "media-seek-forward", ffwd_callback,
        "Fast forward" },
    { "media-skip-forward", next_tune_callback,
        "Go to next subtune" },   /* select next tune */
    { "media-eject", sid_attach_wrapper,
        "Load PSID file" },   /* active file-open dialog */
    { "media-record", fake_callback,
        "Record media" },  /* start recording with current settings*/
    { NULL, NULL, NULL }
};



/** \brief  Create widget with media buttons to control VSID playback
 *
 * \return  GtkGrid
 */
GtkWidget *vsid_control_widget_create(void)
{
    GtkWidget *grid;
    int i;

    grid = vice_gtk3_grid_new_spaced(0, VICE_GTK3_DEFAULT);

    for (i = 0; buttons[i].icon_name != NULL; i++) {
        GtkWidget *button;
        gchar buf[1024];

        g_snprintf(buf, sizeof(buf), "%s-symbolic", buttons[i].icon_name);

        button = gtk_button_new_from_icon_name(buf,
                GTK_ICON_SIZE_LARGE_TOOLBAR);
        /* always show the image, the button would useless without an image */
        gtk_button_set_always_show_image(GTK_BUTTON(button), TRUE);
        /* don't initialy focus on a button */
        gtk_widget_set_can_focus(button, FALSE);
#if 0
        /* don't grab focus when clicked */
        gtk_widget_set_focus_on_click(button, FALSE);
#endif
        gtk_grid_attach(GTK_GRID(grid), button, i, 0, 1,1);
        if (buttons[i].callback != NULL) {
            g_signal_connect(button, "clicked",
                    G_CALLBACK(buttons[i].callback),
                    (gpointer)(buttons[i].icon_name));
        }
        if (buttons[i].tooltip != NULL) {
            gtk_widget_set_tooltip_text(button, buttons[i].tooltip);
        }
    }

    /* add progress bar */
    progress = gtk_progress_bar_new();
    gtk_grid_attach(GTK_GRID(grid), progress, 0, 1, i, 1);

    /* Add loop check button
     *
     * I'm pretty sure there's a loop icon, so perhaps add that to the control
     * buttons in stead of using this check button.
     */
    repeat = gtk_check_button_new_with_label("Loop current song");
    gtk_grid_attach(GTK_GRID(grid), repeat, 0, 2, i, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(repeat), TRUE);
    gtk_widget_set_can_focus(repeat, FALSE);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set number of tunes
 *
 * \param[in]   n   tune count
 */
void vsid_control_widget_set_tune_count(int n)
{
    vsid_state_t *state = vsid_state_lock();

    state->tune_count = n;
    vsid_state_unlock();
}


/** \brief  Set current tune
 *
 * \param[in]   n   tune number
 */
void vsid_control_widget_set_tune_current(int n)
{
    vsid_state_t *state = vsid_state_lock();

    state->tune_current = n;
    vsid_state_unlock();
}


/** \brief  Set default tune
 *
 * \param[in]   n   tune number
 */
void vsid_control_widget_set_tune_default(int n)
{
    vsid_state_t *state = vsid_state_lock();

    state->tune_default = n;

    vsid_state_unlock();
}


/** \brief  Set tune progress bar value
 *
 * \param[in]   fraction    amount to fill (0.0 - 1.0)
 */
void vsid_control_widget_set_progress(gdouble fraction)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), fraction);
}


/** \brief  Play next tune
 */
void vsid_control_widget_next_tune(void)
{
    next_tune_callback(NULL, NULL);
}


/** \brief  Get repeat/loop widget state
 *
 * \return  loop state
 */
gboolean vsid_control_widget_get_repeat(void)
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(repeat));
}
