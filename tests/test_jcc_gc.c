/**
 * test_jcc_gc.c - Tests for the vendored JCC garbage collector
 *
 * Ported from libjccbas's test/src/test_jcc_gc.c (the canonical copy) to
 * this repo's test framework. The collector itself is vendored verbatim —
 * see docs/system/vendored-gc.md. These tests exist so a bad re-vendor
 * fails `make test` here rather than surfacing as a miscompiled COL
 * program.
 */

/* Request POSIX.1-2008 visibility so putenv() (used by the debug-output
 * test) is declared in <stdlib.h> under -std=c11 on glibc. Must precede
 * every system header include. */
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jcccol.h"
#include "test_framework.h"

/* ---- Fixtures for the typed-object (trace/finalize) scenario ---- */

typedef struct {
    void *child;
} rec_t;

static int finalize_count;

static void rec_trace(void *obj, jcc_gc_mark_fn mark) {
    rec_t *r = (rec_t *) obj;
    mark(r->child);
}

static void rec_finalize(void *obj) {
    finalize_count++;
    free(obj);
}

static const jcc_gc_type_t REC_TYPE = { "rec", rec_trace, rec_finalize };

/* ---- Scenarios ---- */

/* Test: threshold defaults to 100 when <= 0; all counters start at 0. */
TEST(init_defaults) {
    jcc_gc_stats_t s;

    jcc_gc_init(0, 0);
    jcc_gc_stats(&s);
    ASSERT(s.threshold == 100);
    ASSERT(s.registered == 0);
    ASSERT(s.live == 0);
    ASSERT(s.collections == 0);
    ASSERT(s.freed == 0);

    jcc_gc_init(50, 0);
    jcc_gc_stats(&s);
    ASSERT(s.threshold == 50);
    return 0;
}

/* Test: a rooted object survives collection; an unrooted one is freed. */
TEST(reachability) {
    jcc_gc_stats_t s;
    void *slot = NULL;

    jcc_gc_init(100, 0);
    jcc_gc_push_frame();
    jcc_gc_add_root(&slot);

    slot = jcc_gc_register(malloc(8));  /* rooted via slot */
    jcc_gc_register(malloc(8));         /* unrooted garbage */

    jcc_gc_collect();
    jcc_gc_stats(&s);
    ASSERT(s.registered == 2);
    ASSERT(s.live == 1);
    ASSERT(s.freed == 1);

    jcc_gc_pop_frame();
    return 0;
}

/* Test: NULL and unregistered slot values are skipped without crashing. */
TEST(ignored_slot_values) {
    jcc_gc_stats_t s;
    int on_stack = 42;
    void *slot_null = NULL;
    void *slot_bogus = &on_stack;  /* a valid address the GC never registered */

    jcc_gc_init(100, 0);
    jcc_gc_push_frame();
    jcc_gc_add_root(&slot_null);
    jcc_gc_add_root(&slot_bogus);

    jcc_gc_register(malloc(8));  /* unrooted */

    jcc_gc_collect();  /* must not crash on NULL / non-heap slot values */
    jcc_gc_stats(&s);
    ASSERT(s.live == 0);
    ASSERT(s.freed == 1);

    jcc_gc_pop_frame();
    return 0;
}

/* Test: global roots, both a scalar range {&slot,1} and an array range. */
TEST(global_roots) {
    jcc_gc_stats_t s;
    void *g0 = NULL;
    void *arr[3] = { NULL, NULL, NULL };
    jcc_gc_root_range_t ranges[] = {
        { &g0, 1 },
        { arr, 3 },
        { NULL, 0 },
    };

    jcc_gc_init(100, 0);
    jcc_gc_set_global_roots(ranges);

    g0 = jcc_gc_register(malloc(8));
    arr[1] = jcc_gc_register(malloc(8));
    jcc_gc_register(malloc(8));  /* unrooted */

    jcc_gc_collect();
    jcc_gc_stats(&s);
    ASSERT(s.live == 2);
    ASSERT(s.freed == 1);
    return 0;
}

/* Test: a nested frame's roots are dropped by pop_frame. */
TEST(frames) {
    jcc_gc_stats_t s;
    void *outer = NULL;
    void *inner = NULL;

    jcc_gc_init(100, 0);

    jcc_gc_push_frame();
    jcc_gc_add_root(&outer);
    outer = jcc_gc_register(malloc(8));

    jcc_gc_push_frame();
    jcc_gc_add_root(&inner);
    inner = jcc_gc_register(malloc(8));

    jcc_gc_collect();
    jcc_gc_stats(&s);
    ASSERT_MSG(s.live == 2, "both frames' roots should be live");

    jcc_gc_pop_frame();  /* drop inner's frame */
    jcc_gc_collect();
    jcc_gc_stats(&s);
    ASSERT_MSG(s.live == 1, "inner should now be unreachable");
    ASSERT(s.freed >= 1);

    jcc_gc_pop_frame();
    return 0;
}

/* Test: a registration at the threshold collects BEFORE inserting, so the
 * block it registers is never reclaimed by its own collection. */
TEST(trigger) {
    jcc_gc_stats_t s;
    char *p;
    int i;

    jcc_gc_init(4, 0);
    for (i = 0; i < 4; i++) {
        jcc_gc_register(malloc(8));  /* unrooted; fills live up to threshold */
    }

    /* live == 4 == threshold -> collect first */
    p = (char *) jcc_gc_register(malloc(8));
    ASSERT(p != NULL);
    p[0] = 'x';  /* p must still be valid: not freed by its own trigger */

    jcc_gc_stats(&s);
    ASSERT(s.collections >= 1);
    ASSERT_MSG(s.live == 1, "the 4 unrooted blocks swept, p inserted");
    return 0;
}

