; pmtest1.asm
; compile: nasm pmtest1.asm -o pmtest1.com
%include "pm.inc"
org 0100h
    jmp LABEL_BEGIN

[SECTION    .gdt]
; GDT
LABEL_GDT:          Descriptor 0, 0, 0
LABEL_DESC_CODE32:  Descriptor 0, SegCode32Len - 1, DA_C + DA_32
LABEL_DESC_VIDEO:   Descriptor 0B8000h, 0FFFFh, DA_DRW
; GDT end

GdtLen  equ $ - LABEL_GDT
GdtPtr  dw  GdtLen
        dd  0

SelectorCode32      equ LABEL_DESC_CODE32   - LABEL_GDT
SelectorVideo       equ LABEL_DESC_VIDEO    - LABEL_GDT

[SECTION    .s16]
[BITS   16]
LABEL_BEGIN:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0100h

    ; initialize
    xor eax, eax
    mov ax,  cs
    shl eax, 4
    add eax, LABEL_SEG_CODE32
    mov word [LABEL_DESC_CODE32 + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_CODE32 + 4], al
    mov byte [LABEL_DESC_CODE32 + 7], ah

    xor eax, eax
    mov ax,  ds
    shl eax, 4
    add eax, LABEL_GDT
    mov dword [GdtPtr + 2], eax

    lgdt [GdtPtr]

    ; 关中断
    cli

    ; 打开A20地址线
    in  al,  92h
    or  al, 00000010b
    out 92h, al

    ; 准备进入保护模式
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    ; 真正进入保护模式
    jmp dword   SelectorCode32:0    ; load SelectorCode32 to cs
                                    ; and jump to SelectorCode32:0

[SECTION    .s32]; 32位代码段
[BITS   32]

LABEL_SEG_CODE32:
    mov ax,  SelectorVideo
    mov gs,  ax
    
    mov edi, (80 * 10 + 0) * 2  ; raw 10, col 0
    mov ah,  0Ch
    mov al,  'P'
    mov [gs:edi], ax

    ; the end
    jmp fin

fin:
    hlt
    jmp fin

SegCode32Len    equ $ - LABEL_SEG_CODE32
; end of [SECTION .s32]
