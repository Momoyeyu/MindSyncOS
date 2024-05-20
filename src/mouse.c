// 鼠标相关

#include "bootpack.h"

// GLOBAL VAR
struct FIFO32 *mousefifo;
int mousedata0;

void enable_mouse(struct FIFO32 *fifo, int data0, struct MouseDescriptor *mdec)
{
    mousefifo = fifo;
    mousedata0 = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;
    return;
}

int mouse_decode(struct MouseDescriptor *mdec, unsigned char dat)
{
    switch (mdec->phase)
    {
    case 0:
        if (dat == 0xfa) /* 进入到等待鼠标的0xfa的状态 */
            mdec->phase = 1;
        return 0;
    case 1:
        if ((dat & 0xc8) == 0x08)
        {
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    case 2:
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    case 3:
        mdec->buf[2] = dat;
        mdec->phase = 1;
        mdec->button = mdec->buf[0] & 0x07;
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0)
        {
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0)
        {
            mdec->y |= 0xffffff00;
        }
        mdec->y = -mdec->y; /* 鼠标的y方向与画面符号相反 */
        return 1;
    default:
        return -1;
    }
}

void inthandler2c(int *esp)
/* 来自PS/2鼠标的中断 */
{
    int data;
    io_out8(PIC1_OCW2, 0x64); // 通知PIC1 IRQ-12中断已经得到处理，可以继续处理下一个中断
    io_out8(PIC0_OCW2, 0x62); // 通知PICo IRQ-2中断已经得到处理，可以继续处理下一个中断
    data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, data + mousedata0);
    return;
}
