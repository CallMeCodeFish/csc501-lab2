/* paging.h */

typedef unsigned int	 bsd_t;

/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

/* backing store map entry */
#define MAX_NUM_BS 8
#define MAX_BS_PAGES 256
#define BS_PRIVATE 1
#define BS_NONPRIVATE 0;

typedef struct __bs_map_list_t{
  int bs_pid;
  int bs_vpno;
  int bs_npages;
  struct __bs_map_list_t *bs_next;
} bs_map_list_t;

typedef struct{
  // int bs_status;			/* MAPPED or UNMAPPED		*/
  // int bs_pid;				/* process id using this slot   */
  // int bs_vpno;				/* starting virtual page number */
  // int bs_npages;			/* number of pages in the store */
  // int bs_sem;				/* semaphore mechanism ?	*/

  int bs_status;			/* MAPPED or UNMAPPED		*/
  int bs_npages;			/* number of pages in the store */
  int bs_sem;				/* semaphore mechanism ?	*/

  int bs_private;  /* is private heap */
  bs_map_list_t *bs_lhead;
  bs_map_list_t *bs_ltail;
} bs_map_t;

/* inversed page table entry*/
#define MAX_FRM_AGE 255 // max frame age in Aging replacement policy
int debug_option; // debug option for grading

typedef struct __fr_map_t{
  int fr_status;			/* MAPPED or UNMAPPED		*/
  int fr_pid;				/* process id using this frame  */
  int fr_vpno;				/* corresponding virtual page no*/
  int fr_refcnt;			/* reference count		*/
  int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
  int fr_dirty;

  //self-defined fields
  struct __fr_map_t *fr_next; // next node in the frame queue
  int fr_index; // frame index in the inverted page table
  int fr_age; // used in Aging replacement policy

}fr_map_t;

extern bs_map_t bsm_tab[]; /* backing store map */
extern fr_map_t frm_tab[]; /* inverted page table */
/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xmunmap(int);

/* given calls for dealing with backing store */

int get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);
SYSCALL init_bsm();
SYSCALL get_bsm(int*);
SYSCALL free_bsm(int);
SYSCALL bsm_lookup(int, long, int*, int*);
SYSCALL bsm_map(int, int, int, int);
SYSCALL bsm_unmap(int, int, int);
void add_list_node(int, int, int, int);
void delete_list_node(int, bs_map_list_t *);

/* functions related to frames */
SYSCALL init_frm();
SYSCALL get_frm(int*);
SYSCALL free_frm(int);
void init_frm_queue();
void frm_enqueue(int);
void frm_remove(int);
int get_frm_by_SC();
int get_frm_by_Aging();
void reset_frm_entry(int);
pt_t * get_pt_entry(int, int);
pd_t * get_pd_entry(int, int);
int get_pt_fr_index(int, int);

/* allocate page directory for a process */
void allocate_page_directory(int);

/* functions related to policy */
SYSCALL srpolicy(int);
SYSCALL grpolicy();

/* globel variables related to frames */
fr_map_t frm_qdummy;
fr_map_t *frm_qhead;
fr_map_t *frm_qtail;

#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame		*/
#define NFRAMES 	1024	/* number of frames		*/

#define BSM_UNMAPPED	0
#define BSM_MAPPED	1

#define FRM_UNMAPPED	0
#define FRM_MAPPED	1

#define FR_PAGE		0
#define FR_TBL		1
#define FR_DIR		2

#define SC 3
#define AGING 4

#define BACKING_STORE_BASE	0x00800000
#define BACKING_STORE_UNIT_SIZE 0x00100000
