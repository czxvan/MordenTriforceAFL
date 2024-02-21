#include "../patches/afl.h"

const char *peekStrZ(CPUArchState *env, target_ulong ptr, int maxlen);

/* return pointer to buf filled with strz from ptr[0..maxlen] */
const char *
peekStrZ(CPUArchState *env, target_ulong ptr, int maxlen)
{
    static char buf[0x1000];
    int i;
    if(maxlen > sizeof buf - 1)
        maxlen = sizeof buf - 1;
    for(i = 0; i < maxlen; i++) {
        char ch = cpu_ldub_data(env, ptr + i);
        if(!ch)
            break;
        buf[i] = ch;
    }
    buf[i] = 0;
    return buf;
}

void helper_aflInterceptLog(CPUArchState *env)
{
    if(!aflStart)
        return;
    aflGotLog = 1;

    FILE *fp = NULL;
    if(fp == NULL) {
        fp = fopen("logstore.txt", "a");
        if(fp) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            fprintf(fp, "\n----\npid %d time %ld.%06ld\n", getpid(), (u_long)tv.tv_sec, (u_long)tv.tv_usec);
        }
    }
    if(!fp) 
        return;

    target_ulong stack = env->regs[R_ESP];
    //target_ulong level = env->regs[R_ESI]; // arg 2
    target_ulong ptext = cpu_ldq_data(env, stack + 0x8); // arg7
    target_ulong len   = cpu_ldq_data(env, stack + 0x10) & 0xffff; // arg8
    const char *msg = peekStrZ(env, ptext, len);
    fprintf(fp, "%s\n", msg);
}

void helper_aflInterceptPanic(void)
{
    if(!aflStart)
        return;
    exit(32);
}