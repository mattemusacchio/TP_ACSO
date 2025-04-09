; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares que pueden llegar a necesitar:
extern malloc
extern free
extern str_concat
extern strlen
extern strcpy

; typedef struct string_proc_list_t {
;     struct string_proc_node_t* first;
;     struct string_proc_node_t* last;
; } string_proc_list;

; string_proc_list* string_proc_list_create(void);
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

; string_proc_node* string_proc_node_create(uint8_t type, char* hash);
string_proc_node_create_asm:
    push rbp
    mov rdi, 32              ; sizeof(string_proc_node)
    call malloc
    test rax, rax
    je .return_null_node
    mov qword [rax], NULL      ; next
    mov qword [rax + 8], NULL  ; previous
    movzx rcx, dil             ; type (en dil, se expande a 64 bits)
    mov byte [rax + 16], cl
    mov rdx, rsi               ; hash (rsi es el segundo argumento)
    mov qword [rax + 24], rdx  ; hash
    pop rbp
    ret
.return_null_node:
    mov rax, NULL
    pop rbp
    ret

; void string_proc_list_add_node(string_proc_list* list, uint8_t type, char* hash);
string_proc_list_add_node_asm:
    push rbp
    mov r9, rdi        ; guardar list
    movzx r8, sil        ; guardar type

    ; llamar a string_proc_node_create_asm(type, hash)
    mov rdi, r8
    mov rsi, rdx
    call string_proc_node_create_asm
    test rax, rax
    je .fin
    mov r10, rax       ; nuevo nodo

    ; if list->first == NULL
    mov rax, [r9]
    test rax, rax
    jne .not_empty

    ; lista vacía
    mov [r9], r10       ; list->first = new_node
    mov [r9 + 8], r10   ; list->last = new_node
    jmp .fin

.not_empty:
    ; nodo anterior = list->last
    mov rax, [r9 + 8]      ; rax = list->last
    mov [rax], r10         ; last->next = new_node
    mov [r10 + 8], rax     ; new_node->previous = last
    mov [r9 + 8], r10      ; list->last = new_node

.fin:
    pop rbp
    ret

; char* string_proc_list_concat(string_proc_list* list, uint8_t type, char* hash);
string_proc_list_concat_asm:
    push rbp
    mov r9, rdi        ; list
    movzx r8, sil        ; type

    ; verificar que list != NULL && hash != NULL
    test r9, r9
    je .return_null
    test rdx, rdx
    je .return_null

    ; rdx = hash
    push r12
    mov r12, rdx
    mov rdi, rdx          ; strlen(hash)
    call strlen
    mov rbx, rax          ; len = strlen(hash)
    inc rax               ; len + 1
    mov rdi, rax
    call malloc
    test rax, rax
    je .return_null

    ; Copiar hash en result
    mov rsi, r12          ; src = hash
    mov rdi, rax          ; dest = malloc'ed buffer
    call strcpy
    pop r12
    mov r11, rax          ; result = return de strcpy (es igual a malloc'ed ptr)


    ; recorrer la lista
    mov r10, [r9]       ; current = list->first
.loop:
    test r10, r10
    je .done

    ; if current->type == type
    movzx eax, byte [r10 + 16]
    cmp al, r8b
    jne .next

    ; llamar a str_concat(result, current->hash)
    mov rdi, r11
    mov rsi, [r10 + 24]
    call str_concat
    test rax, rax
    je .concat_fail

    ; liberar memoria anterior y actualizar result
    mov rdi, r11
    call free
    mov r11, rax

.next:
    mov r10, [r10]       ; current = current->next
    jmp .loop

.done:
    mov rax, r11
    pop rbp
    ret

.concat_fail:
    ; liberar memoria anterior
    mov rdi, r11
    call free
    mov rax, NULL
    pop rbp
    ret

.return_null:
    mov rax, NULL
    pop rbp
    ret
