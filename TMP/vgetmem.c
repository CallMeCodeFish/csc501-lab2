/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	STATWORD ps;
	disable(ps);

	

	int pid = getpid();

	// kprintf("进入vgetmem\n");

	struct	mblock	*p, *q, *leftover, *vmemlist;
	vmemlist = proctab[pid].vmemlist;
	
	if (nbytes==0 || vmemlist->mnext== (struct mblock *) NULL) {
		// kprintf("----> point A\n");
		restore(ps);
		return( (WORD *)SYSERR);
	}

	nbytes = (unsigned int) roundmb(nbytes);

	// kprintf("requested bytes: %d\n", nbytes);
	struct mblock * myptr = vmemlist->mnext;
	// kprintf("available: %d, page = %d\n", myptr->mlen, myptr->mlen / NBPG);


	for (q= vmemlist,p=vmemlist->mnext ;
	     p != (struct mblock *) NULL ;
	     q=p,p=p->mnext)
		if ( p->mlen == nbytes) {
			// kprintf("====\n");
			// kprintf("available bytes: %d\n", p->mlen);
			q->mnext = p->mnext;
			
			restore(ps);
			return( (WORD *)p );
		} else if ( p->mlen > nbytes ) {
			// kprintf(">>>\n");
			// kprintf("available bytes: %d\n", p->mlen);
			leftover = (struct mblock *)( (unsigned)p + nbytes );
			q->mnext = leftover;
			leftover->mnext = p->mnext;
			leftover->mlen = p->mlen - nbytes;
			
			restore(ps);
			return( (WORD *)p );
		}

	// kprintf("----> point B\n");
	restore(ps);
	return( (WORD *)SYSERR );
}


