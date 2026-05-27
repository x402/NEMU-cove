// Description: Smsdia extension - msideip/msideie CSRs and MSDEI interrupt
#include "test_main.h"

/* ------------------------------------------------------------------ */
/*  S-mode trap helpers                                                */
/* ------------------------------------------------------------------ */

static void s_mode_read_msideip(void) {
    uint64_t val;
    asm volatile ("csrr %0, %1" : "=r"(val) : "i"(CSR_MSIDEIP));
    (void)val;
    asm volatile ("ecall");
}

static void s_mode_read_msideie(void) {
    uint64_t val;
    asm volatile ("csrr %0, %1" : "=r"(val) : "i"(CSR_MSIDEIE));
    (void)val;
    asm volatile ("ecall");
}

static void s_mode_write_msideie(void) {
    uint64_t val = 0x1;
    asm volatile ("csrw %0, %1" :: "i"(CSR_MSIDEIE), "r"(val));
    asm volatile ("ecall");
}

static void s_mode_csrrs_msideie(void) {
    /* csrrs with non-zero rs1 from S-mode should trap */
    uint64_t val = 0x1;
    asm volatile ("csrrs zero, %0, %1" :: "i"(CSR_MSIDEIE), "r"(val));
    asm volatile ("ecall");
}

static void s_mode_csrrc_msideie(void) {
    /* csrrc with non-zero rs1 from S-mode should trap */
    uint64_t val = 0x1;
    asm volatile ("csrrc zero, %0, %1" :: "i"(CSR_MSIDEIE), "r"(val));
    asm volatile ("ecall");
}

/* ------------------------------------------------------------------ */
/*  msideip: initial value                                             */
/* ------------------------------------------------------------------ */

