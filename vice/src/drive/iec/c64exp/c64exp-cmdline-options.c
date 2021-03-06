/*
 * c64exp-cmdline-options.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
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

#include <stdio.h>

#include "c64exp-cmdline-options.h"
#include "cmdline.h"
#include "drive.h"
#include "lib.h"

static const cmdline_option_t cmdline_options[] =
{
    { "-profdos1571", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DriveProfDOS1571Name", NULL,
      "<Name>", "Specify name of Professional DOS 1571 ROM image" },
    { "-supercard", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DriveSuperCardName", NULL,
      "<Name>", "Specify name of Supercard+ ROM image" },
    { "-stardos", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DriveStarDosName", NULL,
      "<Name>", "Specify name of StarDOS ROM image" },
    CMDLINE_LIST_END
};

static cmdline_option_t cmd_drive[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<Type>", "Set parallel cable type (0: none, 1: standard, 2: Dolphin DOS 3, 3: Formel 64)" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable Professional DOS" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable Professional DOS" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable Supercard+" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable Supercard+" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable StarDOS" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable StarDOS" },
    CMDLINE_LIST_END
};

int c64exp_cmdline_options_init(void)
{
    int dnr;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        int i;

        cmd_drive[0].name = lib_msprintf("-parallel%i", dnr + 8);
        cmd_drive[0].resource_name
            = lib_msprintf("Drive%iParallelCable", dnr + 8);
        cmd_drive[1].name = lib_msprintf("-drive%iprofdos", dnr + 8);
        cmd_drive[1].resource_name
            = lib_msprintf("Drive%iProfDOS", dnr + 8);
        cmd_drive[2].name = lib_msprintf("+drive%iprofdos", dnr + 8);
        cmd_drive[2].resource_name
            = lib_msprintf("Drive%iProfDOS", dnr + 8);
        cmd_drive[3].name = lib_msprintf("-drive%isupercard", dnr + 8);
        cmd_drive[3].resource_name
            = lib_msprintf("Drive%iSuperCard", dnr + 8);
        cmd_drive[4].name = lib_msprintf("+drive%isupercard", dnr + 8);
        cmd_drive[4].resource_name
            = lib_msprintf("Drive%iSuperCard", dnr + 8);
        cmd_drive[5].name = lib_msprintf("-drive%istardos", dnr + 8);
        cmd_drive[5].resource_name
            = lib_msprintf("Drive%iStarDos", dnr + 8);
        cmd_drive[6].name = lib_msprintf("+drive%istardos", dnr + 8);
        cmd_drive[6].resource_name
            = lib_msprintf("Drive%iStarDos", dnr + 8);

        if (cmdline_register_options(cmd_drive) < 0) {
            return -1;
        }

        for (i = 0; i < 7; i++) {
            lib_free(cmd_drive[i].name);
            lib_free(cmd_drive[i].resource_name);
        }
    }

    return cmdline_register_options(cmdline_options);
}
