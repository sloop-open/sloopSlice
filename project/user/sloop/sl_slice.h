/**
 ******************************************************************************
 * @file    sl_slice
 * @author  sloop
 * @date    2025-04-24
 * @brief   协作式任务切片模块 - 支持第三方黑盒任务时间片调度
 * 
 * 使用 PendSV 异常实现类似 RTOS 的线程切换
 * - root 线程：sloop 主逻辑
 * - slice 线程：第三方黑盒任务
 *****************************************************************************/

#ifndef __sl_slice_H
#define __sl_slice_H

#include "sl_common.h"

/* 启动 slice 任务, 时间片 us 微秒（至少 50us）, 任务入口 slice */
void sl_slice_start(int us, pfunc slice);

/* 停止 slice 系统 */
void sl_slice_stop(void);

#endif /* __sl_slice_H */

/************************** END OF FILE **************************/