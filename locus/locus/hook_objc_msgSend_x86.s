.text
.global _hook_objc_msgSend_x86

_hook_objc_msgSend_x86:
pushq  %rbp
movq %rsp, %rbp
subq $0x210, %rsp

movq  %xmm15, -0x210(%rbp)
movq  %xmm14, -0x200(%rbp)
movq  %xmm13, -0x190(%rbp)
movq  %xmm12, -0x180(%rbp)
movq  %xmm11, -0x170(%rbp)
movq  %xmm10, -0x160(%rbp)
movq  %xmm9, -0x150(%rbp)
movq  %xmm8, -0x140(%rbp)
movq  %xmm7, -0x130(%rbp)
movq  %xmm6, -0x120(%rbp)
movq  %xmm5, -0x110(%rbp)
movq  %xmm4, -0x100(%rbp)
movq  %xmm3, -0x90(%rbp)
movq  %xmm2, -0x80(%rbp)
movq  %xmm1, -0x70(%rbp)
movq  %xmm0, -0x60(%rbp)
movq  %rbx, -0x58(%rbp)
movq  %rdi, -0x50(%rbp)
movq  %rsi, -0x48(%rbp)
movq  %rdx, -0x40(%rbp)
movq  %rcx, -0x38(%rbp)
movq  %r8, -0x30(%rbp)
movq  %r9, -0x28(%rbp)
movq  %r12, -0x20(%rbp)
movq  %r13, -0x18(%rbp)
movq  %r14, -0x10(%rbp)
movq  %r15, -0x8(%rbp)

movq 0x8(%rbp), %r9

call _before_objc_msgSend

movq   -0x210(%rbp), %xmm15
movq   -0x200(%rbp), %xmm14
movq   -0x190(%rbp), %xmm13
movq   -0x180(%rbp), %xmm12
movq   -0x170(%rbp), %xmm11
movq   -0x160(%rbp), %xmm10
movq   -0x150(%rbp), %xmm9
movq   -0x140(%rbp), %xmm8
movq   -0x130(%rbp), %xmm7
movq   -0x120(%rbp), %xmm6
movq   -0x110(%rbp), %xmm5
movq   -0x100(%rbp), %xmm4
movq   -0x90(%rbp), %xmm3
movq   -0x80(%rbp), %xmm2
movq   -0x70(%rbp), %xmm1
movq   -0x60(%rbp), %xmm0
movq   -0x58(%rbp), %rbx
movq   -0x50(%rbp), %rdi
movq   -0x48(%rbp), %rsi
movq   -0x40(%rbp), %rdx
movq   -0x38(%rbp), %rcx
movq   -0x30(%rbp), %r8
movq   -0x28(%rbp), %r9
movq   -0x20(%rbp), %r12
movq   -0x18(%rbp), %r13
movq   -0x10(%rbp), %r14
movq   -0x8(%rbp), %r15

addq $0x210, %rsp

popq %rbp
addq $0x8, %rsp
call *%rax
pushq %rax
pushq %rdx

subq $0x20, %rsp
movq  %xmm0, 0x0(%rsp)
movq  %xmm1, 0x10(%rsp)
call _get_lr
movq %rax, %r10
movq  0x0(%rsp), %xmm0
movq  0x10(%rsp), %xmm1
addq $0x20, %rsp

popq %rdx
popq %rax
push %r10
retq
ret
