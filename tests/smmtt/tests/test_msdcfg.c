// Description: CSR MSDCFG privilege & fields
#include "test_main.h"

/* ------------------------------------------------------------------ */
/*  S-mode trap helpers                                                */
/* ------------------------------------------------------------------ */

static void s_mode_read_msdcfg(void) {
    uint64_t val;
    asm volatile ("csrr %0, 0x74E" : "=r"(val));
    (void)val;
    asm volatile ("ecall");
}

static void s_mode_write_msdcfg(void) {
    uint64_t val = 0x1;
    asm volatile ("csrw %0, %1" :: "i"(CSR_MSDCFG), "r"(val));
    asm volatile ("ecall");
}

static void s_mode_csrrs_msdcfg(void) {
    uint64_t val = 0x1;
    asm volatile ("csrrs zero, %0, %1" :: "i"(CSR_MSDCFG), "r"(val));
    asm volatile ("ecall");
}

static void s_mode_csrrc_msdcfg(void) {
    uint64_t val = 0x1;
    asm volatile ("csrrc zero, %0, %1" :: "i"(CSR_MSDCFG), "r"(val));
    asm volatile ("ecall");
}

/* ------------------------------------------------------------------ */
/*  msdcfg.01: initial value is 0                                     */
/* ------------------------------------------------------------------ */

