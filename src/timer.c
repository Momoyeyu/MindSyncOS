// 计时器

#include "bootpack.h"
#include <stdio.h>

struct TIMERCTL timerctl;

void init_pit(void)
{
    io_out8(PIT_CTRL, 0x34);
    // 0x2e9c = 11932
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    int i;
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    timerctl.using = 0;
    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctl.timers0[i].flags = TIMER_FLAGS_FREE;
    }
    return;
}

struct TIMER *timer_alloc(void)
{
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        if (timerctl.timers0[i].flags == TIMER_FLAGS_FREE)
        {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return NULL; /* 没找到 */
}

void timer_free(struct TIMER *timer)
{
    timer->flags = TIMER_FLAGS_FREE;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    int e, i, j;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();
    io_cli();
    for (i = 0; i < timerctl.using; i++)
        if (timerctl.timers[i]->timeout >= timer->timeout)
            break;
    for (j = timerctl.using; j > i; j--)
        timerctl.timers[j] = timerctl.timers[j - 1];
    timerctl.using += 1;
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(e);
    return;
}

void inthandler20(int *esp)
{
    io_out8(PIC0_OCW2, 0x60); /* 把IRQ-00信号接收完了的信息通知给PIC */
    /* 暂时什么也不做 */
    timerctl.count++;
    if (timerctl.next > timerctl.count)
        return;
    int i, j;
    for (i = 0; i < timerctl.using; i++)
    {
        struct TIMER *timer = timerctl.timers[i];
        if (timer->timeout > timerctl.count)
            break;
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
    }
    timerctl.using -= i;
    for (j = 0; j < timerctl.using; j++)
        timerctl.timers[j] = timerctl.timers[j + i];
    if (timerctl.using > 0)
        timerctl.next = timerctl.timers[0];
    else
        timerctl.next = 0xffffffff;
    return;
}
