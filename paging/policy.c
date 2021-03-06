/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>


extern int page_replace_policy;
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
    STATWORD ps;
    disable(ps);
    /* sanity check ! */
    if (policy != SC && policy != AGING) {
        restore(ps);
        return SYSERR;
    }

    page_replace_policy = policy;
    if (policy == SC) {
      sc_dummy = frm_qhead;
    }

    debug_option = 1;

    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}
