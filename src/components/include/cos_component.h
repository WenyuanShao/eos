/**
 * Copyright 2007 by Gabriel Parmer, gabep1@cs.bu.edu
 *
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 */

#ifndef COS_COMPONENT_H
#define COS_COMPONENT_H

#include <consts.h>
#include <cos_types.h>
#include <errno.h>
#include <util.h>

void libc_init();

/* temporary */
static inline int
call_cap_asm(u32_t cap_no, u32_t op, int arg1, int arg2, int arg3, int arg4)
{
	long fault = 0;
	int  ret;

	cap_no = (cap_no + 1) << COS_CAPABILITY_OFFSET;
	cap_no += op;

	__asm__ __volatile__("pushl %%ebp\n\t"		\
	                     "movl %%esp, %%ebp\n\t"	\
	                     "movl $1f, %%ecx\n\t"	\
	                     "sysenter\n\t"		\
	                     ".align 8\n\t"		\
	                     "jmp 2f\n\t"		\
	                     ".align 8\n\t"		\
	                     "1:\n\t"			\
	                     "movl $0, %%ecx\n\t"	\
	                     "jmp 3f\n\t"		\
	                     "2:\n\t"			\
	                     "movl $1, %%ecx\n\t"	\
	                     "3:\n\t"			\
	                     "popl %%ebp"		\
	                     : "=a"(ret), "=c"(fault)
	                     : "a"(cap_no), "b"(arg1), "S"(arg2), "D"(arg3), "d"(arg4)
	                     : "memory", "cc");

	return ret;
}

static inline int
call_cap_retvals_asm(u32_t cap_no, u32_t op, int arg1, int arg2, int arg3, int arg4,
			 unsigned long *r1, unsigned long *r2, unsigned long *r3)
{
	long fault = 0;
	int  ret;

	cap_no = (cap_no + 1) << COS_CAPABILITY_OFFSET;
	cap_no += op;

	__asm__ __volatile__("pushl %%ebp\n\t"		\
	                     "movl %%esp, %%ebp\n\t"	\
	                     "movl $1f, %%ecx\n\t"	\
	                     "sysenter\n\t"		\
	                     ".align 8\n\t"		\
	                     "jmp 2f\n\t"		\
	                     ".align 8\n\t"		\
	                     "1:\n\t"			\
	                     "movl $0, %%ecx\n\t"	\
	                     "jmp 3f\n\t"		\
	                     "2:\n\t"			\
	                     "movl $1, %%ecx\n\t"	\
	                     "3:\n\t"			\
	                     "popl %%ebp\n\t"		\
	                     : "=a"(ret), "=c"(fault), "=S"(*r1), "=D"(*r2), "=b" (*r3)
	                     : "a"(cap_no), "b"(arg1), "S"(arg2), "D"(arg3), "d"(arg4)
	                     : "memory", "cc");

	return ret;
}

static inline int
call_cap_2retvals_asm(u32_t cap_no, u32_t op, int arg1, int arg2, int arg3, int arg4,
			 unsigned long *r1, unsigned long *r2)
{
	long fault = 0;
	int  ret;

	cap_no = (cap_no + 1) << COS_CAPABILITY_OFFSET;
	cap_no += op;

	__asm__ __volatile__("pushl %%ebp\n\t"		\
	                     "movl %%esp, %%ebp\n\t"	\
	                     "movl $1f, %%ecx\n\t"	\
	                     "sysenter\n\t"		\
	                     ".align 8\n\t"		\
	                     "jmp 2f\n\t"		\
	                     ".align 8\n\t"		\
	                     "1:\n\t"			\
	                     "movl $0, %%ecx\n\t"	\
	                     "jmp 3f\n\t"		\
	                     "2:\n\t"			\
	                     "movl $1, %%ecx\n\t"	\
	                     "3:\n\t"			\
	                     "popl %%ebp\n\t"		\
	                     : "=a"(ret), "=c"(fault), "=S"(*r1), "=D"(*r2)
	                     : "a"(cap_no), "b"(arg1), "S"(arg2), "D"(arg3), "d"(arg4)
	                     : "memory", "cc");

	return ret;
}

