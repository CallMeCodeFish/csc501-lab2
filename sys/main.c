#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>


#define PROC1_VADDR	0x40000000
#define PROC1_VPNO      0x40000
#define PROC2_VADDR     0x80000000
#define PROC2_VPNO      0x80000
#define TEST1_BS	1

void proc1_test1(char *msg, int lck) {
	char *addr;
	int i;

	get_bs(TEST1_BS, 100);

	if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}

	addr = (char*) PROC1_VADDR;
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'A' + i;
	}

	sleep(6);

	for (i = 0; i < 26; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	xmunmap(PROC1_VPNO);
	kprintf("进程1结束!!\n");
	return;
}

void proc1_test2(char *msg, int lck) {
	int *x;

	kprintf("ready to allocate heap space\n");
	x = vgetmem(1024);
	kprintf("heap allocated at %x\n", x);
	*x = 100;
	*(x + 1) = 200;

	kprintf("heap variable: %d %d\n", *x, *(x + 1));
	vfreemem(x, 1024);
	kprintf("进程2结束!!\n");
}

void proc1_test3(char *msg, int lck) {

	char *addr;
	int i;

	addr = (char*) 0x0;

	for (i = 0; i < 1024; i++) {
		*(addr + i * NBPG) = 'B';
	}

	for (i = 0; i < 1024; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	kprintf("进程3结束\n");
	// printQueue();

	return;
}

void proc1_test4() {
	int i;
	char *addr = (char *)0x0;
	while (1) {
		for (i = 4096; i < 4096 + 256; ++i) {
			*(addr + i * NBPG) = 'A';
		}
	}
}

void printQueue() {
	kprintf("\n\n打印信息\n");
	fr_map_t *cur = frm_qhead->fr_next;
	while (cur != NULL) {
		kprintf("frame %d, pid %d\n", cur->fr_index, cur->fr_pid);
		cur = cur->fr_next;
	}
}

int main() {
	int pid1;
	// int pid2;

	kprintf("运行main进程!!\n");

	kprintf("\n1: shared memory\n");
	pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 0, NULL);
	resume(pid1);
	sleep(10);

	

	kprintf("\n2: vgetmem/vfreemem\n");
	pid1 = vcreate(proc1_test2, 2000, 100, 20, "proc1_test2", 0, NULL);
	kprintf("pid %d has private heap\n", pid1);
	resume(pid1);
	sleep(3);	


	kprintf("\n3: Frame test\n");
	pid1 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
	resume(pid1);
	sleep(3);
	// sleep(10);

	
	printQueue();

	// srpolicy(SC);
	// srpolicy(AGING);

	// int pid3 = vcreate(proc1_test4, 2000, 256, 20, "proc1_test4_1", 0, NULL); //48
	// int pid4 = vcreate(proc1_test4, 2000, 256, 20, "proc1_test4_2", 0, NULL); //47
	// int pid5 = vcreate(proc1_test4, 2000, 256, 20, "proc1_test4_3", 0, NULL); //46
	// int pid6 = vcreate(proc1_test4, 2000, 256, 20, "proc1_test4_4", 0, NULL); //45
	// int pid7 = vcreate(proc1_test4, 2000, 256, 20, "proc1_test4_5", 0, NULL); //44

	// resume(pid3);
	// resume(pid4);
	// resume(pid5);
	// resume(pid6);
	// resume(pid7);

	// sleep(2);

	// kill(pid3);
	// kill(pid4);
	// kill(pid5);
	// kill(pid6);
	// kill(pid7);

	// int i;
	// for (i = 0; i < 8; ++i) {
	// 	int pid = vcreate(proc1_test4, 2000, 256, 20, "proc1_test4", 0, NULL);
	// 	resume(pid);
	// }

	// sleep(1);
	
	// for (i = 48; i > 40; --i) {
	// 	kill(i);
	// }

	kprintf("main进程结束!!\n");
	return 0;
}