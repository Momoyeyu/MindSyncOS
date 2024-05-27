// 内存管理

#include "bootpack.h"

unsigned int memtest(unsigned int start, unsigned int end)
{
    char flg486 = 0;
    unsigned int eflags_cache, cr0, i;

    // 确认是否支持缓存（是386还是486）
    eflags_cache = io_load_eflags();
    eflags_cache |= EFLAGS_AC_BIT; /* AC-bit = 1 */
    io_store_eflags(eflags_cache);
    eflags_cache = io_load_eflags();
    /* 如果是386，AC-bit会自动变回0 */
    if ((eflags_cache & EFLAGS_AC_BIT) != 0)
    {
        flg486 = 1;
    }
    eflags_cache &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflags_cache); /* AC-bit = 0 */

    if (flg486 != 0)
    {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE; /* 禁止缓存 */
        store_cr0(cr0);
    }
    i = memtest_sub(start, end);
    if (flg486 != 0)
    {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE; /* 允许缓存 */
        store_cr0(cr0);
    }
    return i;
}

void memman_init(struct MEMMAN *manager)
{
    manager->frees = 0;    /* 可用信息数目 */
    manager->maxfrees = 0; /* 用于观察可用状况：frees的最大值 */
    manager->lostsize = 0; /* 释放失败的内存的大小总和 */
    manager->losts = 0;    /* 释放失败次数 */
    return;
}

unsigned int memman_total(struct MEMMAN *manager) /* 报告空余内存大小的合计 */
{
    unsigned int i, total = 0;
    for (i = 0; i < manager->frees; i++)
    {
        total += manager->free[i].size;
    }
    return total;
}

unsigned int memman_alloc(struct MEMMAN *manager, unsigned int size) /* 分配 */
{
    unsigned int i, a;
    for (i = 0; i < manager->frees; i++)
    {
        if (manager->free[i].size >= size)
        {
            /* 找到了足够大的内存 */
            a = manager->free[i].addr;
            manager->free[i].addr += size;
            manager->free[i].size -= size;
            if (manager->free[i].size == 0)
            {
                /* 如果free[i]变成了0，就减掉一条可用信息 */
                manager->frees--;
                for (; i < manager->frees; i++)
                {
                    manager->free[i] = manager->free[i + 1]; /* 代入结构体 */
                }
            }
            return a;
        }
    }
    return 0; /* 没有可用空间 */
}

int memman_free(struct MEMMAN *manager, unsigned int addr, unsigned int size)
/* 释放 */
{
    int i, j;
    /* 为便于归纳内存，将free[]按照addr的顺序排列 */
    /* 所以，先决定应该放在哪里 */
    for (i = 0; i < manager->frees; i++)
    {
        if (manager->free[i].addr > addr)
        {
            break;
        }
    }
    /* free[i - 1].addr < addr < free[i].addr */
    if (i > 0)
    {
        /* 前面有可用内存 */
        if (manager->free[i - 1].addr + manager->free[i - 1].size == addr)
        {
            /* 可以与前面的可用内存归纳到一起 */
            manager->free[i - 1].size += size;
            /* 后面也有 */
            if ((i < manager->frees) && (addr + size == manager->free[i].addr))
            {
                /* 也可以与后面的可用内存归纳到一起 */
                manager->free[i - 1].size += manager->free[i].size;
                /* manager->free[i]删除，free[i]变成0后归纳到前面去 */
                manager->frees--;
                for (; i < manager->frees; i++)
                {
                    manager->free[i] = manager->free[i + 1]; /* 调整结构体 */
                }
            }
            return 0; /* 成功完成 */
        }
    }
    if (i < manager->frees) /* 不能与前面的可用空间归纳到一起 */
    {
        /* 后面还有 */
        if (addr + size == manager->free[i].addr)
        {
            /* 可以与后面的内容归纳到一起 */
            manager->free[i].addr = addr;
            manager->free[i].size += size;
            return 0; /* 成功完成 */
        }
    }
    /* 既不能与前面归纳到一起，也不能与后面归纳到一起 */
    if (manager->frees < MEMMAN_FREES)
    {
        /* free[i]之后的，向后移动，腾出一点可用空间 */
        for (j = manager->frees; j > i; j--)
        {
            manager->free[j] = manager->free[j - 1];
        }
        manager->frees++;
        if (manager->maxfrees < manager->frees)
        {
            manager->maxfrees = manager->frees; /* 更新最大值 */
        }
        manager->free[i].addr = addr;
        manager->free[i].size = size;
        return 0; /* 成功完成 */
    }
    /* 不能往后移动 */
    manager->losts++;
    manager->lostsize += size;
    return -1; /* 失败 */
}

unsigned int memman_alloc_4k(struct MEMMAN *manager, unsigned int size)
{
    unsigned int addr;
    size = (size + 0xfff) & 0xfffff000;
    addr = memman_alloc(manager, size);
    return addr;
}

int memman_free_4k(struct MEMMAN *manager, unsigned int addr, unsigned int size)
{
    unsigned int addr;
    size = (size + 0xfff) & 0xfffff000;
    addr = memman_free(manager, addr, size);
    return addr;
}
