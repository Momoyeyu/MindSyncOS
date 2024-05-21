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

    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctl.timers0[i].flags = TIMER_FLAGS_FREE;
    }
    struct TIMER *t;
    t = timer_alloc();
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = NULL;
    timerctl.t0 = t;
    timerctl.next = 0xffffffff;
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
    int e;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();
    io_cli();
    struct TIMER *t, *s;
    t = timerctl.t0;
    if (timer->timeout <= t->timeout)
    { // 置首
        timer->next = t;
        timerctl.t0 = timer;
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    s = t;
    for (t = t->next; t != NULL; t = t->next)
    {
        if (timer->timeout <= t->timeout)
        {
            timer->next = t;
            s->next = timer;
            io_store_eflags(e);
            return;
        }
        s = t;
    }
}

void inthandler20(int *esp)
{
    struct TIMER *timer;
    io_out8(PIC0_OCW2, 0x60); /* 把IRQ-00信号接收完了的信息通知给PIC */
    timerctl.count++;
    if (timerctl.next > timerctl.count)
        return;
    timer = timerctl.t0; // 选择第一个 timer
    for (;;)
    {
        if (timer->timeout > timerctl.count)
            break;
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next; // 选择下一个 timer
    }
    timerctl.t0 = timer;
    timerctl.next = timerctl.t0->timeout;
    return;
}
