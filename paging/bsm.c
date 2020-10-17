/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


/* reset the fields of a backing store map entry */
void reset_bsm_entry(int i) {
    // bsm_tab[i].bs_status = BSM_UNMAPPED;
    // bsm_tab[i].bs_pid = -1;
    // bsm_tab[i].bs_sem = 0;
    // bsm_tab[i].bs_npages = 0;
    // bsm_tab[i].bs_vpno = 0;

    bsm_tab[i].bs_status = BSM_UNMAPPED;
    bsm_tab[i].bs_sem = 0;
    bsm_tab[i].bs_npages = 0;
    bsm_tab[i].bs_private = BS_NONPRIVATE;
    bsm_tab[i].bs_ltail = bsm_tab[i].bs_lhead;
}

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    int i;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        // reset_bsm_entry(i);

        // initialize list head node and tail node
        bs_map_list_t *dummy = getmem(sizeof(bs_map_list_t));
        dummy->bs_pid = -1;
        dummy->bs_vpno = 0;
        dummy->bs_npages = 0;
        dummy->bs_next = NULL;
        bsm_tab[i].bs_lhead = dummy;
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

    if (bsm_tab[i].bs_status == BSM_UNMAPPED) {
        return SYSERR;
    }

    bs_map_list_t *curr = bsm_tab[i].bs_lhead->bs_next;
    while (curr != NULL) {
        bs_map_list_t *temp = curr;
        curr = curr->bs_next;
        freemem(temp, sizeof(bs_map_list_t));
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
    int vpno = vaddr / NBPG;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        // if (bsm_tab[i].bs_status == BSM_MAPPED && bsm_tab[i].bs_pid == pid && bsm_tab[i].bs_vpno <= vpno && vpno < bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages) {
        //     *store = i;
        //     *pageth = vpno - bsm_tab[i].bs_vpno;
        //     return OK;
        // }

        if (bsm_tab[i].bs_status == BSM_MAPPED) {
            bs_map_list_t *curr = bsm_tab[i].bs_lhead->bs_next;
            while (curr != NULL) {
                if (curr->bs_pid == pid && curr->bs_vpno <= vpno && vpno < curr->bs_vpno + curr->bs_npages) {
                    *store = i;
                    *pageth = vpno - curr->bs_vpno;
                    return OK;
                }
                curr = curr->bs_next;
            }
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

    // bsm_tab[source].bs_pid = pid;
    // bsm_tab[source].bs_status = BSM_MAPPED;
    // bsm_tab[source].bs_vpno = vpno;
    // bsm_tab[source].bs_npages = npages;

    if (bsm_tab[source].bs_status == BSM_UNMAPPED) {
        bsm_tab[source].bs_status = BSM_MAPPED;
        bsm_tab[source].bs_npages = npages;
    }
    
    add_list_node(pid, vpno, source, npages);

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
    bs_map_list_t *curr;
    for (i = 0; i < MAX_NUM_BS; ++i) {
        // if (bsm_tab[i].bs_pid == pid && bsm_tab[i].bs_vpno == vpno && bsm_tab[i].bs_status == BSM_MAPPED) {
        //     break;
        // }

        if (bsm_tab[i].bs_status == BSM_MAPPED) {
            curr = bsm_tab[i].bs_lhead->bs_next;
            while (curr != NULL) {
                if (curr->bs_pid == pid && curr->bs_vpno <= vpno && vpno < curr->bs_vpno + curr->bs_npages) {
                    break;
                }
                curr = curr->bs_next;
            }

            if (curr != NULL) {
                break;
            }
        }
    }

    if (i == MAX_NUM_BS) {
        // cannot find the backing store
        return SYSERR;
    }

    // free the corresponding frame in the physical memory
    int j;
    for (j = 0; j < NFRAMES; ++j) {
        // if (frm_tab[j].fr_status == FRM_MAPPED && frm_tab[j].fr_type == FR_PAGE && frm_tab[j].fr_pid == pid
        //     && bsm_tab[i].bs_vpno <= frm_tab[j].fr_vpno && frm_tab[j].fr_vpno < bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages) {
        //         free_frm(j);
        // }

        if (frm_tab[j].fr_status == FRM_MAPPED && frm_tab[j].fr_type == FR_PAGE && frm_tab[j].fr_pid == pid
            && curr->bs_vpno <= frm_tab[j].fr_vpno && frm_tab[j].fr_vpno < curr->bs_vpno + curr->bs_npages) 
        {
            free_frm(j);
        }
    }

    // delete mapping in backing store map
    delete_list_node(i, curr);

    if (bsm_tab[i].bs_lhead->bs_next == NULL) {
        reset_bsm_entry(i);
    }

    return OK;
}

// add a node in the list of the corresponding backing store map entry
void add_list_node(int pid, int vpno, int store, int npages) {
    bs_map_list_t *new_node = getmem(sizeof(bs_map_list_t));
    new_node->bs_pid = pid;
    new_node->bs_vpno = vpno;
    new_node->bs_npages = npages;
    new_node->bs_next = NULL;
    bsm_tab[store].bs_ltail->bs_next = new_node;
    bsm_tab[store].bs_ltail = new_node;
}

// delete a node in the list of the corresponding backing store map entry
void delete_list_node(int store, bs_map_list_t *curr) {
    bs_map_list_t *p = bsm_tab[store].bs_lhead;
    bs_map_list_t *q = p->bs_next;
    while (q != curr) {
        p = q;
        q = q->bs_next;
    }
    p->bs_next = q->bs_next;
    if (q == bsm_tab[store].bs_ltail) {
        bsm_tab[store].bs_ltail = p;
    }
    freemem(curr, sizeof(bs_map_list_t));
}