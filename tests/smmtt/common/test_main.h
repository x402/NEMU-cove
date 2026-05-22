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

/* Modularity & Table-driven Declarations */

typedef enum {
    TEST_MODE_M = 0,
    TEST_MODE_S = 1
} test_priv_mode_t;

typedef struct {
    const char *name;             // E.g., "mpt_perm.01 bare mode allows access"
    void (*fn)(void);            // Test core function
    test_priv_mode_t run_mode;   // Mode to run in (M/S)
    int expected_trap;           // Expected mcause (0 or negative if none)
    void (*setup)(void);         // Setup hook (optional, can be NULL)
    void (*teardown)(void);      // Teardown hook (optional, can be NULL)
} test_case_t;

typedef struct {
    const char *name;            // E.g., "mpt_perm"
    const char *description;     // Human-readable description
    const test_case_t *cases;    // Array of test cases
    size_t num_cases;            // Total cases count
} test_module_t;

/* DSL test-suite declaration & auto-registration macros */

#define BEGIN_TEST_MODULE(mod_id) \
    static const test_case_t mod_id##_cases[] = {

#define ADD_TEST_CASE(test_fn, tc_name, priv_mode, exp_trap) \
    { .name = tc_name, .fn = test_fn, .run_mode = priv_mode, .expected_trap = exp_trap, .setup = NULL, .teardown = NULL },

#define ADD_TEST_CASE_HOOKS(test_fn, tc_name, priv_mode, exp_trap, setup_fn, teardown_fn) \
    { .name = tc_name, .fn = test_fn, .run_mode = priv_mode, .expected_trap = exp_trap, .setup = setup_fn, .teardown = teardown_fn },

#define END_TEST_MODULE(mod_id, desc_str) \
    }; \
    const test_module_t mod_id##_module = { \
        .name = #mod_id, \
        .description = desc_str, \
        .cases = mod_id##_cases, \
        .num_cases = sizeof(mod_id##_cases) / sizeof(test_case_t) \
    }; \
    const test_module_t * const __test_module_ptr_##mod_id \
    __attribute__((used, section(".test_registry"))) = &mod_id##_module;

void run_test_case(const test_case_t *tc);
void test_main(void);

#endif // __TEST_MAIN_H__