/* Test: with everything rooted, collections free nothing, so the threshold
 * doubles while live stays above threshold/2. */
TEST(threshold_doubling) {
    jcc_gc_stats_t s;
    void *slots[100];
    jcc_gc_root_range_t ranges[] = { { slots, 100 }, { NULL, 0 } };
    int i;

    memset(slots, 0, sizeof(slots));
    jcc_gc_init(4, 0);
    jcc_gc_set_global_roots(ranges);

    for (i = 0; i < 100; i++) {
        slots[i] = jcc_gc_register(malloc(8));  /* stored into a root at once */
    }

    jcc_gc_stats(&s);
    ASSERT(s.live == 100);
    ASSERT_MSG(s.threshold > 4, "threshold should grow as live exceeds half");
    return 0;
}

/* Test: trace marks interior refs; finalize reclaims unmarked objects. */
TEST(trace_and_finalize) {
    jcc_gc_stats_t s;
    void *root = NULL;
    char *child;
    rec_t *rec;

    jcc_gc_init(100, 0);
    finalize_count = 0;
    jcc_gc_push_frame();
    jcc_gc_add_root(&root);

    /* reachable only via rec->child */
    child = (char *) jcc_gc_register(malloc(8));
    rec = (rec_t *) malloc(sizeof(rec_t));
    rec->child = child;
    jcc_gc_register_object(rec, &REC_TYPE);
    root = rec;

    jcc_gc_collect();
    jcc_gc_stats(&s);
    ASSERT_MSG(s.live == 2, "rec kept by root, child kept by trace");
    ASSERT(finalize_count == 0);

    root = NULL;  /* rec (and thus child) now unreachable */
    jcc_gc_collect();
    jcc_gc_stats(&s);
    ASSERT(s.live == 0);
    ASSERT_MSG(finalize_count == 1, "rec finalized; child plain-freed");

    jcc_gc_pop_frame();
    return 0;
}

/* Test: 1000 string slots as a global root, 100_000 allocations each stored
 * into a random slot. Overwriting a slot orphans its old string, so the GC
 * must collect steadily while every currently-referenced string survives. */
TEST(load) {
    enum { SLOTS = 1000, ALLOCS = 100000 };
    static void *arr[SLOTS];
    jcc_gc_root_range_t ranges[] = { { arr, SLOTS }, { NULL, 0 } };
    jcc_gc_stats_t s;
    int64_t non_null;
    int i;

    memset(arr, 0, sizeof(arr));
    jcc_gc_init(256, 0);
    jcc_gc_set_global_roots(ranges);

    for (i = 0; i < ALLOCS; i++) {
        int r = rand() % SLOTS;
        char *str = (char *) jcc_gc_register(malloc(16));
        snprintf(str, 16, "s%d", i);
        arr[r] = str;  /* stored into a root before the next allocation */
    }

    jcc_gc_collect();  /* final sweep: only strings still held by a slot live */

    non_null = 0;
    for (i = 0; i < SLOTS; i++) {
        if (arr[i] != NULL) {
            non_null++;
        }
    }

    jcc_gc_stats(&s);
    ASSERT(s.registered == ALLOCS);
    ASSERT_MSG(s.collections >= 1, "the threshold must have fired");
    ASSERT_MSG(s.live == non_null, "live == distinct rooted strings");
    ASSERT(s.live <= SLOTS);
    ASSERT_MSG(s.registered == s.freed + s.live, "nothing leaked or lost");
    return 0;
}

/* Test: with debug enabled and JCC_GC_LOG set, shutdown appends the exact
 * exit-stats line JCC's integration tests assert on. Runs last so the
 * atexit-installed shutdown is a no-op afterwards. */
TEST(debug_exit_stats) {
    static char env_buf[512];
    const char *log_path = "test_jcc_gc_debug.log";
    void *root = NULL;
    FILE *f;
    char content[2048];
    size_t n;

    remove(log_path);
    snprintf(env_buf, sizeof(env_buf), "JCC_GC_LOG=%s", log_path);
    putenv(env_buf);

    jcc_gc_init(100, JCC_GC_DEBUG);
    jcc_gc_push_frame();
    jcc_gc_add_root(&root);
    root = jcc_gc_register(malloc(8));  /* rooted, survives */
    jcc_gc_register(malloc(8));         /* unrooted, reclaimed */
    jcc_gc_collect();  /* registered=2 collections=1 freed=1 live=1 */
    jcc_gc_pop_frame();
    jcc_gc_shutdown();  /* writes the exit line, frees the survivor */

    f = fopen(log_path, "r");
    ASSERT(f != NULL);
    n = fread(content, 1, sizeof(content) - 1, f);
    content[n] = '\0';
    fclose(f);

    ASSERT(strstr(content,
        "jcc_gc: exit: registered=2 collections=1 freed=1 live=1") != NULL);

    remove(log_path);
    return 0;
}

int main(void) {
    int total = 0;
    int passed = 0;
    int failed = 0;

    printf("\nRunning garbage collector tests...\n");
    printf("===================================\n\n");

    RUN_TEST(init_defaults);
    RUN_TEST(reachability);
    RUN_TEST(ignored_slot_values);
    RUN_TEST(global_roots);
    RUN_TEST(frames);
    RUN_TEST(trigger);
    RUN_TEST(threshold_doubling);
    RUN_TEST(trace_and_finalize);
    RUN_TEST(load);
    /* Must run last: calls jcc_gc_shutdown(), after which the
     * atexit-installed handler is a no-op. */
    RUN_TEST(debug_exit_stats);

    printf("\n===================================\n");
    printf("Results: %d/%d tests passed", passed, total);
    if (failed > 0) {
        printf(", %d FAILED", failed);
    }
    printf("\n\n");

    return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
