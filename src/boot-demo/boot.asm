;%define    _BOOT_DEBUG_

%ifdef _BOOT_DEBUG_
    org 0100h           ; debug mode, output .com file (running on DOS system)
%else
    org 07c00h          ; boot mode
%endif

    mov ax, cs
    mov ds, ax
    mov es, ax
    call    DispStr
    jmp $
DispStr:
    mov ax, BootMessage
    mov bp, ax          ; load BootMessage address
    mov cx, 16          ; length of BootMessage
    mov ax, 01301h      ; ah = 13h, al = 01h
    mov bx, 000ch       ; bh = 0h (0 page), bl = 0ch (black background, red highlight font)
    mov dl, 0
    int 10h             ; 10h interrupt
    ret
BootMessage:        db  "Hello, OS World!"
times   510-($-$$)  db  0   ; fill the rest by zero
                            ;
dw      0xaa55              ; finish mark
