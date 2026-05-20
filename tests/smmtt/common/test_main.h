/***************************************************************************************
* Test framework header for Smmtt extension testing
***************************************************************************************/

#ifndef __TEST_MAIN_H__
#define __TEST_MAIN_H__

#include <stdint.h>
#include <stddef.h>
#include "csr_defs.h"

extern int total_tests;
extern int passed_tests;
extern int failed_tests;

extern volatile int got_trap;
extern volatile int got_mcause;
extern volatile uint64_t s_mode_return_pc;

void trap_handler(void);
void run_in_s_mode(void (*fn)(void));

void report_pass(const char *name);
void report_fail(const char *name, const char *reason);
void print_summary(void);

void putc(char c);
void puts(const char *s);
void puthex(uint64_t v);

#define TEST(name, body) do { \
    total_tests++; \
    got_trap = 0; \
    got_mcause = 0; \
    body; \
    if (!got_trap) { \
        report_pass(name); \
        passed_tests++; \
    } else { \
        report_fail(name, "unexpected trap"); \
        failed_tests++; \
    } \
} while (0)

#define TEST_TRAP(name, expected_cause, body) do { \
    total_tests++; \
    got_trap = 0; \
    got_mcause = 0; \
    body; \
    if (got_trap && got_mcause == (expected_cause)) { \
        report_pass(name); \
        passed_tests++; \
    } else { \
        report_fail(name, "trap mismatch"); \
        failed_tests++; \
    } \
} while (0)

#define STEST(name, body) do { \
    total_tests++; \
    got_trap = 0; \
    got_mcause = 0; \
    body; \
    if (!got_trap) { \
        report_pass(name); \
        passed_tests++; \
    } else { \
        report_fail(name, "unexpected S-mode trap"); \
        failed_tests++; \
    } \
} while (0)

#define STEST_TRAP(name, expected_cause, body) do { \
    total_tests++; \
    got_trap = 0; \
    got_mcause = 0; \
    body; \
    if (got_trap && got_mcause == (expected_cause)) { \
        report_pass(name); \
        passed_tests++; \
    } else { \
        report_fail(name, "S-mode trap mismatch"); \
        failed_tests++; \
    } \
} while (0)

#define ASSERT(cond) do { if (!(cond)) { \
    report_fail("assertion", #cond); \
    failed_tests++; \
    total_tests++; \
    return; \
} } while (0)

void mpt_setup(uint8_t code_xwr, uint8_t test_xwr, uint8_t nox_xwr);
void mpt_teardown(void);

extern volatile uint64_t mpt_root[];
extern volatile uint64_t mpt_l2[];
extern volatile uint64_t mpt_leaf[];

void test_csr_mmpt(void);
void test_csr_msdcfg(void);
void test_mpt_permissions(void);
void test_fence_instr(void);
void test_mpt_cache(void);
void test_boundary(void);

void test_main(void);

#endif // __TEST_MAIN_H__
