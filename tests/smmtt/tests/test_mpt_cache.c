// Description: MPT Caching & flushes
#include "test_main.h"

#define TEST_DATA_ADDR 0x80100000ULL

/* ------------------------------------------------------------------ */
/*  S-mode trap helpers                                                */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/*  Setup / teardown helpers                                           */
/* ------------------------------------------------------------------ */

static void mpt_cache_01_setup(void) { mpt_setup(XWR_RWX, XWR_R, XWR_R); }

/* Setup: leaf[16] is invalid (V=0) for V=0->V=1 transition test */
static void mpt_cache_10_setup(void) {
    mpt_setup(XWR_RWX, XWR_R, XWR_R);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = 0;
}

/* ------------------------------------------------------------------ */
/*  Test 01: cache hit repeated access                                 */
/*  Load the same address twice in S-mode — second access is a cache   */
/*  hit (no table walk needed).                                        */
/* ------------------------------------------------------------------ */

static void mpt_cache_01_fn(void) {
    run_in_s_mode(s_mode_load_data);
    run_in_s_mode(s_mode_load_data);
}

/* ------------------------------------------------------------------ */
/*  Test 02: MFENCE.PA flushes cache                                   */
/*  Load (caches XWR_R), revoke permissions, MFENCE.PA, then store.    */
/*  MFENCE.PA flushes the cache, so the store re-walks and faults.     */
/* ------------------------------------------------------------------ */

static void mpt_cache_02_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    asm volatile (".word 0x1C000073"); // MFENCE.PA rs1=x0, rs2=x0
    run_in_s_mode(s_mode_store_data);
}

/* ------------------------------------------------------------------ */
/*  Test 03: MMPT write flushes cache                                  */
/*  Load (caches entry), revoke permissions, re-write MMPT (flush),    */
/*  then load — cache miss forces re-walk, finds XWR_NONE -> fault.    */
/* ------------------------------------------------------------------ */

static void mpt_cache_03_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    uint64_t mmpt_val = read_csr(CSR_MMPT);
    write_csr(CSR_MMPT, mmpt_val);
    run_in_s_mode(s_mode_load_data);
}

/* ------------------------------------------------------------------ */
/*  Test 04: sfence.vma flushes cache                                  */
/*  Same pattern: load, revoke, sfence.vma, re-load -> fault.          */
/*  sfence.vma is architecturally required to flush the MPT cache.     */
/* ------------------------------------------------------------------ */

static void mpt_cache_04_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    asm volatile ("sfence.vma");
    run_in_s_mode(s_mode_load_data);
}

/* ------------------------------------------------------------------ */
/*  Test 05: stale cache without fence                                 */
/*  Load (caches XWR_R), revoke permissions with NO fence, re-load.    */
/*  The cache still has the old (XWR_R) entry, so the load succeeds    */
/*  (stale cache hit).                                                 */
/*                                                                     */
/*  NOTE: This test is implementation-dependent. In NEMU, the MPT      */
/*  cache is only flushed by MFENCE.PA, MINVAL.PA, MMPT CSR write,     */
/*  or sfence.vma. Since none of these occur between the leaf          */
/*  modification and the second load, the stale cached entry is used.  */
/*  Other implementations may differ — e.g., hardware may            */
/*  automatically invalidate cache entries on leaf modification.        */
/* ------------------------------------------------------------------ */

static void mpt_cache_05_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    run_in_s_mode(s_mode_load_data);
}

/* ------------------------------------------------------------------ */
/*  Test 06: MFENCE.PA with operands flushes cache                     */
/*  Like test 02 but uses MFENCE.PA with non-zero rs1 and rs2          */
/*  (selective invalidation by address and SDID). In NEMU, the         */
/*  operand-based form still performs a full cache flush.              */
/* ------------------------------------------------------------------ */

static void mpt_cache_06_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    /* MFENCE.PA rs1=a0 (paddr), rs2=a1 (sdid) */
    asm volatile ("li a0, 0x80100000\n"
                  "li a1, 1\n"
                  ".word 0x1CB50073\n");
    run_in_s_mode(s_mode_store_data);
}

/* ------------------------------------------------------------------ */
/*  Test 07: MINVAL.PA flushes cache                                   */
/*  Like test 02 but uses MINVAL.PA instead of MFENCE.PA.              */
/*  MINVAL.PA is also architecturally required to flush the MPT cache. */
/* ------------------------------------------------------------------ */

static void mpt_cache_07_fn(void) {
    run_in_s_mode(s_mode_load_data);
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);
    asm volatile (".word 0x1E000073"); // MINVAL.PA rs1=x0, rs2=x0
    run_in_s_mode(s_mode_store_data);
}

