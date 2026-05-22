// Description: CSR MMPT configuration & WARL checks
#include "test_main.h"

static void s_mode_read_mmpt(void) {
    uint64_t val;
    asm volatile ("csrr %0, 0x382" : "=r"(val));
    (void)val;
    asm volatile ("ecall");
}

static void mmpt_01_fn(void) {
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(val == 0);
}

static void mmpt_02_fn(void) {
    write_csr(CSR_MMPT, (1ULL << 52) | (5ULL << 60));
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val) == 0);
    ASSERT(mmpt_sdid(val) == 0);
    ASSERT(mmpt_ppn(val) == 0);
}

static void mmpt_03_fn(void) {
    uint64_t ppn_val = 0x12345;
    uint64_t write_val = (1ULL << 60) | (1ULL << 52) | ppn_val;
    write_csr(CSR_MMPT, write_val);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val) == 1);
    ASSERT(mmpt_sdid(val) == 1);
    ASSERT(mmpt_ppn(val) == ppn_val);
}

static void mmpt_04_fn(void) {
    uint64_t ppn_val = 0x12345;
    uint64_t write_val = (2ULL << 60) | (2ULL << 52) | ppn_val;
    write_csr(CSR_MMPT, write_val);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val) == 2);
    ASSERT(mmpt_sdid(val) == 2);
    ASSERT(mmpt_ppn(val) == ppn_val);
}

static void mmpt_05_fn(void) {
    uint64_t ppn_val = 0x12345 | 0x7;
    uint64_t write_val = (3ULL << 60) | (1ULL << 52) | ppn_val;
    write_csr(CSR_MMPT, write_val);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val) == 3);
    ASSERT(mmpt_ppn(val) == (ppn_val & ~0x7ULL));
}

static void mmpt_06_fn(void) {
    write_csr(CSR_MMPT, (1ULL << 60));
    uint64_t val1 = read_csr(CSR_MMPT);
    write_csr(CSR_MMPT, (4ULL << 60) | (1ULL << 52));
    uint64_t val2 = read_csr(CSR_MMPT);
    ASSERT(mmpt_mode(val2) == mmpt_mode(val1));
}

static void mmpt_07_fn(void) {
    write_csr(CSR_MMPT, (1ULL << 60) | (0ULL << 52) | 0x1000);
    uint64_t val = read_csr(CSR_MMPT);
    ASSERT(mmpt_sdid(val) == 0);

    write_csr(CSR_MMPT, (1ULL << 60) | (63ULL << 52) | 0x1000);
    val = read_csr(CSR_MMPT);
    ASSERT(mmpt_sdid(val) == 63);
}

static void mmpt_teardown(void) {
    write_csr(CSR_MMPT, 0);
}

BEGIN_TEST_MODULE(mmpt)

ADD_TEST_CASE(mmpt_01_fn, "mmpt.01 mmpt initial value", TEST_MODE_M, 0)
ADD_TEST_CASE_HOOKS(mmpt_02_fn, "mmpt.02 bare mode write clears sdid and ppn", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_03_fn, "mmpt.03 smmpt43 mode write", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_04_fn, "mmpt.04 smmpt52 mode write", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_05_fn, "mmpt.05 smmpt64 ppn alignment", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_06_fn, "mmpt.06 illegal mode warl", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE_HOOKS(mmpt_07_fn, "mmpt.07 sdid boundary values", TEST_MODE_M, 0, NULL, mmpt_teardown)
ADD_TEST_CASE(s_mode_read_mmpt, "mmpt.08 s-mode access mmpt", TEST_MODE_S, EX_II)

END_TEST_MODULE(mmpt, "CSR MMPT configuration & WARL checks")
