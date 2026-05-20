#include "test_main.h"

static void s_mode_mfence_pa(void) {
    asm volatile (".word 0x1C000073");
    asm volatile ("ecall");
}

static void s_mode_minval_pa(void) {
    asm volatile (".word 0x1E000073");
    asm volatile ("ecall");
}

void test_fence_instr(void) {
    TEST("T4.1 m-mode mfence.pa ok", {
        asm volatile (".word 0x1C000073");
    });

    TEST("T4.2 m-mode mfence.pa with operands ok", {
        asm volatile ("li a0, 0x80000000\n"
                      "li a1, 1\n"
                      ".word 0x1CB50073\n");
    });

    STEST_TRAP("T4.3 s-mode mfence.pa fault", EX_II, {
        run_in_s_mode(s_mode_mfence_pa);
    });

    TEST("T4.4 m-mode minval.pa ok", {
        asm volatile (".word 0x1E000073");
    });

    STEST_TRAP("T4.5 s-mode minval.pa fault", EX_II, {
        run_in_s_mode(s_mode_minval_pa);
    });
}
