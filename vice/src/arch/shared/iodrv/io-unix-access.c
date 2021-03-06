/*
 * io-unix-access.c - Unix specific I/O code.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifdef UNIX_COMPILE

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif
#include <stdlib.h>

#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_HW_INOUT_H)
#include <sys/mman.h>
#include <hw/inout.h>
#endif

#ifdef HAVE_MACHINE_SYSARCH_H
#include <machine/sysarch.h>
#endif

#ifdef HAVE_SYS_IO_H
#include <sys/io.h>
#endif

#ifdef HAVE_MACHINE_PIO_H
#include <machine/pio.h>
#endif

#ifdef HAVE_MACHINE_CPUFUNC_H
#include <machine/cpufunc.h>
#endif

#include "log.h"
#include "types.h"

#include "io-access.h"

#if defined(LINUX_COMPILE) || defined(FREEBSD_COMPILE)

#define IO_PORT_ACCESS

static int io_fd = -1;
#endif

#ifdef LINUX_COMPILE
static int device_io_open(void)
{
    io_fd = open("/dev/port", O_RDWR);

    if (io_fd == -1) {
        log_message(LOG_DEFAULT, "Could not open '/dev/port'.");
        return -1;
    }
    log_message(LOG_DEFAULT, "Opened '/dev/port' for I/O access.");
    return 0;
}

static uint8_t device_io_inb(uint16_t addr)
{
    uint8_t b = 0;

    lseek(io_fd, addr, SEEK_SET);
    if (read(io_fd, &b, 1) != 1) {
        log_message(LOG_DEFAULT, "Could not read from '/dev/io'.");
        return 0;
    }
    return b;
}

static void device_io_outb(uint16_t addr, uint8_t val)
{
    lseek(io_fd, addr, SEEK_SET);
    if (write(io_fd, &val, 1) != 1) {
        log_message(LOG_DEFAULT, "Could not write to '/dev/io'.");
    }
}
#endif

#ifdef FREEBSD_COMPILE
static int device_io_open(void)
{
    io_fd = open("/dev/io", O_RDWR);

    if (io_fd == -1) {
        log_message(LOG_DEFAULT, "Could not open '/dev/io'.");
        return -1;
    }
    log_message(LOG_DEFAULT, "Opened '/dev/io' for I/O access.");
    return 0;
}

static uint8_t device_io_inb(uint16_t addr)
{
#ifdef HAVE_INBV
    return inbv(addr);
#else
    return inb(addr);
#endif
}

static void device_io_outb(uint16_t addr, uint8_t val)
{
#ifdef HAVE_OUTBV
    outbv(addr, val);
#else
    outb(addr, val);
#endif
}
#endif

#ifdef IO_PORT_ACCESS
static void device_io_close(void)
{
    close(io_fd);
    io_fd = -1;
}
#endif

#if !defined(FREEBSD_COMPILE) && (defined(HAVE_LIBAMD64) || defined(HAVE_I386_SET_IOPERM))
static void setaccess(u_long * map, u_int bit, int allow)
{
    u_int word;
    u_int shift;
    u_long mask;

    word = bit / 32;
    shift = bit - (word * 32);

    mask = 0x000000001 << shift;
    if (allow) {
        map[word] &= ~mask;
    } else {
        map[word] |= mask;
    }
}
#endif

#if defined(NETBSD_COMPILE) && defined(HAVE_I386_SET_IOPERM)
static int vice_set_ioperm(unsigned long *iomap)
{
    struct i386_set_ioperm_args arg;

    arg.iomap = iomap;
    return sysarch(I386_SET_IOPERM, &arg);
}

static int vice_get_ioperm(unsigned long *iomap)
{
    struct i386_get_ioperm_args arg;

    arg.iomap = iomap;
    return sysarch(I386_GET_IOPERM, &arg);
}
#endif

#if defined(NETBSD_COMPILE) && defined(HAVE_LIBAMD64)
static int vice_set_ioperm(unsigned long *iomap)
{
    struct x86_64_set_ioperm_args arg;

    arg.iomap = iomap;
    return sysarch(X86_64_SET_IOPERM, &arg);
}

static int vice_get_ioperm(unsigned long *iomap)
{
    struct x86_64_get_ioperm_args arg;

    arg.iomap = iomap;
    return sysarch(X86_64_SET_IOPERM, &arg);
}
#endif

#if defined(OPENBSD_COMPILE) && defined(HAVE_I386_SET_IOPERM)
static int vice_set_ioperm(unsigned long *iomap)
{
    return i386_set_ioperm(iomap);
}

static int vice_get_ioperm(unsigned long *iomap)
{
    return i386_get_ioperm(iomap);
}
#endif

#if defined(OPENBSD_COMPILE) && defined(HAVE_LIBAMD64)
static int vice_set_ioperm(unsigned long *iomap)
{
    return amd64_set_ioperm(iomap);
}

static int vice_get_ioperm(unsigned long *iomap)
{
    return amd64_get_ioperm(iomap);
}
#endif

#if defined(NETBSD_COMPILE) && (defined(HAVE_LIBAMD64) || defined(HAVE_I386_SET_IOPERM))
#define VICE_OUTB_DEFINED
static inline void vice_outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1"
                 : : "a"(val), "Nd"(port));
}

static inline uint8_t vice_inb(uint16_t port)
{
    uint8_t ret;

    asm volatile("inb %1, %0"
                 : "=a"(ret) : "Nd"(port));
    return ret;
}
#endif

#ifdef HAVE_MMAP_DEVICE_IO
#define VICE_OUTB_DEFINED
static inline void vice_outb(uint16_t port, uint8_t val)
{
    out8(port, val);
}

static inline uint8_t vice_inb(uint16_t port)
{
    return in8(port);
}
#endif

#ifndef VICE_OUTB_DEFINED
#  ifdef HAVE_OUTB_P
static inline void vice_outb(uint16_t port, uint8_t val)
{
    outb_p(val, port);
}

static inline uint8_t vice_inb(uint16_t port)
{
    return inb_p(port);
}
#  else
#    ifdef OPENBSD_COMPILE
static inline void vice_outb(uint16_t port, uint8_t val)
{
    outb(port, val);
}
#    else
static inline void vice_outb(uint16_t port, uint8_t val)
{
#ifdef HAVE_OUTB
    outb(val, port);
#endif
}
#    endif
static inline uint8_t vice_inb(uint16_t port)
{
#ifdef HAVE_INB
    return inb(port);
#else
    return 0;
#endif
}
#  endif
#endif

void io_access_store(uint16_t addr, uint8_t value)
{
#ifdef IO_PORT_ACCESS
    if (io_fd != -1) {
        device_io_outb(addr, value);
        return;
    }
#endif
    vice_outb(addr, value);
}

uint8_t io_access_read(uint16_t addr)
{
#ifdef IO_PORT_ACCESS
    if (io_fd != -1) {
        return device_io_inb(addr);
    }
#endif
    return vice_inb(addr);
}

int io_access_map(uint16_t addr, uint16_t space)
{
#if defined(HAVE_LIBAMD64) || defined(HAVE_I386_SET_IOPERM)
#  ifndef FREEBSD_COMPILE
    int i;
    u_long iomap[32];
#  endif
#endif

/* Try device driver based I/O first */
#ifdef IO_PORT_ACCESS
    if (device_io_open() != -1) {
        return 0;
    }
