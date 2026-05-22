// Description: CSR MSDCFG privilege & fields
#include "test_main.h"

static void s_mode_read_msdcfg(void) {
    uint64_t val;
    asm volatile ("csrr %0, 0x74E" : "=r"(val));
    (void)val;
    asm volatile ("ecall");
}

static void msdcfg_01_fn(void) {
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT(val == 0);
}

static void msdcfg_02_fn(void) {
    uint64_t write_val = (0xFFULL << 8) | (0x3FULL << 16);
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT((val & (0xFFULL << 8)) == 0);
    ASSERT((val & (0x3FULL << 16)) == 0);
}

static void msdcfg_03_fn(void) {
    uint64_t write_val = 0x3F | (1ULL << 6) | (1ULL << 7);
    write_csr(CSR_MSDCFG, write_val);
    uint64_t val = read_csr(CSR_MSDCFG);
    ASSERT(msdcfg_sidn(val) == 0x3F);
    ASSERT(msdcfg_seda(val) == 1);
    ASSERT(msdcfg_seta(val) == 1);
}

static void msdcfg_teardown(void) {
    write_csr(CSR_MSDCFG, 0);
}

BEGIN_TEST_MODULE(msdcfg)

ADD_TEST_CASE(msdcfg_01_fn, "msdcfg.01 msdcfg initial value", TEST_MODE_M, 0)
ADD_TEST_CASE_HOOKS(msdcfg_02_fn, "msdcfg.02 wpri fields cleared", TEST_MODE_M, 0, NULL, msdcfg_teardown)
ADD_TEST_CASE_HOOKS(msdcfg_03_fn, "msdcfg.03 valid fields preserved", TEST_MODE_M, 0, NULL, msdcfg_teardown)
ADD_TEST_CASE(s_mode_read_msdcfg, "msdcfg.04 s-mode access msdcfg", TEST_MODE_S, EX_II)

END_TEST_MODULE(msdcfg, "CSR MSDCFG privilege & fields")
