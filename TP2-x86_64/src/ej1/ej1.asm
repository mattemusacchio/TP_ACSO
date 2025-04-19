%define NULL 0
%define TRUE 1
%define FALSE 0

section .text
    global string_proc_list_create_asm
    global string_proc_node_create_asm
    global string_proc_list_add_node_asm
    global string_proc_list_concat_asm

    extern malloc
    extern free
    extern strlen
    extern strcpy
    extern str_concat


string_proc_list_create_asm:
    push rbp
    mov rdi, 16                ; sizeof(string_proc_list)
    call malloc
    test rax, rax
    je .return_null
    mov qword [rax], NULL      ; first = NULL
    mov qword [rax + 8], NULL  ; last = NULL
    pop rbp
    ret
.return_null:
    mov rax, NULL
    pop rbp
    ret

string_proc_node_create_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 32

    ; Guardamos argumentos
    mov eax, edi               ; type en eax
    mov [rbp - 32], rsi        ; hash
    mov [rbp - 20], al         ; type (como byte)

    ; malloc(sizeof(string_proc_node) = 32)
    mov edi, 32
    call malloc
    test rax, rax
    je .return_null_node

    ; Guardamos puntero retornado por malloc
    mov [rbp - 8], rax

    ; rax = puntero a string_proc_node
    mov rax, [rbp - 8]
    
    ; type
    movzx edx, byte [rbp - 20]
    mov byte [rax + 16], dl

    ; hash
    mov rdx, [rbp - 32]
    mov [rax + 24], rdx

    ; next = NULL
    mov qword [rax], 0

    ; previous = NULL
    mov qword [rax + 8], 0

    ; retornar el puntero al nodo
    mov rax, [rbp - 8]
    mov rsp, rbp
    pop rbp
    ret

.return_null_node:
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 32

    ; Guardamos argumentos
    mov [rbp - 8], rdi       ; list
    movzx eax, sil           ; type
    mov [rbp - 16], eax
    mov [rbp - 24], rdx      ; hash

    ; Llamamos a string_proc_node_create_asm(type, hash)
    movzx edi, byte [rbp - 16]
    mov rsi, [rbp - 24]
    call string_proc_node_create_asm
    test rax, rax
    je .fin
    mov [rbp - 32], rax      ; new_node

    ; if list->first == NULL
    mov rdi, [rbp - 8]       ; list
    mov rax, [rdi]           ; list->first
    test rax, rax
    jne .not_empty

    ; lista vacía
    mov rax, [rbp - 32]
    mov [rdi], rax           ; list->first = new_node
    mov [rdi + 8], rax       ; list->last = new_node
    jmp .fin

.not_empty:
    ; Obtener last
    mov rax, [rdi + 8]       ; list->last
    mov rdx, [rbp - 32]      ; new_node

    ; last->next = new_node
    mov [rax], rdx

    ; new_node->previous = last
    mov [rdx + 8], rax

    ; list->last = new_node
    mov [rdi + 8], rdx

.fin:
    mov rsp, rbp
    pop rbp
    ret


; char* string_proc_list_concat(string_proc_list* list, uint8_t type, char* hash);
string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 64                    ; Reservamos espacio en stack

    ; Guardamos argumentos
    mov [rbp - 8], rdi             ; list
    movzx eax, sil
    mov [rbp - 16], al             ; type
    mov [rbp - 24], rdx            ; hash

    ; Verificamos si list o hash son NULL
    cmp qword [rbp - 8], 0
    je .return_null
    cmp qword [rbp - 24], 0
    je .return_null

    ; Obtener longitud de hash
    mov rdi, [rbp - 24]
    call strlen
    add rax, 1                     ; espacio para '\0'
    mov rdi, rax
    call malloc
    test rax, rax
    je .return_null
    mov [rbp - 32], rax            ; result

    ; strcpy(result, hash)
    mov rsi, [rbp - 24]
    mov rdi, [rbp - 32]
    call strcpy

    ; current = list->first
    mov rax, [rbp - 8]
    mov rax, [rax]
    mov [rbp - 40], rax            ; current

.loop:
    cmp qword [rbp - 40], 0
    je .done

    ; if current->type == type
    mov rax, [rbp - 40]
    movzx eax, byte [rax + 16]
    cmp al, byte [rbp - 16]
    jne .skip_concat

    ; Llamamos a str_concat(result, current->hash)
    mov rax, [rbp - 40]
    mov rdx, [rax + 24]
    mov rdi, [rbp - 32]
    mov rsi, rdx
    call str_concat
    test rax, rax
    je .concat_fail
    mov [rbp - 48], rax            ; nuevo result

    ; liberar viejo result
    mov rdi, [rbp - 32]
    call free
    mov rax, [rbp - 48]
    mov [rbp - 32], rax

.skip_concat:
    ; current = current->next
    mov rax, [rbp - 40]
    mov rax, [rax]
    mov [rbp - 40], rax
    jmp .loop

.done:
    ; Agregar nuevo nodo a la lista con el string concatenado
    movzx ecx, byte [rbp - 16]     ; type
    mov rdx, [rbp - 32]            ; result (hash)
    mov rax, [rbp - 8]             ; list
    mov esi, ecx
    mov rdi, rax
    call string_proc_list_add_node_asm

    ; Devolver el string concatenado
    mov rax, [rbp - 32]
    leave
    ret

.concat_fail:
    ; liberar result si falló str_concat
    mov rdi, [rbp - 32]
    call free
    mov rax, 0
    leave
    ret

.return_null:
    mov rax, 0
    leave
    ret
