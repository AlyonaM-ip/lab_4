#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

int shared_variable = 42; // Общая глобальная переменная

void* thread_func(void* arg) {
    long tid = syscall(SYS_gettid);
    printf("Дочерний поток [TID: %ld]: Адрес переменной = %p, значение = %d\n", tid, &shared_variable, shared_variable);
    while(1) sleep(1);
    return NULL;
}

int main() {
    pthread_t thread;
    printf("Главный поток   [PID: %d]: Адрес переменной = %p, значение = %d\n", getpid(), &shared_variable, shared_variable);
    
    pthread_create(&thread, NULL, thread_func, NULL);
    while(1) sleep(1);
    return 0;
}
