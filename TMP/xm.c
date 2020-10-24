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
    proctab[pid].store = 8; // public 
	// proctab[pid].vhpno = virtpage;
	// proctab[pid].vhpnpages = npages;
    
    return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
    STATWORD ps;
    disable(ps);
    int pid = getpid();
    // kprintf(">>point 2.1\n");

    // kprintf(">>>point A\n");
    if (bsm_unmap(pid, virtpage, 0) == SYSERR) {
        // kprintf(">>point 2.2->1\n");
        restore(ps);
        return SYSERR;
    }
    // kprintf(">>>point B\n");
    // kprintf(">>point 2.2\n");
    if (proctab[pid].store != 8) {
        //private
        proctab[pid].store = -1;
    } else {
        //public
        bs_map_list_t *curr;
        int i;
        for (i = 0; i < MAX_NUM_BS; ++i) {
            if (bsm_tab[i].bs_status == BSM_MAPPED && bsm_tab[i].bs_private == BS_NONPRIVATE) {
                curr = bsm_tab[i].bs_lhead->bs_next;
                while (curr != NULL) {
                    if (curr->bs_pid == pid) {
                        break;
                    }
                    curr = curr->bs_next;
                }
                if (curr != NULL) {
                    break;
                }
            }
        }
        if (curr == NULL) {
            proctab[pid].store = -1;
        }
    }
    
    // proctab[pid].vhpno = 0;
	// proctab[pid].vhpnpages = 0;
    // kprintf(">>point 2.3\n");

    restore(ps);
    return OK;
}
