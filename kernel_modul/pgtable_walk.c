#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/pgtable.h>
#include <linux/highmem.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS_Student");
MODULE_DESCRIPTION("Linux Kernel Page Table Walker");

static int target_pid = 0;
module_param(target_pid, int, 0644);
MODULE_PARM_DESC(target_pid, "Target process PID");

static unsigned long vaddr = 0;
module_param(vaddr, ulong, 0644);
MODULE_PARM_DESC(vaddr, "Virtual address to translate");

static int __init pgtable_walk_init(void)
{
    struct pid *pid_struct;
    struct task_struct *task;
    struct mm_struct *mm;
    
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    
    unsigned long pfn;
    unsigned long paddr;
    unsigned long offset = vaddr & ~PAGE_MASK;

    pr_info("=== pgtable_walk: СТАРТ ===\n");

    if (target_pid == 0 || vaddr == 0) {
        pr_err("Ошибка: параметры target_pid и vaddr обязательны!\n");
        return -EINVAL;
    }

    pid_struct = find_vpid(target_pid);
    if (!pid_struct) {
        pr_err("Ошибка: PID %d не найден.\n", target_pid);
        return -ESRCH;
    }
    
    task = pid_task(pid_struct, PIDTYPE_PID);
    if (!task) return -ESRCH;

    mm = task->mm;
    if (!mm) {
        pr_err("Ошибка: У процесса нет mm_struct (kernel thread).\n");
        return -EINVAL;
    }

    // Безопасная блокировка контекста памяти
    mmap_read_lock(mm);

    // 1. PGD
    pgd = pgd_offset(mm, vaddr);
    if (pgd_none(*pgd) || pgd_bad(*pgd)) goto out_unlock;

    // 2. P4D (сворачивается на 4-уровневой x86-64)
    p4d = p4d_offset(pgd, vaddr);
    if (p4d_none(*p4d) || p4d_bad(*p4d)) goto out_unlock;

    // 3. PUD
    pud = pud_offset(p4d, vaddr);
    if (pud_none(*pud) || pud_bad(*pud)) goto out_unlock;
    if (pud_leaf(*pud)) {
        pfn = pud_pfn(*pud);
        pr_info("Huge Page (1GB) PFN: 0x%lx\n", pfn);
        goto out_unlock;
    }

    // 4. PMD
    pmd = pmd_offset(pud, vaddr);
    if (pmd_none(*pmd) || pmd_bad(*pmd)) goto out_unlock;
    if (pmd_leaf(*pmd)) {
        pfn = pmd_pfn(*pmd);
        pr_info("Huge Page (2MB) PFN: 0x%lx\n", pfn);
        goto out_unlock;
    }

    // 5. PTE
    pte = pte_offset_kernel(pmd, vaddr);
    if (!pte) goto out_unlock;

    if (!pte_present(*pte)) {
        pr_warn("Страница NOT PRESENT в ОЗУ.\n");
        //pte_unmap(pte);
        goto out_unlock;
    }

    pfn = pte_pfn(*pte);
    paddr = (pfn << PAGE_SHIFT) | offset;
    
    pr_info("PID: %d | VA: 0x%lx -> PFN: 0x%lx | PA: 0x%lx\n", target_pid, vaddr, pfn, paddr);

   // pte_unmap(pte);

out_unlock:
    mmap_read_unlock(mm);
    return 0;
}

static void __exit pgtable_walk_exit(void)
{
    pr_info("=== pgtable_walk: ВЫГРУЖЕН ===\n");
}

module_init(pgtable_walk_init);
module_exit(pgtable_walk_exit);

