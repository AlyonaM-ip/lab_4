#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int target_variable = 100;

int main() {
    printf("Родитель [PID: %d]: Адрес переменной = %p\n", getpid(), &target_variable);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс
        printf("Потомок  [PID: %d] до записи: Адрес = %p, Значение = %d\n", getpid(), &target_variable, target_variable);
        printf("Нажмите Enter в основном терминале для модификации переменной потомком...\n");
        
        // Ждем отмашки через ввод (чтобы успеть проверить адреса до COW)
        getchar(); 
        
        target_variable = 999; // Модификация! Спровоцирует раздвоение физического фрейма
        printf("Потомок  [PID: %d] ПОСЛЕ записи: Адрес = %p, Значение = %d\n", getpid(), &target_variable, target_variable);
        while(1) sleep(1);
    } else {
        // Родительский процесс
        wait(NULL);
    }
    return 0;
}
