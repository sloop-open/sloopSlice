/*==============================================================
 * Minimal root(MSP) <-> slice(PSP) switch
 * Cortex-M0+ / STM32G030
 *
 * 不考虑 R4-R11
 * 仅使用硬件自动压栈现场
 *
 * root  : Thread + MSP
 * slice : Thread + PSP
 * PendSV负责切换
 *=============================================================*/

#include "common.h"

/*==============================================================*/
#define SLICE_STACK_WORDS 128

static uint32_t g_slice_stack[SLICE_STACK_WORDS];

static volatile uint8_t g_started = 0;
static volatile uint8_t g_state = 0;

/*==============================================================*/
static void slice_trap(void)
{
    sl_printf("slice return trap");

    while (1)
    {
    }
}

/*==============================================================
 * 构造首次启动硬件异常栈帧
 *
 * 异常返回时CPU自动弹出:
 * R0,R1,R2,R3,R12,LR,PC,xPSR
 *=============================================================*/
static void slice_stack_init(uint32_t *top, pfunc entry)
{
    uint32_t *sp = top;

    *(--sp) = 0x01000000;                  /* xPSR */
    *(--sp) = ((uint32_t)entry) | 1u;      /* PC   */
    *(--sp) = ((uint32_t)slice_trap) | 1u; /* LR   */
    *(--sp) = 0;                           /* R12  */
    *(--sp) = 0;                           /* R3   */
    *(--sp) = 0;                           /* R2   */
    *(--sp) = 0;                           /* R1   */
    *(--sp) = 0;                           /* R0   */

    __set_PSP((uint32_t)sp);
}

/*==============================================================*/
void sl_slice_start(pfunc task)
{
    slice_stack_init(
        &g_slice_stack[SLICE_STACK_WORDS],
        task);

    NVIC_SetPriority(PendSV_IRQn, 3);

    g_started = 1;
}

/*==============================================================*/
void sl_slice_run(void)
{
    if (!g_started)
        return;

    g_state = 1;

    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

/*==============================================================*/
void sl_slice_yield(void)
{
    g_state = 0;

    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(
        ".syntax unified        \n"

        "ldr r0, =g_state       \n"
        "ldrb r1, [r0]          \n"
        "cmp r1, #1             \n"
        "beq to_slice           \n"

        "to_root:               \n"

        "ldr r0, =0xFFFFFFF9    \n"
        "mov lr, r0             \n"
        "bx lr                  \n"

        "to_slice:              \n"

        "ldr r0, =0xFFFFFFFD    \n"
        "mov lr, r0             \n"
        "bx lr                  \n");
}
