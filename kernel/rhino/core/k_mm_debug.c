/*
 * Copyright (C) 2016 YunOS Project. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>

#include <k_api.h>
#if (YUNOS_CONFIG_MM_BESTFIT > 0 || YUNOS_CONFIG_MM_FIRSTFIT > 0)
#include "k_mm_region.h"
#endif
#if (YUNOS_CONFIG_MM_TLF > 0)
#include "k_mm.h"
#endif
#include "k_mm_debug.h"
#include "yos/log.h"

#if (YUNOS_CONFIG_MM_DEBUG > 0)


extern klist_t g_mm_region_list_head;

#if (YUNOS_CONFIG_MM_LEAKCHECK > 0)


static mm_scan_region_t g_mm_scan_region[YOS_MM_SCAN_REGION_MAX];
static void           **g_leak_match;
static uint32_t         g_recheck_flag = 0;

static uint32_t check_malloc_region(void *adress);
uint32_t if_adress_is_valid(void *adress);
uint32_t dump_mmleak();

uint32_t yunos_mm_leak_region_init(void *start, void *end)
{
    static uint32_t i = 0;

    if (i >= YOS_MM_SCAN_REGION_MAX) {
        return -1;
    }

    if ((start == NULL) || (end == NULL)) {
        return -1;
    }

    g_mm_scan_region[i].start = start;
    g_mm_scan_region[i].end   = end;

    i++;
    return i;
}


static uint32_t check_task_stack(ktask_t *task, void **p)
{
    uint32_t offset = 0;
    kstat_t  rst    = YUNOS_SUCCESS;
    void    *start, *cur, *end;

    start = task->task_stack_base;
    end   = task->task_stack_base + task->stack_size;

    rst =  yunos_task_stack_cur_free(task, &offset);
    if (rst == YUNOS_SUCCESS) {
        cur = task->task_stack_base + task->stack_size - offset;
    } else {
        k_err_proc(YUNOS_SYS_SP_ERR);
        return 0;
    }

    if ((uint32_t)p >= cur &&
        (uint32_t)p  < (uint32_t)end) {
        return 1;
    }
    else if((uint32_t)p >= start && (uint32_t)p  < (uint32_t)cur) {
        return 0;
    }
    /*maybe lost*/
    return 1;
}

static uint32_t check_if_in_stack(void **p)
{
    klist_t *taskhead = &g_kobj_list.task_head;
    klist_t *taskend  = taskhead;
    klist_t *tmp;
    ktask_t *task;



    for (tmp = taskhead->next; tmp != taskend; tmp = tmp->next) {
        task = yunos_list_entry(tmp, ktask_t, task_stats_item);
        if (1 == check_task_stack(task, p)) {
            return 1;
        }
    }
    return 0;
}

uint32_t scan_region(void *start, void *end, void *adress)
{
    void **p = (void **)((uint32_t)start & ~(sizeof(size_t) - 1));
    while ((void *)p < end) {
        if (NULL != p && adress  == *p) {
            g_leak_match = p;
            return 1;
        }

        p++;
    }

    return 0;
}
uint32_t check_mm_leak(uint8_t *adress)
{
    uint32_t rst = 0;
    uint32_t i;

    for (i = 0; i < YOS_MM_SCAN_REGION_MAX; i++) {

        if ((NULL == g_mm_scan_region[i].start) || (NULL == g_mm_scan_region[i].end)) {
            continue;
        }

        if (1 == scan_region(g_mm_scan_region[i].start, g_mm_scan_region[i].end,
                             adress)) {
            return 1;
        }
    }

    rst = check_malloc_region(adress);
    if (1 == rst) {
        return 1;
    }

    return 0;
}

