.text

.globl ndsl_start_coroutine
.type ndsl_start_coroutine, @function
ndsl_start_coroutine:
    # save current registers state
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # save current sp
    movq  %rsp, (%rdi)
    # compute real stack base(x86-64 use descending stack)
    addq  %rcx, %rdx
    # gnu gas x86-64 require stack to be aligned to 128 bytes 
    andq  $0xfffffffffffffff0, %rdx
    subq  $0x8, %rdx

    # switch to coroutine stack
    movq  %rdx, %rsp
    # set rbp stack base
    movq  %rsp, %rbp
    # jump target function
    jmp   *%rsi

.globl ndsl_swap_to
.type ndsl_swap_to, @function
ndsl_swap_to:
    # save current registers state
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # save current sp
    movq  %rsp, (%rdi)
    # swtich stack to caller
    movq  %rsi, %rsp

    # restore caller registers
    popq  %r15
    popq  %r14
    popq  %r13
    popq  %r12
    popq  %r11
    popq  %r10
    popq  %r9
    popq  %r8
    popq  %rsi
    popq  %rdi
    popq  %rbp
    popq  %rdx
    popq  %rcx
    popq  %rbx
    popq  %rax

    xorq  %rax, %rax
    # caller resume return
    ret
    