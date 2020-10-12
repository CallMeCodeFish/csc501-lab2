/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


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
    int i;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        if (bsm_tab[i].bs_status == BSM_UNMAPPED) {
            *avail = i;
            return OK;
        }
    }

    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    if (i < 0 || i >= MAX_NUM_BS) {
        return SYSERR;
    }

    bs_map_t entry = bsm_tab[i];
    if (entry.bs_status == BSM_UNMAPPED) {
        return SYSERR;
    }

    reset_bsm_entry(i);

    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    int i;
    int vpno = vaddr >> 12;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        if (bsm_tab[i].bs_status == BSM_MAPPED && bsm_tab[i].bs_pid == pid && bsm_tab[i].bs_vpno <= vpno && vpno < bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages) {
            *store = i;
            *pageth = vpno - bsm_tab[i].bs_vpno;
            return OK;
        }
    }

    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    int min_vpno = 4096;
    int max_vpno = 0xffffff;

    if (source < 0 || source >= MAX_NUM_BS) {
        return SYSERR;
    }

    if (npages <= 0 || npages > MAX_BS_PAGES) {
        return SYSERR;
    }

    if (vpno < min_vpno || vpno > max_vpno) {
        return SYSERR;
    }

    bsm_tab[source].bs_pid = pid;
    bsm_tab[source].bs_status = BSM_MAPPED;
    bsm_tab[source].bs_vpno = vpno;
    bsm_tab[source].bs_npages = npages;

    return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    int min_vpno = 4096;
    int max_vpno = 0xffffff;

    // check validation
    if (vpno < min_vpno || vpno > max_vpno) {
        return SYSERR;
    }

    // find the index of the backing store
    int i;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        if (bsm_tab[i].bs_pid == pid && bsm_tab[i].bs_vpno == vpno && bsm_tab[i].bs_status == BSM_MAPPED) {
            break;
        }
    }

    if (i == MAX_NUM_BS) {
        // cannot find the backing store
        return SYSERR;
    }

    // free the corresponding frame in the physical memory
    int j;
    for (j = 0; j < NFRAMES; ++j) {
        if (frm_tab[j].fr_status == FRM_MAPPED && frm_tab[j].fr_type == FR_PAGE && frm_tab[j].fr_pid == pid
            && bsm_tab[i].bs_vpno <= frm_tab[j].fr_vpno && frm_tab[j].fr_vpno < bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages) {
                free_frm(j);
            }
    }

    // delete mapping in backing store map
    reset_bsm_entry(i);

    return OK;
}
