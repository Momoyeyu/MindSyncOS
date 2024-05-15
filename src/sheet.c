// 图层相关

#include "bootpack.h"
#include <stdio.h>

struct SheetController *shtctl_init(struct MemoryManager *manager, unsigned char *vram, int xsize, int ysize)
{
    struct SheetController *controller;
    int i;
    controller = (struct SheetController *)memman_alloc_4k(manager, sizeof(struct SheetController));
    if (controller == NULL)
    {
        goto err;
    }
    controller->vram = vram;
    controller->xsize = xsize;
    controller->ysize = ysize;
    controller->top = -1; // 暂时没有任何sheet
    for (i = 0; i < MAX_SHEETS; i++)
    {
        controller->sheets_data[i].flags = SHEET_FREE; // 标记为未使用
    }
err:
    // 将来可以引入错误处理
    return controller;
}

struct Sheet *sheet_alloc(struct SheetController *controller)
{
    struct Sheet *sheet;
    int i;
    for (i = 0; i < MAX_SHEETS; i++)
    {
        if (controller->sheets_data[i].flags == 0)
        {
            sheet = &(controller->sheets_data[i]);
            sheet->flags = SHEET_USED; // 标记为已使用
            sheet->layer = -1;         // 隐藏
            return sheet;
        }
    }
    return NULL; // allocation failed
}

void sheet_set_buf(struct Sheet *sheet, unsigned char *buf, int xsize, int ysize, int color)
{
    sheet->buf = buf;
    sheet->box_width = xsize;
    sheet->box_height = ysize;
    sheet->color = color;
    return;
}

void sheet_set_layer(struct SheetController *controller, struct Sheet *sheet, int layer)
{
    int old = sheet->layer; /* 存储设置前的图层信息 */
    /* 如果指定的图层过高或过低，则进行修正 */
    if (layer > controller->top + 1)
        layer = controller->top + 1;
    else if (layer < -1)
        layer = -1;
    sheet->layer = layer;

    // 重新排列
    int l;
    if (layer < old)
    {
        if (layer < 0) // 隐藏sheet
        {
            for (l = old; l < controller->top; l++) // 降低 (old, controller->top] 的图层
            {
                controller->sheets_ptr[l] = controller->sheets_ptr[l + 1];
                controller->sheets_ptr[l]->layer = l;
            }
            controller->top -= 1;
        }
        else // 降低sheet
        {
            for (l = old; l > layer; l--) // 增高 [layer, old) 的图层
            {
                controller->sheets_ptr[l] = controller->sheets_ptr[l - 1];
                controller->sheets_ptr[l]->layer = l;
            }
            controller->sheets_ptr[layer] = sheet;
        }
        sheet_refresh(controller);
    }
    else if (layer > old)
    {
        if (old >= 0) // 增高sheet
        {
            for (l = old; l < layer; l++) // 降低 (old, layer] 的图层高度
            {
                controller->sheets_ptr[l] = controller->sheets_ptr[l + 1];
                controller->sheets_ptr[l]->layer = l;
            }
        }
        else // 显示sheet
        {
            controller->top += 1;
            for (l = controller->top; l > layer; l--) // 增高 [layer, controller->top] 的图层高度
            {
                controller->sheets_ptr[l] = controller->sheets_ptr[l - 1];
                controller->sheets_ptr[l]->layer = l;
            }
        }
        controller->sheets_ptr[layer] = sheet;
        sheet_refresh(controller);
    }
    return;
}

void sheet_refresh(struct SheetController *controller)
{
    int layer;        // 图层高度
    int buf_x, buf_y; // 相对坐标 (buf_x, buf_y)
    int vx, vy;       // 绝对坐标 (vram_x, vram_y)
    unsigned char *buf, c, *vram = controller->vram;
    struct Sheet *sheet;
    for (layer = 0; layer <= controller->top; layer++) // 从 0 到 top 绘制图层
    {
        sheet = controller->sheets_ptr[layer];
        buf = sheet->buf;
        for (buf_y = 0; buf_y < sheet->box_height; buf_y++)
        {
            vy = sheet->vy0 + buf_y;
            for (buf_x = 0; buf_x < sheet->box_width; buf_x++)
            {
                vx = sheet->vx0 + buf_x;
                c = buf[buf_y * sheet->box_width + buf_x];
                if (c != sheet->color)
                    vram[vy * controller->xsize + vx] = c;
            }
        }
    }
    return;
}

void sheet_slide(struct SheetController *controller, struct Sheet *sheet, int new_vx0, int new_vy0)
{
    sheet->vx0 = new_vx0;
    sheet->vy0 = new_vy0;
    if (sheet->layer >= 0)         /* 如果正在显示*/
        sheet_refresh(controller); /* 按新图层的信息刷新画面 */
    return;
}

void sheet_free(struct SheetController *controller, struct Sheet *sheet)
{
    if (sheet->layer >= 0)
        sheet_set_layer(controller, sheet, -1); // 隐藏
    sheet->flags = SHEET_FREE;                  // 标志释放
    return;
}
