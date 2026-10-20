#!/usr/bin/env python3
import sys
import os
import struct
import matplotlib.pyplot as plt

PAGE_SIZE = 4096

def read_pagemap(pid, vaddr):
    pagemap_path = f"/proc/{pid}/pagemap"
    if not os.path.exists(pagemap_path):
        return None
    
    page_index = vaddr // PAGE_SIZE
    offset = page_index * 8
    
    try:
        with open(pagemap_path, "rb") as f:
            f.seek(offset)
            data = f.read(8)
            if len(data) < 8: return None
            value = struct.unpack("Q", data)[0]
            
            present = (value >> 63) & 1
            pfn = value & 0x7FFFFFFFFFFFFF
            return present, pfn
    except PermissionError:
        print("Ошибка: Требуются права sudo!")
        sys.exit(1)

def main():
    if len(sys.argv) < 2:
        print("Использование: sudo ./pagemap_reader.py <PID> [vaddr_hex_to_check]")
        sys.exit(1)
        
    pid = int(sys.argv[1])
    check_vaddr = int(sys.argv[2], 16) if len(sys.argv) > 2 else None
    
    # Если передан конкретный адрес, делаем быструю точечную проверку
    if check_vaddr is not None:
        res = read_pagemap(pid, check_vaddr)
        if res and res[0]:
            print(f"[Pagemap API] VA: {hex(check_vaddr)} -> PFN: {hex(res[1])}")
        else:
            print(f"[Pagemap API] VA: {hex(check_vaddr)} -> Данные отсутствуют / не в ОЗУ")
        return

    # Иначе сканируем регионы для построения графика (Шаг 2.4)
    vpages, pfns = [], []
    with open(f"/proc/{pid}/maps", "r") as f:
        for line in f:
            parts = line.strip().split()
            if not parts: continue
            start_hex, end_hex = parts[0].split("-")
            start, end = int(start_hex, 16), int(end_hex, 16)
            
            # Пропускаем гигантские маппинги, чтобы не зависать
            if (end - start) < 20 * 1024 * 1024:
                for vaddr in range(start, end, PAGE_SIZE):
                    res = read_pagemap(pid, vaddr)
                    if res and res[0] and res[1] > 0:
                        vpages.append(vaddr // PAGE_SIZE)
                        pfns.append(res[1])

    if vpages:
        plt.figure(figsize=(10, 6))
        plt.scatter(vpages, pfns, alpha=0.5, color='darkblue', s=5)
        plt.title(f"Разреженность физической памяти (PID: {pid})")
        plt.xlabel("Virtual Page Number (VPN)")
        plt.ylabel("Page Frame Number (PFN)")
        plt.grid(True, linestyle=':')
        plt.savefig("memory_scatter.png")
        print("[Успех] График сохранен в memory_scatter.png")

if __name__ == "__main__":
    main()

