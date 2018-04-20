#ifndef ISR_H
#define ISR_H

#include "shared/cos_types.h"
#include "chal/io.h"
#include "chal_asm_inc.h"
#include <inv.h>

enum
{
	IRQ_DIV_BY_ZERO_ERR_FAULT = 0,
	IRQ_DEBUG_TRAP,
	IRQ_BREAKPOINT_TRAP,
	IRQ_OVERFLOW_TRAP,
	IRQ_BOUND_RANGE_EXCEED_FAULT,
	IRQ_INVALID_OPCODE_FAULT,
	IRQ_DEVICE_NOT_AVAIL_FAULT,
	IRQ_DOUBLE_FAULT_ABORT = 8,
	IRQ_INVALID_TSS_FAULT  = 10,
	IRQ_SEG_NOT_PRESENT_FAULT,
	IRQ_STACK_SEG_FAULT,
	IRQ_GEN_PROTECT_FAULT,
	IRQ_PAGE_FAULT,
	IRQ_X87_FLOAT_PT_EXCEPT_FAULT,
	IRQ_ALIGN_CHECK_FAULT,
	IRQ_MACHINE_CHECK_ABORT,
	IRQ_SMID_FLOAT_PT_EXCEPT_FAULT,
	IRQ_VIRTUALIZATION_EXCEPT_FAULT = 20,
	IRQ_SECURITY_EXCEPT_FAULT       = 30,
};

extern void div_by_zero_err_fault_irq(struct pt_regs *);
extern void debug_trap_irq(struct pt_regs *);
extern void breakpoint_trap_irq(struct pt_regs *);
extern void overflow_trap_irq(struct pt_regs *);
extern void bound_range_exceed_fault_irq(struct pt_regs *);
extern void invalid_opcode_fault_irq(struct pt_regs *);
extern void device_not_avail_fault_irq(struct pt_regs *);
extern void double_fault_abort_irq(struct pt_regs *);
extern void invalid_tss_fault_irq(struct pt_regs *);
extern void seg_not_present_fault_irq(struct pt_regs *);
extern void stack_seg_fault_irq(struct pt_regs *);
extern void gen_protect_fault_irq(struct pt_regs *);
extern void page_fault_irq(struct pt_regs *);
extern void x87_float_pt_except_fault_irq(struct pt_regs *);
extern void align_check_fault_irq(struct pt_regs *);
extern void machine_check_abort_irq(struct pt_regs *);
extern void smid_float_pt_except_fault_irq(struct pt_regs *);
extern void virtualization_except_fault_irq(struct pt_regs *);
extern void security_except_fault_irq(struct pt_regs *);

extern void periodic_irq(struct pt_regs *);
extern void keyboard_irq(struct pt_regs *);
extern void handler_hw_34(struct pt_regs *);
extern void handler_hw_35(struct pt_regs *);
extern void serial_irq(struct pt_regs *);
extern void handler_hw_37(struct pt_regs *);
extern void handler_hw_38(struct pt_regs *);
extern void handler_hw_39(struct pt_regs *);
extern void oneshot_irq(struct pt_regs *);
extern void handler_hw_41(struct pt_regs *);
extern void handler_hw_42(struct pt_regs *);
extern void handler_hw_43(struct pt_regs *);
extern void handler_hw_44(struct pt_regs *);
extern void handler_hw_45(struct pt_regs *);
extern void handler_hw_46(struct pt_regs *);
extern void handler_hw_47(struct pt_regs *);
extern void handler_hw_48(struct pt_regs *);
extern void handler_hw_49(struct pt_regs *);
extern void handler_hw_50(struct pt_regs *);
extern void handler_hw_51(struct pt_regs *);
extern void handler_hw_52(struct pt_regs *);
extern void handler_hw_53(struct pt_regs *);
extern void handler_hw_54(struct pt_regs *);
extern void handler_hw_55(struct pt_regs *);
extern void handler_hw_56(struct pt_regs *);
extern void handler_hw_57(struct pt_regs *);
extern void handler_hw_58(struct pt_regs *);
extern void handler_hw_59(struct pt_regs *);
extern void handler_hw_60(struct pt_regs *);
extern void handler_hw_61(struct pt_regs *);
extern void handler_hw_62(struct pt_regs *);
extern void lapic_spurious_irq(struct pt_regs *);
extern void lapic_timer_irq(struct pt_regs *);

static void
ack_irq(int n)
{
	if (n >= 40) outb(0xA0, 0x20); /* Send reset signal to slave */
	outb(0x20, 0x20);
}

#endif /* ISR_H */
