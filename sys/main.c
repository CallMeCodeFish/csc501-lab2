/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
#define TPASSED 1
#define TFAILED 0

#define MYVADDR1	    0x40000000
#define MYVPNO1      0x40000
#define MYVADDR2     0x80000000
#define MYVPNO2      0x80000
#define MYBS1	1
#define MAX_BSTORE 8

#ifndef MAXNPG
#define MAXNPG 256
#endif

#ifndef NBPG
#define NBPG 4096
#endif

#define assert(x,error) if(!(x)){ \
            kprintf(error);\
            return;\
            }

void proc_test1(int *ret) {
    char *addr0 = (char *)0x410000; // page 1040
	char *addr1 = (char*)0x40000000; //1G
    int  i= ((unsigned long)addr1)>>12;

    //should not trigger page fault
	*(addr0) = 'D';
	get_bs(MYBS1, 100);

	if (xmmap(i, MYBS1, 100) == SYSERR) {
	    *ret = TFAILED;
	    return;
	}

	for (i=0; i<16; i++){
	    *addr1 = 'A'+i;   //trigger page fault for every iteration
	    addr1 += NBPG;	//increment by one page each time
	}

	addr1 = (char*)0x40000000; //1G
	for (i=0; i<16; i++){
		if (*addr1 != 'A'+i){
			*ret = TFAILED;
			return;
			//kprintf("0x%08x: %c\n", addr1, *addr1);
		}
		addr1 += NBPG;       //increment by one page each time
	}

	xmunmap(0x40000000>>12);
	release_bs(MYBS1);
	return;
}

void test1()
{
	int mypid;
    int ret = TPASSED;

	kprintf("\nTest 1: Testing xmmap.\n");
    mypid = create(proc_test1, 2000, 20, "proc_test1", 1, &ret);
    resume(mypid);
    sleep(4);
    kill(mypid);
    if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}
/*----------------------------------------------------------------*/
void proc_test2(int i,int j,int* ret,int s) {
	char *addr;
	int bsize;
	int r;

	// kprintf("test2: proc_test: bsm table status %d\n", bsm_tab[i].bs_status);
	// kprintf("process %d: %d\n", 48, proctab[48].pstate);
	bsize = get_bs(i, j);
    // kprintf("return %d\n",bsize);
	if (bsize != 50) {
		// kprintf("test 2: proc_test2: bsize != 50\n");
		*ret = TFAILED;
	}
		

	r = xmmap(MYVPNO1, i, j);
	if (j<=50 && r == SYSERR){
		// kprintf("test 2: proc_test2: j<=50 && r == SYSERR\n");
		*ret = TFAILED;
	}
	if (j> 50 && r != SYSERR){
		// kprintf("test 2: proc_test2: j> 50 && r != SYSERR\n");
		*ret = TFAILED;
	}
	sleep(s);
	if (r != SYSERR) xmunmap(MYVPNO1);
	release_bs(i);
	return;
}


void test2() {
	int pids[MAX_BSTORE];
	int mypid;
	int i,j;

	int ret = TPASSED;
	kprintf("\nTest 2: Testing backing store operations\n");

	mypid = create(proc_test2, 2000, 20, "proc_test2", 4, 1,50,&ret,4);
	// kprintf("mypid: %d\n", mypid);
	resume(mypid);
	sleep(2);
	// kprintf("test2: mypid: ret = %d\n", ret);

	for(i=1;i<=5;i++){
		pids[i] = create(proc_test2, 2000, 20, "proc_test2", 4, 1,i*20,&ret,0);
		// kprintf("test2: i = %d: mypid status = %d\n", i, proctab[mypid].pstate);
		resume(pids[i]);
	}
	sleep(3);

	kill(mypid);

	for(i=1;i<=5;i++){
		kill(pids[i]);
	}

	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}
/*-------------------------------------------------------------------------------------*/
void proc1_test3(int i,int* ret) {
	char *addr;
	int bsize;
	int r;

	get_bs(i, 100);

	if (xmmap(MYVPNO1, i, 100) == SYSERR) {
	    *ret = TFAILED;
	    return 0;
	}
	sleep(4);
	xmunmap(MYVPNO1);
	release_bs(i);
	return;
}
void proc2_test3() {

	/*do nothing*/
	sleep(1);
	return;
}

