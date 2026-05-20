#include "test_main.h"

#define TEST_DATA_ADDR 0x80100000ULL

static void s_mode_load_data(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    uint64_t val = *ptr;
    (void)val;
    asm volatile ("ecall");
}

static void s_mode_store_data(void) {
    volatile uint64_t *ptr = (uint64_t *)TEST_DATA_ADDR;
    *ptr = 0xCAFEBABE;
    asm volatile ("ecall");
}

void test_mpt_cache(void) {
    // T5.1: repeated access uses cache (verify no fault on second access)
    STEST("T5.1 cache hit repeated access", {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        run_in_s_mode(s_mode_load_data);
        run_in_s_mode(s_mode_load_data);
        mpt_teardown();
    });

    // T5.2: modify MPT -> MFENCE.PA -> new permission生效
    STEST_TRAP("T5.2 mfence.pa flushes cache", EX_SAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        run_in_s_mode(s_mode_load_data);
        // Modify MPT table in memory to deny write
        extern volatile uint64_t mpt_leaf[];
        mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
        // MFENCE.PA to flush cache
        asm volatile (".word 0x1C000073");
        // Now store should fail
        run_in_s_mode(s_mode_store_data);
        mpt_teardown();
    });

    // T5.3: write mmpt CSR flushes cache
    STEST_TRAP("T5.3 mmpt write flushes cache", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        run_in_s_mode(s_mode_load_data);
        // Modify MPT to deny read
        extern volatile uint64_t mpt_leaf[];
        mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
        // Write mmpt (same value but triggers flush)
        uint64_t mmpt_val = read_csr(CSR_MMPT);
        write_csr(CSR_MMPT, mmpt_val);
        // Now load should fail
        run_in_s_mode(s_mode_load_data);
        mpt_teardown();
    });

    // T5.4: SFENCE.VMA flushes MPT cache
    STEST_TRAP("T5.4 sfence.vma flushes cache", EX_LAF, {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        run_in_s_mode(s_mode_load_data);
        extern volatile uint64_t mpt_leaf[];
        mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
        asm volatile ("sfence.vma");
        run_in_s_mode(s_mode_load_data);
        mpt_teardown();
    });

    // T5.5: stale cache without fence (demonstrates cache exists)
    STEST("T5.5 stale cache without fence", {
        mpt_setup(XWR_RWX, XWR_R, XWR_R);
        run_in_s_mode(s_mode_load_data);
        extern volatile uint64_t mpt_leaf[];
        mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
        // No fence executed - cache should still have old permission
        run_in_s_mode(s_mode_load_data);
        mpt_teardown();
    });
}