/***************************************************************************************
* CSR definitions for Smmtt extension testing
***************************************************************************************/

#ifndef __CSR_DEFS_H__
#define __CSR_DEFS_H__

#define CSR_MMPT    0x382
#define CSR_MSDCFG  0x74E
#define CSR_MSIDEIE 0x74F
#define CSR_MSIDEIP 0xF4F
#define CSR_MIP     0x344
#define CSR_MIE     0x304
#define CSR_MIDELEG 0x303
#define CSR_HIDELEG 0x603
#define CSR_SIE     0x104

#define MSTATUS_SPP (1ULL << 8)
#define MSTATUS_SPIE (1ULL << 5)
#define MSTATUS_MIE  (1ULL << 3)

#define MSTATUS_MPP_M  0x3
#define MSTATUS_MPP_S  0x1
#define MSTATUS_MPP_MASK (0x3ULL << 11)

#define MCAUSE_MASK     0x8000000000000000ULL
#define MCAUSE_CODE_MASK 0x7FFFFFFFFFFFFFFFULL

#define EX_II  2   // Illegal instruction
#define EX_LAF 5   // Load access fault
#define EX_SAF 7   // Store/AMO access fault
#define EX_IAF 1   // Instruction access fault

#define PGSIZE 4096
#define PGSHIFT 12

// mmpt field extraction
static inline uint64_t mmpt_mode(uint64_t val) { return (val >> 60) & 0xF; }
static inline uint64_t mmpt_sdid(uint64_t val) { return (val >> 52) & 0x3F; }
static inline uint64_t mmpt_ppn(uint64_t val)  { return val & 0xFFFFFFFFFFFULL; }

// msdcfg field extraction
static inline uint64_t msdcfg_sidn(uint64_t val) { return val & 0x3F; }
static inline uint64_t msdcfg_seda(uint64_t val) { return (val >> 6) & 1; }
static inline uint64_t msdcfg_seta(uint64_t val) { return (val >> 7) & 1; }

// MSDEI interrupt bit (bit 14 in mip/mie/sip/sie)
#define IRQ_MSDEI  14
#define MIP_MSDEIP (1ULL << IRQ_MSDEI)
#define MIE_MSDEIE (1ULL << IRQ_MSDEI)

// MPTE encodings
#define MPTE_V (1ULL << 0)
#define MPTE_L (1ULL << 1)
#define MPTE_N (1ULL << 2)

// NAPOT leaf: G field at bits [15:12]
#define NAPOT_G_SHIFT 12
#define NAPOT_G_4     4  // 2MiB

// XWR permissions
#define XWR_NONE  0
#define XWR_R     1
#define XWR_W     2
#define XWR_RW    3
#define XWR_X     4
#define XWR_RX    5
#define XWR_WX    6
#define XWR_RWX   7

// Inline CSR read/write
#define read_csr(csr) ({ uint64_t __v; asm volatile ("csrr %0, %1" : "=r"(__v) : "i"(csr)); __v; })
#define write_csr(csr, val) ({ asm volatile ("csrw %0, %1" :: "i"(csr), "r"(val)); })
#define set_csr(csr, val) ({ asm volatile ("csrs %0, %1" :: "i"(csr), "r"(val)); })
#define clear_csr(csr, val) ({ asm volatile ("csrc %0, %1" :: "i"(csr), "r"(val)); })

#endif // __CSR_DEFS_H__
