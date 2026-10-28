sudo su
# 1. Сбрасываем старый трассировщик и ставим режим function (отслеживание вызовов функций)
echo function > /sys/kernel/debug/tracing/current_tracer

# 2. Задаем фильтр: нам интересна ТОЛЬКО функция ядра handle_mm_fault
echo handle_mm_fault > /sys/kernel/debug/tracing/set_ftrace_filter

# 3. Включаем трассировку
echo 1 > /sys/kernel/debug/tracing/tracing_on

# 4. Запускаем твой аллокатор памяти из папки homework
./my_alloc

# 5. Выключаем трассировку, чтобы логи не забивались лишней информацией
echo 0 > /sys/kernel/debug/tracing/tracing_on

# 6. Выводим первые 30 строк перехваченного лога ядра
cat /sys/kernel/debug/tracing/trace | head -n 30

# Выходим из режима root
exit
