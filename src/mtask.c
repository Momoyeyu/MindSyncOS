#include "bootpack.h"
#include <stdio.h>

struct TIMER *mt_timer;
int mt_tr;

void mt_init(void)
{ // 设置任务切换事件间隔
    mt_timer = timer_alloc();
    // 这里没有必要使用timer_init
    // 因为在发生超时的时候不需要向FIFO缓冲区写入
    timer_settime(mt_timer, 2);
    mt_tr = 3 * 8;
    return;
}

void mt_taskswitch(void)
{
    if (mt_tr == 3 * 8)
        mt_tr = 4 * 8;
    else
        mt_tr = 3 * 8;
    timer_settime(mt_timer, 2);
    farjmp(0, mt_tr);
    return;
}
