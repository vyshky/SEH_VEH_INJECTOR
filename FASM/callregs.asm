format MS64 COFF

section '.text' code readable executable

public callProcessFunctionEx

;✔ Базовое правило Windows x64 ABI:
;| Момент                  | значение                                                                |
;| ----------------------- | ------------------------------------------------------------------------|
;| после `call`            |   RSP % 16 == 8                                                         |
;| перед любым `call`      |   RSP % 16 == 0                                                         |
;| shadow space (32 байта) | не влияет на alignment, но обязательно, он должен быть больше           |
;|                         |  всех пушей по байтам, так как много push reg могут выйти за шадоу спейс|
;|                         |   и шадой спейс будет указывать на те пуши в стеке                      |
;|                         |    и сломает их после вызова функции call                               |



; void callProcessFunctionEx(void* ADDRESS, CPU_REGS* REGS)
; После call стек всегда не выровнен == 8,
; из-за того что rip пушится на стек == 8 
; push rbx выравнивает == 0
; push rbp ломает == 8
; push rsi выравнивает == 0
; push rdi ломает == 8


callProcessFunctionEx:
    ; Вход:
    ;   RCX = адрес функции для вызова
    ;   RDX = указатель на структуру CPU_REGS
    
    ; Сохраняем non-volatile регистры
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    
    ; Выделяем shadow space и выравниваем стек
    ; После 8 push (64 байта) RSP уже выровнен по 16
    ; Выделяем еще 32 байта shadow space
    sub rsp, 32
    
    ; Сохраняем адрес функции и указатель на структуру
    mov r14, rcx        ; R14 = адрес функции
    mov r15, rdx        ; R15 = указатель на структуру
    
    ; Загружаем volatile регистры из структуры
    mov rax, [r15 + 0]    ; RAX
    mov rcx, [r15 + 16]   ; RCX (первый параметр)
    mov rdx, [r15 + 24]   ; RDX (второй параметр)
    mov r8,  [r15 + 56]   ; R8  (третий параметр)
    mov r9,  [r15 + 64]   ; R9  (четвертый параметр)
    mov r10, [r15 + 72]   ; R10
    mov r11, [r15 + 80]   ; R11
    
    ; Загружаем non-volatile регистры из структуры
    ; (кроме R14, R15 которые уже используются)
    mov rbx, [r15 + 8]    ; RBX
    mov rsi, [r15 + 40]   ; RSI
    mov rdi, [r15 + 48]   ; RDI
    mov r12, [r15 + 88]   ; R12
    mov r13, [r15 + 96]   ; R13
    
    ; RBP особый случай - не загружаем из структуры, 
    ; чтобы не сломать стековый фрейм
    
    ; Вызываем функцию
    call r14
    
    ; Сохраняем volatile регистры обратно в структуру
    mov [r15 + 0], rax    ; RAX (результат)
    mov [r15 + 16], rcx   ; RCX
    mov [r15 + 24], rdx   ; RDX
    mov [r15 + 56], r8    ; R8
    mov [r15 + 64], r9    ; R9
    mov [r15 + 72], r10   ; R10
    mov [r15 + 80], r11   ; R11
    
    ; Сохраняем non-volatile регистры
    mov [r15 + 8], rbx    ; RBX
    mov [r15 + 40], rsi   ; RSI
    mov [r15 + 48], rdi   ; RDI
    mov [r15 + 88], r12   ; R12
    mov [r15 + 96], r13   ; R13
    
    ; Восстанавливаем shadow space
    add rsp, 32
    
    ; Восстанавливаем non-volatile регистры
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    
    ret

; Регистр	Caller-saved (можно портить)	Callee-saved (обязан восстановить)
; RAX                    ✔                       ✘
; RCX                    ✔                       ✘
; RDX                    ✔                       ✘
; R8                     ✔                       ✘
; R9                     ✔                       ✘
; R10                    ✔                       ✘
; R11                    ✔                       ✘
; RBX                    ✘                       ✔
; RBP                    ✘                       ✔ (если используешь как frame ptr)
; RSI                    ✘                       ✔
; RDI                    ✘                       ✔
; R12                    ✘                       ✔
; R13                    ✘                       ✔
; R14                    ✘                       ✔
; R15                    ✘                       ✔
; RSP                    ✘                       ✔ (обязан вернуть как был!)