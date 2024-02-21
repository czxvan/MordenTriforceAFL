#include "../patches/afl.h"

extern target_ulong afl_start_code, afl_end_code;
target_ulong startForkserver(CPUArchState *env, target_ulong enableTicks);
target_ulong startWork(CPUArchState *env, target_ulong ptr);
target_ulong getWork(CPUArchState *env, target_ulong ptr, target_ulong sz);
target_ulong doneWork(target_ulong val);
const char *peekStrZ(CPUArchState *env, target_ulong ptr, int maxlen);

target_ulong startForkserver(CPUArchState *env, target_ulong enableTicks)
{
    //printf("pid %d: startForkServer\n", getpid()); fflush(stdout);
    if(afl_fork_child) {
        /* 
         * we've already started a fork server. perhaps a test case
         * accidentally triggered startForkserver again.  Exit the
         * test case without error.
         */
        exit(0);
    }
#ifdef CONFIG_USER_ONLY
    /* we're running in the main thread, get right to it! */
    afl_setup();
    afl_forkserver(env);
#else
    /*
     * we're running in a cpu thread. we'll exit the cpu thread
     * and notify the iothread.  The iothread will run the forkserver
     * and in the child will restart the cpu thread which will continue
     * execution.
     * N.B. We assume a single cpu here!
     */
    aflEnableTicks = enableTicks;
    afl_wants_cpu_to_stop = 1;
#endif
    return 0;
}

/* copy work into ptr[0..sz].  Assumes memory range is locked. */
target_ulong getWork(CPUArchState *env, target_ulong ptr, target_ulong sz)
{
    target_ulong retsz;
    FILE *fp;
    unsigned char ch;
    qemu_log("get_work!\n");
    //printf("pid %d: getWork %lx %lx\n", getpid(), ptr, sz);fflush(stdout);
    assert(aflStart == 0);
    fp = fopen(aflFile, "rb");
    if(!fp) {
         perror(aflFile);
         return -1;
    }
    retsz = 0;
    while(retsz < sz) {
        if(fread(&ch, 1, 1, fp) == 0)
            break;
        cpu_stb_data(env, ptr, ch);
        retsz ++;
        ptr ++;
    }
    fclose(fp);
    return retsz;
}

target_ulong startWork(CPUArchState *env, target_ulong ptr)
{
    target_ulong start, end;

    // qemu_log("pid %d: ptr %lx\n", getpid(), ptr);
    start = cpu_ldq_data(env, ptr);
    end = cpu_ldq_data(env, ptr + sizeof start);
    qemu_log("pid %d: startWork %lx - %lx\n", getpid(), start, end);

    afl_start_code = start;
    afl_end_code   = end;
    aflGotLog = 0;
    aflStart = 1;
    // qemu_log("aflStart = %d\n", aflStart);
    return 0;
}

target_ulong doneWork(target_ulong val)
{
    //printf("pid %d: doneWork %lx\n", getpid(), val);fflush(stdout);
    assert(aflStart == 1);
/* detecting logging as crashes hasnt been helpful and
   has occasionally been a problem.  We'll leave it to
   a post-analysis phase to look over dmesg output for
   our corpus.
 */
#ifdef LETSNOT 
    if(aflGotLog)
        exit(64 | val);
#endif
    exit(val); /* exit forkserver child */
}

uint32_t helper_aflCall32(CPUArchState *env, uint32_t code, uint32_t a0, uint32_t a1) {
    return (uint32_t)helper_aflCall(env, code, a0, a1);
}

uint64_t helper_aflCall(CPUArchState *env, uint64_t code, uint64_t a0, uint64_t a1) {
    qemu_log("helper_aflCall(%p, %ld, %ld, %ld)\n", env, code, a0, a1);
    switch(code) {
    case 1: return startForkserver(env, a0);
    case 2: return getWork(env, a0, a1);
    case 3: return startWork(env, a0);
    case 4: return doneWork(a0);
    default: return -1;
    }
}

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