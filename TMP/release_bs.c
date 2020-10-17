#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {
    /* release the backing store with ID bs_id */

    // bs_id is not in valid range
    if (bs_id < 0 || bs_id >= MAX_NUM_BS) {
        return SYSERR;
    }

    // int pid = getpid();

    // // pid of the current process != pid of the backing store
    // if (pid != bsm_tab[bs_id].bs_pid) {
    //     restore(ps);
    //     return SYSERR;
    // }

    return free_bsm(bs_id);
}