static void msdcfg_01_fn(void) {
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.02: WPRI fields cleared (bits 15:8 and 21:16)              */
/* ------------------------------------------------------------------ */

static void msdcfg_02_fn(void) {
    uint64_t write_val = (0xFFULL << 8) | (0x3FULL << 16);
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT((val & (0xFFULL << 8)) == 0);
    ASSERT((val & (0x3FULL << 16)) == 0);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.03: valid fields preserved (SIDN=0x3F, SEDA=1, SETA=1)    */
/* ------------------------------------------------------------------ */

static void msdcfg_03_fn(void) {
    uint64_t write_val = 0x3F | (1ULL << 6) | (1ULL << 7);
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT(msdcfg_sidn(val) == 0x3F);
    ASSERT(msdcfg_seda(val) == 1);
    ASSERT(msdcfg_seta(val) == 1);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.08: SIDN=0 boundary                                        */
/* ------------------------------------------------------------------ */

static void msdcfg_08_fn(void) {
    /* Write SEDA=1, SETA=1 with SIDN=0 — verify SIDN stays 0 */
    uint64_t write_val = (1ULL << 6) | (1ULL << 7); /* SEDA=1, SETA=1, SIDN=0 */
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT(msdcfg_sidn(val) == 0);
    ASSERT(msdcfg_seda(val) == 1);
    ASSERT(msdcfg_seta(val) == 1);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.09: SIDN=63 boundary (max value)                           */
/* ------------------------------------------------------------------ */

static void msdcfg_09_fn(void) {
    /* Write SIDN=63, SEDA=1, SETA=1 — verify all three are preserved */
    uint64_t write_val = 0x3F | (1ULL << 6) | (1ULL << 7);
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT(msdcfg_sidn(val) == 0x3F);
    ASSERT(msdcfg_seda(val) == 1);
    ASSERT(msdcfg_seta(val) == 1);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.10: SEDA=1 alone                                           */
/* ------------------------------------------------------------------ */

static void msdcfg_10_fn(void) {
    uint64_t write_val = (1ULL << 6); /* SEDA=1 only */
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT(msdcfg_seda(val) == 1);
    ASSERT(msdcfg_seta(val) == 0);
    ASSERT(msdcfg_sidn(val) == 0);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.11: SETA=1 alone                                           */
/* ------------------------------------------------------------------ */

static void msdcfg_11_fn(void) {
    uint64_t write_val = (1ULL << 7); /* SETA=1 only */
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT(msdcfg_seta(val) == 1);
    ASSERT(msdcfg_seda(val) == 0);
    ASSERT(msdcfg_sidn(val) == 0);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.12: Upper 32 bits (63:32) pad0 is WPRI, cleared to 0      */
/* ------------------------------------------------------------------ */

static void msdcfg_12_fn(void) {
    /* Write all-ones to upper 32 bits — pad0 field must be cleared */
    uint64_t write_val = 0xFFFFFFFF00000000ULL;
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT((val >> 32) == 0);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.13: csrrs with rs1=0 from M-mode reads without trap        */
/* ------------------------------------------------------------------ */

static void msdcfg_13_fn(void) {
    /* csrrs with rs1=x0 is a pure read — must NOT trap even on a
       writable CSR. Reads current value (0 from teardown). */
    uint64_t val;
    asm volatile ("csrrs %0, %1, x0" : "=r"(val) : "i"(CSR_MSDCFG));
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  msdcfg.14: csrrs with rs1!=0 from M-mode sets bits and reads old */
/* ------------------------------------------------------------------ */

static void msdcfg_14_fn(void) {
    /* Setup: write known value */
    write_csr(CSR_MSDCFG, 0x3F); /* SIDN=0x3F, SEDA=0, SETA=0 */

    /* csrrs with rs1 = SEDA (bit 6) — should set SEDA, return old value */
    uint64_t old;
    asm volatile ("csrrs %0, %1, %2" : "=r"(old) : "i"(CSR_MSDCFG), "r"((uint64_t)(1ULL << 6)));

    ASSERT(msdcfg_sidn(old) == 0x3F); /* old value preserved SIDN */
    ASSERT(msdcfg_seda(old) == 0);    /* old value had SEDA=0 */

    uint64_t new_val = read_csr(CSR_MSDCFG);
    ASSERT(msdcfg_seda(new_val) == 1); /* new value has SEDA=1 set */
    ASSERT(msdcfg_sidn(new_val) == 0x3F); /* SIDN unchanged */
}

/* ------------------------------------------------------------------ */
/*  Teardown                                                           */
/* ------------------------------------------------------------------ */

static void msdcfg_teardown(void) {
    write_csr(CSR_MSDCFG, 0);
}

/* ------------------------------------------------------------------ */
/*  Test module definition                                             */
/* ------------------------------------------------------------------ */

BEGIN_TEST_MODULE(msdcfg)

/* Baseline M-mode tests */
ADD_TEST_CASE(msdcfg_01_fn, "msdcfg.01 msdcfg initial value", TEST_MODE_M, 0)
ADD_TEST_CASE_HOOKS(msdcfg_02_fn, "msdcfg.02 wpri fields cleared", TEST_MODE_M, 0, NULL, msdcfg_teardown)
ADD_TEST_CASE_HOOKS(msdcfg_03_fn, "msdcfg.03 valid fields preserved", TEST_MODE_M, 0, NULL, msdcfg_teardown)

/* S-mode access trap tests — msdcfg is M-mode-only (CSR 0x74E, priv=0x2) */
ADD_TEST_CASE(s_mode_read_msdcfg, "msdcfg.04 s-mode csrr msdcfg traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_write_msdcfg, "msdcfg.05 s-mode csrw msdcfg traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_csrrs_msdcfg, "msdcfg.06 s-mode csrrs msdcfg traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_csrrc_msdcfg, "msdcfg.07 s-mode csrrc msdcfg traps", TEST_MODE_S, EX_II)

/* Field boundary and individual bit tests */
ADD_TEST_CASE_HOOKS(msdcfg_08_fn, "msdcfg.08 sidn=0 boundary", TEST_MODE_M, 0, NULL, msdcfg_teardown)
ADD_TEST_CASE_HOOKS(msdcfg_09_fn, "msdcfg.09 sidn=63 boundary", TEST_MODE_M, 0, NULL, msdcfg_teardown)
ADD_TEST_CASE_HOOKS(msdcfg_10_fn, "msdcfg.10 seda=1 only", TEST_MODE_M, 0, NULL, msdcfg_teardown)
ADD_TEST_CASE_HOOKS(msdcfg_11_fn, "msdcfg.11 seta=1 only", TEST_MODE_M, 0, NULL, msdcfg_teardown)

/* Upper bits WPRI and CSRRS edge cases */
ADD_TEST_CASE_HOOKS(msdcfg_12_fn, "msdcfg.12 upper bits pad0 cleared", TEST_MODE_M, 0, NULL, msdcfg_teardown)
ADD_TEST_CASE(msdcfg_13_fn, "msdcfg.13 csrrs rs1=0 reads without trap", TEST_MODE_M, 0)
ADD_TEST_CASE_HOOKS(msdcfg_14_fn, "msdcfg.14 csrrs rs1!=0 sets bits and reads old", TEST_MODE_M, 0, NULL, msdcfg_teardown)

END_TEST_MODULE(msdcfg, "CSR MSDCFG privilege & fields")
