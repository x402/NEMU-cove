CROSS_COMPILE ?= riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS = -march=rv64gc -mabi=lp64d -mcmodel=medany -nostdlib -ffreestanding -O2 -Wall \
         -Icommon -fno-stack-protector -fno-asynchronous-unwind-tables
LDFLAGS = -T common/link.ld

COMMON_SRCS = common/start.S common/test_main.c
OUTDIR = build

# 1. Automatically find all test source files
TEST_SRCS = $(wildcard tests/test_*.c)

# 2. Extract module IDs (e.g., mmpt, msdcfg, mpt_perm, mfence, mpt_cache, mpt_invalid)
MODULES = $(patsubst tests/test_%.c,%,$(TEST_SRCS))

.PHONY: all clean $(MODULES)

all: $(OUTDIR)/smmtt-all.bin

# Macro rule for compiling individual modules
define make-module-target
$(1): $$(OUTDIR)/smmtt-$(1).bin

$$(OUTDIR)/smmtt-$(1).bin: tests/test_$(1).c $$(COMMON_SRCS) | $$(OUTDIR)
	$$(CC) $$(CFLAGS) $$(COMMON_SRCS) tests/test_$(1).c $$(LDFLAGS) -o $$(OUTDIR)/smmtt-$(1).elf
	$$(OBJCOPY) -O binary $$(OUTDIR)/smmtt-$(1).elf $$@
endef

$(OUTDIR):
	mkdir -p $(OUTDIR)

# Monolithic all-in-one suite target
$(OUTDIR)/smmtt-all.bin: $(TEST_SRCS) $(COMMON_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) $(COMMON_SRCS) $(TEST_SRCS) $(LDFLAGS) -o $(OUTDIR)/smmtt-all.elf
	$(OBJCOPY) -O binary $(OUTDIR)/smmtt-all.elf $@

# Automatically generate targets for all discovered modules!
$(foreach mod,$(MODULES),$(eval $(call make-module-target,$(mod))))

clean:
	rm -rf $(OUTDIR)
