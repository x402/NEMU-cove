// Description: CSR MMPT configuration & WARL checks
#include "test_main.h"

/* ------------------------------------------------------------------ */
/*  S-mode trap helpers                                                */
/* ------------------------------------------------------------------ */

static void s_mode_read_mmpt(void) {
    uint64_t val;
    asm volatile ("csrr %0, 0x382" : "=r"(val));
    (void)val;
    asm volatile ("ecall");
}

static void s_mode_write_mmpt(void) {
    uint64_t val = 0x1;
    asm volatile ("csrw 0x382, %0" :: "r"(val));
    asm volatile ("ecall");
}

static void s_mode_csrrs_mmpt(void) {
    /* csrrs with non-zero rs1 from S-mode should trap EX_II */
    uint64_t val = 0x1;
    asm volatile ("csrrs zero, 0x382, %0" :: "r"(val));
    asm volatile ("ecall");
}

static void s_mode_csrrc_mmpt(void) {
    /* csrrc with non-zero rs1 from S-mode should trap EX_II */
    uint64_t val = 0x1;
    asm volatile ("csrrc zero, 0x382, %0" :: "r"(val));
    asm volatile ("ecall");
}

/* For MPT cache flush test — S-mode load from a page tracked by MPT */
#define TEST_DATA_ADDR 0x80100000ULL

static void s_mode_load_data(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t val = *ptr;
    (void)val;
    asm volatile ("ecall");
}

/* ------------------------------------------------------------------ */
/*  mmpt.01 — initial value is 0                                       */
/* ------------------------------------------------------------------ */

static void mmpt_01_fn(void) {
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  mmpt.02 — bare mode write clears sdid and ppn                      */
/*                                                                     */
/*  Writes mode=5 (reserved, >3). Since the reset value of mode is 0   */
/*  (Bare) and mode>3 WARL-keeps the old value, mode stays 0.          */
/*  The Bare (mode==0) path then forces sdid=0 and ppn=0, overriding   */
/*  the sdid=1 that was written in the raw value.                     */
/* ------------------------------------------------------------------ */

static void mmpt_02_fn(void) {
    /* mode=5 (>3) → WARL keeps old mode=0 → Bare path clears sdid/ppn */
    write_csr(CSR_MMPT, (1ULL << 52) | (5ULL << 60));
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val) == 0);
    ASSERT(mmpt_sdid(val) == 0);
    ASSERT(mmpt_ppn(val) == 0);
}

/* ------------------------------------------------------------------ */
/*  mmpt.03 — Smmpt43 mode write (mode=1)                              */
/* ------------------------------------------------------------------ */

static void mmpt_03_fn(void) {
    uint64_t ppn_val = 0x12345;
    uint64_t write_val = (1ULL << 60) | (1ULL << 52) | ppn_val;
    write_csr(CSR_MMPT, write_val);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val) == 1);
    ASSERT(mmpt_sdid(val) == 1);
    ASSERT(mmpt_ppn(val) == ppn_val);
}

/* ------------------------------------------------------------------ */
/*  mmpt.04 — Smmpt52 mode write (mode=2)                              */
/* ------------------------------------------------------------------ */

static void mmpt_04_fn(void) {
    uint64_t ppn_val = 0x12345;
    uint64_t write_val = (2ULL << 60) | (2ULL << 52) | ppn_val;
    write_csr(CSR_MMPT, write_val);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val) == 2);
    ASSERT(mmpt_sdid(val) == 2);
    ASSERT(mmpt_ppn(val) == ppn_val);
}

/* ------------------------------------------------------------------ */
/*  mmpt.05 — Smmpt64 PPN alignment                                    */
/*                                                                     */
/*  When MODE=Smmpt64 (3), the root page table must be aligned to      */
/*  32KiB, so the low 3 bits of PPN are cleared by hardware.          */
/* ------------------------------------------------------------------ */

