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

#include <k_api.h>

#if (YUNOS_CONFIG_HW_COUNT > 0)
void soc_hw_timer_init(void)
{
}

hr_timer_t soc_hr_hw_cnt_get(void)
{
    return 0;
}

lr_timer_t soc_lr_hw_cnt_get(void)
{
    return 0;
}
#endif /* YUNOS_CONFIG_HW_COUNT */

#if (YUNOS_CONFIG_INTRPT_GUARD > 0)
void soc_intrpt_guard(void)
{
}
#endif

#if (YUNOS_CONFIG_INTRPT_STACK_REMAIN_GET > 0)
size_t soc_intrpt_stack_remain_get(void)
{
    return 0;
}
#endif

#if (YUNOS_CONFIG_INTRPT_STACK_OVF_CHECK > 0)
void soc_intrpt_stack_ovf_check(void)
{
}
#endif

#if (YUNOS_CONFIG_DYNTICKLESS > 0)
void soc_tick_interrupt_set(tick_t next_ticks,tick_t elapsed_ticks)
{
}

tick_t soc_elapsed_ticks_get(void)
{
    return 0;
}
#endif

#if (YUNOS_CONFIG_KOBJ_DYN_ALLOC > 0)

#define       SYS_DYN_POOL_SIZE (80 * 1024)
size_t        sys_pool_start[SYS_DYN_POOL_SIZE / sizeof(size_t)];
k_mm_region_t g_mm_region[] = {{(uint8_t*)&sys_pool_start, SYS_DYN_POOL_SIZE}};

#endif

void soc_err_proc(kstat_t err)
{
    (void)err;
}

yunos_err_proc_t g_err_proc = soc_err_proc;

