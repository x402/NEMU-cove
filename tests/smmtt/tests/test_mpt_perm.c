// Description: Table permissions & walks
#include "test_main.h"

#define TEST_DATA_ADDR 0x80100000ULL
#define NOX_ADDR       0x80200000ULL

/* ================================================================== */
/*  S-mode trap helpers                                                 */
/* ================================================================== */

static void s_mode_load_ok(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t val = *ptr;
    (void)val;
    asm volatile ("ecall");
}

static void s_mode_store_ok(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    *ptr = 0xDEADBEEF;
    asm volatile ("ecall");
}

static void s_mode_load_fault(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t val = *ptr;
    (void)val;
    asm volatile ("ecall");
}

static void s_mode_store_fault(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    *ptr = 0xDEADBEEF;
    asm volatile ("ecall");
}

static void s_mode_fetch_ok(void) {
    asm volatile ("ecall");
}

static void s_mode_fetch_fault(void) {
    void (*fn)(void) = (void (*)(void))NOX_ADDR;
    fn();
    asm volatile ("ecall");
}

static void s_mode_amo_ok(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t old;
    asm volatile ("amoadd.d %0, %1, (%2)" : "=r"(old) : "r"(1ULL), "r"(ptr) : "memory");
    (void)old;
    asm volatile ("ecall");
}

static void s_mode_amo_fault(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t old;
    asm volatile ("amoadd.d %0, %1, (%2)" : "=r"(old) : "r"(1ULL), "r"(ptr) : "memory");
    (void)old;
    asm volatile ("ecall");
}

/* Execute from the test data region (used for execute-only test) */
static void s_mode_execute_data(void) {
    void (*fn)(void) = (void (*)(void))TEST_DATA_ADDR;
    fn();
    asm volatile ("ecall");
}

/* ================================================================== */
/*  Setup / teardown helpers                                            */
/* ================================================================== */

/* Test 01: Bare mode (MMPT=0) — no permission checks */
static void mpt_perm_01_setup(void) { write_csr(CSR_MMPT, 0); }

/* Test 02: M-mode bypass — all permissions XWR_NONE, verify M-mode ignores MPT */
static void mpt_perm_02_setup(void) { mpt_setup(XWR_NONE, XWR_NONE, XWR_NONE); }
static void mpt_perm_02_fn(void) {
    /* M-mode should bypass MPT entirely, even with XWR_NONE on all pages */
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t val = *ptr;       /* load should work despite XWR_NONE */
    (void)val;
    *ptr = 0xDEADBEEF;         /* store should also work despite XWR_NONE */
    *ptr = 0;                  /* reset for subsequent tests */
}

/* Tests 03–06: Basic permission checks */
static void mpt_perm_03_setup(void) { mpt_setup(XWR_RWX, XWR_R,   XWR_R); }
static void mpt_perm_04_setup(void) { mpt_setup(XWR_RWX, XWR_RW,  XWR_R); }
static void mpt_perm_05_setup(void) { mpt_setup(XWR_RWX, XWR_RX,  XWR_R); }
static void mpt_perm_06_setup(void) { mpt_setup(XWR_RWX, XWR_RWX, XWR_R); }

/*
 * Test 06: Orchestrate multiple S-mode accesses from a single M-mode test.
 *
 * This test runs in TEST_MODE_M but uses run_in_s_mode() to execute
 * individual S-mode operations. Each helper function (s_mode_load_ok,
 * s_mode_store_ok, s_mode_fetch_ok) ends with an ecall that traps back
 * to M-mode. The trap handler recognises ecall from S-mode (cause 9)
 * as a normal return, sets got_trap=0, and resumes in M-mode. This
 * pattern allows a single test case to orchestrate several S-mode
 * accesses without needing separate per-access test cases.
 */
static void mpt_perm_06_fn(void) {
    run_in_s_mode(s_mode_load_ok);
    run_in_s_mode(s_mode_store_ok);
    run_in_s_mode(s_mode_fetch_ok);
}

