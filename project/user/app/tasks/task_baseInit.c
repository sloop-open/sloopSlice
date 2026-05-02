/**
 ******************************************************************************
 * @file    task_baseInit
 * @author  sloop
 * @date    2025-1-13
 * @brief   基础初始化
 *****************************************************************************/

#include "common.h"
#include "sl_slice.h"

/* 基础驱动初始化 */
void task_baseInit(void)
{
    /* 初次进入任务时，执行一次 */
    SL_INIT;

    /* 启动 slice 系统 */
    sl_slice_start(50, task_slice);

    sl_goto(task_idle);

    /* 任务结束，不再执行时，释放资源 */
    SL_FREE;

    /* 下方开始进入任务运行逻辑 */
    SL_RUN;
}

/************************** END OF FILE **************************/
