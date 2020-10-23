/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);

	int i;
    // free pages of this process
    for (i = 0; i < NFRAMES; ++i) {
        if (frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_pid == pid && frm_tab[i].fr_type == FR_PAGE) {
            free_frm(i);
        }
    }

    // free page directory of this process
    for (i = 0; i < NFRAMES; ++i) {
        if (frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_pid == pid && frm_tab[i].fr_type == FR_DIR) {
            frm_remove(i);
            reset_frm_entry(i);
            break;
        }
    }

	// free bsm tab
	free_bsm(proctab[pid].store);

	// kprintf("进入kill!! pid = %d, 当前pid = %d\n", pid, currpid);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	// kprintf("here!! numproc = %d\n", numproc);

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