static void smsdia_01_fn(void) {
    /* msideip is read-only; initial value should be 0 (no interrupts pending) */
    uint64_t val = read_csr(CSR_MSIDEIP);
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  msideip: M-mode csrw triggers EX_II (read-only CSR)                */
/*  After the M-mode trap handler resumes, msideip must still be 0.   */
/* ------------------------------------------------------------------ */

static void smsdia_02_fn(void) {
    /* msideip at 0xF4F has bits[11:10]=0b11 = read-only.
       csrw (write) from M-mode triggers EX_II.
       After trap handler resumes, msideip remains 0. */
    write_csr(CSR_MSIDEIP, 0xFF);
    uint64_t val = read_csr(CSR_MSIDEIP);
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  msideip: csrrs with rs1=0 reads without trap (read-only CSR edge)  */
/* ------------------------------------------------------------------ */

static void smsdia_03_fn(void) {
    /* Per RISC-V spec, csrrs with rs1=x0 is a pure read — even for
       read-only CSRs, it should NOT trap. The is_write flag is false
       when rs1=0, so csr_readonly_permit_check passes. */
    uint64_t val;
    asm volatile ("csrrs %0, %1, x0" : "=r"(val) : "i"(CSR_MSIDEIP));
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  msideip: csrrs with rs1!=0 triggers EX_II (read-only CSR edge)     */
/* ------------------------------------------------------------------ */

static void smsdia_04_fn(void) {
    /* csrrs with non-zero rs1 attempts to set bits on a read-only CSR.
       csr_permit_check(csrid, rs1!=0) → is_write=true → EX_II.
       After M-mode trap handler resumes, msideip remains 0. */
    uint64_t val = 0x1;
    asm volatile ("csrrs zero, %0, %1" :: "i"(CSR_MSIDEIP), "r"(val));
    /* If we reach here, the csrrs did NOT trap — verify value unchanged */
    val = read_csr(CSR_MSIDEIP);
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  msideie: initial value                                             */
/* ------------------------------------------------------------------ */

static void smsdia_05_fn(void) {
    uint64_t val = read_csr(CSR_MSIDEIE);
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  msideie: read-write byte                                           */
/* ------------------------------------------------------------------ */

static void smsdia_06_fn(void) {
    uint64_t write_val = 0xFF;
    write_csr(CSR_MSIDEIE, write_val);
    uint64_t val = read_csr(CSR_MSIDEIE);
    ASSERT(val == write_val);
}

/* ------------------------------------------------------------------ */
/*  msideie: read-write 64-bit                                        */
/* ------------------------------------------------------------------ */

static void smsdia_07_fn(void) {
    uint64_t write_val = 0xAAAAAAAAAAAAAAAAULL;
    write_csr(CSR_MSIDEIE, write_val);
    uint64_t val = read_csr(CSR_MSIDEIE);
    ASSERT(val == write_val);
}

/* ------------------------------------------------------------------ */
/*  msideie: all-ones boundary                                         */
/* ------------------------------------------------------------------ */

static void smsdia_08_fn(void) {
    uint64_t write_val = 0xFFFFFFFFFFFFFFFFULL;
    write_csr(CSR_MSIDEIE, write_val);
    uint64_t val = read_csr(CSR_MSIDEIE);
    ASSERT(val == write_val);
}

/* ------------------------------------------------------------------ */
/*  msideie: clear by writing 0                                       */
/* ------------------------------------------------------------------ */

static void smsdia_09_fn(void) {
    write_csr(CSR_MSIDEIE, 0xFFFF);
    uint64_t val = read_csr(CSR_MSIDEIE);
    ASSERT(val == 0xFFFF);

    write_csr(CSR_MSIDEIE, 0);
    val = read_csr(CSR_MSIDEIE);
    ASSERT(val == 0);
}

/* ------------------------------------------------------------------ */
/*  MSDEI: mip.MSDEIP reflects msideip & msideie                     */
/* ------------------------------------------------------------------ */

static void smsdia_10_fn(void) {
    /* When msideie is 0, mip.MSDEIP should be 0 regardless of msideip */
    write_csr(CSR_MSIDEIE, 0);
    uint64_t mip_val = read_csr(CSR_MIP);
    ASSERT((mip_val & MIP_MSDEIP) == 0);
}

static void smsdia_11_fn(void) {
    /* Set msideie to enable SID 0; msideip is always 0 in NEMU
       (no external interrupt injection), so MSDEIP remains 0. */
    write_csr(CSR_MSIDEIE, 0x1);
    uint64_t mip_val = read_csr(CSR_MIP);
    ASSERT((mip_val & MIP_MSDEIP) == 0);
}

/* ------------------------------------------------------------------ */
/*  MSDEI: mie.MSDEIE enables the interrupt                           */
/* ------------------------------------------------------------------ */

static void smsdia_12_fn(void) {
    set_csr(CSR_MIE, MIE_MSDEIE);
    uint64_t new_mie = read_csr(CSR_MIE);
    ASSERT((new_mie & MIE_MSDEIE) != 0);

    clear_csr(CSR_MIE, MIE_MSDEIE);
    new_mie = read_csr(CSR_MIE);
    ASSERT((new_mie & MIE_MSDEIE) == 0);
}

/* ------------------------------------------------------------------ */
/*  MSDEI: mideleg can delegate MSDEI to S-mode                       */
/* ------------------------------------------------------------------ */

static void smsdia_13_fn(void) {
    set_csr(CSR_MIDELEG, (1ULL << IRQ_MSDEI));
    uint64_t new_mideleg = read_csr(CSR_MIDELEG);
    ASSERT((new_mideleg & (1ULL << IRQ_MSDEI)) != 0);

    clear_csr(CSR_MIDELEG, (1ULL << IRQ_MSDEI));
    new_mideleg = read_csr(CSR_MIDELEG);
    ASSERT((new_mideleg & (1ULL << IRQ_MSDEI)) == 0);
}

/* ------------------------------------------------------------------ */
/*  MSDEI: sie.MSDEIE visible after mideleg delegation                */
/* ------------------------------------------------------------------ */

static void smsdia_14_fn(void) {
    /* After delegating MSDEI to S-mode via mideleg, the MSDEI bit
       should be visible in sie (S-mode interrupt enable register).
       We read sie from M-mode to verify the bit is present. */
    set_csr(CSR_MIDELEG, (1ULL << IRQ_MSDEI));
    set_csr(CSR_MIE, MIE_MSDEIE);

    /* sie = mie & mideleg (for delegated bits) */
    uint64_t sie_val = read_csr(CSR_SIE);
    ASSERT((sie_val & MIE_MSDEIE) != 0);

    /* Clear */
    clear_csr(CSR_MIE, MIE_MSDEIE);
    clear_csr(CSR_MIDELEG, (1ULL << IRQ_MSDEI));
}

/* ------------------------------------------------------------------ */
/*  MSDEI: hideleg bit 14 is read-only zero (no VS-mode delegation)   */
/* ------------------------------------------------------------------ */

static void smsdia_15_fn(void) {
    /* Per spec: MSDEI cannot be delegated to VS-mode.
       hideleg bit 14 must be read-only zero. */
    uint64_t hideleg_val = read_csr(CSR_HIDELEG);
    ASSERT((hideleg_val & (1ULL << IRQ_MSDEI)) == 0);

    /* Attempt to set bit 14 — should remain 0 */
    set_csr(CSR_HIDELEG, (1ULL << IRQ_MSDEI));
    hideleg_val = read_csr(CSR_HIDELEG);
    ASSERT((hideleg_val & (1ULL << IRQ_MSDEI)) == 0);
}

/* ------------------------------------------------------------------ */
/*  Teardown: clear all smsdia state                                   */
/* ------------------------------------------------------------------ */

static void smsdia_teardown(void) {
    write_csr(CSR_MSIDEIE, 0);
    /* msideip is read-only, no need to clear */
    clear_csr(CSR_MIE, MIE_MSDEIE);
    clear_csr(CSR_MIDELEG, (1ULL << IRQ_MSDEI));
}

/* ------------------------------------------------------------------ */
/*  Test module definition                                              */
/* ------------------------------------------------------------------ */

BEGIN_TEST_MODULE(smsdia)

ADD_TEST_CASE(smsdia_01_fn, "smsdia.01 msideip initial value is zero", TEST_MODE_M, 0)
ADD_TEST_CASE(smsdia_02_fn, "smsdia.02 m-mode csrw msideip triggers EX_II", TEST_MODE_M, EX_II)
ADD_TEST_CASE(smsdia_03_fn, "smsdia.03 csrrs msideip rs1=0 reads ok", TEST_MODE_M, 0)
ADD_TEST_CASE(smsdia_04_fn, "smsdia.04 csrrs msideip rs1!=0 triggers EX_II", TEST_MODE_M, EX_II)
ADD_TEST_CASE_HOOKS(smsdia_05_fn, "smsdia.05 msideie initial value is zero", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_06_fn, "smsdia.06 msideie read-write byte", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_07_fn, "smsdia.07 msideie read-write 64-bit", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_08_fn, "smsdia.08 msideie all-ones boundary", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_09_fn, "smsdia.09 msideie clear to zero", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_10_fn, "smsdia.10 mip.MSDEIP zero when msideie zero", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_11_fn, "smsdia.11 mip.MSDEIP zero when msideip zero", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_12_fn, "smsdia.12 mie.MSDEIE read-write", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_13_fn, "smsdia.13 mideleg.MSDEI delegation", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_14_fn, "smsdia.14 sie.MSDEIE visible after delegation", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE_HOOKS(smsdia_15_fn, "smsdia.15 hideleg bit14 read-only zero", TEST_MODE_M, 0, NULL, smsdia_teardown)
ADD_TEST_CASE(s_mode_read_msideip, "smsdia.16 s-mode csrr msideip traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_read_msideie, "smsdia.17 s-mode csrr msideie traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_write_msideie, "smsdia.18 s-mode csrw msideie traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_csrrs_msideie, "smsdia.19 s-mode csrrs msideie traps", TEST_MODE_S, EX_II)
ADD_TEST_CASE(s_mode_csrrc_msideie, "smsdia.20 s-mode csrrc msideie traps", TEST_MODE_S, EX_II)

END_TEST_MODULE(smsdia, "Smsdia extension - msideip/msideie CSRs and MSDEI interrupt")