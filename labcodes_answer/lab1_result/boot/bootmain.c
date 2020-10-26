#include <defs.h>
#include <x86.h>
#include <elf.h>

/* *********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an ELF kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(bootasm.S and bootmain.c) is the bootloader.
 *    It should be stored in the first sector of the disk.
 *
 *  * The 2nd sector onward holds the kernel image.
 *
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive)
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in bootasm.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls bootmain()
 *
 *  * bootmain() in this file takes over, reads in the kernel and jumps to it.
 * */
unsigned int    SECTSIZE  =      512 ;

/* 0x10000 = 64KB，即该地址应该是内核代码在内存中的起始地址 */
struct elfhdr * ELFHDR    =      ((struct elfhdr *)0x10000) ;     // scratch space  ( 为什么偏偏是这个地址????? )

/* waitdisk - wait for disk ready */
static void
waitdisk(void) {
    while ((inb(0x1F7) & 0xC0) != 0x40)
        /* do nothing */;
}

/* readsect - read a single sector at @secno into @dst */
/* https://blog.csdn.net/ml_1995/article/details/51044260 内联汇编基础语法 */
static void
readsect(void *dst, uint32_t secno) {
    // wait for disk to be ready
    waitdisk();

    outb(0x1F2, 1);                         // count = 1      只读取一个扇区
    outb(0x1F3, secno & 0xFF);              /* 要读取扇区的编号 */
    outb(0x1F4, (secno >> 8) & 0xFF);       /* 用来存放读写柱面的低8位字节  */
    outb(0x1F5, (secno >> 16) & 0xFF);      /* 用来存放读写柱面的高2位字节 */
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);  /* 用来存放要读/写的磁盘号及磁头号 */
    outb(0x1F7, 0x20);                      // cmd 0x20 - read sectors

    // wait for disk to be ready
    waitdisk();

    // read a sector
    insl(0x1F0, dst, SECTSIZE / 4);  /* insl 是双字输入，所以这里除以4 */
}

/* *
 * readseg - read @count bytes at @offset from kernel into virtual address @va,
 * might copy more than asked.
 * */
static void
readseg(uintptr_t va, uint32_t count, uint32_t offset) {
    uintptr_t end_va = va + count;

    // round down to sector boundary
    va -= offset % SECTSIZE;

    // translate from bytes to sectors; kernel starts at sector 1
    /* 读入的首个扇区存的是bootloader，所以这里要
       *   往后偏一个扇区才是放内核代码的扇区
       */
    uint32_t secno = (offset / SECTSIZE) + 1;

    // If this is too slow, we could read lots of sectors at a time.
    // We'd write more to memory than asked, but it doesn't matter --
    // we load in increasing order.
    for (; va < end_va; va += SECTSIZE, secno ++) {
        readsect((void *)va, secno);
    }
}

/* bootmain - the entry of bootloader */
void
bootmain(void) {
    // read the 1st page off disk
    /* 一口气读入8个扇区，其中还包括存放bootloader 代码的扇区 */
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);

    // is this a valid ELF?
    if (ELFHDR->e_magic != ELF_MAGIC) {
        goto bad;
    }

    struct proghdr *ph, *eph;

    // load each program segment (ignores ph flags)
    /* ELF头部有描述ELF文件应加载到内存什么位置的描述表
       *   这一步是从elf 文件中获取程序描述表头
       */
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    eph = ph + ELFHDR->e_phnum;  

    /* 按照描述表将ELF文件中数据载入内存 */
    for (; ph < eph; ph ++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }

    // call the entry point from the ELF header
    // note: does not return
    /* 函数指针，e_entry保存的是地址，即第一条内核代码的地址
       *   该函数是否为kern_init() 函数???
       */
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();

bad:
    outw(0x8A00, 0x8A00);
    outw(0x8A00, 0x8E00);

    /* do nothing */
    while (1);
}

