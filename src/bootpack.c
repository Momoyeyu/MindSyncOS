// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;
void init_keyboard(void);

struct MOUSE_DEC
{
    unsigned char buf[3], phase;
    int x, y, btn;
};

void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx = (bootinfo->scrnx - 16) / 2;
    int my = (bootinfo->scrny - 28 - 16) / 2;
    int i;

    // initialize GDT & IDT
    init_gdtidt();
    init_pic();
    io_sti(); // 初始化完 GDT IDT PIC 之后才开中断

    // initialize devices
    io_out8(PIC0_IMR, 0xf9); /* 允许PIC1和键盘中断（11111001） */
    io_out8(PIC1_IMR, 0xef); /* 允许鼠标中断（11101111） */
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    init_keyboard();

    // say hello
    init_palette();
    init_screen(bootinfo->vram, bootinfo->scrnx, bootinfo->scrny);
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 97, 97, COL8_000000, "Momoyeyu OS");
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 96, 96, COL8_FFFFFF, "Momoyeyu OS");

    // draw cursor
    init_cursor(mcursor, COL8_008484);
    putblock8(bootinfo->vram, bootinfo->scrnx, mx, my, mcursor, 16, 16);

    struct MOUSE_DEC mdec;

    enable_mouse(&mdec);

    // 无限循环，等待硬件中断
    for (;;)
    {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo))
        {
            if (fifo8_status(&keyfifo))
            {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%02X", i);
                boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_008484, 8, 16, 23, 31);
                putfont_asc(bootinfo->vram, bootinfo->scrnx, 8, 16, COL8_FFFFFF, s);
            }
            else if (fifo8_status(&mousefifo))
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i))
                {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0)
                    {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0)
                    {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0)
                    {
                        s[2] = 'C';
                    }
                    boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfont_asc(bootinfo->vram, bootinfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* 隐藏鼠标 */
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0)
                        mx = 0;
                    else if (mx > bootinfo->scrnx - 16)
                        mx = bootinfo->scrnx - 16;
                    if (my < 0)
                        my = 0;
                    else if (my > bootinfo->scrny - 16)
                        my = bootinfo->scrny - 16;
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_008484, 0, 0, 79, 15); /* 隐藏旧坐标 */
                    putfont_asc(bootinfo->vram, bootinfo->scrnx, 0, 0, COL8_FFFFFF, s);   /* 显示新坐标 */
                    putblock8(bootinfo->vram, bootinfo->scrnx, mx, my, mcursor, 16, 16);
                }
            }
        }
        else
        {
            io_stihlt();
        }
    }
}

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

void wait_KBC_sendready(void)
{
    for (;;)
    {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
            break;
    }
    return;
}

void init_keyboard(void)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE); // 模式设定
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE); // 模式号码设定
    return;
}

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

void enable_mouse(struct MOUSE_DEC *mdec)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;
    return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
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
        mdec->btn = mdec->buf[0] & 0x07;
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
