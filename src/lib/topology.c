/*
 * This file is part of yokeybob.
 *
 * Copyright © 2018 Solus Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "topology.h"

#define MIB (1024 * 1024)

/**
 * Default swappiness value for no-swap systems
 */
#define YB_DEFAULT_SWAPPINESS 10

/**
 * Return the available memory in mib
 */
static int64_t yb_available_memory(void)
{
        long page_size = -1;
        long page_count = -1;

        /* Grab the page size */
#ifdef _SC_PAGESIZE
        page_size = sysconf(_SC_PAGESIZE);
#endif
        if (page_size == -1) {
#ifdef PAGESIZE
                page_size = sysconf(PAGESIZE);
#else
                return -1;
#endif
        }

        /* If for some reason we can't grab this, return -1 */
        page_count = sysconf(_SC_PHYS_PAGES);
        if (page_count == -1) {
                return -1;
        }

        return (page_count * page_size) / MIB;
}

/**
 * Determine if swap is available on this system by consulting /proc/swaps
 *
 * We don't really parse /proc/swaps, instead we just make sure we read more
 * than one line from the file (as the header always exists) and assume that
 * the kernel isn't messing us around with invalid entries. It is not our job
 * to validate that swap works, but that it is attached.
 */
static bool yb_has_swap(void)
{
        FILE *fp = NULL;
        char *bfr = NULL;
        size_t n = 0;
        ssize_t read = 0;
        int line_count = 0;

        fp = fopen("/proc/swaps", "r");
        if (!fp) {
                return false;
        }

        while ((read = getline(&bfr, &n, fp)) > 0) {
                if (read < 1) {
                        goto next_line;
                }

                ++line_count;
        next_line:
                free(bfr);
                bfr = NULL;

                /* No sense in more allocs here */
                if (line_count > 1) {
                        break;
                }
        }

        if (bfr) {
                free(bfr);
        }

        fclose(fp);

        return line_count > 1;
}

/**
 * Nothing fancy here yet.
 */
bool yb_topology_init(YbTopology *self)
{
        self->memory.mem_mib = yb_available_memory();
        self->memory.swap_avail = yb_has_swap();

        return true;
}

int8_t yb_topology_get_swappiness(YbTopology *self)
{
        if (!self->memory.swap_avail) {
                return YB_DEFAULT_SWAPPINESS;
        }

        /* Around 8GB memory, low swappiness */
        if (self->memory.mem_mib >= 8192) {
                return YB_DEFAULT_SWAPPINESS;
        }

        /* More than 5gb, go for 20 swappiness */
        if (self->memory.mem_mib >= 5120) {
                return 20;
        }

        /* Floating around 3+/4gb & IGP */
        if (self->memory.mem_mib >= 3000) {
                return 30;
        }

        /* Extremely limited, be bolder with swappiness */
        return 40;
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
