# sloopSlice

一款专为 STM32 微控制器设计的**独特任务调度协同框架**，核心创新在于支持**黑盒任务切片**并入协作式调度体系。

## 简介

sloopSlice 是一款基于线程原理实现的轻量级嵌入式任务调度框架，专为资源受限的 STM32 微控制器打造。其核心创新在于**黑盒任务切片机制**，能够将第三方闭源代码或无法修改的黑盒任务无缝并入协作式调度框架，实现时间片轮转调度。

### 核心设计理念

- **黑盒兼容**：无需修改第三方代码即可将其纳入调度体系
- **线程级切片**：基于 PendSV 异常实现类线程的上下文切换
- **协作式调度**：结合事件驱动与时间片轮转的混合调度模型
- **零侵入集成**：黑盒任务保持完全独立，无需添加调度点

## 核心特性

### ? 黑盒任务切片（核心创新）

- **线程级上下文切换**：基于 PendSV 异常实现，模拟线程的时间片轮转
- **黑盒零侵入**：第三方闭源代码无需任何修改即可接入调度
- **时间片可控**：可配置切片粒度（最小 50us），灵活控制任务执行时间
- **无缝协同**：与协作式任务共享同一调度框架，实现混合调度

### ? 协作式任务调度

- **超时任务** - 延时后执行一次
- **周期任务** - 周期性重复执行
- **多次任务** - 执行指定次数后自动停止
- **并行任务** - 非阻塞后台任务
- **单次任务** - 延迟到主循环执行（适用于中断中复杂逻辑下放）
- **互斥任务切换** - 基于 `sl_goto()` 的协作式上下文切换

### ? 系统级特性

- **CPU 负载监控** - 内置性能追踪
- **RTT 调试支持** - 实时日志输出
- **资源占用低** - 适合资源受限的微控制器

## 硬件平台

- **MCU**: STM32G030xx
- **调试**: SEGGER RTT

## 目录结构

```
project/
├── Core/               # STM32 HAL 核心文件
├── Drivers/            # STM32 外设驱动
├── user/
│   ├── app/
│   │   ├── config/     # sloop 配置
│   │   └── tasks/      # 用户任务定义
│   └── sloop/
│       ├── RTT/        # SEGGER RTT 调试
│       └── kernel/      # sloop 内核实现
└── MDK-ARM/            # Keil uVision 工程文件
```

## 配置说明

编辑 `user/app/config/sl_config.h` 配置任务上限和 RTT 设置：

```c
#define SL_TIMEOUT_LIMIT   16   // 超时任务上限
#define SL_CYCLE_LIMIT     16   // 周期任务上限
#define SL_MULTIPLE_LIMIT  16   // 多次任务上限
#define SL_PARALLEL_LIMIT  32   // 并行任务上限
#define SL_ONCE_LIMIT      16   // 单次任务上限

#define SL_RTT_ENABLE      1    // 启用 RTT 打印
#define SL_RTT_BUFFER_SIZE 2048 // RTT 缓冲区大小
```

## API 参考

### 系统

```c
void sloop_init(void);          // 初始化 sloop 系统
void sloop(void);               // 运行 sloop 调度器
void sl_tick_irq(void);         // 在定时器中断中调用
uint32_t sl_get_tick(void);     // 获取当前 tick
void sl_delay(int ms);          // 阻塞式延时
```

### 任务管理

```c
// 超时任务
void sl_timeout_start(int ms, pfunc task);
void sl_timeout_stop(pfunc task);

// 周期任务
void sl_cycle_start(int ms, pfunc task);
void sl_cycle_stop(pfunc task);

// 多次任务
void sl_multiple_start(int num, int ms, pfunc task);
void sl_multiple_stop(pfunc task);

// 并行任务
void sl_task_start(pfunc task);
void sl_task_stop(pfunc task);

// 单次任务（用于中断中延迟执行）
void sl_task_once(pfunc task);
```

### 互斥任务切换

```c
void sl_goto(pfunc task);       // 切换到指定任务
char sl_wait(int ms);           // 非阻塞等待（0: 完成, 1: 被中断）
char sl_wait_bare(void);        // 非阻塞裸等待
void sl_wait_break(void);       // 跳出等待
void sl_wait_continue(void);    // 继续执行等待后的操作
char sl_is_waiting(void);       // 获取等待状态
```

### Slice 任务（黑盒切片核心 API）

Slice 任务是 sloopSlice 的核心创新，基于**线程原理**实现黑盒任务的时间片调度：

```c
void sl_slice_start(int us, pfunc slice);  // 启动黑盒切片任务（时间片 >= 50us）
void sl_slice_stop(void);                  // 停止切片系统
```

**工作原理**：

1. **PendSV 触发**：当切片时间到达时，触发 PendSV 异常
2. **上下文保存**：自动保存当前任务的寄存器状态到栈
3. **任务切换**：切换到协作式调度主循环执行其他任务
4. **上下文恢复**：下次切片时间到达时恢复寄存器状态继续执行

这种机制使得黑盒任务看起来像独立线程一样运行，但其本质是基于时间片的协作式调度。

## 使用示例

### 基本任务调度

```c
void my_task(void)
{
    while (1)
    {
        sl_printf("Task running");
        sl_delay(1000);
    }
}

void _main(void)
{
    sloop_init();

    // 启动周期任务
    sl_cycle_start(500, my_task);

    // 启动并行任务
    sl_task_start(another_task);

    // 跳转到第一个任务
    sl_goto(task_baseInit);

    while (1)
    {
        sloop();
    }
}
```

### 黑盒任务切片（核心用法）

```c
// 第三方黑盒任务（无法修改的闭源代码）
void blackbox_task(void)
{
    while (1)
    {
        // 密集计算或长时间阻塞操作
        // 无需添加任何调度点
    }
}

void _main(void)
{
    sloop_init();

    // 启动黑盒切片任务，每 100us 切出一次
    // 黑盒任务会在时间片内执行，超时自动切出
    sl_slice_start(100, blackbox_task);

    // 启动其他协作式任务
    sl_cycle_start(500, periodic_task);

    while (1)
    {
        sloop();
    }
}
```

**关键特性**：`blackbox_task` 无需任何修改即可与协作式任务共享 CPU，实现伪并行执行。

## 编译构建

### Keil uVision

在 Keil uVision 中打开 `project/MDK-ARM/project.uvprojx` 进行编译。

### CMake (GCC)

```bash
cd project
mkdir build && cd build
cmake ..
make
```

## 许可证

详见 LICENSE 文件。