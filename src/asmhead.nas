; haribote-os boot asm
; TAB=4

BOTPAK	EQU		0x00280000		; bootpack的加载地址
DSKCAC	EQU		0x00100000		; 磁盘缓存的位置
DSKCAC0	EQU		0x00008000		; 磁盘缓存的位置（实模式）

; BOOT_INFO相关
CYLS	EQU		0x0ff0			; 引导扇区设置的
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; 颜色信息，多少位颜色？
SCRNX	EQU		0x0ff4			; 分辨率X
SCRNY	EQU		0x0ff6			; 分辨率Y
VRAM	EQU		0x0ff8			; 图形缓冲区的起始地址

		ORG		0xc200			; 这个程序被加载到的位置

; 设置屏幕模式

		MOV		AL,0x13			; VGA图形，320x200x8bit颜色
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 记录屏幕模式供C语言参考
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; 从BIOS获取键盘LED状态

		MOV		AH,0x02
		INT		0x16 			; 键盘BIOS
		MOV		[LEDS],AL

; 让PIC不接受任何中断
; 在AT兼容机的规范中，如果初始化PIC，不先执行CLI，有时会挂起
; PIC的初始化稍后进行

		MOV		AL,0xff			; 这是一个掩码值，用于设置中断掩码寄存器（IMR），0xff表示屏蔽全部
		OUT		0x21,AL			; PIC0_IMR = 0x21, 主PIC
		NOP						; 有些机型如果OUT指令连续执行可能无法正常工作
		OUT		0xa1,AL			; PIC1_IMR = 0xa1，从PIC

		CLI						; 同样在CPU级别禁止中断

; 允许CPU访问1MB以上的内存，设置A20GATE

		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL			; io_out8(PORT_KEYCMD, KEYCMD_WRITE_OUTPORT);
		CALL	waitkbdout
		MOV		AL,0xdf			; 启用A20
		OUT		0x60,AL
		CALL	waitkbdout

; 进入保护模式

[INSTRSET "i486p"]				; 表示希望使用486指令集

		LGDT	[GDTR0]			; 设置临时全局描述符表（GDT）
		MOV		EAX,CR0			; CR0 是一个控制寄存器，只有操作系统能操作它
		AND		EAX,0x7fffffff	; 将位31设置为0（禁止分页）
		OR		EAX,0x00000001	; 将位0设置为1（进入保护模式）
		MOV		CR0,EAX			; 这一步正式使得处理器从实模式切换到保护模式。
		JMP		pipelineflush	; 切换到保护模式后必须要执行JMP，否则会有指令残留在流水线上
								; 因为模式变了，就要重新解释一遍，所以加入了JMP指令。
pipelineflush:
		MOV		AX,1*8			; 读写可执行的32位段
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; 传输bootpack

		MOV		ESI,bootpack	; 源地址
		MOV		EDI,BOTPAK		; 目的地址
		MOV		ECX,512*1024/4
		CALL	memcpy

; 顺便将磁盘数据也转移到原本的位置

; 首先是从引导扇区开始

		MOV		ESI,0x7c00		; 源地址
		MOV		EDI,DSKCAC		; 目的地址
		MOV		ECX,512/4		; 转送数据大小是以双字为单位的，所以数据大小用字节数除以4
		CALL	memcpy

; 剩余的全部

		MOV		ESI,DSKCAC0+512	; 源地址
		MOV		EDI,DSKCAC+512	; 目的地址
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面数转换为字节数/4
		SUB		ECX,512/4		; 减去IPL的部分
		CALL	memcpy

; 在asmhead中必须完成的事情已经全部完成，
; 接下来交给bootpack处理

; 启动bootpack

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有需要传输的东西
		MOV		ESI,[EBX+20]	; 源地址
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 目的地址
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 栈初始值
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		AL, 0x64
		AND		AL, 0x02		; 与操作，只保留AL的第2位（键盘输出缓冲器空标志位）
		IN 		AL, 0x60 		; 空读（为了清空数据接收缓冲区中的垃圾数据）
		JNZ		waitkbdout		; 如果AND的结果不为0则跳转到waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 如果减法的结果不为0则跳转到memcpy
		RET
; 如果没有忘记放入地址大小前缀，memcpy也可以用字符串指令来写

		ALIGNB	16				; 对齐地址（到地址能被16整除的时候）
GDT0:
		RESB	8				; 空选择符
		DW		0xffff,0x0000,0x9200,0x00cf	; 读写可执行的32位段
		DW		0xffff,0x0000,0x9a28,0x0047	; 可执行的32位段（bootpack用）

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:						; bootpack.c的代码将被填充到后面
