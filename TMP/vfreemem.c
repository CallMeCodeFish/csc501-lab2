/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	STATWORD ps;    
	struct	mblock	*p, *q;
	unsigned top;

	// if (size==0 || (unsigned)block>(unsigned)maxaddr
	//     || ((unsigned)block)<((unsigned) &end)) {
	// 		kprintf(">>> point -1\n");
	// 		return(SYSERR);
	// 	}

	if (size==0 || ((unsigned)block)<((unsigned) &end)) {
			// kprintf(">>> point -1\n");
			return(SYSERR);
	}
		
	size = (unsigned)roundmb(size);
	disable(ps);

	int pid = getpid();

	struct mblock *vmemlist;
	vmemlist = proctab[pid].vmemlist;
	// kprintf(">>> point 0\n");

	for( p=vmemlist->mnext,q= vmemlist;
	     p != (struct mblock *) NULL && p < block ;
	     q=p,p=p->mnext )
		;
	if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= vmemlist) ||
	    (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
			// kprintf(">>> point 1\n");
		restore(ps);
		return(SYSERR);
	}
	if ( q!= vmemlist && top == (unsigned)block ) {
		// kprintf(">>> point 2\n");
		q->mlen += size;
	} else {
		// kprintf(">>> point 3\n");
		block->mlen = size;
		block->mnext = p;
		q->mnext = block;
		q = block;
	}
	if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
		// kprintf(">>> point 4\n");
		q->mlen += p->mlen;
		q->mnext = p->mnext;
	}
	// kprintf(">>> point 5\n");
	restore(ps);
	return(OK);
}
