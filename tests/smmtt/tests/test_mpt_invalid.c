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

static void s_mode_load_data_ok(void) {
    volatile uint64_t *ptr = (uint64_t *)test_data_ptr;
    uint64_t val = *ptr;
    (void)val;
    asm volatile ("ecall");
}

// Non-leaf entry with N=1 but L=0 (non-leaf NAPOT) -> fault at walk time
static void mpt_invalid_01_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_l2[];
    mpt_l2[65] = MPTE_V | MPTE_N | ((uint64_t)mpt_leaf >> 12 << 10);
}

// Leaf entry with reserved bits [7:3] != 0 -> fault at leaf check
static void mpt_invalid_02_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_R << 8) | (1ULL << 3);
}

// Non-leaf entry with reserved bits [63:54] != 0 -> fault at walk time
static void mpt_invalid_03_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_l2[];
    mpt_l2[65] = MPTE_V | (1ULL << 54) | ((uint64_t)mpt_leaf >> 12 << 10);
}

// PA exceeds 43-bit maximum address for smmpt43 mode (mode=1) -> PA range check fault
// 0x80000800000 has bit 43 set, which is checked by max_pa_mask = ~((1ULL<<43)-1)
static void mpt_invalid_04_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    test_data_ptr = 0x80000800000ULL;
}

static void mpt_invalid_04_teardown(void) {
    test_data_ptr = TEST_DATA_ADDR;
    mpt_teardown();
}

// MPT root table PPN points outside physical memory -> walk can't read root entry.
// Since MMPT is active with an invalid root PPN, ALL S-mode accesses (including
// instruction fetch) fail the MPT walk, resulting in EX_IAF (instruction access fault)
// before the data load can even be attempted.
static void mpt_invalid_05_setup(void) {
    write_csr(CSR_MMPT, (1ULL << 60) | (1ULL << 52) | 0xFFFFFFFFFFFULL);
}

static void mpt_invalid_05_teardown(void) {
    write_csr(CSR_MMPT, 0);
}

// NAPOT leaf entry with G != 4 -> fault at leaf check
static void mpt_invalid_06_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | MPTE_N | (3ULL << NAPOT_G_SHIFT) | (XWR_R << 8);
}

// Leaf entry with V=0 (invalid leaf) -> fault at V-bit check
static void mpt_invalid_07_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = 0;
}

// Non-leaf entry with V=0 (invalid mid-level entry) -> fault at V-bit check
static void mpt_invalid_08_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_l2[];
    mpt_l2[65] = 0;
}

// Leaf entry valid but XWR=0 for the accessed page index -> permission check fails
static void mpt_invalid_09_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    // Valid leaf (V=1, L=1, reserved bits clear) but pi=0 has XWR_NONE
    // pi=1 gets XWR_R to keep entry structurally valid
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_R << (8 + 1*3));
}

// Valid NAPOT leaf with G=4 allows access (positive test)
static void mpt_invalid_10_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | MPTE_N | (NAPOT_G_4 << NAPOT_G_SHIFT) | (XWR_R << 8);
}

// Leaf entry with reserved XWR=2 (write-only) does not permit loads -> fault
// Per spec, XWR=2 is reserved; NEMU checks individual R/W/X bits, so W-only != R -> fault
static void mpt_invalid_11_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    // Valid leaf with pi=0 having XWR=2 (W-only, reserved encoding)
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_W << 8);
}

// Leaf entry with reserved XWR=6 (write-execute) does not permit loads -> fault
// Per spec, XWR=6 is reserved; NEMU checks individual bits, so WX lacks R bit -> fault
static void mpt_invalid_12_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    // Valid leaf with pi=0 having XWR=6 (WX, reserved encoding)
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_WX << 8);
}

BEGIN_TEST_MODULE(mpt_invalid)

ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.01 non-leaf napot triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.02 leaf reserved bits trigger fault", TEST_MODE_S, EX_LAF, mpt_invalid_02_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.03 non-leaf reserved bits trigger fault", TEST_MODE_S, EX_LAF, mpt_invalid_03_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.04 pa exceeds max address for mode triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_04_setup, mpt_invalid_04_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.05 mpt outside pmem triggers fault", TEST_MODE_S, EX_IAF, mpt_invalid_05_setup, mpt_invalid_05_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.06 napot invalid g triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_06_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.07 leaf v=0 triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_07_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.08 non-leaf v=0 triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_08_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.09 leaf xwr=0 triggers fault", TEST_MODE_S, EX_LAF, mpt_invalid_09_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data_ok, "mpt_invalid.10 napot g=4 valid leaf succeeds", TEST_MODE_S, 0, mpt_invalid_10_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.11 reserved xwr=2 denies load", TEST_MODE_S, EX_LAF, mpt_invalid_11_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(s_mode_load_data, "mpt_invalid.12 reserved xwr=6 denies load", TEST_MODE_S, EX_LAF, mpt_invalid_12_setup, mpt_teardown)

END_TEST_MODULE(mpt_invalid, "Malformed entry checks")