/* ------------------------------------------------------------------ */
/*  Test 08: cache flush then re-access                                */
/*  Load (caches XWR_R), upgrade to XWR_RW, MFENCE.PA (flush),         */
/*  then store — the cache is repopulated with the new permissions     */
/*  and the store succeeds.                                            */
/* ------------------------------------------------------------------ */

static void mpt_cache_08_fn(void) {
    run_in_s_mode(s_mode_load_data);      /* Cache populated with XWR_R */
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_RW << 8);  /* Add write permission */
    asm volatile (".word 0x1C000073");    /* MFENCE.PA — flush cache */
    run_in_s_mode(s_mode_store_data);     /* Re-walk, cache repopulated, store OK */
}

/* ------------------------------------------------------------------ */
/*  Test 09: different SDID forces cache re-walk                       */
/*  Load (caches entry tagged with SDID=1), revoke permissions,        */
/*  change MMPT to SDID=2 (same root PPN). The MMPT write flushes the  */
/*  cache, so the second load re-walks and finds XWR_NONE -> fault.    */
/*  Without the flush or SDID tag, the stale cache entry would match.  */
/* ------------------------------------------------------------------ */

static void mpt_cache_09_fn(void) {
    run_in_s_mode(s_mode_load_data);      /* Cache with SDID=1, XWR_R */

    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_NONE << 8);  /* Remove perms */

    /* Change MMPT to SDID=2, same root PPN — flushes cache */
    extern volatile uint64_t mpt_root[];
    uint64_t new_mmpt = (1ULL << 60) | (2ULL << 52)
                      | (((uint64_t)mpt_root >> 12) & 0xFFFFFFFFFFFULL);
    write_csr(CSR_MMPT, new_mmpt);

    run_in_s_mode(s_mode_load_data);      /* Re-walk with SDID=2 -> EX_LAF */
}

/* ------------------------------------------------------------------ */
/*  Test 10: V=0 to V=1 transition visible without fence               */
/*  Per spec: "A change from Invalid to Valid eventually becomes       */
/*  visible within bounded time without explicit fence."               */
/*  First access with invalid leaf faults; after making the leaf       */
/*  valid, a subsequent access succeeds without any fence.             */
/*                                                                      */
/*  This test is split into two test cases:                             */
/*  - 10a: first access faults (EX_LAF) with invalid leaf             */
/*  - 10b: second access succeeds after making leaf valid (no fence)   */
/* ------------------------------------------------------------------ */

static void mpt_cache_10a_fn(void) {
    /* Leaf is invalid (V=0) from setup — access should fault */
    run_in_s_mode(s_mode_load_data);
}

static void mpt_cache_10b_fn(void) {
    /* Make leaf valid — no fence needed per spec */
    extern volatile uint64_t mpt_leaf[];
    mpt_leaf[16] = MPTE_V | MPTE_L | (XWR_R << 8);
    /* Access should succeed without any fence */
    run_in_s_mode(s_mode_load_data);
}

/* ------------------------------------------------------------------ */
/*  Test module definition                                              */
/* ------------------------------------------------------------------ */

BEGIN_TEST_MODULE(mpt_cache)

ADD_TEST_CASE_HOOKS(mpt_cache_01_fn, "mpt_cache.01 cache hit repeated access",     TEST_MODE_M, 0,      mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_02_fn, "mpt_cache.02 mfence.pa flushes cache",       TEST_MODE_M, EX_SAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_03_fn, "mpt_cache.03 mmpt write flushes cache",      TEST_MODE_M, EX_LAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_04_fn, "mpt_cache.04 sfence.vma flushes cache",      TEST_MODE_M, EX_LAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_05_fn, "mpt_cache.05 stale cache without fence",     TEST_MODE_M, 0,      mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_06_fn, "mpt_cache.06 mfence.pa with operands flush", TEST_MODE_M, EX_SAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_07_fn, "mpt_cache.07 minval.pa flushes cache",       TEST_MODE_M, EX_SAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_08_fn, "mpt_cache.08 flush then re-access",          TEST_MODE_M, 0,      mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_09_fn, "mpt_cache.09 sdid change forces re-walk",    TEST_MODE_M, EX_LAF, mpt_cache_01_setup, mpt_teardown)
ADD_TEST_CASE_HOOKS(mpt_cache_10a_fn, "mpt_cache.10a v=0 leaf triggers fault",     TEST_MODE_M, EX_LAF, mpt_cache_10_setup, NULL)
ADD_TEST_CASE_HOOKS(mpt_cache_10b_fn, "mpt_cache.10b v=0 to v=1 no fence needed",   TEST_MODE_M, 0,      NULL,               mpt_teardown)

END_TEST_MODULE(mpt_cache, "MPT Caching & flushes")
