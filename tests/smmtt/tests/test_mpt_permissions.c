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

void test_mpt_permissions(void) {
    TEST("T3.1 bare mode allows access", {
        write_csr(CSR_MMPT, 0);
        run_in_s_mode(s_mode_load_ok);
    });

    TEST("T3.2 m-mode bypasses mpt", {
        mpt_setup(XWR_NONE, XWR_NONE, XWR_NONE);
        volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
        uint64_t val = *ptr;
        (void)val;
        mpt_teardown();
    });

    STEST("T3.3 read permission allows load", {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        run_in_s_mode(s_mode_load_ok);
        mpt_teardown();
    });

    STEST("T3.4 rw permission allows store", {
        mpt_setup(XWR_RWX, XWR_RW, XWR_R);
        run_in_s_mode(s_mode_store_ok);
        mpt_teardown();
    });

    STEST("T3.5 rx permission allows fetch", {
        mpt_setup(XWR_RWX, XWR_RX, XWR_R);
        run_in_s_mode(s_mode_fetch_ok);
        mpt_teardown();
    });

    STEST("T3.6 rwx permission allows all", {
        mpt_setup(XWR_RWX, XWR_RWX, XWR_R);
        run_in_s_mode(s_mode_load_ok);
        run_in_s_mode(s_mode_store_ok);
        run_in_s_mode(s_mode_fetch_ok);
        mpt_teardown();
    });

    STEST_TRAP("T3.7 no access triggers laf", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_NONE, XWR_R);
        run_in_s_mode(s_mode_load_fault);
        mpt_teardown();
    });

    STEST_TRAP("T3.8 read-only denies store", EX_SAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        run_in_s_mode(s_mode_store_fault);
        mpt_teardown();
    });

    STEST_TRAP("T3.9 read-only denies fetch", EX_IAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        *(volatile uint32_t *)NOX_ADDR = 0x00000073;
        run_in_s_mode(s_mode_fetch_fault);
        mpt_teardown();
    });

    STEST_TRAP("T3.10 invalid mpte triggers fault", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        extern volatile uint64_t mpt_leaf[];
        mpt_leaf[16] = 0;
        run_in_s_mode(s_mode_load_fault);
        mpt_teardown();
    });

    STEST("T3.11 multi-level walk succeeds", {
        mpt_setup(XWR_RWX, XWR_RW, XWR_R);
        run_in_s_mode(s_mode_store_ok);
        mpt_teardown();
    });

    STEST("T3.12 napot superpage works", {
        extern volatile uint64_t mpt_leaf[];
        mpt_setup(XWR_RWX, XWR_NONE, XWR_R);
        mpt_leaf[16] = MPTE_V | MPTE_L | MPTE_N | (NAPOT_G_4 << NAPOT_G_SHIFT) | (XWR_RW << 8);
        run_in_s_mode(s_mode_store_ok);
        mpt_teardown();
    });

    STEST_TRAP("T3.13 pmp priority over mpt", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_NONE, XWR_R);
        run_in_s_mode(s_mode_load_fault);
        mpt_teardown();
    });

    STEST("T3.14 amo operation checks write", {
        mpt_setup(XWR_RWX, XWR_RW, XWR_R);
        run_in_s_mode(s_mode_amo_ok);
        mpt_teardown();
    });

    STEST_TRAP("T3.15 amo denied without write", EX_SAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        run_in_s_mode(s_mode_amo_fault);
        mpt_teardown();
    });
}
