/*
 * tpid.h
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

#ifndef VICE_TPID_H
#define VICE_TPID_H

#include "types.h"

struct diskunit_context_s;
struct tpi_context_s;

extern void tpid_setup_context(struct diskunit_context_s *ctxptr);

extern void tpid_init(struct diskunit_context_s *ctxptr);
extern void tpid_store(struct diskunit_context_s *ctxptr, uint16_t addr, uint8_t byte);
extern uint8_t tpid_read(struct diskunit_context_s *ctxptr, uint16_t addr);
extern uint8_t tpid_peek(struct diskunit_context_s *ctxptr, uint16_t addr);
extern int tpid_dump(diskunit_context_t *ctxptr, uint16_t addr);

#endif