static void mmpt_05_fn(void) {
    uint64_t ppn_val = 0x12345 | 0x7;  /* low 3 bits set */
    uint64_t write_val = (3ULL << 60) | (1ULL << 52) | ppn_val;
    write_csr(CSR_MMPT, write_val);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val) == 3);
    ASSERT(mmpt_ppn(val) == (ppn_val & ~0x7ULL));  /* low 3 bits cleared */
}

/* ------------------------------------------------------------------ */
/*  mmpt.06 — Illegal mode WARL                                        */
/*                                                                     */
/*  Modes 4-13 are reserved. Writing a reserved mode (e.g., 4) keeps   */
/*  the previous valid mode value.                                     */
/* ------------------------------------------------------------------ */

static void mmpt_06_fn(void) {
    /* First establish a valid mode (Smmpt43 = 1) */
    write_csr(CSR_MMPT, (1ULL << 60));
    uint64_t val1 = read_csr(CSR_MMPT);

    /* Write mode=4 (reserved) with sdid=1 — should keep mode=1 */
    write_csr(CSR_MMPT, (4ULL << 60) | (1ULL << 52));
    uint64_t val2 = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val2) == mmpt_mode(val1));
}

/* ------------------------------------------------------------------ */
/*  mmpt.07 — SDID boundary values (0 and 63)                          */
/*                                                                     */
/*  SDID is a 6-bit WARL field supporting values 0 through 63          */
/*  (SDIDMAX=63). Both extremes are legal.                             */
/* ------------------------------------------------------------------ */

static void mmpt_07_fn(void) {
    /* SDID = 0 (minimum) */
    write_csr(CSR_MMPT, (1ULL << 60) | (0ULL << 52) | 0x1000);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_sdid(val) == 0);

    /* SDID = 63 (maximum) */
    write_csr(CSR_MMPT, (1ULL << 60) | (63ULL << 52) | 0x1000);
    val = read_csr(CSR_MMPT);
    ASSERT(mmpt_sdid(val) == 63);
}

/* ------------------------------------------------------------------ */
/*  mmpt.12 — M-mode csrrs with rs1=0 reads without trap               */
/*                                                                     */
/*  Per RISC-V spec, csrrs with rs1=x0 is a pure read — even for      */
/*  M-mode-only CSRs, the is_write flag is false, so no trap.         */
/* ------------------------------------------------------------------ */

static void mmpt_12_fn(void) {
    /* Set up mmpt with a known value first */
    uint64_t ppn_val = 0x12345;
    uint64_t setup_val = (1ULL << 60) | (1ULL << 52) | ppn_val;
    write_csr(CSR_MMPT, setup_val);

    /* Read back via normal csrr to get expected value */
    uint64_t expected = read_csr(CSR_MMPT);

    /* csrrs with rs1=x0 — should read without trapping */
    uint64_t val;
    asm volatile ("csrrs %0, %1, x0" : "=r"(val) : "i"(CSR_MMPT));
    ASSERT(val == expected);
}

/* ------------------------------------------------------------------ */
/*  mmpt.13 — Mode transition test (mode 1 -> mode 2)                  */
/*                                                                     */
/*  Writing a legal mode value should update mode correctly.           */
/*  Verify that both mode and other fields (sdid, ppn) are preserved.  */
/* ------------------------------------------------------------------ */

static void mmpt_13_fn(void) {
    /* Write mode=1 (Smmpt43), sdid=1, ppn=0x1000 */
    write_csr(CSR_MMPT, (1ULL << 60) | (1ULL << 52) | 0x1000);
    uint64_t val1 = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val1) == 1);
    ASSERT(mmpt_sdid(val1) == 1);
    ASSERT(mmpt_ppn(val1) == 0x1000);

    /* Transition to mode=2 (Smmpt52), sdid=2, ppn=0x2000 */
    write_csr(CSR_MMPT, (2ULL << 60) | (2ULL << 52) | 0x2000);
    uint64_t val2 = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val2) == 2);
    ASSERT(mmpt_sdid(val2) == 2);
    ASSERT(mmpt_ppn(val2) == 0x2000);
}

