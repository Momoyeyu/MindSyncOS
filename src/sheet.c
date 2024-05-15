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
    sheet->bxsize = xsize;
    sheet->bysize = ysize;
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
        sheet_refresh(controller, sheet, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize);
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
        sheet_refresh(controller, sheet, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize);
    }
    return;
}

void sheet_refresh(struct SheetController *controller, struct Sheet *sheet, int bx0, int by0, int bx1, int by1)
{
    if (sheet->layer >= 0)
        sheet_refresh_sub(controller, sheet->vx0 + bx0, sheet->vy0 + by0, sheet->vx0 + bx1, sheet->vy0 + by1);
    return;
}

void sheet_slide(struct SheetController *controller, struct Sheet *sheet, int new_vx0, int new_vy0)
{
    int old_x = sheet->vx0;
    int old_y = sheet->vy0;
    sheet->vx0 = new_vx0;
    sheet->vy0 = new_vy0;
    /* 如果正在显示*/
    if (sheet->layer >= 0)
    { // 采用局部更新
        sheet_refresh_sub(controller, old_x, old_y, old_x + sheet->bxsize, old_y + sheet->bysize);
        sheet_refresh_sub(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize);
    }
    return;
}

void sheet_free(struct SheetController *controller, struct Sheet *sheet)
{
    if (sheet->layer >= 0)
        sheet_set_layer(controller, sheet, -1); // 隐藏
    sheet->flags = SHEET_FREE;                  // 标志释放
    return;
}

// 只更新 [(vx0, vy0), (vx1, vy1)) 内的显存
void sheet_refresh_sub(struct SheetController *controller, int vx0, int vy0, int vx1, int vy1)
{
    int h, bx, by, vx, vy;
    int bx0, bx1, by0, by1;
    unsigned char *buf, c, *vram = controller->vram;
    struct Sheet *sheet;
    for (h = 0; h <= controller->top; h++)
    {
        sheet = controller->sheets_ptr[h];
        buf = sheet->buf;
        // 缩小搜索范围，减少if次数
        bx0 = vx0 - sheet->vx0;
        bx1 = vx1 - sheet->vx0;
        by0 = vy0 - sheet->vy0;
        by1 = vy1 - sheet->vy0;
        if (bx0 < 0)
            bx0 = 0;
        if (bx1 > sheet->bxsize)
            bx1 = sheet->bxsize;
        if (by0 < 0)
            by0 = 0;
        if (by1 > sheet->bysize)
            by1 = sheet->bysize;
        for (by = by0; by < by1; by++)
        {
            vy = sheet->vy0 + by;
            for (bx = bx0; bx < bx1; bx++)
            {
                vx = sheet->vx0 + bx;
                c = buf[by * sheet->bxsize + bx];
                if (c != sheet->color) // 减小访存次数
                    vram[vy * controller->xsize + vx] = c;
            }
        }
    }
    return;
}