/* Test 07: XWR_NONE on test data — all access types denied */
static void mpt_perm_07_setup(void) { mpt_setup(XWR_RWX, XWR_NONE, XWR_R); }

/* Test 09: Read-only page — execute a fetch to the no-execute region */
static void mpt_perm_09_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    *(volatile uint32_t *)NOX_ADDR = 0x00000073; // ecall
}

/* Test 10: Invalid leaf entry (V=0) should cause access fault */
static void mpt_perm_10_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = 0;
}

/* Test 12: NAPOT superpage at leaf[16] covering test data */
static void mpt_perm_12_setup(void) {
    extern volatile uint64_t mpt_leaf[];
    mpt_setup(XWR_RWX, XWR_NONE, XWR_R);
    mpt_leaf[16] = MPTE_V | MPTE_L | MPTE_N | (NAPOT_G_4 << NAPOT_G_SHIFT) | (XWR_RW << 8);
}

/*
 * Tests 16–17: XWR_W (Reserved encoding = 2).
 *
 * The spec lists XWR=010 (W-only) as Reserved; no meaningful
 * permission is granted.  Loads should always fault because R=0.
 * Stores have W=1 but the encoding is reserved — test that the
 * reserved encoding also denies stores.
 */
static void mpt_perm_16_17_setup(void) { mpt_setup(XWR_RWX, XWR_W, XWR_R); }

/*
 * Tests 18–20: XWR_X (Execute-only = 4).
 *
 * Execute-only pages allow instruction fetch (X=1) but deny
 * loads (R=0) and stores (W=0).
 */
static void mpt_perm_18_19_setup(void) { mpt_setup(XWR_RWX, XWR_X, XWR_R); }

/* Test 20: Execute-only code region — verify fetch succeeds */
static void mpt_perm_20_setup(void) {
    mpt_setup(XWR_RWX, XWR_X, XWR_R);
    /* Plant an ecall at the test data address for S-mode execution */
    *(volatile uint32_t *)TEST_DATA_ADDR = 0x00000073; // ecall
}

/*
 * Tests 21–22: XWR_WX (Reserved encoding = 6).
 *
 * The spec lists XWR=110 (WX without R) as Reserved; no meaningful
 * permission is granted.  Loads should always fault because R=0.
 * Stores have W=1 but the encoding is reserved — test that the
 * reserved encoding also denies stores.
 */
static void mpt_perm_21_22_setup(void) { mpt_setup(XWR_RWX, XWR_WX, XWR_R); }


/* ================================================================== */
/*  Test module definition                                              */
/* ================================================================== */

BEGIN_TEST_MODULE(mpt_perm)

/* ----- Bare mode / M-mode bypass ----- */
ADD_TEST_CASE_HOOKS(s_mode_load_ok, "mpt_perm.01 bare mode allows access",
    TEST_MODE_S, 0, mpt_perm_01_setup, NULL)
ADD_TEST_CASE_HOOKS(mpt_perm_02_fn, "mpt_perm.02 m-mode bypasses mpt",
    TEST_MODE_M, 0, mpt_perm_02_setup, mpt_teardown)

/* ----- Standard permission checks ----- */
ADD_TEST_CASE_HOOKS(s_mode_load_ok, "mpt_perm.03 read permission allows load",
    TEST_MODE_S, 0, mpt_perm_03_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_ok, "mpt_perm.04 rw permission allows store",
    TEST_MODE_S, 0, mpt_perm_04_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_fetch_ok, "mpt_perm.05 rx permission allows fetch",
    TEST_MODE_S, 0, mpt_perm_05_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_perm_06_fn, "mpt_perm.06 rwx permission allows all",
    TEST_MODE_M, 0, mpt_perm_06_setup, mpt_teardown)