static inline int
cap_switch_thd(u32_t cap_no)
{
	return call_cap_asm(cap_no, 0, 0, 0, 0, 0);
}

static inline int
call_cap(u32_t cap_no, int arg1, int arg2, int arg3, int arg4)
{
	return call_cap_asm(cap_no, 0, arg1, arg2, arg3, arg4);
}

static inline int
call_cap_op(u32_t cap_no, u32_t op_code, int arg1, int arg2, int arg3, int arg4)
{
	return call_cap_asm(cap_no, op_code, arg1, arg2, arg3, arg4);
}

static void
cos_print(char *s, int len)
{
	// FIXME: casting from a pointer to an int can be lossy
	call_cap(PRINT_CAP_TEMP, (int) s, len, 0, 0);
}

extern struct cos_component_information cos_comp_info;

static inline long
get_stk_data(int offset)
{
	unsigned long curr_stk_pointer;

	__asm__("movl %%esp, %0;" : "=r"(curr_stk_pointer));
	/*
	 * We save the CPU_ID and thread id in the stack for fast
	 * access.  We want to find the struct cos_stk (see the stkmgr
	 * interface) so that we can then offset into it and get the
	 * cpu_id.  This struct is at the _top_ of the current stack,
	 * and cpu_id is at the top of the struct (it is a u32_t).
	 */
	return *(long *)((curr_stk_pointer & ~(COS_STACK_SZ - 1)) + COS_STACK_SZ - offset * sizeof(u32_t));
}

#define GET_CURR_CPU cos_cpuid()

static inline long
cos_cpuid(void)
{
#if NUM_CPU == 1
	return 0;
#endif
	/*
	 * see comments in the get_stk_data above.
	 */
	return get_stk_data(CPUID_OFFSET);
}

static inline unsigned short int
cos_get_thd_id(void)
{
	/*
	 * see comments in the get_stk_data above.
	 */
	return get_stk_data(THDID_OFFSET);
}

static inline invtoken_t
cos_inv_token(void)
{
	return get_stk_data(INVTOKEN_OFFSET);
}

typedef u16_t cos_thdid_t;

static cos_thdid_t
cos_thdid(void)
{
	return cos_get_thd_id();
}

#define ERR_THROW(errval, label) \
	do {                     \
		ret = errval;    \
		goto label;      \
	} while (0)

static inline long
cos_spd_id(void)
{
	return cos_comp_info.cos_this_spd_id;
}

static inline void *
cos_get_heap_ptr(void)
{
	return (void *)cos_comp_info.cos_heap_ptr;
}

static inline void
cos_set_heap_ptr(void *addr)
{
	cos_comp_info.cos_heap_ptr = (vaddr_t)addr;
}

static inline char *
cos_init_args(void)
{
	return cos_comp_info.init_string;
}

#define COS_EXTERN_FN(fn) __cos_extern_##fn

static inline long
cos_cmpxchg(volatile void *memory, long anticipated, long result)
{
	long ret;

	__asm__ __volatile__("call cos_atomic_cmpxchg"
	                     : "=d"(ret)
	                     : "a"(anticipated), "b"(memory), "c"(result)
	                     : "cc", "memory");

	return ret;
}

/* A uni-processor variant with less overhead but that doesn't
 * guarantee atomicity across cores. */
static inline int
cos_cas_up(unsigned long *target, unsigned long cmp, unsigned long updated)
{
	char z;
	__asm__ __volatile__("cmpxchgl %2, %0; setz %1"
	                     : "+m"(*target), "=a"(z)
	                     : "q"(updated), "a"(cmp)
	                     : "memory", "cc");
	return (int)z;
}

