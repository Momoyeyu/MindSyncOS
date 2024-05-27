#include "bootpack.h"
#include <stdio.h>
struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TIMER *mt_timer;
int mt_tr;

struct TASK *task_init(struct MEMMAN *memman)
{
    int i;
    struct TASK *task;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
    taskctl = (struct TASKCTL *)memman_alloc_4k(memman, sizeof(struct TASKCTL));
    for (i = 0; i < MAX_TASKS; i++)
    {
        taskctl->tasks0[i].flags = 0; // 未使用标志
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
    }
    task = task_alloc();
    task->flags = 2; // 活动中标志
    taskctl->running = 1;
    taskctl->now = 0;
    taskctl->tasks[0] = task;
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, 2);
    return task;
}

// 更改标记，添加任务到就绪队列
struct TASK *task_alloc(void)
{
    int i;
    struct TASK *task;
    for (i = 0; i < MAX_TASKS; i++)
    {
        if (taskctl->tasks0[i].flags == 0)
        {
            task = &taskctl->tasks0[i];
            task->flags = 1;               // 正在使用的标志
            task->tss.eflags = 0x00000202; // IF = 1;
            task->tss.eax = 0;             // 这里先置为0
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;
            return task;
        }
    }
    return NULL; /*全部正在使用*/
}

// 更改标记为活动中，添加任务到就绪队列
void task_run(struct TASK *task)
{
    task->flags = 2; /*活动中标志*/
    taskctl->tasks[taskctl->running] = task;
    taskctl->running++;
    return;
}

// 任务按轮盘顺序切换
void task_switch(void)
{
    timer_settime(task_timer, 2);
    if (taskctl->running >= 2)
    {
        taskctl->now++;
        if (taskctl->now == taskctl->running)
            taskctl->now = 0;
        farjmp(0, taskctl->tasks[taskctl->now]->sel);
    }
    return;
}

// 将任务休眠
void task_sleep(struct TASK *task)
{
    int i;
    char ts = 0;
    if (task->flags == 2)
    { // 只有指定任务处于唤醒状态执行休眠
        if (task == taskctl->tasks[taskctl->now])
            ts = 1; // 让当前执行的任务休眠的话，稍后需要进行任务切换
        for (i = 0; i < taskctl->running; i++)
        { // 寻找task所在的位置
            if (taskctl->tasks[i] == task)
                break;
        }
        taskctl->running--;
        if (i < taskctl->now)
        { // 当前执行的task标记位要前移
            taskctl->now--;
        }
        for (; i < taskctl->running; i++)
        { // task之后的任务前移
            taskctl->tasks[i] = taskctl->tasks[i + 1];
        }
        task->flags = 1; // 设置为休眠状态
        if (ts != 0)
        { // 需要切换任务
            if (taskctl->now >= taskctl->running)
            { // 如果now的值出现异常，则进行修正
                taskctl->now = 0;
            }
            farjmp(0, taskctl->tasks[taskctl->now]->sel);
        }
    }
    return;
}
