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
    // will only be called by process created by create()
    int pid = getpid();

    // check if the backing store is private
    if (bsm_tab[source].bs_private == BS_PRIVATE) {
        return SYSERR;
    }

    if (bsm_map(pid, virtpage, source, npages) == SYSERR) {
        return SYSERR;
    }

    //used for pfint
    proctab[pid].store = source;
	proctab[pid].vhpno = virtpage;
	proctab[pid].vhpnpages = npages;
    
    return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
    int pid = getpid();

    if (bsm_unmap(pid, virtpage, 0) == SYSERR) {
        return SYSERR;
    }

    proctab[pid].store = -1;
    proctab[pid].vhpno = 0;
	proctab[pid].vhpnpages = 0;

    return OK;
}
