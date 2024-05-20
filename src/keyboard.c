// 键盘相关

#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

void wait_KBC_sendready(void)
{

    for (;;)
    {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
            break;
    }
    return;
}

void init_keyboard(struct FIFO32 *fifo, int data0)
{
    keyfifo = fifo;
    keydata0 = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE); // 模式设定
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE); // 模式号码设定
    return;
}

void inthandler21(int *esp)
/* 来自PS/2键盘的中断 */
{
    int data;
    io_out8(PIC0_OCW2, 0x61); // 通知PIC0 IRQ-1中断已经得到处理，可以继续处理下一个中断
    data = io_in8(PORT_KEYDAT);
    fifo32_put(keyfifo, data + keydata0);
    return;
}
