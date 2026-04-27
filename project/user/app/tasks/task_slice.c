/**
 ******************************************************************************
 * @file    task_slice
 * @author  sloop
 * @date    2025-04-24
 * @brief   slice 任务示例 - 第三方黑盒任务
 *
 * 展示如何使用 slice 功能将第三方任务集成到 sloop 框架中
 *****************************************************************************/

#include "common.h"

void task_slice(void)
{
    while (1)
    {
        sl_printf("slice run");
        sl_printf(" CONTROL=0x%02lx", __get_CONTROL());

        sl_delay(500);
        sl_slice_yield();
    }
}

/************************** END OF FILE **************************/