static uint32_t recheck(void *start, void *end)
{
    void **p    = (void **)((uint32_t)start & ~(sizeof(size_t) - 1));

    g_recheck_flag = 1;

    while ((void *)p <= end) {
        if (NULL != p && 1 == if_adress_is_valid(*p)) {
            if ( 1 == check_mm_leak(*p)) {
                g_recheck_flag = 0;
                return 1;
            }
        }
        p++;
    }

    g_recheck_flag = 0;

    return 0;
}
#if (YUNOS_CONFIG_MM_TLF > 0)
uint32_t check_malloc_region(void *adress)
{
    uint32_t            rst = 0;
    k_mm_region_info_t *reginfo, *nextreg;
    k_mm_list_t *next, *cur;

    NULL_PARA_CHK(g_kmm_head);

    reginfo = g_kmm_head->regioninfo;
    while (reginfo) {
        VGF(VALGRIND_MAKE_MEM_DEFINED(reginfo, sizeof(k_mm_region_info_t)));
        cur = (k_mm_list_t *) ((char *) reginfo - MMLIST_HEAD_SIZE);
        /*jump first blk*/
        cur = NEXT_MM_BLK(cur->mbinfo.buffer, cur->size & YUNOS_MM_BLKSIZE_MASK);
        while (cur) {
            VGF(VALGRIND_MAKE_MEM_DEFINED(cur, MMLIST_HEAD_SIZE));
            if ((cur->size & YUNOS_MM_BLKSIZE_MASK)) {
                next = NEXT_MM_BLK(cur->mbinfo.buffer, cur->size & YUNOS_MM_BLKSIZE_MASK);
                if (0 == g_recheck_flag && !(cur->size & YUNOS_MM_FREE)) {
                    if (yunos_cur_task_get()->task_stack_base >= cur->mbinfo.buffer
                        && yunos_cur_task_get()->task_stack_base < next) {
                            cur = next;
                            continue;
                        }
                    rst = scan_region(cur->mbinfo.buffer,(void*) next, adress);
                    if (1 == rst) {
                        VGF(VALGRIND_MAKE_MEM_NOACCESS(cur, MMLIST_HEAD_SIZE));
                        VGF(VALGRIND_MAKE_MEM_NOACCESS(reginfo, sizeof(k_mm_region_info_t)));
                        return check_if_in_stack(g_leak_match);
                    }
                }
            } else {
                next = NULL;
            }
            if (1 == g_recheck_flag &&
                (uint32_t)adress >= (uint32_t)cur->mbinfo.buffer &&
                (uint32_t)adress < (uint32_t)next) {
                VGF(VALGRIND_MAKE_MEM_NOACCESS(cur, MMLIST_HEAD_SIZE));
                VGF(VALGRIND_MAKE_MEM_NOACCESS(reginfo, sizeof(k_mm_region_info_t)));
                return 1;
            }
            VGF(VALGRIND_MAKE_MEM_NOACCESS(cur, MMLIST_HEAD_SIZE));
            cur = next;
        }
        nextreg = reginfo->next;
        VGF(VALGRIND_MAKE_MEM_NOACCESS(reginfo, sizeof(k_mm_region_info_t)));
        reginfo = nextreg;
    }

    return 0;
}



uint32_t if_adress_is_valid(void *adress)
{
    uint32_t            rst = 0;
    k_mm_region_info_t *reginfo, *nextreg;
    k_mm_list_t *next, *cur;

    reginfo = g_kmm_head->regioninfo;
    while (reginfo) {
        VGF(VALGRIND_MAKE_MEM_DEFINED(reginfo, sizeof(k_mm_region_info_t)));
        cur = (k_mm_list_t *) ((char *) reginfo - MMLIST_HEAD_SIZE);
        /*jump first blk*/
        cur = NEXT_MM_BLK(cur->mbinfo.buffer, cur->size & YUNOS_MM_BLKSIZE_MASK);
        while (cur) {
            VGF(VALGRIND_MAKE_MEM_DEFINED(cur, MMLIST_HEAD_SIZE));
            if ((cur->size & YUNOS_MM_BLKSIZE_MASK)) {
                next = NEXT_MM_BLK(cur->mbinfo.buffer, cur->size & YUNOS_MM_BLKSIZE_MASK);
                if (!(cur->size & YUNOS_MM_FREE) &&
                     (uint32_t)adress >= (uint32_t)cur->mbinfo.buffer && (uint32_t)adress < next ) {
                    VGF(VALGRIND_MAKE_MEM_NOACCESS(cur, MMLIST_HEAD_SIZE));
                    VGF(VALGRIND_MAKE_MEM_NOACCESS(reginfo, sizeof(k_mm_region_info_t)));
                    return 1;
                }

            } else {
                next = NULL;
            }
            VGF(VALGRIND_MAKE_MEM_NOACCESS(cur, MMLIST_HEAD_SIZE));
            cur = next;
        }
        nextreg = reginfo->next;
        VGF(VALGRIND_MAKE_MEM_NOACCESS(reginfo, sizeof(k_mm_region_info_t)));
        reginfo = nextreg;
    }

    return 0;
}


