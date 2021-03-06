/*
 * driveimage.c
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
#include <string.h>

#include "diskconstants.h"
#include "diskimage.h"
#include "drive.h"
#include "driveimage.h"
#include "drivetypes.h"
#include "gcr.h"
#include "log.h"
#include "types.h"


/* Logging goes here.  */
static log_t driveimage_log = LOG_DEFAULT;

int drive_image_type_to_drive_type(unsigned int type)
{
    switch (type) {
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D64:
            return DRIVE_TYPE_1541II;
        case DISK_IMAGE_TYPE_G71:
        case DISK_IMAGE_TYPE_D71:
            return DRIVE_TYPE_1571;
        case DISK_IMAGE_TYPE_D81:
            return DRIVE_TYPE_1581;
        case DISK_IMAGE_TYPE_D1M:
        case DISK_IMAGE_TYPE_D2M:
            return DRIVE_TYPE_2000;
        case DISK_IMAGE_TYPE_D4M:
            return DRIVE_TYPE_4000;
        case DISK_IMAGE_TYPE_D67:
            return DRIVE_TYPE_2040;
        case DISK_IMAGE_TYPE_D80:
            return DRIVE_TYPE_8050;
        case DISK_IMAGE_TYPE_D82:
            return DRIVE_TYPE_8250;
        case DISK_IMAGE_TYPE_D90:
            return DRIVE_TYPE_9000;
        case DISK_IMAGE_TYPE_DHD:
            return DRIVE_TYPE_CMDHD;
    }
    return DRIVE_TYPE_NONE;
}

int drive_check_image_format(unsigned int format, unsigned int dnr)
{
    diskunit_context_t *unit = diskunit_context[dnr];

    switch (format) {
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
#endif
            if (unit->type != DRIVE_TYPE_1540
                && unit->type != DRIVE_TYPE_1541
                && unit->type != DRIVE_TYPE_1541II
                && unit->type != DRIVE_TYPE_1551
                && unit->type != DRIVE_TYPE_1570
                && unit->type != DRIVE_TYPE_1571
                && unit->type != DRIVE_TYPE_1571CR
                && unit->type != DRIVE_TYPE_2031
                && unit->type != DRIVE_TYPE_2040 /* FIXME: only read compat */
                && unit->type != DRIVE_TYPE_3040
                && unit->type != DRIVE_TYPE_4040) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_G71:
            if ((unit->type != DRIVE_TYPE_1571)
                && (unit->type != DRIVE_TYPE_1571CR)) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_D67:
            /* New drives and 2031, 3040 and 4040 are only read compatible.  */
            if (unit->type != DRIVE_TYPE_1540
                && unit->type != DRIVE_TYPE_1541
                && unit->type != DRIVE_TYPE_1541II
                && unit->type != DRIVE_TYPE_1551
                && unit->type != DRIVE_TYPE_1570
                && unit->type != DRIVE_TYPE_1571
                && unit->type != DRIVE_TYPE_1571CR
                && unit->type != DRIVE_TYPE_2031
                && unit->type != DRIVE_TYPE_2040
                && unit->type != DRIVE_TYPE_3040
                && unit->type != DRIVE_TYPE_4040) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_D71:
            if (unit->type != DRIVE_TYPE_1571
                && unit->type != DRIVE_TYPE_1571CR) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_D81:
            if (unit->type != DRIVE_TYPE_1581
                && unit->type != DRIVE_TYPE_2000
                && unit->type != DRIVE_TYPE_4000) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            if ((unit->type != DRIVE_TYPE_1001)
                && (unit->type != DRIVE_TYPE_8050)
                && (unit->type != DRIVE_TYPE_8250)) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_D90:
            if (unit->type != DRIVE_TYPE_9000) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_D1M:
        case DISK_IMAGE_TYPE_D2M:
        case DISK_IMAGE_TYPE_D4M:
            if (unit->type != DRIVE_TYPE_2000
                && unit->type != DRIVE_TYPE_4000) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_DHD:
            if (unit->type != DRIVE_TYPE_CMDHD) {
                return -1;
            }
            break;
        default:
            return -1;
    }
    return 0;
}

