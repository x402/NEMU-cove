// Description: MPT Caching & flushes
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

static void mpt_cache_01_setup(void) { mpt_setup(XWR_RWX, XWR_R, XWR_R); }
static void mpt_cache_01_fn(void) {
    run_in_s_mode(s_mode_load_data);
    run_in_s_mode(s_mode_load_data);
}

static void mpt_cache_02_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    asm volatile (".word 0x1C000073"); // MFENCE.PA
    run_in_s_mode(s_mode_store_data);
}

static void mpt_cache_03_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    uint64_t mmpt_val = read_csr(CSR_MMPT);
    write_csr(CSR_MMPT, mmpt_val);
    run_in_s_mode(s_mode_load_data);
}

static void mpt_cache_04_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    asm volatile ("sfence.vma");
    run_in_s_mode(s_mode_load_data);
}

static void mpt_cache_05_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    run_in_s_mode(s_mode_load_data);
}

BEGIN_TEST_MODULE(mpt_cache)

ADD_TEST_CASE_HOOKS(mpt_cache_01_fn, "mpt_cache.01 cache hit repeated access", TEST_MODE_M, 0, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_02_fn, "mpt_cache.02 mfence.pa flushes cache", TEST_MODE_M, EX_SAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_03_fn, "mpt_cache.03 mmpt write flushes cache", TEST_MODE_M, EX_LAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_04_fn, "mpt_cache.04 sfence.vma flushes cache", TEST_MODE_M, EX_LAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_05_fn, "mpt_cache.05 stale cache without fence", TEST_MODE_M, 0, mpt_cache_01_setup, mpt_teardown)

END_TEST_MODULE(mpt_cache, "MPT Caching & flushes")