uint32_t dump_mmleak()
{
    uint32_t            rst = 0;
    k_mm_region_info_t *reginfo, *nextreg;
    k_mm_list_t *next, *cur;

    yunos_sched_disable();

    reginfo = g_kmm_head->regioninfo;
    while (reginfo) {
        VGF(VALGRIND_MAKE_MEM_DEFINED(reginfo, sizeof(k_mm_region_info_t)));
        cur = (k_mm_list_t *) ((char *) reginfo - MMLIST_HEAD_SIZE);
        /*jump first blk*/
        cur = NEXT_MM_BLK(cur->mbinfo.buffer, cur->size & YUNOS_MM_BLKSIZE_MASK);
        while (cur) {
            VGF(VALGRIND_MAKE_MEM_DEFINED(cur, MMLIST_HEAD_SIZE));
            if ((cur->size & YUNOS_MM_BLKSIZE_MASK)) {
                next = NEXT_MM_BLK(cur->mbinfo.buffer, cur->size & YUNOS_MM_BLKSIZE_MASK);
                 if (!(cur->size & YUNOS_MM_FREE) &&
                     0 == check_mm_leak(cur->mbinfo.buffer)
                     && 0 == recheck((void*)cur->mbinfo.buffer , (void*)next)) {
                     printf("adress:0x%0x owner:0x%0x len:%-5d type:%s\r\n",
                            (void*)cur->mbinfo.buffer, cur->owner,
                            cur->size&YUNOS_MM_BLKSIZE_MASK, "leak");
                 }

            } else {
                next = NULL;
            }
            VGF(VALGRIND_MAKE_MEM_NOACCESS(cur, MMLIST_HEAD_SIZE));
            cur = next;
        }
        nextreg = reginfo->next;
        VGF(VALGRIND_MAKE_MEM_NOACCESS(reginfo, sizeof(k_mm_region_info_t)));
        reginfo = nextreg;
    }

    yunos_sched_enable();
    return 0;
}
#endif


#if (YUNOS_CONFIG_MM_BESTFIT > 0 || YUNOS_CONFIG_MM_FIRSTFIT > 0)
static uint32_t check_malloc_region(void *adress)
{
    uint32_t            rst = 0;
    void               *start  = NULL;
    k_mm_region_list_t *tmp = NULL;
    klist_t            *head;
    klist_t            *end;
    klist_t            *cur;
    klist_t            *region_list_cur;
    klist_t            *region_list_head;
    k_mm_region_head_t *cur_region;

    NULL_PARA_CHK(g_mm_region_list_head.next);
    NULL_PARA_CHK(g_mm_region_list_head.prev);

    region_list_head = &g_mm_region_list_head;

    for (region_list_cur = region_list_head->next;
         region_list_cur != region_list_head; region_list_cur = region_list_cur->next) {
        cur_region = yunos_list_entry(region_list_cur, k_mm_region_head_t, regionlist);
        head = &cur_region->alloced;
        end = head;

        for (cur = head->next; cur != end; cur = cur->next) {
            tmp = yunos_list_entry(cur, k_mm_region_list_t, list);
            start  = (void *)((uint32_t)tmp + sizeof(k_mm_region_list_t));
            if (0 == g_recheck_flag && YUNOS_MM_REGION_ALLOCED == tmp->type) {
                rst = scan_region(start, (void *)((uint32_t)start + tmp->len), adress);
                if (1 == rst) {
                    return check_if_in_stack(g_leak_match);
                }
            }

            if (1 == g_recheck_flag &&
                (uint32_t)adress >= (uint32_t)start &&
                (uint32_t)adress < (uint32_t)start + tmp->len) {
                return 1;
            }
        }
    }
    return 0;
}



uint32_t if_adress_is_valid(void *adress)
{
    void               *start  = NULL;
    k_mm_region_list_t *tmp = NULL;
    klist_t            *head;
    klist_t            *end;
    klist_t            *cur;
    klist_t            *region_list_cur;
    klist_t            *region_list_head;
    k_mm_region_head_t *cur_region;

    NULL_PARA_CHK(g_mm_region_list_head.next);
    NULL_PARA_CHK(g_mm_region_list_head.prev);

    region_list_head =  &g_mm_region_list_head;

    for (region_list_cur = region_list_head->next;
         region_list_cur != region_list_head; region_list_cur = region_list_cur->next) {
        cur_region = yunos_list_entry(region_list_cur, k_mm_region_head_t, regionlist);
        head = &cur_region->alloced;
        end = head;

        for (cur = head->next; cur != end; cur = cur->next) {
            tmp = yunos_list_entry(cur, k_mm_region_list_t, list);
            start  = (void *)((uint32_t)tmp + sizeof(k_mm_region_list_t));
            if ((uint32_t)adress >= (uint32_t)start &&
                (uint32_t)adress < (uint32_t)start + tmp->len) {
                return 1;
            }
        }
    }
    return 0;
}


