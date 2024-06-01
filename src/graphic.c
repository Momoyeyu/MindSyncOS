// 图像显示处理相关

#include "bootpack.h"

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

void init_screen8(char *vram, int xsize, int ysize)
{
    // 使用boxfill8函数填充屏幕区域，指定颜色和坐标范围
    boxfill8(vram, xsize, COL8_008484, 0, 0, xsize - 1, ysize - 29); // background color

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
    int mw = (xsize - 8 * 11) / 2;
    int mh = (ysize - 28 - 16) / 2;
    putfonts8_asc(vram, xsize, mw + 1, mh + 1, COL8_000000, "Momoyeyu OS");
    putfonts8_asc(vram, xsize, mw, mh, COL8_FFFFFF, "Momoyeyu OS");
    return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
    int i;
    char *p, d /* data */;
    for (i = 0; i < 16; i++)
    {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0)
            p[0] = c;
        if ((d & 0x40) != 0)
            p[1] = c;
        if ((d & 0x20) != 0)
            p[2] = c;
        if ((d & 0x10) != 0)
            p[3] = c;
        if ((d & 0x08) != 0)
            p[4] = c;
        if ((d & 0x04) != 0)
            p[5] = c;
        if ((d & 0x02) != 0)
            p[6] = c;
        if ((d & 0x01) != 0)
            p[7] = c;
    }
    return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char c, char *s)
{
    extern char hankaku[4096];
    for (; *s != 0; s++)
    {
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
    return;
}

void init_mouse_cursor8(char *mouse, char background_color)
{
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"};
    int x, y;
    for (x = 0; x < 16; x++)
    {
        for (y = 0; y < 16; y++)
        {
            if (cursor[y][x] == '*')
                mouse[x + 16 * y] = COL8_000000;
            else if (cursor[y][x] == 'O')
                mouse[x + 16 * y] = COL8_FFFFFF;
            else if (cursor[y][x] == '.')
                mouse[x + 16 * y] = background_color;
        }
    }
    return;
}

void putblock8(char *vram, int xsize, int x, int y, char *buf, int width, int height)
{
    int h, w;
    for (h = 0; h < height; h++)
    {
        for (w = 0; w < width; w++)
        {
            vram[x + w + xsize * (y + h)] = buf[w + h * width];
        }
    }
    return;
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
    make_wtitle8(buf, xsize, title, act);
    return;
}

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act)
{
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"};
    int x, y;
    char c, tc, tbc;
    if (act != 0)
    {
        tc = COL8_FFFFFF;
        tbc = COL8_000084;
    }
    else
    {
        tc = COL8_C6C6C6;
        tbc = COL8_848484;
    }
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    putfonts8_asc(buf, xsize, 24, 4, tc, title);
    for (y = 0; y < 14; y++)
    {
        for (x = 0; x < 16; x++)
        {
            c = closebtn[y][x];
            if (c == '@')
                c = COL8_000000;
            else if (c == '$')
                c = COL8_848484;
            else if (c == 'Q')
                c = COL8_C6C6C6;
            else
                c = COL8_FFFFFF;
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

void putfonts8_asc_sht(struct SHEET *sht, int x0, int y0, int c, int bc, char *s, int l)
{
    boxfill8(sht->buf, sht->bxsize, bc, x0, y0, x0 + 8 * l - 1, y0 + 15);
    putfonts8_asc(sht->buf, sht->bxsize, x0, y0, c, s);
    sheet_refresh(sht, x0, y0, x0 + 8 * l, y0 + 16);
    return;
}
