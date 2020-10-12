/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{   
    int i;
    for (i = 0; i < NFRAMES; ++i) {
        reset_frm_entry(i);
    }

    return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
    STATWORD ps;
    disable(ps);

    // find if there is a free page
    int i;
    for (i = 0; i < NFRAMES; ++i) {
        if (frm_tab[i].fr_status == FRM_UNMAPPED) {
            *avail = i;
            frm_enqueue(i);
            restore(ps);
            return OK;
        }
    }

    // page replacement
    if (grpolicy() == SC) {
        *avail = get_frm_by_SC();
    } else {
        *avail = get_frm_by_Aging();
    }

    free_frm(*avail);
    frm_enqueue(*avail);

    // print the evicted page
    if (debug_option == 1) {
        kprintf("Frame evicted: %d\n", *avail);
    }

    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
    pt_t *pt_entry = get_pt_entry(frm_tab[i].fr_pid, frm_tab[i].fr_vpno);
    pt_entry->pt_pres = 0;
    
    int pt_fr_index = get_pt_fr_index(frm_tab[i].fr_pid, frm_tab[i].fr_vpno);
    if (--frm_tab[pt_fr_index].fr_refcnt == 0) {
        // revoke the frame containing the page table
        frm_remove(pt_fr_index);
        reset_frm_entry(pt_fr_index);
        pd_t * pd_entry = get_pd_entry(frm_tab[i].fr_pid, frm_tab[i].fr_vpno);
        pd_entry->pd_pres = 0;
    }

    if (pt_entry->pt_dirty == 1) {
        // write back to the backing store
        int store;
        int pageth;
        bsm_lookup(frm_tab[i].fr_pid, frm_tab[i].fr_vpno * NBPG, &store, &pageth);
        write_bs((FRAME0 + i) * NBPG, store, pageth);
    }

    // remove the FR_PAGE page from the frame queue
    frm_remove(i);
    reset_frm_entry(i);

    return OK;
}



/* reset the inverted page table entry */
void reset_frm_entry(int i) {
    frm_tab[i].fr_dirty = 0;
    frm_tab[i].fr_pid = -1;
    frm_tab[i].fr_refcnt = 0;
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_type = FR_PAGE;
    frm_tab[i].fr_vpno = 0;
    frm_tab[i].fr_next = NULL;
    frm_tab[i].fr_age = 0;
    frm_tab[i].fr_index = i;
}

// initialize the queue for frames
void init_frm_queue() {
    frm_qdummy.fr_status = FRM_MAPPED;
    frm_qdummy.fr_pid = -1;
    frm_qdummy.fr_vpno = 0;
    frm_qdummy.fr_refcnt = 0;
    frm_qdummy.fr_type = FR_PAGE;
    frm_qdummy.fr_dirty = 0;
    frm_qdummy.fr_next = NULL;
    frm_qdummy.fr_index = -1;
    frm_qdummy.fr_age = 0;
    frm_qhead = frm_qtail = &frm_qdummy;
}

// frame enqueue
void frm_enqueue(int i) {
    frm_qtail->fr_next = &frm_tab[i];
    frm_qtail = frm_qtail->fr_next;
}

// remove the i-th frame from the queue (used by SC replacement policy)
void frm_remove(int i) {
    fr_map_t *p = frm_qhead;
    fr_map_t *q = p->fr_next;

    while (p->fr_next != NULL) {
        if (q->fr_index == i) {
            break;
        }
        p = q;
        q = q->fr_next;
    }

    if (q != NULL) {
        p->fr_next = q->fr_next;
        if (q == frm_qtail) {
            frm_qtail = p;
        }
    }
}

