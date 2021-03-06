//
//  hookObjcSend.c
//  fishhookdemo
//
//  Created by FanFamily on 2018/7/21.
//  Copyright © 2018年 Family Fan. All rights reserved.
//

#include "locusImpl.h"

#include "fishhook.h"
#include <pthread.h>
#import <objc/message.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <dispatch/dispatch.h>
#import <objc/runtime.h>
#include "LocusArgPrinter.h"

static pthread_t _main_ptread;

static int lcs_print = 0;
static int lcs_switch_open = 1;
static int lcs_switch_close = 0;
static pthread_key_t _thread_lr_stack_key;
static LCSFilterBlock _filter_block = NULL;
static long _due = 0;
static pthread_key_t _thread_switch_key;
static char _log_path[1024];
static FILE * _log_file = NULL;
__unused static id (*orig_objc_msgSend)(id, SEL, ...);

typedef struct {
    void *pre;
    void *next;
    uintptr_t lr;
    long long time;
    char* obj;
    char* sel;
    long level;
} thread_lr_stack;

void lcs_open()
{
    pthread_setspecific(_thread_switch_key, &lcs_switch_open);
}

void lcs_close()
{
    pthread_setspecific(_thread_switch_key, &lcs_switch_close);
}

void before_objc_msgSend(id self, SEL sel, ...) {
    void* p_switch = pthread_getspecific(_thread_switch_key);
    int lcs_switch = 1;
    if (p_switch == NULL) {
        lcs_open();
    } else {
        lcs_switch = *(int *)p_switch;
    }
    
    int (^filter_block)(void) = ^(){
        if (_filter_block == NULL) {
            return 0;
        }
        int result = 0;
        lcs_close();
        result = _filter_block((char *)object_getClassName(self), (char *)sel);
        lcs_open();
        return result;
    };

    int filter_result = 0;
    if (lcs_print > 0 && lcs_switch > 0) {
        filter_result = filter_block();
        if (filter_result > 0) {
            printf("class %s, selector %s\n", (char *)object_getClassName(self), (char *)sel);
        }
        
        if (filter_result > 1) {
            // arm64 not support
#ifndef __arm64__
            lcs_close();
            va_list argptr;
            va_start(argptr, sel);
            printArgs((char *)object_getClassName(self), (char *)sel, argptr);
            va_end(argptr);
            lcs_open();
#endif
        }
    }

}

