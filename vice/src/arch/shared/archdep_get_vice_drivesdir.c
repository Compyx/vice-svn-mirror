/** \file   archdep_get_vice_drivesdir.c
 * \brief   Get path to VICE's DRIVES dir
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep_get_vice_datadir.h"
#include "lib.h"
#include "util.h"

#include "archdep_get_vice_drivesdir.h"


/** \brief  Get the absolute path to the VICE DRIVES directory
 *
 * \return  Path to VICE datadir/DRIVES
 *
 * \note    Free result after use with lib_free().
 */
char *archdep_get_vice_drivesdir(void)
{
    char *datadir;
    char *drivesdir;

    datadir = archdep_get_vice_datadir();
    drivesdir = util_join_paths(datadir, "DRIVES", NULL);
    lib_free(datadir);
    return drivesdir;
}
