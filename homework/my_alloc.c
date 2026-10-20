#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define POOL_SIZE (1UL << 30)       /* 1 GiB виртуальной памяти */
#define WRITE_SIZE (100 * 1024 * 1024) /* Пишем в первые 100 MiB */
#define PAGE_SIZE 4096              /* Размер страницы 4 KiB */
#define STEP_10MIB (10 * 1024 * 1024)

// Функция для чтения текущего VmRSS (физической памяти) из /proc/self/status
unsigned long get_vm_rss() {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    
    char line[128];
    unsigned long rss = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%lu", &rss);
            break;
        }
    }
    fclose(f);
    return rss; // Возвращает значение в КБ
}

int main() {
    printf("=== Старт программы my_alloc ===\n");
    printf("VmRSS до выделения памяти: %lu KB\n", get_vm_rss());

    // Шаг 1.1: Выделяем 1 GiB анонимной памяти
    void *pool = mmap(NULL, POOL_SIZE, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pool == MAP_FAILED) {
        perror("mmap сбой");
        return 1;
    }

    // Шаг 1.2: Проверяем память сразу после mmap
    printf("VmRSS сразу после mmap() (до записи): %lu KB\n", get_vm_rss());
    printf("Обратите внимание: виртуально выделен 1 ГБ, но VmRSS почти не изменился!\n\n");

    // Шаг 1.4: Цикл последовательной записи с шагом 4 КиБ (размер страницы)
    char *ptr = (char *)pool;
    size_t written = 0;
    size_t next_report = STEP_10MIB;

    printf("%-20s | %-20s\n", "Записано данных (MiB)", "Текущий VmRSS (KB)");
    printf("---------------------------------------------\n");
    
    while (written < WRITE_SIZE) {
        // Каждое обращение к новой странице вызывает контролируемый Demand-zero page fault
        ptr[written] = 1; 
        written += PAGE_SIZE;

        // Каждые 10 МиБ выводим контрольную точку
        if (written >= next_report) {
            printf("%-20lu | %-20lu\n", written / (1024 * 1024), get_vm_rss());
            next_report += STEP_10MIB;
        }
    }

    printf("\nЗапись 100 MiB завершена. Итоговый VmRSS: %lu KB\n", get_vm_rss());

    // Освобождаем пул
    munmap(pool, POOL_SIZE);
    return 0;
}

