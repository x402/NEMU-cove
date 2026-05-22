// Description: MFENCE.PA & MINVAL.PA invalidation tests
#include "test_main.h"

static void s_mode_mfence_pa(void) {
    asm volatile (".word 0x1C000073");
    asm volatile ("ecall");
}

static void s_mode_minval_pa(void) {
    asm volatile (".word 0x1E000073");
    asm volatile ("ecall");
}

static void mfence_01_fn(void) {
    asm volatile (".word 0x1C000073");
}

static void mfence_02_fn(void) {
    asm volatile ("li a0, 0x80000000\n"
                  "li a1, 1\n"
                  ".word 0x1CB50073\n");
}

static void mfence_04_fn(void) {
    asm volatile (".word 0x1E000073");
}

BEGIN_TEST_MODULE(mfence)

ADD_TEST_CASE(mfence_01_fn, "mfence.01 m-mode mfence.pa ok", TEST_MODE_M, 0)
ADD_TEST_CASE(mfence_02_fn, "mfence.02 m-mode mfence.pa with operands ok", TEST_MODE_M, 0)
ADD_TEST_CASE(s_mode_mfence_pa, "mfence.03 s-mode mfence.pa fault", TEST_MODE_S, EX_II)
ADD_TEST_CASE(mfence_04_fn, "mfence.04 m-mode minval.pa ok", TEST_MODE_M, 0)
ADD_TEST_CASE(s_mode_minval_pa, "mfence.05 s-mode minval.pa fault", TEST_MODE_S, EX_II)

END_TEST_MODULE(mfence, "MFENCE.PA & MINVAL.PA")
