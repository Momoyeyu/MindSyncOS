// 中断处理
#include "bootpack.h"

void init_pic(void)
/* PIC的初始化 */
{
    io_out8(PIC0_IMR, 0xff); /* 不接受任何中断 */
    io_out8(PIC1_IMR, 0xff); /* 不接受任何中断 */

    io_out8(PIC0_ICW1, 0x11);   /* 边缘触发模式 */
    io_out8(PIC0_ICW2, 0x20);   /* IRQ0-7在INT20-27处接收 */
    io_out8(PIC0_ICW3, 1 << 2); /* PIC1通过IRQ2连接 */
    io_out8(PIC0_ICW4, 0x01);   /* 非缓冲模式 */

    io_out8(PIC1_ICW1, 0x11); /* 边缘触发模式 */
    io_out8(PIC1_ICW2, 0x28); /* IRQ8-15在INT28-2f处接收 */
    io_out8(PIC1_ICW3, 2);    /* PIC1通过IRQ2连接 */
    io_out8(PIC1_ICW4, 0x01); /* 非缓冲模式 */

    io_out8(PIC0_IMR, 0xfb); /* 11111011 除了PIC1之外全部禁止 */
    io_out8(PIC1_IMR, 0xff); /* 11111111 不接受任何中断 */

    return;
}

void inthandler21(int *esp)
/* 来自PS/2键盘的中断 */
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
    putfont_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 21 (IRQ-1) : PS/2 keyboard");
    for (;;)
    {
        io_hlt();
    }
}

void inthandler2c(int *esp)
/* 来自PS/2鼠标的中断 */
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
    putfont_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12) : PS/2 mouse");
    for (;;)
    {
        io_hlt();
    }
}

void inthandler27(int *esp)
/* 处理来自PIC0的非完全中断 */
/* 在Athlon64X2等机器上，由于芯片组的原因，在初始化PIC时可能会触发这个中断一次 */
/* 这个中断处理函数对于该中断不做任何处理，只是简单地忽略它 */
/* 为什么不需要做任何处理？
   → 因为这个中断是由PIC初始化时的电气噪声引起的，所以没有必要认真地进行处理。 */
{
    io_out8(PIC0_OCW2, 0x67); /* 通知PIC已接收完毕IRQ-07（参考7-1） */
    return;
}