/* Attach a disk image to the true drive emulation. */
int drive_image_attach(disk_image_t *image, unsigned int unit, unsigned int drv)
{
    unsigned int dnr;
    drive_t *drive;

    if (unit < 8 || unit >= 8 + NUM_DISK_UNITS) {
        return -1;
    }

    dnr = unit - 8;
    drive = diskunit_context[dnr]->drives[drv];

    if (drive_check_image_format(image->type, dnr) < 0) {
        return -1;
    }

    drive->read_only = image->read_only;
    drive->attach_clk = diskunit_clk[dnr];
    if (drive->detach_clk > (CLOCK)0) {
        drive->attach_detach_clk = diskunit_clk[dnr];
    }
    drive->ask_extend_disk_image = DRIVE_EXTEND_ASK;

    switch (image->type) {
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_D67:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_G71:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
#endif
        case DISK_IMAGE_TYPE_P64:
            disk_image_attach_log(image, driveimage_log, unit, drv);
            break;
        default:
            return -1;
    }

    drive->image = image;
    drive->image->gcr = drive->gcr;
    drive->image->p64 = (void*)drive->p64;

    if (disk_image_read_image(drive->image) < 0) {
        drive->image = NULL;
        return -1;
    }
    if (drive->image->type == DISK_IMAGE_TYPE_P64) {
        drive->P64_image_loaded = 1;
        drive->P64_dirty = 0;
    } else {
        drive->GCR_image_loaded = 1;
    }
    drive->complicated_image_loaded = ((drive->image->type == DISK_IMAGE_TYPE_P64)
                                       || (drive->image->type == DISK_IMAGE_TYPE_G64)
                                       || (drive->image->type == DISK_IMAGE_TYPE_G71));
    drive_set_half_track(drive->current_half_track, drive->side, drive);
    return 0;
}

/* Detach a disk image from the true drive emulation. */
int drive_image_detach(disk_image_t *image, unsigned int unit, unsigned int drv)
{
    unsigned int dnr, i;
    diskunit_context_t *diskunit;
    drive_t *drive;

    if (unit < 8 || unit >= 8 + NUM_DISK_UNITS) {
        return -1;
    }

    dnr = unit - 8;
    diskunit = diskunit_context[dnr];
    drive = diskunit->drives[drv];

    if (drive->image != NULL) {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D64:
            case DISK_IMAGE_TYPE_D67:
            case DISK_IMAGE_TYPE_D71:
            case DISK_IMAGE_TYPE_G64:
            case DISK_IMAGE_TYPE_G71:
            case DISK_IMAGE_TYPE_P64:
#ifdef HAVE_X64_IMAGE
            case DISK_IMAGE_TYPE_X64:
#endif
                disk_image_detach_log(image, driveimage_log, unit, drv);
                break;
            default:
                return -1;
        }
    }

    if (drive->P64_image_loaded && drive->P64_dirty) {
        drive->P64_dirty = 0;
        if (disk_image_write_p64_image(drive->image) < 0) {
            log_error(diskunit->log, "Cannot write disk image back.");
        }
    } else {
        drive_gcr_data_writeback(drive);
    }

    for (i = 0; i < MAX_GCR_TRACKS; i++) {
        if (drive->gcr->tracks[i].data) {
            lib_free(drive->gcr->tracks[i].data);
            drive->gcr->tracks[i].data = NULL;
            drive->gcr->tracks[i].size = 0;
        }
    }
    drive->detach_clk = diskunit_clk[dnr];
    drive->GCR_image_loaded = 0;
    drive->P64_image_loaded = 0;
    drive->read_only = 0;
    drive->image = NULL;
    drive_set_half_track(drive->current_half_track, drive->side, drive);

    return 0;
}

void drive_image_init(void)
{
    driveimage_log = log_open("DriveImage");
}