// get a free frame using SC replacement policy
int get_frm_by_SC() {
    fr_map_t *p = frm_qhead;
    fr_map_t *q = p->fr_next;

    int canFind = 0;
    pt_t *pt_entry;

    // first traversal
    while (p->fr_next != NULL) {
        if (q->fr_type == FR_PAGE) {
            pt_entry = get_pt_entry(q->fr_pid, q->fr_vpno);
            if (pt_entry->pt_acc == 0) {
                canFind = 1;
                break;
            } else {
                pt_entry->pt_acc = 0;
                p = q;
                q = q->fr_next;
            }
        } else {
            p = q;
            q = q->fr_next;
        }
    }

    if (canFind == 0) {
        // second traversal
        p = frm_qhead;
        q = p->fr_next;
        while (p->fr_next != NULL) {
            if (q->fr_type == FR_PAGE) {
                pt_entry = get_pt_entry(q->fr_pid, q->fr_vpno);
                break;
            } else {
                p = q;
                q = q->fr_next;
            }
        }
    }

    return q->fr_index;
}


// get a free frame using Aging replacement policy
int get_frm_by_Aging() {
    fr_map_t *p = frm_qhead;
    fr_map_t *q = p->fr_next;
    int min_age = 255;

    // find the min_age in the queue
    while (p->fr_next != NULL) {
        if (q->fr_type == FR_PAGE) {
            // decrease age by half
            q->fr_age /= 2;

            pt_t *pt_entry = get_pt_entry(q->fr_pid, q->fr_vpno);
            if (pt_entry->pt_acc == 1) {
                q->fr_age += 128;
                if (q->fr_age > 255) {
                    q->fr_age = 255;
                }
            }

            // update min_age
            if (q->fr_age < min_age) {
                min_age = q->fr_age;
            }
        }
        p = q;
        q = q->fr_next;
    }

    // find the first frame of which the age equals min_age
    p = frm_qhead;
    q = p->fr_next;
    while (p->fr_next != NULL) {
        if (q->fr_age == min_age) {
            break;
        }
        p = q;
        q = q->fr_next;
    }

    return q->fr_index;
}

// get the page table entry using pid and virtual page number
pt_t * get_pt_entry(int pid, int vpno) {
    unsigned long a = vpno * NBPG;

    virt_addr_t *vaddr; 
    vaddr = (virt_addr_t *) &a;
    unsigned int pd_offset = vaddr->pd_offset;
    unsigned int pt_offset = vaddr->pt_offset;

    int pdbr = proctab[pid].pdbr;

    pd_t *pd_entry;
    pd_entry = (pd_t*) (pdbr + pd_offset * sizeof(pd_t));

    pt_t * pt_entry;
    pt_entry = (pt_t *) ((pd_entry->pd_base * NBPG) + (pt_offset * sizeof(pt_t)));

    return pt_entry;
}

// get the page directory entry using pid and virtual  page number
pd_t * get_pd_entry(int pid, int vpno) {
    unsigned long a = vpno * NBPG;

    virt_addr_t *vaddr;
    vaddr = (virt_addr_t *) &a;
    unsigned int pd_offset = vaddr->pd_offset;

    int pdbr = proctab[pid].pdbr;

    pd_t *pd_entry;
    pd_entry = (pd_t *) (pdbr + pd_offset * sizeof(pd_t));

    return pd_entry;
}

// get the index of the physical frame
int get_pt_fr_index(int pid, int vpno) {
    pd_t *pd_entry = get_pd_entry(pid, vpno);

    return pd_entry->pd_base - FRAME0;
}

// allocate a page directory for a newly created process
void allocate_page_directory(int pid) {
    // get a free frame
    int frm_index;
    get_frm(&frm_index); // possibly return SYSERR

    frm_tab[frm_index].fr_status = FRM_MAPPED;
    frm_tab[frm_index].fr_pid = pid;
    frm_tab[frm_index].fr_type = FR_DIR;

    // bind the page to the pdbr of the process
    proctab[pid].pdbr = (FRAME0 + frm_index) * NBPG;

    // bind the first four page table to the page directory
    pd_t *pd_entry;
    pd_entry = (pd_t *) proctab[pid].pdbr;

    int i;
    for (i = 0; i < 4; ++i) {
        pd_entry[i].pd_write = 1;
        pd_entry[i].pd_pres = 1;
        pd_entry[i].pd_base = FRAME0 + i;
    }
}