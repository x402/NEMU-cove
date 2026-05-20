#include "test_main.h"

static void s_mode_read_msdcfg(void) {
    uint64_t val;
    asm volatile ("csrr %0, 0x74E" : "=r"(val));
    (void)val;
    asm volatile ("ecall");
}

void test_csr_msdcfg(void) {
    TEST("T2.1 msdcfg initial value", {
        uint64_t val = read_csr(CSR_MSDCFG);
        ASSERT(val == 0);
    });

    TEST("T2.2 wpri fields cleared", {
        uint64_t write_val = (0xFFULL << 8) | (0x3FULL << 16);
        write_csr(CSR_MSDCFG, write_val);
        uint64_t val = read_csr(CSR_MSDCFG);
        ASSERT((val & (0xFFULL << 8)) == 0);
        ASSERT((val & (0x3FULL << 16)) == 0);
    });

    TEST("T2.3 valid fields preserved", {
        uint64_t write_val = 0x3F | (1ULL << 6) | (1ULL << 7);
        write_csr(CSR_MSDCFG, write_val);
        uint64_t val = read_csr(CSR_MSDCFG);
        ASSERT(msdcfg_sidn(val) == 0x3F);
        ASSERT(msdcfg_seda(val) == 1);
        ASSERT(msdcfg_seta(val) == 1);
    });

    STEST_TRAP("T2.4 s-mode access msdcfg", EX_II, {
        run_in_s_mode(s_mode_read_msdcfg);
    });

    write_csr(CSR_MSDCFG, 0);
}