void test3() {
	int pids[MAX_BSTORE];
	int mypid;
	int i,j;

	int ret = TPASSED;
	kprintf("\nTest 3: Private heap is exclusive\n");


	for(i=0;i < MAX_BSTORE;i++){
		pids[i] = create(proc1_test3, 2000, 20, "proc1_test3", 2, i,&ret);
		if (pids[i] == SYSERR){
			ret = TFAILED;
		}else{
			resume(pids[i]);
		}
	}
	sleep(1);
	mypid = vcreate(proc2_test3, 2000, 100, 20, "proc2_test3", 0, NULL);
	if (mypid != SYSERR)
		ret = TFAILED;

	for(i=0;i < MAX_BSTORE;i++){
		kill(pids[i]);
	}
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}
/*-------------------------------------------------------------------------------------*/

void proc1_test4(int* ret) {
	char *addr;
	int i;

	get_bs(MYBS1, 100);

	if (xmmap(MYVPNO1, MYBS1, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		*ret = TFAILED;
		sleep(3);
		return;
	}

	addr = (char*) MYVADDR1;
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'A' + i;
		// if (i == 0) {
		// 	kprintf("===> proc1 modify: 0x%08x, %d\n", (addr + i * NBPG), *(addr + i * NBPG) - 'A');
		// }
	}

	// kprintf("======> proc1 finished writing!!!\n");

	sleep(6);

	// kprintf(">>point1\n");

	/*Shoud see what proc 2 updated*/
	for (i = 0; i < 26; i++) {
		/*expected output is abcde.....*/
		if (*(addr + i * NBPG) != 'a'+i){
			// kprintf("===>proc 1 read error\n");
			*ret = TFAILED;
			break;
		}
		//kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	// kprintf(">>point2\n");


	xmunmap(MYVPNO1);

	// kprintf(">>point3\n");
	release_bs(MYBS1);
	// kprintf(">>point4\n");
	return;
}

void proc2_test4(int *ret) {
	char *addr;
	int i;

	get_bs(MYBS1, 100);

	if (xmmap(MYVPNO2, MYBS1, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		*ret = TFAILED;
		sleep(3);
		return;
	}

	// // check data using physical address
	// addr = (FRAME0 + NFRAMES + MYBS1 * MAX_BS_PAGES) * NBPG;
	// kprintf("===>checking data! addr = 0x%08x\n", addr);
	// int j;
	// for (j = 0; j < 26; ++j) {
	// 	kprintf("read: j = %d, value = %d\n", j, *(addr + j * NBPG));
	// }

	// // check the mapping of the bsm
	// bs_map_list_t * cur = bsm_tab[MYBS1].bs_lhead->bs_next;
	// while (cur != NULL) {
	// 	kprintf("===>打印: pid = %d, vpno = 0x%08x\n", cur->bs_pid, cur->bs_vpno);
	// 	cur = cur->bs_next;
	// }


	addr = (char*) MYVADDR2;

	// kprintf("addr: 0x%08x, value: %c\n", addr, *addr);
	// kprintf("====================\n");

	// kprintf("===>p1\n");

	/*Shoud see what proc 1 updated*/
	for (i = 0; i < 26; i++) {
		/*expected output is ABCDEF.....*/
		// kprintf("in loop\n");
		if (*(addr + i * NBPG) != 'A'+i){
			// kprintf("====>not the same\n");
			// kprintf("addr = 0x%08x, i = %d, value = %c\n", (addr + i * NBPG), i, *(addr + i * NBPG));
			// kprintf("===>proc 2 read error\n");
			*ret = TFAILED;
			break;
		}
		//kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	// kprintf("!!!!!proc2 here\n");

	/*Update the content, proc1 should see it*/
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'a' + i;
	}

	xmunmap(MYVPNO2);

	

	release_bs(MYBS1);
	// // check the mapping of the bsm
	// bs_map_list_t * cur = bsm_tab[MYBS1].bs_lhead->bs_next;
	// kprintf("@@@@准备打印!!!\n");
	// while (cur != NULL) {
	// 	kprintf("===>打印: pid = %d, vpno = 0x%08x\n", cur->bs_pid, cur->bs_vpno);
	// 	cur = cur->bs_next;
	// }
	// kprintf("status=%d \n", bsm_tab[1].bs_status);
	return;
}

void test4() {
	int pid1;
	int pid2;
	int ret = TPASSED;
	kprintf("\nTest 4: Shared backing store\n");

	pid1 = create(proc1_test4, 2000, 20, "proc1_test4", 1, &ret);
	pid2 = create(proc2_test4, 2000, 20, "proc2_test4", 1, &ret);

	// kprintf("pid1 = %d\n", pid1);
	// kprintf("pid2 = %d\n", pid2);

	resume(pid1);
	sleep(3);

	// kprintf("test4: ret1 = %d\n", ret);
	// kprintf("test4: pid1 status = %d\n", proctab[pid1].pstate);

	resume(pid2);

	sleep(10);

	// kprintf("test4: ret1 = %d\n", ret);
	// kprintf("test4: pid1 status = %d\n", proctab[pid1].pstate);
	// kprintf("test4: ret2 = %d\n", ret);
	// kprintf("test4: pid2 status = %d\n", proctab[pid2].pstate);

	// kill(pid1);
	// kill(pid2);
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}
/*-------------------------------------------------------------------------------------*/
void proc1_test5(int* ret) {
	int *x;
	int *y;
	int *z;

	//kprintf("ready to allocate heap space\n");
	x = vgetmem(1024);
	if ((x == NULL) || (x < 0x1000000)) {
		*ret = TFAILED;
	    //kprintf("heap allocated at %x (address should be > 0x1000000 (16MB))\n", x);
    }
	if (x == NULL)
		return;

	*x = 100;
	*(x + 1) = 200;

	//kprintf("heap variable: %d %d (should print 100 and 200)\n", *x, *(x + 1));
	if ((*x != 100) || (*(x+1) != 200))
		*ret = TFAILED;
	vfreemem(x, 1024);

	x = vgetmem((MAXNPG + 1) * NBPG); //try to acquire a space that is bigger than size of one backing store
	if (x != SYSERR) {
		*ret = TFAILED;
    }

	x = vgetmem(50*NBPG);
	y = vgetmem(50*NBPG);
	z = vgetmem(50*NBPG);
	if ((x == SYSERR) || (y == SYSERR) || (z != SYSERR)){
		*ret = TFAILED;
		if (x != NULL) vfreemem(x, 50*NBPG);
		if (y != NULL) vfreemem(y, 50*NBPG);
		if (z != NULL) vfreemem(z, 50*NBPG);
		return;
	}
	vfreemem(y, 50*NBPG);
	z = vgetmem(50*NBPG);
	if (z == SYSERR){
		*ret = TFAILED;
	}
	if (x != NULL) vfreemem(x, 50*NBPG);
	if (z != NULL) vfreemem(z, 50*NBPG);
	return;


}

void test5() {
	int pid1;
	int ret = TPASSED;

	kprintf("\nTest 5: vgetmem/vfreemem\n");
	pid1 = vcreate(proc1_test5, 2000, 100, 20, "proc1_test5", 1, &ret);

	//kprintf("pid %d has private heap\n", pid1);
	resume(pid1);
	sleep(3);
	kill(pid1);
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}

/*-------------------------------------------------------------------------------------*/


void proc1_test6(int *ret) {

    char *vaddr, *addr0, *addr_lastframe, *addr_last;
	int i, j;
	int tempaddr;
    int addrs[1200];

    int maxpage = (NFRAMES - (5 + 1 + 1 + 1));

	int vaddr_beg = 0x40000000;//1GB or page 262144
	int vpno;


	for(i = 0; i < MAX_BSTORE; i++){
		tempaddr = vaddr_beg + 127 * NBPG * i;
		vaddr = (char *) tempaddr;
		vpno = tempaddr >> 12;
		get_bs(i, 127);
		if (xmmap(vpno, i, 127) == SYSERR) {
			*ret = TFAILED;
			kprintf("xmmap call failed\n");
			sleep(3);
			return;
		}

		for (j = 0; j < 127; j++) {
			*(vaddr + j * NBPG) = 'A' + i;
		}

		for (j = 0; j < 127; j++) {
			if (*(vaddr + j * NBPG) != 'A'+i){
				*ret = TFAILED;
				break;
			}
			//kprintf("0x%08x:%c ", vaddr + j * NBPG, *(vaddr + j * NBPG));
		}
		xmunmap(vpno);
		release_bs(i);
	}


	return;
}

void test6(){
	int pid1;
	int ret = TPASSED;
	kprintf("\nTest 6: Stress testing\n");

	pid1 = create(proc1_test6, 2000, 50, "proc1_test6",1,&ret);

	resume(pid1);
	sleep(4);
	kill(pid1);
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}

/*-------------------------------------------------------------------------------------*/
void test_func7()
{
		int PAGE0 = 0x40000;
		int i,j,temp;
		int addrs[1200];
		int cnt = 0;
		//can go up to  (NFRAMES - 5 frames for null prc - 1pd for main - 1pd + 1pt frames for this proc)
		//frame for pages will be from 1032-2047
		int maxpage = (NFRAMES - (5 + 1 + 1 + 1));
        //int maxpage = (NFRAMES - 25);


		for (i=0;i<=maxpage/150;i++){
				if(get_bs(i,150) == SYSERR)
				{
						kprintf("get_bs call failed \n");
						return;
				}
				if (xmmap(PAGE0+i*150, i, 150) == SYSERR) {
						kprintf("xmmap call failed\n");
						return;
				}
				for(j=0;j < 150;j++)
				{
						//store the virtual addresses
						addrs[cnt++] = (PAGE0+(i*150) + j) << 12;
				}
		}

		/* all of these should generate page fault, no page replacement yet
		   acquire all free frames, starting from 1032 to 2047, lower frames are acquired first
		   */
		for(i=0; i < maxpage; i++)
		{
				*((int *)addrs[i]) = i + 1;
		}

		//trigger page replacement, this should clear all access bits of all pages
		//expected output: frame 1032 will be swapped out
		kprintf("\n\t 7.1 Expected replaced frame: 1032\n\t");
		*((int *)addrs[maxpage]) = maxpage + 1;

		for(i=1; i <= maxpage; i++)
		{

				if ((i != 600) && (i != 800))  //reset access bits of all pages except these
						*((int *)addrs[i])= i+1;

		}
		//Expected page to be swapped: 1032+600 = 1632
		kprintf("\n\t 7.2 Expected replaced frame: 1632\n\t");
		*((int *)addrs[maxpage+1]) = maxpage + 2;
		temp = *((int *)addrs[maxpage+1]);
		if (temp != maxpage +2)
			kprintf("\tFAILED!\n");

		kprintf("\n\t 7.3 Expected replaced frame: 1832\n\t");
		*((int *)addrs[maxpage+2]) = maxpage + 3;
		temp = *((int *)addrs[maxpage+2]);
		if (temp != maxpage +3)
			kprintf("\tFAILED!\n");


		for (i=0;i<=maxpage/150;i++){
				xmunmap(PAGE0+(i*150));
				release_bs(i);
		}

}
void test7(){
	int pid1;
	int ret = TPASSED;

	kprintf("\nTest 7: Test SC page replacement policy\n");
	srpolicy(SC);
	pid1 = create(test_func7, 2000, 20, "test_func7", 0, NULL);

	resume(pid1);
	sleep(10);
	kill(pid1);

     /*
	pid1 = create(test_func7, 2000, 20, "test_func7", 0, NULL);
	resume(pid1);
	sleep(10);
	kill(pid1);
    */


	kprintf("\n\t Finished! Check error and replaced frames\n");
}
/*-------------------------------------------------------------------------------------*/
void test_func8()
{
		STATWORD ps;
		int PAGE0 = 0x40000;
		int i,j,temp=0;
		int addrs[1200];
		int cnt = 0;

		//can go up to  (NFRAMES - 5 frames for null prc - 1pd for main - 1pd + 1pt frames for this proc)
		int maxpage = (NFRAMES - (5 + 1 + 1 + 1)); //=1016
        //int maxpage = (NFRAMES - 25);


		for (i=0;i<=maxpage/150;i++){
				if(get_bs(i,150) == SYSERR)
				{
						kprintf("get_bs call failed \n");
						return;
				}
				if (xmmap(PAGE0+i*150, i, 150) == SYSERR) {
						kprintf("xmmap call failed\n");
						return;
				}
				for(j=0;j < 150;j++)
				{
						//store the virtual addresses
						addrs[cnt++] = (PAGE0+(i*150) + j) << 12;
				}
		}


		/* all of these should generate page fault, no page replacement yet
		   acquire all free frames, starting from 1032 to 2047, lower frames are acquired first
		   */
		for(i=0; i < maxpage; i++)
		{
				*((int *)addrs[i]) = i + 1;  //bring all pages in, only referece bits are set

		}

		sleep(3); //after sleep, all reference bits should be cleared

		disable(ps); //reduce the possibility of trigger reference bit clearing routine while testing

		for(i=0; i < maxpage/2; i++)
		{
				*((int *)addrs[i]) = i + 1; //set both ref bits and dirty bits for these pages
		}

        kprintf("\t 8.1 Expected replaced frame: %d\n\t",1032+maxpage/2);
		enable(ps);  //to allow page fault
		// trigger page replace ment, expected output: frame 1032+maxpage/2=1540 will be swapped out
		// this test might have a different result (with very very low possibility) if bit clearing routine is called before executing the following instruction
		*((int *)addrs[maxpage]) = maxpage + 1;
        temp = *((int *)addrs[maxpage]);
		if (temp != maxpage +1)
			kprintf("\tFAILED!\n");

		sleep(3); //after sleep, all reference bits should be cleared

		disable(ps); //reduce the possibility of trigger reference bit clearing routine while testing

		for(i=0; i < maxpage/3; i++)
		{
				*((int *)addrs[i]) = i + 1; //set both ref bits and dirty bits for these pages
		}

        kprintf("\t 8.2 Expected replaced frame: %d\n\t",1032+maxpage/3);
		enable(ps);  //to allow page fault
		// trigger page replace ment, expected output: frame 1032+maxpage/3=1540 will be swapped out
		// this test might have a different result (with very very low possibility) if bit clearing routine is called before executing the following instruction
		*((int *)addrs[maxpage+1]) = maxpage + 2;
        temp = *((int *)addrs[maxpage+1]);
		if (temp != maxpage +2)
			kprintf("\tFAILED!\n");


		for (i=0;i<=maxpage/150;i++){
				xmunmap(PAGE0+(i*150));
				release_bs(i);
		}


}
void test8(){
	int pid1;

	kprintf("\nTest 8: Test AGING page replacement policy\n");
	srpolicy(AGING);
    kprintf("\n\t First run:\n");
	pid1 = create(test_func8, 2000, 20, "test_func8", 0, NULL);

	resume(pid1);
	sleep(10);
	kill(pid1);

    kprintf("\n\t Second run (test where killing process is handled correctly):\n");
	pid1 = create(test_func8, 2000, 20, "test_func8", 0, NULL);
	resume(pid1);
	sleep(10);
	kill(pid1);

	kprintf("\n\t Finished! Check error and replaced frames\n");
}

void mytest() {
	get_bs(MYBS1, 100);

		if (xmmap(MYVPNO2, MYBS1, 100) == SYSERR) {
			kprintf("xmmap call failed\n");
			sleep(3);
			return;
		}

		char * addr;
		addr = (char*) MYVADDR2;
		int i;
		
		for (i = 0; i < 26; i++) {
			/*expected output is ABCDEF.....*/
			kprintf("i = %d\n", i);
			kprintf("value: %d\n", *(addr + i * NBPG));
			// if (*(addr + i * NBPG) ){
			// 	kprintf("addr = 0x%08x, i = %d, value = %d\n", (addr + i * NBPG), i, *(addr + i * NBPG) - 'A');
			// 	kprintf("===>proc 2 read error\n");
			// }
			
			// kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
		}
}

int main() {
	// shutdown();
    int i, s;
    char buf[8];

	kprintf("\n\nHello World, Xinu lives\n\n");

    // kprintf("Please Input:\n");
	// while ((i = read(CONSOLE, buf, sizeof(buf))) < 1)
	// 	;
	// buf[i] = 0;
	// s = atoi(buf);
    // kprintf("Get %d\n", s);

    // switch(s) {
        // case 1:
    	//    test1();
        //    break;
        // case 2:
    	//    test2();
        //    break;
        // case 3:
    	//    test3();
        //    break;
        // case 4:
    	//    test4();
		
        //    break;
        // case 5:
    	//    test5();
        //    break;
        // case 6:
    	   test6();
        //    break;
        // case 7:
           test7();
        //    break;
        // case 8:
           test8();
        //    break;
        // default:
        //    kprintf("No test selected\n");
    // }


    shutdown();
}