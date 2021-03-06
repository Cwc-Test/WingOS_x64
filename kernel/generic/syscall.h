#pragma once
#include <lock.h>
#include <stdint.h>
enum syscall_codes
{
    NULL_SYSCALL = 0,                 // or debug syscall
    SEND_SERVICE_SYSCALL = 1,         // send a message to a service
    READ_SERVICE_SYSCALL = 2,         // read all message that you have
    GET_RESPONSE_SERVICE_SYSCALL = 3, // if the message sended has been responded
    GET_PROCESS_GLOBAL_DATA = 4,      // get process global data, if arg1 (target) is nullptr, return self global data, else return a process global data return -1 if there is an error
    SEND_SERVICE_SYSCALL_PID = 5,
    MEMORY_ALLOC = 6, // pmm alloc
    MEMORY_FREE = 7,  // pmm free
    FILE_OPEN = 8,
    FILE_CLOSE = 9,
    FILE_READ = 10,
    FILE_WRITE = 11,
    FILE_SEEK = 12,
    NANO_SLEEP = 13,
    GET_PID = 14,
};
typedef long int time_t;
struct timespec
{
    time_t tv_sec; /* time in seconds */
    long tv_nsec;  /* time in nanoseconds */
};
extern lock_type lck_syscall;

void init_syscall();
uint64_t syscall(uint64_t syscall_id, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, InterruptStackFrame *stackframe);
