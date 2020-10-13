/* userret.c - userret */

#include <conf.h>
#include <kernel.h>
#include <paging.h>

/*------------------------------------------------------------------------
 * userret  --  entered when a process exits by return
 *------------------------------------------------------------------------
 */
int userret()
{
    STATWORD ps;
    disable(ps);

    int pid = getpid();
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

    restore(ps);
	kill( getpid() );
    return(OK);
}
