/*
 * tcbm-resources.c
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

#include "driverom.h"
#include "lib.h"
#include "resources.h"
#include "tcbm-resources.h"
#include "tcbmrom.h"
#include "util.h"

static char *dos_rom_name_1551 = NULL;

static int set_dos_rom_name_1551(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_1551, val)) {
        return 0;
    }

    return tcbmrom_load_1551();
}

static const resource_string_t resources_string[] = {
    { "DosName1551", DRIVE_ROM1551_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_1551, set_dos_rom_name_1551, NULL },
    RESOURCE_STRING_LIST_END
};

int tcbm_resources_init(void)
{
    return resources_register_string(resources_string);
}

void tcbm_resources_shutdown(void)
{
    lib_free(dos_rom_name_1551);
}
