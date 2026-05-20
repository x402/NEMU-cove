/***************************************************************************************
* Test framework: trap handler, S-mode switch, serial output
***************************************************************************************/

#include "test_main.h"

#define SERIAL_PORT (*(volatile char *)0x310b0000)

int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;

volatile int got_trap = 0;
volatile int got_mcause = 0;
volatile uint64_t s_mode_return_pc = 0;

void putc(char c) { SERIAL_PORT = c; }

void puts(const char *s) {
    while (*s) putc(*s++);
}

void puthex(uint64_t v) {
    puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (v >> i) & 0xF;
        if (digit < 10) putc('0' + digit);
        else putc('a' + digit - 10);
    }
}

void report_pass(const char *name) {
    puts("[PASS] "); puts(name); putc('\n');
}

void report_fail(const char *name, const char *reason) {
    puts("[FAIL] "); puts(name); puts(" ("); puts(reason); puts(")\n");
}

void print_summary(void) {
    puts("\n========== Summary ==========\n");
    puts("Total:  "); puthex(total_tests); puts("\n");
    puts("Passed: "); puthex(passed_tests); puts("\n");
    puts("Failed: "); puthex(failed_tests); puts("\n");
    if (failed_tests == 0) puts("ALL TESTS PASSED\n");
    else puts("SOME TESTS FAILED\n");
    puts("=============================\n");
}

void trap_handler(void) {
    uint64_t mcause = read_csr(0x342);
    uint64_t mepc = read_csr(0x341);
    uint64_t mstatus = read_csr(0x300);
    int prev_mode = (mstatus >> 11) & 0x3;

    if (prev_mode == 0x1) {
        if ((mcause & MCAUSE_CODE_MASK) == 9) {
            got_trap = 0;
        } else {
            got_trap = 1;
            got_mcause = mcause & MCAUSE_CODE_MASK;
        }
        write_csr(0x341, s_mode_return_pc);
        mstatus = (mstatus & ~MSTATUS_MPP_MASK) | (MSTATUS_MPP_M << 11);
        write_csr(0x300, mstatus);
    } else {
        got_trap = 1;
        got_mcause = mcause & MCAUSE_CODE_MASK;
        if (!(mcause & (1ULL << 63))) {
            write_csr(0x341, mepc + 4);
        }
    }
}

static volatile uint64_t s_mode_saved_ra;
static volatile uint64_t s_mode_saved_sp;

void run_in_s_mode(void (*fn)(void)) {
    asm volatile (
        "la t0, s_mode_return_pc\n"
        "la t1, 1f\n"
        "sd t1, 0(t0)\n"
        "la t0, s_mode_saved_ra\n"
        "sd ra, 0(t0)\n"
        "la t0, s_mode_saved_sp\n"
        "sd sp, 0(t0)\n"
        "csrr t0, mstatus\n"
        "li t1, ~0x1800\n"
        "and t0, t0, t1\n"
        "li t1, 0x800\n"
        "or t0, t0, t1\n"
        "csrw mstatus, t0\n"
        "csrw mepc, %0\n"
        "mret\n"
        "1:\n"
        "la t0, s_mode_saved_ra\n"
        "ld ra, 0(t0)\n"
        "la t0, s_mode_saved_sp\n"
        "ld sp, 0(t0)\n"
        :
        : "r"(fn)
        : "t0", "t1", "memory"
    );
}

__attribute__((aligned(4096))) volatile uint64_t mpt_root[512];
__attribute__((aligned(4096))) volatile uint64_t mpt_l2[512];
__attribute__((aligned(4096))) volatile uint64_t mpt_leaf[512];

static inline uint64_t mpte_nonleaf(uint64_t next_pa) {
    return MPTE_V | ((next_pa >> 12) << 10);
}

static inline uint64_t mpte_leaf(uint8_t xwr) {
    uint64_t val = MPTE_V | MPTE_L;
    for (int pi = 0; pi < 16; pi++) {
        val |= ((uint64_t)(xwr & 0x7) << (8 + pi * 3));
    }
    return val;
}

void mpt_setup(uint8_t code_xwr, uint8_t test_xwr, uint8_t nox_xwr) {
    for (int i = 0; i < 512; i++) {
        mpt_root[i] = 0;
        mpt_l2[i] = 0;
        mpt_leaf[i] = 0;
    }
    mpt_root[0] = mpte_nonleaf((uint64_t)mpt_l2);
    for (int i = 0; i < 512; i++) {
        mpt_l2[i] = mpte_nonleaf((uint64_t)mpt_leaf);
    }
    for (int i = 0; i < 512; i++) {
        mpt_leaf[i] = mpte_leaf(XWR_RWX);
    }
    mpt_leaf[0] = mpte_leaf(code_xwr);
    mpt_leaf[16] = mpte_leaf(test_xwr);
    mpt_leaf[32] = mpte_leaf(nox_xwr);
    uint64_t mmpt_val = ((uint64_t)1 << 60) | ((uint64_t)1 << 52)
                      | (((uint64_t)mpt_root >> 12) & 0xFFFFFFFFFFFULL);
    write_csr(CSR_MMPT, mmpt_val);
}

void mpt_teardown(void) {
    write_csr(CSR_MMPT, 0);
}

void test_main(void) {
#ifdef TEST_MODULE_MMPT
    test_csr_mmpt();
#elif defined(TEST_MODULE_MSDCFG)
    test_csr_msdcfg();
#elif defined(TEST_MODULE_MPT)
    test_mpt_permissions();
#elif defined(TEST_MODULE_FENCE)
    test_fence_instr();
#elif defined(TEST_MODULE_CACHE)
    test_mpt_cache();
#elif defined(TEST_MODULE_BOUNDARY)
    test_boundary();
#else
    test_csr_mmpt();
    test_csr_msdcfg();
    test_mpt_permissions();
    test_fence_instr();
    test_mpt_cache();
    test_boundary();
#endif
    print_summary();
}
