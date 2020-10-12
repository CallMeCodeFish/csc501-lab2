/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
    popl pferrcode // store error code in a global variable
    pushfl // store flag register
    cli // clear all interrupts
    pushal // save all general registers
    call pfint // page fault handling
    popal // restore general registers
    popfl // restore flag register
    iret // iret

