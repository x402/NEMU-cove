#include "test_main.h"

#define TEST_DATA_ADDR 0x82100000ULL

static volatile uint64_t test_data_ptr = TEST_DATA_ADDR;

static void s_mode_load_data(void) {
    volatile uint64_t *ptr = (uint64_t *)test_data_ptr;
    uint64_t val = *ptr;
    (void)val;
    asm volatile ("ecall");
}

void test_boundary(void) {
    // T6.1: non-leaf with NAPOT=1, L=0
    STEST_TRAP("T6.1 non-leaf napot triggers fault", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        extern volatile uint64_t mpt_l2[];
        mpt_l2[65] = MPTE_V | MPTE_N | ((uint64_t)mpt_leaf >> 12 << 10);
        run_in_s_mode(s_mode_load_data);
        mpt_teardown();
    });

    // T6.2: leaf with reserved bits [7:3] non-zero
    STEST_TRAP("T6.2 leaf reserved bits trigger fault", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        extern volatile uint64_t mpt_leaf[];
        mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_R << 8) | (1ULL << 3);
        run_in_s_mode(s_mode_load_data);
        mpt_teardown();
    });

    // T6.3: non-leaf with reserved bits [63:54] non-zero
    STEST_TRAP("T6.3 non-leaf reserved bits trigger fault", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        extern volatile uint64_t mpt_l2[];
        mpt_l2[65] = MPTE_V | (1ULL << 54) | ((uint64_t)mpt_leaf >> 12 << 10);
        run_in_s_mode(s_mode_load_data);
        mpt_teardown();
    });

    // T6.4: PA out of address space range (Smmpt43: pa >= 2^43)
    STEST_TRAP("T6.4 pa out of range triggers fault", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        test_data_ptr = 0x80000800000ULL;
        run_in_s_mode(s_mode_load_data);
        test_data_ptr = TEST_DATA_ADDR;
        mpt_teardown();
    });

    // T6.5: MPT pointer outside pmem
    STEST_TRAP("T6.5 mpt outside pmem triggers fault", EX_IAF, {
        write_csr(CSR_MMPT, (1ULL << 60) | (1ULL << 52) | 0xFFFFFFFFFFFULL);
        run_in_s_mode(s_mode_load_data);
        write_csr(CSR_MMPT, 0);
    });

    // T6.6: NAPOT with G != 4
    STEST_TRAP("T6.6 napot invalid g triggers fault", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        extern volatile uint64_t mpt_leaf[];
        mpt_leaf[16] = MPTE_V | MPTE_L | MPTE_N | (3ULL << NAPOT_G_SHIFT) | (XWR_R << 8);
        run_in_s_mode(s_mode_load_data);
        mpt_teardown();
    });
}