long long lcs_getCurrentTime() {
    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

uintptr_t save_lr(id self, SEL sel, uintptr_t lr)
{
    thread_lr_stack* ls = pthread_getspecific(_thread_lr_stack_key);
    if (ls == NULL) {
        ls = malloc(sizeof(thread_lr_stack));
        ls->pre = NULL;
        ls->next = NULL;
        ls->lr = lr;
        ls->obj = (char *)object_getClassName(self);
        ls->sel = (char *)sel;
        ls->time = lcs_getCurrentTime();
        ls->level = 0;
        pthread_setspecific(_thread_lr_stack_key, (void *)ls);
    } else {
        thread_lr_stack* next = malloc(sizeof(thread_lr_stack));
        ls->next = next;
        next->pre = ls;
        next->next = NULL;
        next->lr = lr;
        next->obj = (char *)object_getClassName(self);
        next->sel = (char *)sel;
        next->time = lcs_getCurrentTime();
        next->level = ls->level + 1;
        pthread_setspecific(_thread_lr_stack_key, (void *)next);
    }
    return (uintptr_t)orig_objc_msgSend;
}

void write_method_due_log(char* obj, char* sel, long due, long level) {
    if (_main_ptread == pthread_self()) {
        if (level == 0) {
            fprintf(_log_file, "↑ due time %ld [%s %s]\n", due, obj, sel);
        } else {
            fprintf(_log_file, "due time %ld [%s %s]\n", due, obj, sel);
        }
    }
}

uintptr_t get_lr() {
    thread_lr_stack* ls = pthread_getspecific(_thread_lr_stack_key);
    pthread_setspecific(_thread_lr_stack_key, (void *)ls->pre);
    uintptr_t lr = ls->lr;
    long due = (long)(lcs_getCurrentTime() - ls->time);
    if (_due > 0 && due > _due) {
        write_method_due_log(ls->obj, ls->sel, due, ls->level);
    }
    free(ls);
    return lr;
}

void lcs_stop_print() {
    lcs_print = 0;
}

void lcs_resume_print() {
    lcs_print = 1;
}

extern id hook_objc_msgSend(id, SEL, ...);

#ifdef __arm64__

__asm__  (
          ".text\n"
          ".global _hook_objc_msgSend\n"
          
          "_hook_objc_msgSend:\n"
          
          "stp q6, q7, [sp, #-32]!\n"
          "stp q4, q5, [sp, #-32]!\n"
          "stp q2, q3, [sp, #-32]!\n"
          "stp q0, q1, [sp, #-32]!\n"
          
          "stp x6, x7, [sp, #-16]!\n"
          "stp x4, x5, [sp, #-16]!\n"
          "stp x2, x3, [sp, #-16]!\n"
          "stp x0, x1, [sp, #-16]!\n"
          "stp x8, lr, [sp, #-16]!\n"
          
          "bl _before_objc_msgSend\n"
          
          "ldp x8, lr, [sp], #16\n"
          "ldp x0, x1, [sp], #16\n"
          "stp x0, x1, [sp, #-16]!\n"
          "stp x8, lr, [sp, #-16]!\n"
          
          "mov x2, lr\n"
          
          "bl _save_lr\n"
          
          "mov x9, x0\n"
          
          "ldp x8, lr, [sp], #16\n"
          "ldp x0, x1, [sp], #16\n"
          "ldp x2, x3, [sp], #16\n"
          "ldp x4, x5, [sp], #16\n"
          "ldp x6, x7, [sp], #16\n"
          
          "ldp q0, q1, [sp], #32\n"
          "ldp q2, q3, [sp], #32\n"
          "ldp q4, q5, [sp], #32\n"
          "ldp q6, q7, [sp], #32\n"
          
          "blr x9\n"
          "stp x0, x1, [sp, #-16]!\n"
          "stp q0, q1, [sp, #-32]!\n"
          "bl _get_lr\n"
          "mov lr, x0\n"
          "ldp q0, q1, [sp], #32\n"
          "ldp x0, x1, [sp], #16\n"
          "ret\n"
          
          "ret\n"
);

#elif __LP64__
__asm__  (
          ".text\n"
          ".global _hook_objc_msgSend\n"
          
          "_hook_objc_msgSend:\n"
          "pushq  %rbp\n"
          "movq %rsp, %rbp\n"
          "subq $0x210, %rsp\n"
          
          "movq  %xmm15, -0x210(%rbp)\n"
          "movq  %xmm14, -0x200(%rbp)\n"
          "movq  %xmm13, -0x190(%rbp)\n"
          "movq  %xmm12, -0x180(%rbp)\n"
          "movq  %xmm11, -0x170(%rbp)\n"
          "movq  %xmm10, -0x160(%rbp)\n"
          "movq  %xmm9, -0x150(%rbp)\n"
          "movq  %xmm8, -0x140(%rbp)\n"
          "movq  %xmm7, -0x130(%rbp)\n"
          "movq  %xmm6, -0x120(%rbp)\n"
          "movq  %xmm5, -0x110(%rbp)\n"
          "movq  %xmm4, -0x100(%rbp)\n"
          "movq  %xmm3, -0x90(%rbp)\n"
          "movq  %xmm2, -0x80(%rbp)\n"
          "movq  %xmm1, -0x70(%rbp)\n"
          "movq  %xmm0, -0x60(%rbp)\n"
          "movq  %rbx, -0x58(%rbp)\n"
          "movq  %rdi, -0x50(%rbp)\n"
          "movq  %rsi, -0x48(%rbp)\n"
          "movq  %rdx, -0x40(%rbp)\n"
          "movq  %rcx, -0x38(%rbp)\n"
          "movq  %r8, -0x30(%rbp)\n"
          "movq  %r9, -0x28(%rbp)\n"
          "movq  %r12, -0x20(%rbp)\n"
          "movq  %r13, -0x18(%rbp)\n"
          "movq  %r14, -0x10(%rbp)\n"
          "movq  %r15, -0x8(%rbp)\n"
          
          "call _before_objc_msgSend\n"
          
          "movq   -0x50(%rbp), %rdi\n"
          "movq   -0x48(%rbp), %rsi\n"
          "movq 0x8(%rbp), %rdx\n"
          
          "call _save_lr\n"
          
          "movq   -0x210(%rbp), %xmm15\n"
          "movq   -0x200(%rbp), %xmm14\n"
          "movq   -0x190(%rbp), %xmm13\n"
          "movq   -0x180(%rbp), %xmm12\n"
          "movq   -0x170(%rbp), %xmm11\n"
          "movq   -0x160(%rbp), %xmm10\n"
          "movq   -0x150(%rbp), %xmm9\n"
          "movq   -0x140(%rbp), %xmm8\n"
          "movq   -0x130(%rbp), %xmm7\n"
          "movq   -0x120(%rbp), %xmm6\n"
          "movq   -0x110(%rbp), %xmm5\n"
          "movq   -0x100(%rbp), %xmm4\n"
          "movq   -0x90(%rbp), %xmm3\n"
          "movq   -0x80(%rbp), %xmm2\n"
          "movq   -0x70(%rbp), %xmm1\n"
          "movq   -0x60(%rbp), %xmm0\n"
          "movq   -0x58(%rbp), %rbx\n"
          "movq   -0x50(%rbp), %rdi\n"
          "movq   -0x48(%rbp), %rsi\n"
          "movq   -0x40(%rbp), %rdx\n"
          "movq   -0x38(%rbp), %rcx\n"
          "movq   -0x30(%rbp), %r8\n"
          "movq   -0x28(%rbp), %r9\n"
          "movq   -0x20(%rbp), %r12\n"
          "movq   -0x18(%rbp), %r13\n"
          "movq   -0x10(%rbp), %r14\n"
          "movq   -0x8(%rbp), %r15\n"
          
          "addq $0x210, %rsp\n"
          
          "popq %rbp\n"
          "addq $0x8, %rsp\n"
          "call *%rax\n"
          "pushq %rax\n"
          "pushq %rdx\n"
          
          "subq $0x20, %rsp\n"
          "movq  %xmm0, 0x0(%rsp)\n"
          "movq  %xmm1, 0x10(%rsp)\n"
          "call _get_lr\n"
          "movq %rax, %r10\n"
          "movq  0x0(%rsp), %xmm0\n"
          "movq  0x10(%rsp), %xmm1\n"
          "addq $0x20, %rsp\n"
          
          "popq %rdx\n"
          "popq %rax\n"
          "push %r10\n"
          "retq\n"
          "ret\n"
          );
#else
    id hook_objc_msgSend(id obj, SEL sel, ...) {
        return nil;
    }
#endif

void lcs_start(LCSFilterBlock filter) {
    
    _filter_block = filter;
    
    lcs_resume_print();
    
    pthread_key_create(&_thread_switch_key, NULL);
    pthread_key_create(&_thread_lr_stack_key, NULL);
    rebind_symbols((struct rebinding[1]){{"objc_msgSend", hook_objc_msgSend, (void *)&orig_objc_msgSend}}, 1);
}

void lcs_start_performance(long ms, char* log_path) {
    
    memset(_log_path, 0, 1024);
    memcpy(_log_path, log_path, strlen(log_path));
    
    _log_file = fopen(log_path, "w");
    if (_log_file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    
    _due = ms;
    _main_ptread = pthread_self();
    pthread_key_create(&_thread_lr_stack_key, NULL);
    rebind_symbols((struct rebinding[1]){{"objc_msgSend", hook_objc_msgSend, (void *)&orig_objc_msgSend}}, 1);
}


