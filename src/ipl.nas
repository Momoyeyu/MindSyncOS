; haribote-ipl
; TAB=4

CYLS	EQU		10				; 定义一个常量CYLS，表示要加载的扇区总数

		ORG		0x7c00			; 设置程序的起始位置，0x7c00是BIOS加载引导扇区的默认地址

; 以下是针对标准FAT12格式软盘的一些描述

		JMP		entry			; 跳转到程序的入口点entry
		DB		0x90			; 填充一个字节，值为0x90，表示NOP（无操作）指令
		DB		"HARIBOTE"		; 引导扇区名称，可以自由命名（8字节）
		DW		512				; 一个扇区的大小（必须为512）
		DB		1				; 每个簇的大小（必须为1个扇区）
		DW		1				; FAT开始的扇区号（通常从1扇区开始）
		DB		2				; FAT的数量（必须为2）
		DW		224				; 根目录的大小（通常为224个目录项）
		DW		2880			; 磁盘的总扇区数（必须为2880）
		DB		0xf0			; 媒体类型（必须为0xf0）
		DW		9				; FAT区域的长度（必须为9扇区）
		DW		18				; 每磁道的扇区数（必须为18）
		DW		2				; 磁头数（必须为2）
		DD		0				; 如果没有使用分区，则此处为0
		DD		2880			; 再次指定磁盘大小
		DB		0,0,0x29		; 不清楚具体含义，但通常设置为这些值
		DD		0xffffffff		; 卷序列号，通常设置为一个随机数或特定的值
		DB		"HARIBOTEOS "	; 磁盘名称（11字节）
		DB		"FAT12   "		; 文件系统格式名称（8字节）
		RESB	18				; 保留18个字节的空间

; 程序的主体部分

entry:
		MOV		AX,0			; 初始化寄存器AX
		MOV		SS,AX			; 设置堆栈段寄存器
		MOV		SP,0x7c00		; 设置堆栈指针
		MOV		DS,AX			; 设置数据段寄存器

; 从磁盘读取数据

		MOV		AX,0x0820		; 设置ES寄存器，用于存放读取的数据 (从磁盘读进来后放在内存的位置)
		MOV		ES,AX
		MOV		CH,0			; 设置CX寄存器的高8位，即柱面号，从0开始
		MOV		DH,0			; 设置DH寄存器，即磁头号，从0开始
		MOV		CL,2			; 设置CL寄存器，即起始扇区号，从2开始 (1在最开始已经读了)
readloop:
		MOV		SI,0			; 初始化SI寄存器，用于记录失败次数
retry:
		MOV		AH,0x02			; 设置AH寄存器，0x02表示读取磁盘
		MOV		AL,1			; 设置AL寄存器，表示要读取的扇区数，这里是1扇区
		MOV		BX,0			; 设置BX寄存器，通常用于存放缓冲区的段地址，这里未使用
		MOV		DL,0x00			; 设置DL寄存器，表示要读取的驱动器号，0x00表示A驱动器
		INT		0x13			; 调用磁盘BIOS中断，执行读取操作
		JNC		next			; 如果没有发生错误，跳转到next (JNC: Jump if Not Carry)
		ADD		SI,1			; 如果读取失败，增加失败次数
		CMP		SI,5			; 比较失败次数是否达到5次
		JAE		error			; 如果达到5次，跳转到error处理
		MOV		AH,0x00		    ; 重置磁盘操作
		MOV		DL,0x00			; 重置驱动器号
		INT		0x13			; 调用磁盘BIOS中断，执行重置操作
		JMP		retry			; 重试读取
next:
		MOV		AX,ES			; 增加ES寄存器的值，移动到下一个缓冲区
		ADD		AX,0x0020		; 由于ADD ES,0x020指令不可用，因此使用这种方式增加20h（32字节）

        ; 因为每个扇区的大小为512字节，在实模式下通常使用16位的段地址和偏移地址
        ; 所以这里通过增加 0x0020 来实际移动 0x0200（512字节），为下一个扇区的数据做准备。

		MOV		ES,AX			; 更新ES寄存器
		ADD		CL,1			; 增加CL寄存器的值，即下一个扇区号
		CMP		CL,18			; 比较CL是否达到每磁道的扇区数
		JBE		readloop		; 如果CL小于等于18，继续readloop
		MOV		CL,1			; 重置CL寄存器，即扇区号从1开始
		ADD		DH,1			; 增加DH寄存器的值，即下一个磁头号
		CMP		DH,2			; 比较DH是否达到磁头数
		JB		readloop		; 如果DH小于2，继续readloop
		MOV		DH,0			; 重置DH寄存器
		ADD		CH,1			; 增加CH寄存器的值，即下一个柱面号
		CMP		CH, CYLS		; 比较CH是否达到CYLS定义的柱面数
		JB		readloop		; 如果CH小于CYLS，继续readloop

; 读取完毕，执行haribote.sys

		MOV		[0x0ff0],CH		; 记录IPL读取到的柱面号
		JMP		0xc200			; 跳转到0xc200地址执行

error:
		MOV		SI,msg			; 加载错误消息的地址到SI
putloop:
		MOV		AL,[SI]			; 从SI指向的内存地址读取一个字节到AL
		ADD		SI,1			; 增加SI的值，指向下一个字节
		CMP		AL,0			; 比较AL是否为0，即字符串的结束
		JE		fin			    ; 如果是0，跳转到fin
		MOV		AH,0x0e			; 设置AH寄存器，0x0e表示字符显示功能
		MOV		BX,15			; 设置BX寄存器，15表示使用白色字符
		INT		0x10			; 调用视频BIOS中断，显示字符
		JMP		putloop		    ; 继续下一个字符
fin:
		HLT						; 停止CPU直到下一个中断
		JMP		fin			    ; 无限循环，直到系统重启或有其他中断

msg:
		DB		0x0a, 0x0a		; 定义两个换行符
		DB		"load error"	; 定义错误消息字符串
		DB		0x0a			; 定义一个换行符
		DB		0				; 字符串结束符

		RESB	0x7dfe-$		; 用0填充剩余空间直到0x7dfe (0x7dfe 本身不包括在内)

		DB		0x55, 0xaa		; 引导扇区的结束标志，必须为55AA
