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

#ifndef __MPT_CACHE_H__
#define __MPT_CACHE_H__

#include <common.h>
#include <string.h>

#ifdef CONFIG_RV_SMMTT

#define MPTC_SIZE_SHIFT 12
#define MPTC_SIZE (1 << MPTC_SIZE_SHIFT)

typedef struct {
  paddr_t ppn;    // physical page number (tag)
  uint32_t sdid;  // supervisor domain ID (tag)
  uint8_t xwr;    // cached XWR permissions (3 bits)
  uint8_t valid;  // entry is valid
} MPTCacheEntry;

extern MPTCacheEntry mpt_cache[MPTC_SIZE];

void mptc_flush();
void mptc_init();

#endif // CONFIG_RV_SMMTT

#endif // __MPT_CACHE_H__
