CROSS_COMPILE ?= riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS = -march=rv64gc -mabi=lp64d -mcmodel=medany -nostdlib -ffreestanding -O2 -Wall \
         -Icommon -fno-stack-protector -fno-asynchronous-unwind-tables
LDFLAGS = -T common/link.ld

COMMON_SRCS = common/start.S common/test_main.c

# Test modules
MMPT_SRCS = $(COMMON_SRCS) tests/test_csr_mmpt.c
MSDCFG_SRCS = $(COMMON_SRCS) tests/test_csr_msdcfg.c
MPT_SRCS = $(COMMON_SRCS) tests/test_mpt_permissions.c
FENCE_SRCS = $(COMMON_SRCS) tests/test_fence_instr.c
CACHE_SRCS = $(COMMON_SRCS) tests/test_mpt_cache.c
BOUNDARY_SRCS = $(COMMON_SRCS) tests/test_boundary.c

ALL_SRCS = $(COMMON_SRCS) \
           tests/test_csr_mmpt.c \
           tests/test_csr_msdcfg.c \
           tests/test_mpt_permissions.c \
           tests/test_fence_instr.c \
           tests/test_mpt_cache.c \
           tests/test_boundary.c

OUTDIR = build

.PHONY: all mmpt msdcfg mpt fence cache boundary clean

all: $(OUTDIR)/smmtt-all.bin

mmpt: $(OUTDIR)/smmtt-mmpt.bin

msdcfg: $(OUTDIR)/smmtt-msdcfg.bin

mpt: $(OUTDIR)/smmtt-mpt.bin

fence: $(OUTDIR)/smmtt-fence.bin

cache: $(OUTDIR)/smmtt-cache.bin

boundary: $(OUTDIR)/smmtt-boundary.bin

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(OUTDIR)/smmtt-all.bin: $(ALL_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) $(ALL_SRCS) $(LDFLAGS) -o $(OUTDIR)/smmtt-all.elf
	$(OBJCOPY) -O binary $(OUTDIR)/smmtt-all.elf $@

$(OUTDIR)/smmtt-mmpt.bin: $(MMPT_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) -DTEST_MODULE_MMPT $(MMPT_SRCS) $(LDFLAGS) -o $(OUTDIR)/smmtt-mmpt.elf
	$(OBJCOPY) -O binary $(OUTDIR)/smmtt-mmpt.elf $@

$(OUTDIR)/smmtt-msdcfg.bin: $(MSDCFG_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) -DTEST_MODULE_MSDCFG $(MSDCFG_SRCS) $(LDFLAGS) -o $(OUTDIR)/smmtt-msdcfg.elf
	$(OBJCOPY) -O binary $(OUTDIR)/smmtt-msdcfg.elf $@

$(OUTDIR)/smmtt-mpt.bin: $(MPT_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) -DTEST_MODULE_MPT $(MPT_SRCS) $(LDFLAGS) -o $(OUTDIR)/smmtt-mpt.elf
	$(OBJCOPY) -O binary $(OUTDIR)/smmtt-mpt.elf $@

$(OUTDIR)/smmtt-fence.bin: $(FENCE_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) -DTEST_MODULE_FENCE $(FENCE_SRCS) $(LDFLAGS) -o $(OUTDIR)/smmtt-fence.elf
	$(OBJCOPY) -O binary $(OUTDIR)/smmtt-fence.elf $@

$(OUTDIR)/smmtt-cache.bin: $(CACHE_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) -DTEST_MODULE_CACHE $(CACHE_SRCS) $(LDFLAGS) -o $(OUTDIR)/smmtt-cache.elf
	$(OBJCOPY) -O binary $(OUTDIR)/smmtt-cache.elf $@

$(OUTDIR)/smmtt-boundary.bin: $(BOUNDARY_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) -DTEST_MODULE_BOUNDARY $(BOUNDARY_SRCS) $(LDFLAGS) -o $(OUTDIR)/smmtt-boundary.elf
	$(OBJCOPY) -O binary $(OUTDIR)/smmtt-boundary.elf $@

clean:
	rm -rf $(OUTDIR)