uint32_t dump_mmleak()
{
    uint32_t            i    = 0;
    void               *start  = NULL;
    k_mm_region_list_t *min  = NULL;
    klist_t            *head = NULL;
    klist_t            *end  = NULL;
    klist_t            *cur;
    klist_t            *region_list_cur;
    klist_t            *region_list_head;
    k_mm_region_head_t *cur_region;

    yunos_sched_disable();

    NULL_PARA_CHK(g_mm_region_list_head.next);
    NULL_PARA_CHK(g_mm_region_list_head.prev);

    region_list_head = &g_mm_region_list_head;

    VGF(VALGRIND_MAKE_MEM_DEFINED(region_list_head, sizeof(k_mm_region_head_t)));

    for (region_list_cur = region_list_head->next;
         region_list_cur != region_list_head;) {
        cur_region = yunos_list_entry(region_list_cur, k_mm_region_head_t, regionlist);

        VGF(VALGRIND_MAKE_MEM_DEFINED(cur_region, sizeof(k_mm_region_head_t)));
        start  = (void *)((uint32_t)tmp + sizeof(k_mm_region_list_t));

        head = &cur_region->alloced;
        end = head;

        if (1 == is_klist_empty(head)) {
            yunos_sched_enable();
            return 0;
        }

        for (cur = head->next; cur != end;) {
            min = yunos_list_entry(cur, k_mm_region_list_t, list);

            VGF(VALGRIND_MAKE_MEM_DEFINED(min, sizeof(k_mm_region_list_t)));

            if (YUNOS_MM_REGION_ALLOCED ==  min->type &&
                0 == check_mm_leak((uint8_t *)(min + 1))
                && 0 == recheck(start,(uint32_t)start + min->len)) {
#if (YUNOS_CONFIG_MM_DEBUG > 0)
                printf("[%-4d]:adress:0x%0x owner:0x%0x len:%-5d type:%s\r\n", i,
                       (uint32_t)min + sizeof(k_mm_region_list_t), min->owner,  min->len, "leak");
#else
                printf("[%-4d]:adress:0x%0x  len:%-5d type:%s\r\n", i,
                       (uint32_t)min + sizeof(k_mm_region_list_t), min->len, "leak");
#endif
            }
            i++;
            cur = cur->next;
            VGF(VALGRIND_MAKE_MEM_NOACCESS(min, sizeof(k_mm_region_list_t)));

        }

        region_list_cur = region_list_cur->next;
        VGF(VALGRIND_MAKE_MEM_NOACCESS(cur_region, sizeof(k_mm_region_head_t)));

    }
    yunos_sched_enable();

    return 1;
}

