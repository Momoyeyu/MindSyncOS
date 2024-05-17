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
    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctl.timer[i].flags = TIMER_FLAGS_FREE;
    }
    return;
}

struct TIMER *timer_alloc(void)
{
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        if (timerctl.timer[i].flags == TIMER_FLAGS_FREE)
        {
            timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timer[i];
        }
    }
    return NULL; /* 没找到 */
}

void timer_free(struct TIMER *timer)
{
    timer->flags = TIMER_FLAGS_FREE;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    if (timerctl.next > timer->timeout)
        timerctl.next = timer->timeout;
    return;
}

void inthandler20(int *esp)
{
    io_out8(PIC0_OCW2, 0x60); /* 把IRQ-00信号接收完了的信息通知给PIC */
    /* 暂时什么也不做 */
    timerctl.count++;
    if (timerctl.next > timerctl.count)
        return;
    timerctl.next = 0xffffffff;
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        struct TIMER *timer = &(timerctl.timer[i]);
        if (timer->flags == TIMER_FLAGS_USING)
        {
            if (timer->timeout <= timerctl.count)
            {
                timer->flags = TIMER_FLAGS_ALLOC;
                fifo8_put(timer->fifo, timerctl.timer[i].data);
            }
            else
            {
                if (timerctl.next > timer->timeout)
                    timerctl.next = timer->timeout;
            }
        }
    }
    return;
}