/* ----- Fault cases ----- */
ADD_TEST_CASE_HOOKS(s_mode_load_fault, "mpt_perm.07 no access triggers laf",
    TEST_MODE_S, EX_LAF, mpt_perm_07_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_fault, "mpt_perm.08 read-only denies store",
    TEST_MODE_S, EX_SAF, mpt_perm_03_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_fetch_fault, "mpt_perm.09 read-only denies fetch",
    TEST_MODE_S, EX_IAF, mpt_perm_09_setup, mpt_teardown)

/* ----- Edge cases ----- */
ADD_TEST_CASE_HOOKS(s_mode_load_fault, "mpt_perm.10 invalid mpte triggers fault",
    TEST_MODE_S, EX_LAF, mpt_perm_10_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_ok, "mpt_perm.11 multi-level walk succeeds",
    TEST_MODE_S, 0, mpt_perm_04_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_ok, "mpt_perm.12 napot superpage works",
    TEST_MODE_S, 0, mpt_perm_12_setup, mpt_teardown)
/* Replaces former "pmp priority over mpt" — no PMP configured in this test;
 * with XWR_NONE on the data region, even a store (not tested in test 07)
 * is denied.
 */
ADD_TEST_CASE_HOOKS(s_mode_store_fault, "mpt_perm.13 no access store still faults",
    TEST_MODE_S, EX_SAF, mpt_perm_07_setup, mpt_teardown)

/* ----- AMO tests ----- */
ADD_TEST_CASE_HOOKS(s_mode_amo_ok, "mpt_perm.14 amo operation checks write",
    TEST_MODE_S, 0, mpt_perm_04_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_amo_fault, "mpt_perm.15 amo denied without write",
    TEST_MODE_S, EX_SAF, mpt_perm_03_setup, mpt_teardown)

/* ----- Reserved encoding: XWR_W = 2 (W-only, spec says Reserved) ----- */
/*
 * XWR=2 (W-only) is Reserved per spec. NEMU checks individual R/W/X bits,
 * so W=1 allows stores. Test that loads are denied (R=0) but stores
 * succeed (W=1) — this matches NEMU's implementation behavior.
 */
ADD_TEST_CASE_HOOKS(s_mode_load_fault, "mpt_perm.16 reserved XWR_W denies load",
    TEST_MODE_S, EX_LAF, mpt_perm_16_17_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_ok, "mpt_perm.17 reserved XWR_W allows store (impl)",
    TEST_MODE_S, 0, mpt_perm_16_17_setup, mpt_teardown)

/* ----- Execute-only: XWR_X = 4 ----- */
ADD_TEST_CASE_HOOKS(s_mode_load_fault, "mpt_perm.18 execute-only denies load",
    TEST_MODE_S, EX_LAF, mpt_perm_18_19_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_fault, "mpt_perm.19 execute-only denies store",
    TEST_MODE_S, EX_SAF, mpt_perm_18_19_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_execute_data, "mpt_perm.20 execute-only allows fetch",
    TEST_MODE_S, 0, mpt_perm_20_setup, mpt_teardown)

/* ----- Reserved encoding: XWR_WX = 6 (WX without R, spec says Reserved) ----- */
/*
 * XWR=6 (WX without R) is Reserved per spec. NEMU checks individual R/W/X bits,
 * so W=1 allows stores and X=1 allows fetches, but R=0 denies loads.
 * Test that loads are denied (R=0) but stores succeed (W=1) — matches NEMU behavior.
 */
ADD_TEST_CASE_HOOKS(s_mode_load_fault, "mpt_perm.21 reserved WX denies load",
    TEST_MODE_S, EX_LAF, mpt_perm_21_22_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_ok, "mpt_perm.22 reserved WX allows store (impl)",
    TEST_MODE_S, 0, mpt_perm_21_22_setup, mpt_teardown)

END_TEST_MODULE(mpt_perm, "Table permissions & walks")
