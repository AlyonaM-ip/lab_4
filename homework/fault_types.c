#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define PAGE_SIZE 4096

void run_demand_zero() {
    // 100 страниц анонимной памяти
    void *anon = mmap(NULL, PAGE_SIZE * 100, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    for (int i = 0; i < 100; i++) {
        *((char *)anon + i * PAGE_SIZE) = 1; /* Ожидаем ~100 minor/demand-zero faults */
    }
    munmap(anon, PAGE_SIZE * 100);
}

void run_cow_fault() {
    void *shared = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    *((char *)shared) = 42; // Выделяем физическую страницу в родителе
    
    pid_t pid = fork();
    if (pid == 0) {
        *((char *)shared) = 99; /* Запись потомком провоцирует COW fault */
        _exit(0);
    } else {
        waitpid(pid, NULL, 0);
    }
    munmap(shared, PAGE_SIZE);
}

void run_file_backed() {
    // Открываем файл (он должен быть создан заранее, например размером 4КБ)
    int fd = open("testfile.bin", O_RDONLY);
    if (fd < 0) return;

    void *filemap = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (filemap != MAP_FAILED) {
        volatile char c = *((char *)filemap); /* Первое чтение файла через mmap */
        (void)c;
        munmap(filemap, PAGE_SIZE);
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <mode: zero|cow|file>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "zero") == 0) run_demand_zero();
    else if (strcmp(argv[1], "cow") == 0) run_cow_fault();
    else if (strcmp(argv[1], "file") == 0) run_file_backed();
    
    return 0;
}

