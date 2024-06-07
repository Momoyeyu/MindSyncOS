#include "bootpack.h"

void file_readfat(int *fat, unsigned char *img)
{ // 将磁盘映像中的FAT解压缩
    int i, j = 0;
    for (i = 0; i < 2880; i += 2)
    {
        fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
        j += 3;
    }
    return;
}

void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
    int i;
    for (;;)
    {
        if (size <= 512)
        {
            for (i = 0; i < size; i++)
            {
                buf[i] = img[clustno * 512 + i];
            }
            break;
        }
        for (i = 0; i < 512; i++)
        {
            buf[i] = img[clustno * 512 + i];
        }
        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
    return;
}
//
// 0x01……只读文件（不可写入）
// 0x02……隐藏文件
// 0x04……系统文件
// 0x08……非文件信息（比如磁盘名称等）
// 0x10……目录
//
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
    int i, j;
    char s[12];
    // 初始化，置空
    for (j = 0; j < 11; j++)
        s[j] = ' ';
    j = 0;
    // 将用户输入的name与ext写入数组s
    for (i = 0; name[i] != 0; i++)
    {
        if (j >= 11)
            return 0; // 超过范围，没有找到
        if (name[i] == '.' && j <= 8)
            j = 8; // 开始写文件后缀
        else
        {
            s[j] = name[i];
            if ('a' <= s[j] && s[j] <= 'z')
                s[j] -= 0x20; // 将小写字母转换为大写字母
            j++;
        }
    }
    // 寻找name与ext匹配的文件
    for (i = 0; i < max; i++)
    {
        if (finfo[i].name[0] == 0x00) // 0x00 表示此后没有更多文件信息
            break;
        if ((finfo[i].type & 0x18) == 0) // 排除“非文件信息”与“目录”
        {
            for (j = 0; j < 11; j++)
                if (finfo[i].name[j] != s[j])
                    goto next; // 匹配失败，下一位
            return finfo + i;  // 全部匹配成功找到文件
        }
    next:
    }
    return 0; // 没有找到
}