uint32_t dumpsys_mm_info_func(char *buf, uint32_t len)
{
    (void)buf;
    (void)len;
    uint32_t i = 0;
    k_mm_region_list_t *min  = NULL;
    klist_t *head = NULL;
    klist_t *end  = NULL;
    klist_t *cur  = NULL;
    klist_t *region_list_cur = NULL , *region_list_head = NULL;
    k_mm_region_head_t *cur_region;

    NULL_PARA_CHK(g_mm_region_list_head.next);
    NULL_PARA_CHK(g_mm_region_list_head.prev);

    region_list_head =  &g_mm_region_list_head;

    VGF(VALGRIND_MAKE_MEM_DEFINED(region_list_head, sizeof(k_mm_region_head_t)));

    for (region_list_cur = region_list_head->next;
         region_list_cur != region_list_head;) {
        cur_region = yunos_list_entry(region_list_cur, k_mm_region_head_t, regionlist);

        VGF(VALGRIND_MAKE_MEM_DEFINED(cur_region, sizeof(k_mm_region_head_t)));

        printf("----------------------------------------------------------------------\r\n");
        printf("region info frag number:%d free size:%d\r\n", cur_region->frag_num,
               cur_region->freesize);

        head = &cur_region->probe;
        end = head;

        if (1 == is_klist_empty(head)) {
            printf("the memory list is empty\r\n");
            continue;
        }

        printf("free list: \r\n");

        for (cur = head->next; cur != end; ) {
            min = yunos_list_entry(cur, k_mm_region_list_t, list);

            VGF(VALGRIND_MAKE_MEM_DEFINED(min, sizeof(k_mm_region_list_t)));

#if (YUNOS_CONFIG_MM_DEBUG > 0)
            printf("[%-4d]:adress:0x%0x                  len:%-5d type:%s  flag:0x%0x\r\n",
                   i,
                   (uint32_t)min + sizeof(k_mm_region_list_t), min->len, "free", min->dye);
#else
            printf("[%-4d]:adress:0x%0x                  len:%-5d type:%s\r\n", i,
                   (uint32_t)min + sizeof(k_mm_region_list_t), min->len, "free");
#endif
            i++;
            cur = cur->next;

            VGF(VALGRIND_MAKE_MEM_NOACCESS(min, sizeof(k_mm_region_list_t)));

        }
        i = 0;
        head = &cur_region->alloced;
        end = head;
        printf("alloced list: \r\n");
        for (cur = head->next; cur != end;) {
            min = yunos_list_entry(cur, k_mm_region_list_t, list);

            VGF(VALGRIND_MAKE_MEM_DEFINED(min, sizeof(k_mm_region_list_t)));

#if (YUNOS_CONFIG_MM_DEBUG > 0)
            if ((YUNOS_MM_REGION_CORRUPT_DYE & min->dye) != YUNOS_MM_REGION_CORRUPT_DYE) {
                printf("[%-4d]:adress:0x%0x owner:0x%0x len:%-5d type:%s flag:0x%0x\r\n", i,
                       (uint32_t)min + sizeof(k_mm_region_list_t), min->owner,  min->len, "corrupt",
                       min->dye);
            }
            printf("[%-4d]:adress:0x%0x owner:0x%0x len:%-5d type:%s flag:0x%0x\r\n", i,
                   (uint32_t)min + sizeof(k_mm_region_list_t), min->owner,  min->len, "taken",
                   min->dye);
#else
            printf("[%-4d]:adress:0x%0x len:%-5d type:%s\r\n", i,
                   (uint32_t)min + sizeof(k_mm_region_list_t), min->len, "taken");

#endif
            i++;
            cur = cur->next;

            VGF(VALGRIND_MAKE_MEM_NOACCESS(min, sizeof(k_mm_region_list_t)));

        }

        region_list_cur = region_list_cur->next;

        VGF(VALGRIND_MAKE_MEM_NOACCESS(cur_region, sizeof(k_mm_region_head_t)));

    }
    return YUNOS_SUCCESS;
}
#endif
#endif

#if (YUNOS_CONFIG_MM_TLF > 0)

void print_block(k_mm_list_t *b)
{
    if (!b) {
        return;
    }
    printf("%p ", b);
    if (b->size & YUNOS_MM_FREE) {

#if (YUNOS_CONFIG_MM_DEBUG > 0u)
        if(b->dye != YUNOS_MM_FREE_DYE){
            printf("!");
        }
        else{
            printf(" ");
        }
#endif
        printf("free ");
    } else {
#if (YUNOS_CONFIG_MM_DEBUG > 0u)
        if(b->dye != YUNOS_MM_CORRUPT_DYE){
            printf("!");
        }
        else{
            printf(" ");
        }
#endif
        printf("used ");
    }
    if ((b->size & YUNOS_MM_BLKSIZE_MASK)) {
        printf(" %6lu ", (unsigned long) (b->size & YUNOS_MM_BLKSIZE_MASK));
    } else {
        printf(" sentinel ");
    }

#if (YUNOS_CONFIG_MM_DEBUG > 0u)
    printf(" %8x ",b->dye);
    printf(" 0x%-8x ",b->owner);
#endif

    if (b->size & YUNOS_MM_PREVFREE) {
        printf("pre-free [%8p];", b->prev);
    }
    else {
        printf("pre-used;");
    }

    if (b->size & YUNOS_MM_FREE) {
        VGF(VALGRIND_MAKE_MEM_DEFINED(&b->mbinfo, sizeof(struct free_ptr_struct)));
        printf(" free[%8p,%8p] ", b->mbinfo.free_ptr.prev, b->mbinfo.free_ptr.next);
        VGF(VALGRIND_MAKE_MEM_NOACCESS(&b->mbinfo, sizeof(struct free_ptr_struct)));
    }
    printf("\r\n");

}

