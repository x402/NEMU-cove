/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "../local-include/mpt.h"
#include "../local-include/mpt-cache.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <cpu/cpu.h>
#include <debug.h>
#include <macro.h>
#include <memory/host.h>
#include <memory/paddr.h>
#include <string.h>

#ifdef CONFIG_RV_SMMTT

MPTCacheEntry mpt_cache[MPTC_SIZE];

void mptc_flush() {
  memset(mpt_cache, 0, sizeof(mpt_cache));
}

void mptc_init() {
  mptc_flush();
}

static void mpt_access_fault(int type, vaddr_t vaddr) {
  int cause;
  if (type == MEM_TYPE_IFETCH) {
    cause = EX_IAF;
  } else if (type == MEM_TYPE_WRITE) {
    cause = EX_SAF;
  } else {
    cause = EX_LAF;
  }
  cpu.trapInfo.tval = vaddr;
  longjmp_exception(cause);
}

bool isa_mpt_check_permission(paddr_t pa, int len, int type, int mode, vaddr_t vaddr) {
  if (mode == MODE_M) {
    return true;
  }

  uint64_t mmpt_mode = mmpt->mode;
  if (mmpt_mode == 0) {
    return true;
  }

  paddr_t cache_ppn = pa >> 12;
  int idx = cache_ppn % MPTC_SIZE;
  if (mpt_cache[idx].valid && mpt_cache[idx].ppn == cache_ppn && mpt_cache[idx].sdid == mmpt->sdid) {
    uint8_t xwr = mpt_cache[idx].xwr;
    bool permitted = false;
    if (type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ || type == MEM_TYPE_WRITE_READ) {
      permitted = (xwr & 0x1);
    } else if (type == MEM_TYPE_WRITE) {
      permitted = (xwr & 0x2);
    } else if (type == MEM_TYPE_IFETCH) {
      permitted = (xwr & 0x4);
    }
    if (!permitted) {
      mpt_access_fault(type, vaddr);
    }
    return true;
  }

  int levels;
  uint64_t max_pa_mask;

  switch (mmpt_mode) {
    case 1:
      levels = 3;
      max_pa_mask = ~((1ULL << 43) - 1);
      break;
    case 2:
      levels = 4;
      max_pa_mask = ~((1ULL << 52) - 1);
      break;
    case 3:
      levels = 5;
      max_pa_mask = 0;
      break;
    default:
      return true;
  }

  if (max_pa_mask && (pa & max_pa_mask)) {
    mpt_access_fault(type, vaddr);
  }

  paddr_t a = (paddr_t)mmpt->ppn << 12;
  int i = levels - 1;

  while (1) {
    uint64_t index;
    if (i == levels - 1) {
      int shift = 16 + (levels - 1) * 9;
      if (levels == 5) {
        index = BITS(pa, 63, 52);
      } else {
        index = BITS(pa, shift + 8, shift);
      }
    } else {
      int shift = 16 + i * 9;
      index = BITS(pa, shift + 8, shift);
    }

    paddr_t mpte_addr = a + index * 8;
    if (!in_pmem(mpte_addr)) {
      mpt_access_fault(type, vaddr);
    }
    word_t mpte = host_read(guest_to_host(mpte_addr), 8);

    if (!(mpte & 1)) {
      mpt_access_fault(type, vaddr);
    }

    bool is_leaf = (mpte >> 1) & 1;
    bool is_napot = (mpte >> 2) & 1;

    if (!is_leaf && is_napot) {
      mpt_access_fault(type, vaddr);
    }

    if (is_leaf) {
      uint8_t xwr;
      if (!is_napot) {
        if (BITS(mpte, 7, 3) != 0) {
          mpt_access_fault(type, vaddr);
        }
        if (BITS(mpte, 63, 56) != 0) {
          mpt_access_fault(type, vaddr);
        }

        int pi;
        if (i > 0) {
          int pn_shift = 16 + (i - 1) * 9;
          pi = BITS(pa, pn_shift + 8, pn_shift + 5);
        } else {
          pi = BITS(pa, 15, 12);
        }
        xwr = (mpte >> (8 + pi * 3)) & 0x7;
      } else {
        uint64_t g = BITS(mpte, 15, 12);
        if (g != 4) {
          mpt_access_fault(type, vaddr);
        }
        if (BITS(mpte, 11, 11) != 0) {
          mpt_access_fault(type, vaddr);
        }
        if (BITS(mpte, 63, 16) != 0) {
          mpt_access_fault(type, vaddr);
        }
        if (BITS(mpte, 7, 3) != 0) {
          mpt_access_fault(type, vaddr);
        }
        xwr = BITS(mpte, 10, 8);
      }

      bool permitted = false;
      if (type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ || type == MEM_TYPE_WRITE_READ) {
        permitted = (xwr & 0x1);
      } else if (type == MEM_TYPE_WRITE) {
        permitted = (xwr & 0x2);
      } else if (type == MEM_TYPE_IFETCH) {
        permitted = (xwr & 0x4);
      }

      if (!permitted) {
        mpt_access_fault(type, vaddr);
      }
      mpt_cache[idx].ppn = cache_ppn;
      mpt_cache[idx].sdid = mmpt->sdid;
      mpt_cache[idx].xwr = xwr;
      mpt_cache[idx].valid = 1;
      return true;
    } else {
      if (BITS(mpte, 63, 54) != 0) {
        mpt_access_fault(type, vaddr);
      }
      if (BITS(mpte, 9, 2) != 0) {
        mpt_access_fault(type, vaddr);
      }

      paddr_t ppn = BITS(mpte, 53, 10);
      a = ppn << 12;
      i--;
      if (i < 0) {
        mpt_access_fault(type, vaddr);
      }
    }
  }
}

#endif // CONFIG_RV_SMMTT
