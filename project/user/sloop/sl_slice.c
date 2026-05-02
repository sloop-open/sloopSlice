/*==============================================================
 * root(MSP) <-> slice(PSP) switch
 * Cortex-M0+ / STM32G030
 *
 * 手动保存 R4-R11
 * 硬件自动保存:
 * R0-R3,R12,LR,PC,xPSR
 *
 * root  : Thread + MSP
 * slice : Thread + PSP
 * PendSV 负责切换
 *=============================================================*/

#include "common.h"
#include "tim.h"

void sl_slice_run(void);
void sl_slice_yield(void);

/*==============================================================*/
#define SLICE_STACK_WORDS 128

static uint32_t slice_stack[SLICE_STACK_WORDS];

static volatile uint8_t to_slice;

/*==============================================================*/
static void slice_trap(void)
{
    sl_printf("slice return trap");

    while (1)
    {
    }
}

/*==============================================================
 * 首次启动 slice 栈
 *
 * [R4-R11 software frame]
 * [hardware exception frame]
 *=============================================================*/
static void slice_stack_init(pfunc entry)
{
    uint32_t *sp = &slice_stack[SLICE_STACK_WORDS];

    /* hardware frame */
    *(--sp) = 0x01000000;                  /* xPSR */
    *(--sp) = ((uint32_t)entry) | 1u;      /* PC   */
    *(--sp) = ((uint32_t)slice_trap) | 1u; /* LR   */
    *(--sp) = 0;                           /* R12  */
    *(--sp) = 0;                           /* R3   */
    *(--sp) = 0;                           /* R2   */
    *(--sp) = 0;                           /* R1   */
    *(--sp) = 0;                           /* R0   */

    /* software frame R4-R11 */
    for (int i = 0; i < 8; i++)
        *(--sp) = 0;

    __set_PSP((uint32_t)sp);
}

/*==============================================================*/
void sl_slice_start(pfunc task)
{
    __HAL_TIM_SET_AUTORELOAD(&htim14, (200 - 1));

    HAL_NVIC_SetPriority(TIM14_IRQn, 3, 0);
    HAL_NVIC_SetPriority(PendSV_IRQn, 3, 0);

    slice_stack_init(task);

    sl_task_start(sl_slice_run);

    sl_printf("slice start");
}

/*==============================================================*/
void sl_slice_stop(void)
{
    HAL_TIM_Base_Stop_IT(&htim14);

    sl_task_stop(sl_slice_run);

    sl_printf("slice stop");
}

/*==============================================================*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim14)
    {
        sl_slice_yield();
    }
}

/*==============================================================*/
void sl_slice_run(void)
{
    HAL_TIM_Base_Stop_IT(&htim14);

    __HAL_TIM_SET_COUNTER(&htim14, 0);

    __HAL_TIM_CLEAR_IT(&htim14, TIM_IT_UPDATE);

    HAL_TIM_Base_Start_IT(&htim14);

    to_slice = 1;

    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

/*==============================================================*/
void sl_slice_yield(void)
{
    HAL_TIM_Base_Stop_IT(&htim14);

    to_slice = 0;

    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

/*==============================================================
 * PendSV
 *=============================================================*/
__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(
        "cpsid i                        \n"
        ".syntax unified                \n"

        "ldr r0, =to_slice             \n"
        "ldrb r1, [r0]                 \n"
        "cmp r1, #1                    \n"
        "beq to_slice_path             \n"

        /*==================================================
         * slice -> root
         * save PSP / restore MSP
         *=================================================*/
        "to_root_path:                 \n"

        /* save slice R4-R11 to PSP */
        "mrs r0, psp                   \n"
        "subs r0, #32                  \n"

        "str r4, [r0,#0]               \n"
        "str r5, [r0,#4]               \n"
        "str r6, [r0,#8]               \n"
        "str r7, [r0,#12]              \n"

        "mov r1, r8                    \n"
        "str r1, [r0,#16]              \n"
        "mov r1, r9                    \n"
        "str r1, [r0,#20]              \n"
        "mov r1, r10                   \n"
        "str r1, [r0,#24]              \n"
        "mov r1, r11                   \n"
        "str r1, [r0,#28]              \n"

        "msr psp, r0                   \n"

        /* restore root R4-R11 from MSP */
        "mrs r0, msp                   \n"

        "ldr r4, [r0,#0]               \n"
        "ldr r5, [r0,#4]               \n"
        "ldr r6, [r0,#8]               \n"
        "ldr r7, [r0,#12]              \n"

        "ldr r1, [r0,#16]              \n"
        "mov r8, r1                    \n"
        "ldr r1, [r0,#20]              \n"
        "mov r9, r1                    \n"
        "ldr r1, [r0,#24]              \n"
        "mov r10, r1                   \n"
        "ldr r1, [r0,#28]              \n"
        "mov r11, r1                   \n"

        "adds r0, #32                  \n"
        "msr msp, r0                   \n"

        "ldr r0, =0xFFFFFFF9           \n"
        "mov lr, r0                    \n"
        "cpsie i                       \n"
        "bx lr                         \n"

        /*==================================================
         * root -> slice
         * save MSP / restore PSP
         *=================================================*/
        "to_slice_path:                \n"

        /* save root R4-R11 to MSP */
        "mrs r0, msp                   \n"
        "subs r0, #32                  \n"

        "str r4, [r0,#0]               \n"
        "str r5, [r0,#4]               \n"
        "str r6, [r0,#8]               \n"
        "str r7, [r0,#12]              \n"

        "mov r1, r8                    \n"
        "str r1, [r0,#16]              \n"
        "mov r1, r9                    \n"
        "str r1, [r0,#20]              \n"
        "mov r1, r10                   \n"
        "str r1, [r0,#24]              \n"
        "mov r1, r11                   \n"
        "str r1, [r0,#28]              \n"

        "msr msp, r0                   \n"

        /* restore slice R4-R11 from PSP */
        "mrs r0, psp                   \n"

        "ldr r4, [r0,#0]               \n"
        "ldr r5, [r0,#4]               \n"
        "ldr r6, [r0,#8]               \n"
        "ldr r7, [r0,#12]              \n"

        "ldr r1, [r0,#16]              \n"
        "mov r8, r1                    \n"
        "ldr r1, [r0,#20]              \n"
        "mov r9, r1                    \n"
        "ldr r1, [r0,#24]              \n"
        "mov r10, r1                   \n"
        "ldr r1, [r0,#28]              \n"
        "mov r11, r1                   \n"

        "adds r0, #32                  \n"
        "msr psp, r0                   \n"

        "ldr r0, =0xFFFFFFFD           \n"
        "mov lr, r0                    \n"
        "cpsie i                       \n"
        "bx lr                         \n");
}
