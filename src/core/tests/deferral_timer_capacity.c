// SPDX-FileCopyrightText: 2026 Ben Jarvis
//
// SPDX-License-Identifier: LGPL-2.1-only

/*
 * More armed deferrals, and more timers, than the initial 256-entry arrays.
 * evpl_defer() and evpl_timer_insert() used to write past the end of those
 * fixed arrays (heap-buffer-overflow under AddressSanitizer); they must grow.
 *
 *   - 1000 distinct deferrals armed before the loop runs are each delivered
 *     exactly once;
 *   - 1000 timers can be added and then removed (heap growth and removal),
 *     with no overflow.  Delivery timing of timers is not checked here.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "evpl/evpl.h"

#define N 1000

static int fired[N];

static void
deferral_cb(
    struct evpl *evpl,
    void        *arg)
{
    (void) evpl;
    fired[(int) (intptr_t) arg]++;
} /* deferral_cb */

static void
timer_cb(
    struct evpl       *evpl,
    struct evpl_timer *timer)
{
    (void) evpl;
    (void) timer;
} /* timer_cb */

int
main(void)
{
    static struct evpl_deferral deferrals[N];
    static struct evpl_timer    timers[N];
    struct evpl                *evpl = evpl_create(NULL);
    int                         done, i, failures = 0;

    for (i = 0; i < N; i++) {
        evpl_deferral_init(&deferrals[i], deferral_cb, (void *) (intptr_t) i);
        evpl_defer(evpl, &deferrals[i]);
    }

    do {
        evpl_continue(evpl);
        done = 1;
        for (i = 0; i < N; i++) {
            if (!fired[i]) {
                done = 0;
                break;
            }
        }
    } while (!done);

    for (i = 0; i < N; i++) {
        if (fired[i] != 1) {
            fprintf(stderr, "deferral %d fired %d times\n", i, fired[i]);
            failures++;
        }
    }

    /* A long interval so none comes due while we add and remove them. */
    for (i = 0; i < N; i++) {
        evpl_add_timer(evpl, &timers[i], timer_cb, 3600ULL * 1000000ULL);
    }
    for (i = N - 1; i >= 0; i--) {
        evpl_remove_timer(evpl, &timers[i]);
    }

    evpl_destroy(evpl);

    if (failures) {
        return 1;
    }
    printf("deferral_timer_capacity: %d deferrals delivered once each; %d timers "
           "added and removed\n", N, N);
    return 0;
} /* main */
