/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

// int canPrint = 1;
/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
    STATWORD ps;
    disable(ps);

    // get faulted address
    unsigned long a = read_cr2();

    virt_addr_t *addr;
    addr = (virt_addr_t *) &a;

    // get the virtual page number of the faulted address
    int vpno = addr->pd_offset * NFRAMES + addr->pt_offset;

    int pid = getpid();

    // kprintf("addr: 0x%08x, pid: %d\n", a, pid);
    // kprintf("Illegal faulted address: 0x%08x for process id: %d\n", a, pid);

    // check if the faulted address is legal
    int store;
    int pageth;

    // if (vpno < proctab[pid].vhpno || vpno >= proctab[pid].vhpno + proctab[pid].vhpnpages) {
    if (proctab[pid].store == -1 || bsm_lookup(pid, a, &store, &pageth) == SYSERR) {
        kprintf("Illegal faulted address: 0x%08x for process id: %d. The process will be killed!\n", a, pid);
        // if (canPrint == 1) {
        //     kprintf("Illegal faulted address: 0x%08x for process %d\n", a, pid);
        //     canPrint = 0;
        // kprintf("vpno: %d, proctab[pid].vhpno: %d, proctab[pid].vhpnpages: %d\n", vpno, proctab[pid].vhpno, proctab[pid].vhpnpages);
        //     kprintf("number of proc: %d\n", numproc);
        // }
        
        kill(pid);
        restore(ps);
        return SYSERR;
    }

    unsigned long pdbr = proctab[pid].pdbr;
    unsigned int pd_offset = addr->pd_offset;
    unsigned int pt_offset = addr->pt_offset;
    unsigned int pg_offset = addr->pg_offset;

    pd_t *pd_entry; // point to the page table
    pd_entry = (pd_t *) (pdbr + pd_offset * sizeof(pd_t));

    int pt_frm_index;
    if (pd_entry->pd_pres == 0) {
        // the page table does not exist, create a page for the page table
        get_frm(&pt_frm_index);

        // set fields in the frame table entry
        frm_tab[pt_frm_index].fr_status = FRM_MAPPED;
        frm_tab[pt_frm_index].fr_type = FR_TBL;
        frm_tab[pt_frm_index].fr_pid = pid;

        // set the page directory entry
        pd_entry->pd_pres = 1;
        pd_entry->pd_write = 1;
        pd_entry->pd_user = 0;
        pd_entry->pd_pwt = 0;
        pd_entry->pd_pcd = 0;
        pd_entry->pd_acc = 0;
        pd_entry->pd_mbz = 0;
        pd_entry->pd_fmb = 0;
        pd_entry->pd_global = 0;
        pd_entry->pd_avail = 0;
        pd_entry->pd_base = FRAME0 + pt_frm_index;
        // kprintf("&&&& gain= %d\n", pt_frm_index);

        // set fields of pt table
        pt_t * pt_entry;
        pt_entry = (pt_t *) (pd_entry->pd_base * NBPG);
        int i;
        for (i = 0; i < NFRAMES; ++i) {
            pt_entry[i].pt_pres = 0;
			pt_entry[i].pt_write = 1;
			pt_entry[i].pt_user = 0;
			pt_entry[i].pt_pwt = 0;
			pt_entry[i].pt_pcd = 0;
			pt_entry[i].pt_acc = 0;
			pt_entry[i].pt_dirty = 0;
			pt_entry[i].pt_mbz = 0;
			pt_entry[i].pt_global = 0;
			pt_entry[i].pt_avail = 0;
        }

    } else {
        pt_frm_index = pd_entry->pd_base - FRAME0;
        // kprintf("---------here %d\n", pt_frm_index);
    }

    // kprintf("=====> <======\n");

    // create a page for data
    int pg_frm_index;
    get_frm(&pg_frm_index);

    // read data from the backing store
    // int store;
    // int pageth;
    // bsm_lookup(pid, a, &store, &pageth);
    read_bs((char *) ((FRAME0 + pg_frm_index) * NBPG), store, pageth);

    // kprintf("=====> 11<======\n");
    // set fields in the page
    frm_tab[pg_frm_index].fr_status = FRM_MAPPED;
    frm_tab[pg_frm_index].fr_type = FR_PAGE;
    frm_tab[pg_frm_index].fr_pid = pid;
    frm_tab[pg_frm_index].fr_vpno = vpno;
    frm_tab[pg_frm_index].fr_dirty = 0;

    // update metadata of the page table
    pt_t *pt_entry;
    pt_entry = (pt_t *) (pd_entry->pd_base * NBPG + pt_offset * sizeof(pt_t));

    pt_entry->pt_pres = 1;
    pt_entry->pt_write = 1;
    pt_entry->pt_user = 0;
    pt_entry->pt_pwt = 0;
    pt_entry->pt_pcd = 0;
    pt_entry->pt_acc = 0;
    pt_entry->pt_dirty = 0;
    pt_entry->pt_mbz = 0;
    pt_entry->pt_global = 0;
    pt_entry->pt_avail = 0;
    pt_entry->pt_base = FRAME0 + pg_frm_index;

    frm_tab[pt_frm_index].fr_refcnt++;

    // kprintf("&&&& pt index: %d, count: %d\n", pt_frm_index, frm_tab[pt_frm_index].fr_refcnt);
    // kprintf("=====>33 <======\n");

    restore(ps);
    return OK;
}