#endif

#ifdef HAVE_MMAP_DEVICE_IO
    if (mmap_device_io(space, addr) != MAP_FAILED) {
        return 0;
    }
#endif

#if defined(HAVE_LIBAMD64) || defined(HAVE_I386_SET_IOPERM)
#  ifndef FREEBSD_COMPILE
    if (vice_get_ioperm(iomap) != -1) {
        for (i = 0; i < space; ++i) {
            setaccess(iomap, addr + i, 1);
        }
        if (vice_set_ioperm(iomap) != -1) {
            return 0;
        }
    }
#  else
    if (i386_set_ioperm(addr, space, 1) != -1) {
        return 0;
    }
#  endif
#endif

#ifdef HAVE_IOPERM
    if (ioperm(addr, space, 1) != -1) {
       return 0;
    }
#endif
    return -1;
}

void io_access_unmap(uint16_t addr, uint16_t space)
{
#if defined(HAVE_LIBAMD64) || defined(HAVE_I386_SET_IOPERM)
#  ifndef FREEBSD_COMPILE
    int i;
    u_long iomap[32];
#  endif
#endif

#ifdef IO_PORT_ACCESS
    if (io_fd != -1) {
        device_io_close();
        return;
    }
#endif

#ifdef HAVE_MMAP_DEVICE_IO
    munmap_device_io(space, addr);
#endif

#if defined(HAVE_LIBAMD64) || defined(HAVE_I386_SET_IOPERM)
#  ifndef FREEBSD_COMPILE
    if (vice_get_ioperm(iomap) != -1) {
        for (i = 0; i < space; ++i) {
            setaccess(iomap, addr + i, 0);
        }
        vice_set_ioperm(iomap);
   }
#  else
    i386_set_ioperm(addr, space, 0);
#  endif
#endif

#ifdef HAVE_IOPERM
    ioperm(addr, space, 0);
#endif

}
#endif
