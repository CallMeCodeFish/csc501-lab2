/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bsm_tab[MAX_NUM_BS];

/* reset the fields of a backing store map entry */
void reset_bsm_entry(int i) {
    bsm_tab[i].bs_status = BSM_UNMAPPED;
    bsm_tab[i].bs_pid = -1;
    bsm_tab[i].bs_sem = 0;
    bsm_tab[i].bs_npages = 0;
    bsm_tab[i].bs_vpno = 0;
}

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    int i;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        reset_bsm_entry(i);
    }

    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD ps;
    disable(ps);

    int i;
    for (int i = 0; i < MAX_NUM_BS; ++i) {
        if (bsm_tab[i].bs_status == BSM_UNMAPPED) {
            *avail = i;
            restore(ps);
            return OK;
        }
    }

    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    STATWORD ps;
    disable(ps);

    if (i < 0 || i >= MAX_NUM_BS) {
        restore(ps);
        return SYSERR;
    }

    bs_map_t entry = bsm_tab[i];
    if (entry.bs_status == BSM_UNMAPPED) {
        restore(ps);
        return SYSERR;
    }

    reset_bsm_entry(entry);

    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    STATWORD ps;
    disable(ps);

    int i;
    int vpno = vaddr >> 12;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        if (bsm_tab[i].bs_status == BSM_MAPPED && bsm_tab[i].pid == pid && bsm_tab[i].bs_vpno <= vpno && vpno < bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages) {
            *store = i;
            *pageth = vpno - bsm_tab[i].bs_vpno;
            restore(ps);
            return OK;
        }
    }

    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD ps;
    disable(ps);

    int min_vpno = 4096;
    int max_vpno = 0xffffff;

    if (source < 0 || source >= MAX_NUM_BS) {
        restore(ps);
        return SYSERR;
    }

    if (npages <= 0 || npages > MAX_BS_PAGES) {
        restore(ps);
        return SYSERR;
    }

    if (vpno < min_vpno || vpno > max_vpno) {
        restore(ps);
        return SYSERR;
    }

    bsm_tab[source].bs_pid = pid;
    bsm_tab[source].bs_status = BSM_MAPPED;
    bsm_tab[source].bs_vpno = vpno;
    bsm_tab[source].bs_npages = npages;

    restore(ps);
    return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    STATWORD ps;
    disable(ps);  

    // check validation
    if (vpno < min_vpno || vpno > max_vpno) {
        restore(ps);
        return SYSERR;
    }

    int min_vpno = 4096;
    int max_vpno = 0xffffff;

    // find the index of the backing store
    int i;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        if (bsm_tab[i].bs_pid == pid && bsm_tab[i].bs_vpno == vpno && bsm_tab[i].bs_status == BSM_MAPPED) {
            break;
        }
    }

    if (i == MAX_NUM_BS) {
        // cannot find the backing store
        restore(ps);
        return SYSERR;
    }

    // free the corresponding frame in the physical memory
    int j;
    for (j = 0; j < NFRAMES; ++j) {
        if (frm_tab[j].fr_status == FRM_MAPPED && frm_tab[j].fr_type == FR_PAGE && frm_tab[j].fr_pid == pid
            && bsm_tab[i].bs_vpno <= frm_tab[j].fr_vpno && frm_tab[j] < bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages) {
                free_frm(j);
            }
    }

    // delete mapping in backing store map
    reset_bsm_entry(i);

    restore(ps);
    return OK;
}
