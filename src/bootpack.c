// 定义了几个用于硬件操作的函数，这些函数的具体实现没有给出
void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

// 初始化调色板的函数
void init_palette(void);
// 设置调色板颜色的函数
void set_palette(int start, int end, unsigned char *rgb);
// 填充矩形区域的函数
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);

// 定义了一些颜色常量，用于在函数中指定颜色
#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

void HariMain(void)
{
    char *vram;
    int xsize, ysize;

    // 初始化调色板
    init_palette();
    // 设置显存地址
    vram = (char *)0xa0000;
    // 设置屏幕分辨率
    xsize = 320;
    ysize = 200;

    // 使用boxfill8函数填充屏幕区域，指定颜色和坐标范围
    boxfill8(vram, xsize, COL8_008484, 0, 0, xsize - 1, ysize - 29);
    boxfill8(vram, xsize, COL8_C6C6C6, 0, ysize - 28, xsize - 1, ysize - 28);
    boxfill8(vram, xsize, COL8_FFFFFF, 0, ysize - 27, xsize - 1, ysize - 27);
    boxfill8(vram, xsize, COL8_C6C6C6, 0, ysize - 26, xsize - 1, ysize - 1);

    boxfill8(vram, xsize, COL8_FFFFFF, 3, ysize - 24, 59, ysize - 24);
    boxfill8(vram, xsize, COL8_FFFFFF, 2, ysize - 24, 2, ysize - 4);
    boxfill8(vram, xsize, COL8_848484, 3, ysize - 4, 59, ysize - 4);
    boxfill8(vram, xsize, COL8_848484, 59, ysize - 23, 59, ysize - 5);
    boxfill8(vram, xsize, COL8_000000, 2, ysize - 3, 59, ysize - 3);
    boxfill8(vram, xsize, COL8_000000, 60, ysize - 24, 60, ysize - 3);

    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize - 4, ysize - 24);
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize - 4);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize - 3, xsize - 4, ysize - 3);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 3, ysize - 24, xsize - 3, ysize - 3);

    // 继续使用boxfill8函数填充其他区域
    // ...（略过一些填充代码）

    // 无限循环，等待硬件中断
    for (;;)
    {
        io_hlt();
    }
}

void init_palette(void)
{
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00, /*  0:黒 */
        0xff, 0x00, 0x00, /*  1:明るい赤 */
        0x00, 0xff, 0x00, /*  2:明るい緑 */
        0xff, 0xff, 0x00, /*  3:明るい黄色 */
        0x00, 0x00, 0xff, /*  4:明るい青 */
        0xff, 0x00, 0xff, /*  5:明るい紫 */
        0x00, 0xff, 0xff, /*  6:明るい水色 */
        0xff, 0xff, 0xff, /*  7:白 */
        0xc6, 0xc6, 0xc6, /*  8:明るい灰色 */
        0x84, 0x00, 0x00, /*  9:暗い赤 */
        0x00, 0x84, 0x00, /* 10:暗い緑 */
        0x84, 0x84, 0x00, /* 11:暗い黄色 */
        0x00, 0x00, 0x84, /* 12:暗い青 */
        0x84, 0x00, 0x84, /* 13:暗い紫 */
        0x00, 0x84, 0x84, /* 14:暗い水色 */
        0x84, 0x84, 0x84  /* 15:暗い灰色 */
    };
    set_palette(0, 15, table_rgb);
    return;

    /* 静态字符命令仅用于数据，相当于汇编语言中的DB命令 */
}

void set_palette(int start, int end, unsigned char *rgb)
{
    int i, eflags;
    // 记录当前的中断允许标志
    eflags = io_load_eflags();
    // 禁用中断
    io_cli();
    // 发送调色板起始索引到端口0x03c8
    io_out8(0x03c8, start);
    // 遍历调色板索引
    for (i = start; i <= end; i++)
    {
        // 发送RGB值到端口0x03c9，每组颜色值需要除以4
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        // 移动到下一组RGB值
        rgb += 3;
    }
    // 恢复中断允许标志
    io_store_eflags(eflags);
    return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; y++)
    {
        for (x = x0; x <= x1; x++)
            // 填充颜色c到vram的指定位置
            vram[y * xsize + x] = c;
    }
    return;
}