static inline void *
cos_get_prealloc_page(void)
{
	char *h;
	long  r;
	do {
		h = (char *)cos_comp_info.cos_heap_alloc_extent;
		if (!h || (char *)cos_comp_info.cos_heap_allocated >= h) return NULL;
		r = (long)h + PAGE_SIZE;
	} while (cos_cmpxchg(&cos_comp_info.cos_heap_allocated, (long)h, r) != r);

	return h;
}

/* allocate and release a page in the vas */
extern void *cos_get_vas_page(void);
extern void  cos_release_vas_page(void *p);

/* only if the heap pointer is pre_addr, set it to post_addr */
static inline void
cos_set_heap_ptr_conditional(void *pre_addr, void *post_addr)
{
	cos_cmpxchg(&cos_comp_info.cos_heap_ptr, (long)pre_addr, (long)post_addr);
}

/* from linux source in string.h */
static inline void *
cos_memcpy(void *to, const void *from, int n)
{
	int d0, d1, d2;

	__asm__ __volatile__("rep ; movsl\n\t"
	                     "movl %4,%%ecx\n\t"
	                     "andl $3,%%ecx\n\t"
#if 1 /* want to pay 2 byte penalty for a chance to skip microcoded rep? */
	                     "jz 1f\n\t"
#endif
	                     "rep ; movsb\n\t"
	                     "1:"
	                     : "=&c"(d0), "=&D"(d1), "=&S"(d2)
	                     : "0"(n / 4), "g"(n), "1"((long)to), "2"((long)from)
	                     : "memory");

	return (to);
}

static inline void *
cos_memset(void *s, char c, int count)
{
	int d0, d1;
	__asm__ __volatile__("rep\n\t"
	                     "stosb"
	                     : "=&c"(d0), "=&D"(d1)
	                     : "a"(c), "1"(s), "0"(count)
	                     : "memory");
	return s;
}

/* compiler branch prediction hints */
#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define CFORCEINLINE __attribute__((always_inline))
#define CWEAKSYMB __attribute__((weak))
/*
 * A composite constructor (deconstructor): will be executed before
 * other component execution (after component execution).  CRECOV is a
 * function that should be called if one of the depended-on components
 * has failed (e.g. the function serves as a callback notification).
 */
#define CCTOR __attribute__((constructor))
#define CDTOR __attribute__((destructor)) /* currently unused! */
#define CRECOV(fnname) long crecov_##fnname##_ptr __attribute__((section(".crecov"))) = (long)fnname

static inline void
section_fnptrs_execute(long *list)
{
	int i;

	for (i = 0; i < list[0]; i++) {
		typedef void (*ctors_t)(void);
		ctors_t ctors = (ctors_t)list[i + 1];
		ctors();
	}
}

static void
constructors_execute(void)
{
	extern long __CTOR_LIST__;
	extern long __INIT_ARRAY_LIST__;
	section_fnptrs_execute(&__CTOR_LIST__);
	section_fnptrs_execute(&__INIT_ARRAY_LIST__);
}
static void
destructors_execute(void)
{
	extern long __DTOR_LIST__;
	extern long __FINI_ARRAY_LIST__;
	section_fnptrs_execute(&__DTOR_LIST__);
	section_fnptrs_execute(&__FINI_ARRAY_LIST__);
}
static void
recoveryfns_execute(void)
{
	extern long __CRECOV_LIST__;
	section_fnptrs_execute(&__CRECOV_LIST__);
}

#define FAIL() *(int *)NULL = 0

struct cos_array {
	char *mem;
	int   sz;
}; /* TODO: remove */
#define prevent_tail_call(ret) __asm__("" : "=r"(ret) : "m"(ret))
#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A"(val))

#ifndef STR
#define STRX(x) #x
#define STR(x) STRX(x)
#endif

struct __thd_init_data {
	void *fn;
	void *data;
};

typedef u32_t cbuf_t; /* TODO: remove when we have cbuf.h */

#endif
