/** \file   archdep_mkdir.c
 * \brief   Create a directory
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
#include "archdep_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if defined(BEOS_COMPILE) || defined(UNIX_COMPILE)
# include <unistd.h>
# include <sys/stat.h>
# include <sys/types.h>
#endif

#ifdef WINDOWS_COMPILE
# include <direct.h>
#endif

#include "archdep_exit.h"
#include "log.h"

#include "archdep_mkdir.h"


/** \brief  Create a directory \a pathname with \a mode
 *
 * \param[in]   pathname    directory to create
 * \param[in]   mode        access mode of directory (ignored on some systems)
 *
 * \return  0 on success, -1 on error
 */
int archdep_mkdir(const char *pathname, int mode)
{
#if defined(BEOS_COMPILE) || defined(UNIX_COMPILE)
    return mkdir(pathname, (mode_t)mode);
#elif defined(WINDOWS_COMPILE)
    return _mkdir(pathname);
#else
    log_error(LOG_ERR,
            "%s(): not implemented for current system, whoops!\n",
            __func__);
    archdep_vice_exit(1);
#endif
}
