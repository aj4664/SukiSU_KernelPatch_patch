/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Hook memory patching macros and utilities
 */

#ifndef _HOOK_PATCH_H_
#define _HOOK_PATCH_H_

#include <stdint.h>

#ifndef dsb
#define dsb(opt) asm volatile("dsb " #opt : : : "memory")
#endif

#ifndef isb
#define isb() asm volatile("isb" : : : "memory")
#endif

#ifndef dmb
#define dmb(opt) asm volatile("dmb " #opt : : : "memory")
#endif

#ifndef PTE_RDONLY
#define PTE_RDONLY      (1UL << 7)
#endif

#define SAFE_MEMORY_PATCH_BEGIN(va, entry, ori_prot) do { \
    entry = pgtable_entry_kernel(va); \
    ori_prot = *entry; \
    modify_entry_kernel(va, entry, (ori_prot | PTE_DBM) & ~PTE_RDONLY); \
    dsb(ish); \
} while(0)

#define SAFE_MEMORY_PATCH_END(va, entry, ori_prot) do { \
    dsb(ish); \
    flush_icache_all(); \
    modify_entry_kernel(va, entry, ori_prot); \
    dsb(ish); \
} while(0)

#define ATOMIC_PATCH_INSTRUCTION(addr, new_inst) do { \
    volatile uint32_t *target = (volatile uint32_t *)(addr); \
    dmb(ish); \
    *target = (new_inst); \
    dmb(ish); \
} while(0)

#define BATCH_PATCH_INSTRUCTIONS(base_addr, instructions, count) do { \
    volatile uint32_t *target = (volatile uint32_t *)(base_addr); \
    for (int32_t __i = 0; __i < (count); __i++) { \
        dmb(ish); \
        target[__i] = (instructions)[__i]; \
    } \
    dmb(ish); \
} while(0)

#define SAFE_PATCH_WITH_CHECK(va, instructions, count, error_label) do { \
    if (is_bad_address((void *)(va))) { \
        goto error_label; \
    } \
    uint64_t *__entry; \
    uint64_t __ori_prot; \
    SAFE_MEMORY_PATCH_BEGIN(va, __entry, __ori_prot); \
    BATCH_PATCH_INSTRUCTIONS(va, instructions, count); \
    SAFE_MEMORY_PATCH_END(va, __entry, __ori_prot); \
} while(0)

#ifdef CONFIG_SMP
#define CPU_STOP_MACHINE_BEGIN() \
    preempt_disable()

#define CPU_STOP_MACHINE_END() \
    preempt_enable()
#else
#define CPU_STOP_MACHINE_BEGIN() do {} while(0)
#define CPU_STOP_MACHINE_END() do {} while(0)
#endif

#define FLUSH_INSTRUCTION_CACHE() do { \
    dsb(ish); \
    flush_icache_all(); \
    isb(); \
} while(0)

#define FLUSH_TLB_PAGE(va) do { \
    flush_tlb_kernel_page(va); \
    dsb(ish); \
} while(0)

#endif /* _HOOK_PATCH_H_ */