/* ------------------------------------------------------------------ */
/*  mmpt.14 — SDID WARL truncation                                     */
/*                                                                     */
/*  SDID is a 6-bit field. Writing a value with bits beyond bit 5      */
/*  truncates to the low 6 bits. Test with 64 (0x40) and 127 (0x7F).  */
/* ------------------------------------------------------------------ */

static void mmpt_14_fn(void) {
    uint64_t ppn_val = 0x1000;

    /* SDID=64 (0x40) — bit 6 set, should truncate to 0 */
    write_csr(CSR_MMPT, (1ULL << 60) | (64ULL << 52) | ppn_val);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_sdid(val) == 0);
    ASSERT(mmpt_mode(val) == 1);
    ASSERT(mmpt_ppn(val) == ppn_val);

    /* SDID=127 (0x7F) — bits 5:0 = 0x3F = 63 */
    write_csr(CSR_MMPT, (1ULL << 60) | (127ULL << 52) | ppn_val);
    val = read_csr(CSR_MMPT);
    ASSERT(mmpt_sdid(val) == 63);
    ASSERT(mmpt_mode(val) == 1);
    ASSERT(mmpt_ppn(val) == ppn_val);
}

/* ------------------------------------------------------------------ */
/*  mmpt.15 — MPT cache flush on mmpt write                            */
/*                                                                     */
/*  Writing mmpt must flush the MPT cache. Set up MPT tables, do an    */
/*  S-mode access (caches walk), modify a leaf entry, rewrite mmpt,    */
/*  and verify the second access re-walks and sees the changed entry.  */
/* ------------------------------------------------------------------ */

static void mmpt_15_setup(void) {
    /* MPT: code=RWX, test=R, nox=R — first S-mode load succeeds */
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
}

static void mmpt_15_fn(void) {
    /* First access caches the MPT walk (read permitted → succeeds) */
    run_in_s_mode(s_mode_load_data);

    /* Remove read permission from the leaf entry covering TEST_DATA_ADDR */
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);

    /* Rewrite mmpt with the same value to flush MPT cache */
    uint64_t mmpt_val = read_csr(CSR_MMPT);
    write_csr(CSR_MMPT, mmpt_val);

    /* Second access re-walks MPT → finds no read permission → EX_LAF */
    run_in_s_mode(s_mode_load_data);
}

/* ------------------------------------------------------------------ */
/*  Teardown helpers                                                   */
/* ------------------------------------------------------------------ */

static void mmpt_teardown(void) {
    write_csr(CSR_MMPT, 0);
}

/* ------------------------------------------------------------------ */
/*  Test module definition                                              */
/* ------------------------------------------------------------------ */

BEGIN_TEST_MODULE(mmpt)

ADD_TEST_CASE(mmpt_01_fn, "mmpt.01 mmpt initial value", TEST_MODE_M, 0)
ADD_TEST_CASE_HOOKS(mmpt_02_fn, "mmpt.02 bare mode write clears sdid and ppn", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_03_fn, "mmpt.03 smmpt43 mode write", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_04_fn, "mmpt.04 smmpt52 mode write", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_05_fn, "mmpt.05 smmpt64 ppn alignment", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_06_fn, "mmpt.06 illegal mode warl", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_07_fn, "mmpt.07 sdid boundary values", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE(s_mode_read_mmpt, "mmpt.08 s-mode csrr mmpt traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_write_mmpt, "mmpt.09 s-mode csrw mmpt traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_csrrs_mmpt, "mmpt.10 s-mode csrrs mmpt traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_csrrc_mmpt, "mmpt.11 s-mode csrrc mmpt traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE_HOOKS(mmpt_12_fn, "mmpt.12 m-mode csrrs rs1=0 reads ok", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_13_fn, "mmpt.13 mode transition 1->2", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_14_fn, "mmpt.14 sdid warl truncation", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_15_fn, "mmpt.15 mmpt write flushes mpt cache", TEST_MODE_M, EX_LAF, mmpt_15_setup, mpt_teardown)

END_TEST_MODULE(mmpt, "CSR MMPT configuration & WARL checks")
