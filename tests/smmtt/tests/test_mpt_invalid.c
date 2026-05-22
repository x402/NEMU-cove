// Description: Malformed entry checks
#include "test_main.h"

#define TEST_DATA_ADDR 0x82100000ULL

static volatile uint64_t test_data_ptr = TEST_DATA_ADDR;

static void s_mode_load_data(void) {
    volatile uint64_t *ptr = (uint64_t *)test_data_ptr;
    uint64_t val = *ptr;
    (void)val;
    asm volatile ("ecall");
}

static void mpt_invalid_01_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_l2[];
    mpt_l2[65] = MPTE_V | MPTE_N | ((uint64_t)mpt_leaf >> 12 << 10);
}

static void mpt_invalid_02_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_R << 8) | (1ULL << 3);
}

static void mpt_invalid_03_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_l2[];
    mpt_l2[65] = MPTE_V | (1ULL << 54) | ((uint64_t)mpt_leaf >> 12 << 10);
}

static void mpt_invalid_04_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    test_data_ptr = 0x80000800000ULL;
}

static void mpt_invalid_04_teardown(void) {
    test_data_ptr = TEST_DATA_ADDR;
    mpt_teardown();
}

static void mpt_invalid_05_setup(void) {
    write_csr(CSR_MMPT, (1ULL << 60) | (1ULL << 52) | 0xFFFFFFFFFFFULL);
}

static void mpt_invalid_05_teardown(void) {
    write_csr(CSR_MMPT, 0);
}

static void mpt_invalid_06_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | MPTE_N | (3ULL << NAPOT_G_SHIFT) | (XWR_R << 8);
}

BEGIN_TEST_MODULE(mpt_invalid)

ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.01 non-leaf napot triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.02 leaf reserved bits trigger fault", TEST_MODE_S, EX_LAF, mpt_invalid_02_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.03 non-leaf reserved bits trigger fault", TEST_MODE_S, EX_LAF, mpt_invalid_03_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.04 pa out of range triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_04_setup, mpt_invalid_04_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.05 mpt outside pmem triggers fault", TEST_MODE_S, EX_IAF, mpt_invalid_05_setup, mpt_invalid_05_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.06 napot invalid g triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_06_setup, mpt_teardown)

END_TEST_MODULE(mpt_invalid, "Malformed entry checks")
