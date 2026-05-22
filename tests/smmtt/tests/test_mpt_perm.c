// Description: Table permissions & walks
#include "test_main.h"

#define TEST_DATA_ADDR 0x80100000ULL
#define NOX_ADDR       0x80200000ULL

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

static void mpt_perm_01_setup(void) { write_csr(CSR_MMPT, 0); }
static void mpt_perm_02_setup(void) { mpt_setup(XWR_NONE, XWR_NONE, XWR_NONE); }
static void mpt_perm_02_fn(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t val = *ptr;
    (void)val;
}

static void mpt_perm_03_setup(void) { mpt_setup(XWR_RWX, XWR_R, XWR_R); }
static void mpt_perm_04_setup(void) { mpt_setup(XWR_RWX, XWR_RW, XWR_R); }
static void mpt_perm_05_setup(void) { mpt_setup(XWR_RWX, XWR_RX, XWR_R); }
static void mpt_perm_06_setup(void) { mpt_setup(XWR_RWX, XWR_RWX, XWR_R); }
static void mpt_perm_06_fn(void) {
    run_in_s_mode(s_mode_load_ok);
    run_in_s_mode(s_mode_store_ok);
    run_in_s_mode(s_mode_fetch_ok);
}

static void mpt_perm_07_setup(void) { mpt_setup(XWR_RWX, XWR_NONE, XWR_R); }
static void mpt_perm_09_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    *(volatile uint32_t *)NOX_ADDR = 0x00000073; // ecall
}

static void mpt_perm_10_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = 0;
}

static void mpt_perm_12_setup(void) {
    extern volatile uint64_t mpt_leaf[];
    mpt_setup(XWR_RWX, XWR_NONE, XWR_R);
    mpt_leaf[16] = MPTE_V | MPTE_L | MPTE_N | (NAPOT_G_4 << NAPOT_G_SHIFT) | (XWR_RW << 8);
}

BEGIN_TEST_MODULE(mpt_perm)

ADD_TEST_CASE_HOOKS(s_mode_load_ok, "mpt_perm.01 bare mode allows access", TEST_MODE_S, 0, mpt_perm_01_setup, NULL)
ADD_TEST_CASE_HOOKS(mpt_perm_02_fn, "mpt_perm.02 m-mode bypasses mpt", TEST_MODE_M, 0, mpt_perm_02_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_ok, "mpt_perm.03 read permission allows load", TEST_MODE_S, 0, mpt_perm_03_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_ok, "mpt_perm.04 rw permission allows store", TEST_MODE_S, 0, mpt_perm_04_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_fetch_ok, "mpt_perm.05 rx permission allows fetch", TEST_MODE_S, 0, mpt_perm_05_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_perm_06_fn, "mpt_perm.06 rwx permission allows all", TEST_MODE_M, 0, mpt_perm_06_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_fault, "mpt_perm.07 no access triggers laf", TEST_MODE_S, EX_LAF, mpt_perm_07_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_fault, "mpt_perm.08 read-only denies store", TEST_MODE_S, EX_SAF, mpt_perm_03_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_fetch_fault, "mpt_perm.09 read-only denies fetch", TEST_MODE_S, EX_IAF, mpt_perm_09_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_fault, "mpt_perm.10 invalid mpte triggers fault", TEST_MODE_S, EX_LAF, mpt_perm_10_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_ok, "mpt_perm.11 multi-level walk succeeds", TEST_MODE_S, 0, mpt_perm_04_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_store_ok, "mpt_perm.12 napot superpage works", TEST_MODE_S, 0, mpt_perm_12_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_fault, "mpt_perm.13 pmp priority over mpt", TEST_MODE_S, EX_LAF, mpt_perm_07_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_amo_ok, "mpt_perm.14 amo operation checks write", TEST_MODE_S, 0, mpt_perm_04_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_amo_fault, "mpt_perm.15 amo denied without write", TEST_MODE_S, EX_SAF, mpt_perm_03_setup, mpt_teardown)

END_TEST_MODULE(mpt_perm, "Table permissions & walks")
