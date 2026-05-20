#include "test_main.h"

static void s_mode_read_mmpt(void) {
    uint64_t val;
    asm volatile ("csrr %0, 0x382" : "=r"(val));
    (void)val;
    asm volatile ("ecall");
}

void test_csr_mmpt(void) {
    TEST("T1.1 mmpt initial value", {
        uint64_t val = read_csr(CSR_MMPT);
        ASSERT(val == 0);
    });

    TEST("T1.2 bare mode write clears sdid and ppn", {
        write_csr(CSR_MMPT, (1ULL << 52) | (5ULL << 60));
        uint64_t val = read_csr(CSR_MMPT);
        ASSERT(mmpt_mode(val) == 0);
        ASSERT(mmpt_sdid(val) == 0);
        ASSERT(mmpt_ppn(val) == 0);
    });

    TEST("T1.3 smmpt43 mode write", {
        uint64_t ppn_val = 0x12345;
        uint64_t write_val = (1ULL << 60) | (1ULL << 52) | ppn_val;
        write_csr(CSR_MMPT, write_val);
        uint64_t val = read_csr(CSR_MMPT);
        ASSERT(mmpt_mode(val) == 1);
        ASSERT(mmpt_sdid(val) == 1);
        ASSERT(mmpt_ppn(val) == ppn_val);
    });

    TEST("T1.4 smmpt52 mode write", {
        uint64_t ppn_val = 0x12345;
        uint64_t write_val = (2ULL << 60) | (2ULL << 52) | ppn_val;
        write_csr(CSR_MMPT, write_val);
        uint64_t val = read_csr(CSR_MMPT);
        ASSERT(mmpt_mode(val) == 2);
        ASSERT(mmpt_sdid(val) == 2);
        ASSERT(mmpt_ppn(val) == ppn_val);
    });

    TEST("T1.5 smmpt64 ppn alignment", {
        uint64_t ppn_val = 0x12345 | 0x7;
        uint64_t write_val = (3ULL << 60) | (1ULL << 52) | ppn_val;
        write_csr(CSR_MMPT, write_val);
        uint64_t val = read_csr(CSR_MMPT);
        ASSERT(mmpt_mode(val) == 3);
        ASSERT(mmpt_ppn(val) == (ppn_val & ~0x7ULL));
    });

    TEST("T1.6 illegal mode warl", {
        write_csr(CSR_MMPT, (1ULL << 60));
        uint64_t val1 = read_csr(CSR_MMPT);
        write_csr(CSR_MMPT, (4ULL << 60) | (1ULL << 52));
        uint64_t val2 = read_csr(CSR_MMPT);
        ASSERT(mmpt_mode(val2) == mmpt_mode(val1));
    });

    TEST("T1.7 sdid boundary values", {
        write_csr(CSR_MMPT, (1ULL << 60) | (0ULL << 52) | 0x1000);
        uint64_t val = read_csr(CSR_MMPT);
        ASSERT(mmpt_sdid(val) == 0);

        write_csr(CSR_MMPT, (1ULL << 60) | (63ULL << 52) | 0x1000);
        val = read_csr(CSR_MMPT);
        ASSERT(mmpt_sdid(val) == 63);
    });

    write_csr(CSR_MMPT, 0);

    STEST_TRAP("T1.8 s-mode access mmpt", EX_II, {
        run_in_s_mode(s_mode_read_mmpt);
    });
}