void dump_kmm_free_map(k_mm_head *mmhead)
{
    k_mm_list_t *next, *tmp;
    int         i, j;

    if (!mmhead) {
        return;
    }

    printf("address,  stat   size     dye     caller   pre-stat    point\r\n");

    printf("FL bitmap: 0x%x\r\n", (unsigned) mmhead->fl_bitmap);

    for (i = 0; i < FLT_SIZE; i++) {
        if (mmhead->sl_bitmap[i]) {
            printf("SL bitmap 0x%x\r\n", (unsigned) mmhead->sl_bitmap[i]);
        }

        for (j = 0; j < SLT_SIZE; j++) {
            next = mmhead->mm_tbl[i][j];
            if (next) {
                printf("-> [%d][%d]\r\n", i, j);
            }
            while (next) {
                VGF(VALGRIND_MAKE_MEM_DEFINED(next, MMLIST_HEAD_SIZE));
                print_block(next);
                VGF(VALGRIND_MAKE_MEM_DEFINED(&next->mbinfo, sizeof(struct free_ptr_struct)));
                tmp = next->mbinfo.free_ptr.next;
                VGF(VALGRIND_MAKE_MEM_NOACCESS(&next->mbinfo, sizeof(struct free_ptr_struct)));
                VGF(VALGRIND_MAKE_MEM_NOACCESS(next, MMLIST_HEAD_SIZE));
                next = tmp;
            }
        }
    }
}

void dump_kmm_map(k_mm_head *mmhead)
{
    k_mm_region_info_t *reginfo, *nextreg;
    k_mm_list_t *next, *cur;

    if (!mmhead) {
        return;
    }

    printf("ALL BLOCKS\r\n");
    printf("address,  stat   size     dye     caller   pre-stat    point\r\n");
    reginfo = mmhead->regioninfo;
    while (reginfo) {
        VGF(VALGRIND_MAKE_MEM_DEFINED(reginfo, sizeof(k_mm_region_info_t)));
        cur = (k_mm_list_t *) ((char *) reginfo - MMLIST_HEAD_SIZE);
        while (cur) {
            VGF(VALGRIND_MAKE_MEM_DEFINED(cur, MMLIST_HEAD_SIZE));
            print_block(cur);
            if ((cur->size & YUNOS_MM_BLKSIZE_MASK)) {
                next = NEXT_MM_BLK(cur->mbinfo.buffer, cur->size & YUNOS_MM_BLKSIZE_MASK);
            } else {
                next = NULL;
            }
            VGF(VALGRIND_MAKE_MEM_NOACCESS(cur, MMLIST_HEAD_SIZE));
            cur = next;
        }
        nextreg = reginfo->next;
        VGF(VALGRIND_MAKE_MEM_NOACCESS(reginfo, sizeof(k_mm_region_info_t)));
        reginfo = nextreg;
    }
}

void dump_kmm_statistic_info(k_mm_head *mmhead)
{
    int i = 0;

    if (!mmhead) {
        return;
    }
#if (K_MM_STATISTIC > 0)
    printf("     free     |     used     |     maxused\r\n");
    printf("  %10d  |  %10d  |  %10d\r\n", mmhead->free_size, mmhead->used_size,
           mmhead->maxused_size);
    printf("\r\n-----------------alloc size statistic:-----------------\r\n");
    for (i = 0; i < MAX_MM_BIT - 1; i++) {
        if (i % 4 == 0 && i != 0) {
            printf("\r\n");
        }
        printf("[2^%02d] bytes: %5d   |", (i + 2), mmhead->mm_size_stats[i]);
    }
    printf("\r\n");
#endif
}

uint32_t dumpsys_mm_info_func(char *buf, uint32_t len)
{
    CPSR_ALLOC();

    YUNOS_CRITICAL_ENTER();

    VGF(VALGRIND_MAKE_MEM_DEFINED(g_kmm_head, sizeof(k_mm_head)));
    printf("\r\n------------------------------- all memory blocks --------------------------------- \r\n");
    printf("g_kmm_head = %8x\r\n",(unsigned int)g_kmm_head);

    dump_kmm_map(g_kmm_head);
    printf("\r\n----------------------------- all free memory blocks ------------------------------- \r\n");
    dump_kmm_free_map(g_kmm_head);
    printf("\r\n------------------------- memory allocation statistic ------------------------------ \r\n");
    dump_kmm_statistic_info(g_kmm_head);
    VGF(VALGRIND_MAKE_MEM_NOACCESS(g_kmm_head, sizeof(k_mm_head)));

    YUNOS_CRITICAL_EXIT();

    return YUNOS_SUCCESS;
}


#endif


#endif

