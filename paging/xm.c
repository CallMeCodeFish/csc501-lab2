/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
    int pid = getpid();

    if (bsm_map(pid, virtpage, source, npages) == SYSERR) {
        return SYSERR;
    }

    proctab[pid].store = source;
	proctab[pid].vhpno = virtpage;
	proctab[pid].vhpnpages = npages;
	// proctab[pid].vmemlist->mnext = 4096 * NBPG;
	// proctab[pid].vmemlist->mnext->mlen = hsize * NBPG;
	// proctab[pid].vmemlist->mnext->mnext = NULL;
    
    return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
    int pid = getpid();

    return bsm_unmap(pid, virtpage, 0);
}
