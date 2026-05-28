// Description: MFENCE.PA & MINVAL.PA invalidation tests
#include "test_main.h"

#define TEST_DATA_ADDR 0x80100000ULL

/* ------------------------------------------------------------------ */
/*  S-mode trap helpers                                                */
/* ------------------------------------------------------------------ */

static void s_mode_mfence_pa(void) {
    asm volatile (".word 0x1C000073");
    asm volatile ("ecall");
}

static void s_mode_minval_pa(void) {
    asm volatile (".word 0x1E000073");
    asm volatile ("ecall");
}

static void s_mode_mfence_pa_with_operands(void) {
    asm volatile ("li a0, 0x80000000\n"
                  "li a1, 1\n"
                  ".word 0x1CB50073\n");
    asm volatile ("ecall");
}

static void s_mode_minval_pa_with_operands(void) {
    asm volatile ("li a0, 0x80000000\n"
                  "li a1, 1\n"
                  ".word 0x1EB50073\n");
    asm volatile ("ecall");
}

/* S-mode load helper for cache flush functional tests */
static void s_mode_load_from_test(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t val = *ptr;
    (void)val;
    asm volatile ("ecall");
}

/* ------------------------------------------------------------------ */
/*  M-mode test functions                                              */
/* ------------------------------------------------------------------ */

static void mfence_01_fn(void) {
    asm volatile (".word 0x1C000073");
}

static void mfence_02_fn(void) {
    asm volatile ("li a0, 0x80000000\n"
                  "li a1, 1\n"
                  ".word 0x1CB50073\n");
}

static void mfence_04_fn(void) {
    asm volatile (".word 0x1E000073");
}

/* MFENCE.PA with rs1!=x0, rs2=x0 (address-only fence) */
static void mfence_rs1_rs2x0_fn(void) {
    asm volatile ("li a0, 0x80000000\n"
                  ".word 0x1C050073\n");   // rs1=a0(10), rs2=x0(0)
}

/* MFENCE.PA with rs1=x0, rs2!=x0 (SDID-only fence) */
static void mfence_rs1x0_rs2_fn(void) {
    asm volatile ("li a1, 1\n"
                  ".word 0x1CB00073\n");   // rs1=x0(0), rs2=a1(11)
}

/* MFENCE.PA with rs1!=x0, rs2!=x0 using values different from mfence.02 */
static void mfence_both_operands_fn(void) {
    asm volatile ("li a0, 0x90000000\n"
                  "li a1, 7\n"
                  ".word 0x1CB50073\n");   // rs1=a0(10), rs2=a1(11)
}

/* MINVAL.PA with operands a0,a1 */
static void minval_operands_fn(void) {
    asm volatile ("li a0, 0x80000000\n"
                  "li a1, 1\n"
                  ".word 0x1EB50073\n");
}

/* MFENCE.PA flushes MPT cache — functional test */
static void mfence_cache_flush_fn(void) {
    /* First S-mode load caches the PTE with read permission */
    run_in_s_mode(s_mode_load_from_test);
    /* Modify leaf PTE to remove all permissions */
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    /* Execute MFENCE.PA to flush the stale cached PTE */
    asm volatile (".word 0x1C000073");
    /* Second S-mode load should fault (new permissions took effect) */
    run_in_s_mode(s_mode_load_from_test);
}

/* MINVAL.PA flushes MPT cache — functional test */
static void minval_cache_flush_fn(void) {
    /* First S-mode load caches the PTE with read permission */
    run_in_s_mode(s_mode_load_from_test);
    /* Modify leaf PTE to remove all permissions */
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    /* Execute MINVAL.PA to flush the stale cached PTE */
    asm volatile (".word 0x1E000073");
    /* Second S-mode load should fault (new permissions took effect) */
    run_in_s_mode(s_mode_load_from_test);
}

/* ------------------------------------------------------------------ */
/*  Setup for cache flush tests                                       */
/* ------------------------------------------------------------------ */

static void cache_flush_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
}

/* ------------------------------------------------------------------ */
/*  Test module definition                                              */
/* ------------------------------------------------------------------ */

BEGIN_TEST_MODULE(mfence)

/* Existing tests (preserved numbering) */
ADD_TEST_CASE(mfence_01_fn, "mfence.01 m-mode mfence.pa ok", TEST_MODE_M, 0)
ADD_TEST_CASE(mfence_02_fn, "mfence.02 m-mode mfence.pa with operands ok", TEST_MODE_M, 0)
ADD_TEST_CASE(s_mode_mfence_pa, "mfence.03 s-mode mfence.pa fault", TEST_MODE_S, EX_II)
ADD_TEST_CASE(mfence_04_fn, "mfence.04 m-mode minval.pa ok", TEST_MODE_M, 0)
ADD_TEST_CASE(s_mode_minval_pa, "mfence.05 s-mode minval.pa fault", TEST_MODE_S, EX_II)

/* rs1/rs2 combination tests */
ADD_TEST_CASE(mfence_rs1_rs2x0_fn, "mfence.06 m-mode mfence.pa rs1!=x0,rs2=x0 ok", TEST_MODE_M, 0)
ADD_TEST_CASE(mfence_rs1x0_rs2_fn, "mfence.07 m-mode mfence.pa rs1=x0,rs2!=x0 ok", TEST_MODE_M, 0)
ADD_TEST_CASE(mfence_both_operands_fn, "mfence.08 m-mode mfence.pa rs1!=x0,rs2!=x0 (diff vals) ok", TEST_MODE_M, 0)

/* MINVAL.PA with operands */
ADD_TEST_CASE(minval_operands_fn, "mfence.09 m-mode minval.pa with operands ok", TEST_MODE_M, 0)

/* S-mode trap tests with operands */
ADD_TEST_CASE(s_mode_mfence_pa_with_operands, "mfence.10 s-mode mfence.pa with operands fault", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_minval_pa_with_operands, "mfence.11 s-mode minval.pa with operands fault", TEST_MODE_S, EX_II)

/* Functional cache flush tests */
ADD_TEST_CASE_HOOKS(mfence_cache_flush_fn, "mfence.12 mfence.pa flushes mpt cache", TEST_MODE_M, EX_LAF, cache_flush_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(minval_cache_flush_fn, "mfence.13 minval.pa flushes mpt cache", TEST_MODE_M, EX_LAF, cache_flush_setup, mpt_teardown)

END_TEST_MODULE(mfence, "MFENCE.PA & MINVAL.PA")
