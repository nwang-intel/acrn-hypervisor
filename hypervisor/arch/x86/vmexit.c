/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hypervisor.h>

/*
 * According to "SDM APPENDIX C VMX BASIC EXIT REASONS",
 * there are 65 Basic Exit Reasons.
 */
#define NR_VMX_EXIT_REASONS	65U

static int unhandled_vmexit_handler(struct acrn_vcpu *vcpu);
static int xsetbv_vmexit_handler(struct acrn_vcpu *vcpu);
static int wbinvd_vmexit_handler(struct acrn_vcpu *vcpu);
static int preemption_timeout_handler(struct acrn_vcpu *vcpu);

/* VM Dispatch table for Exit condition handling */
static const struct vm_exit_dispatch dispatch_table[NR_VMX_EXIT_REASONS] = {
	[VMX_EXIT_REASON_EXCEPTION_OR_NMI] = {
		.handler = exception_vmexit_handler},
	[VMX_EXIT_REASON_EXTERNAL_INTERRUPT] = {
		.handler = external_interrupt_vmexit_handler},
	[VMX_EXIT_REASON_TRIPLE_FAULT] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_INIT_SIGNAL] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_STARTUP_IPI] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_IO_SMI] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_OTHER_SMI] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_INTERRUPT_WINDOW] = {
		.handler = interrupt_window_vmexit_handler},
	[VMX_EXIT_REASON_NMI_WINDOW] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_TASK_SWITCH] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_CPUID] = {
		.handler = cpuid_vmexit_handler},
	[VMX_EXIT_REASON_GETSEC] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_HLT] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_INVD] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_INVLPG] = {
		.handler = unhandled_vmexit_handler,},
	[VMX_EXIT_REASON_RDPMC] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_RDTSC] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_RSM] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMCALL] = {
		.handler = vmcall_vmexit_handler},
	[VMX_EXIT_REASON_VMCLEAR] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMLAUNCH] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMPTRLD] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMPTRST] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMREAD] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMRESUME] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMWRITE] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMXOFF] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMXON] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_CR_ACCESS] = {
		.handler = cr_access_vmexit_handler,
		.need_exit_qualification = 1},
	[VMX_EXIT_REASON_DR_ACCESS] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_IO_INSTRUCTION] = {
		.handler = pio_instr_vmexit_handler,
		.need_exit_qualification = 1},
	[VMX_EXIT_REASON_RDMSR] = {
		.handler = rdmsr_vmexit_handler},
	[VMX_EXIT_REASON_WRMSR] = {
		.handler = wrmsr_vmexit_handler},
	[VMX_EXIT_REASON_ENTRY_FAILURE_INVALID_GUEST_STATE] = {
		.handler = unhandled_vmexit_handler,
		.need_exit_qualification = 1},
	[VMX_EXIT_REASON_ENTRY_FAILURE_MSR_LOADING] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_MWAIT] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_MONITOR_TRAP] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_MONITOR] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_PAUSE] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_ENTRY_FAILURE_MACHINE_CHECK] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_TPR_BELOW_THRESHOLD] = {
		.handler = tpr_below_threshold_vmexit_handler},
	[VMX_EXIT_REASON_APIC_ACCESS] = {
		.handler = apic_access_vmexit_handler,
		.need_exit_qualification = 1},
	[VMX_EXIT_REASON_VIRTUALIZED_EOI] = {
		.handler = veoi_vmexit_handler,
		.need_exit_qualification = 1},
	[VMX_EXIT_REASON_GDTR_IDTR_ACCESS] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_LDTR_TR_ACCESS] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_EPT_VIOLATION] = {
		.handler = ept_violation_vmexit_handler,
		.need_exit_qualification = 1},
	[VMX_EXIT_REASON_EPT_MISCONFIGURATION] = {
		.handler = ept_misconfig_vmexit_handler,
		.need_exit_qualification = 1},
	[VMX_EXIT_REASON_INVEPT] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_RDTSCP] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED] = {
		.handler = preemption_timeout_handler},
	[VMX_EXIT_REASON_INVVPID] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_WBINVD] = {
		.handler = wbinvd_vmexit_handler},
	[VMX_EXIT_REASON_XSETBV] = {
		.handler = xsetbv_vmexit_handler},
	[VMX_EXIT_REASON_APIC_WRITE] = {
		.handler = apic_write_vmexit_handler,
		.need_exit_qualification = 1},
	[VMX_EXIT_REASON_RDRAND] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_INVPCID] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_VMFUNC] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_ENCLS] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_RDSEED] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_XSAVES] = {
		.handler = unhandled_vmexit_handler},
	[VMX_EXIT_REASON_XRSTORS] = {
		.handler = unhandled_vmexit_handler}
};

int vmexit_handler(struct acrn_vcpu *vcpu)
{
	struct vm_exit_dispatch *dispatch = NULL;
	uint16_t basic_exit_reason;
	int ret;

	if (get_cpu_id() != vcpu->pcpu_id) {
		pr_fatal("vcpu is not running on its pcpu!");
		return -EINVAL;
	}

	/* Obtain interrupt info */
	vcpu->arch.idt_vectoring_info =
	    exec_vmread32(VMX_IDT_VEC_INFO_FIELD);
	/* Filter out HW exception & NMI */
	if ((vcpu->arch.idt_vectoring_info & VMX_INT_INFO_VALID) != 0U) {
		uint32_t vector_info = vcpu->arch.idt_vectoring_info;
		uint32_t vector = vector_info & 0xffU;
		uint32_t type = (vector_info & VMX_INT_TYPE_MASK) >> 8U;
		uint32_t err_code = 0U;

		if (type == VMX_INT_TYPE_HW_EXP) {
			if ((vector_info & VMX_INT_INFO_ERR_CODE_VALID) != 0U) {
				err_code = exec_vmread32(VMX_IDT_VEC_ERROR_CODE);
			}
			(void)vcpu_queue_exception(vcpu, vector, err_code);
			vcpu->arch.idt_vectoring_info = 0U;
		} else if (type == VMX_INT_TYPE_NMI) {
			vcpu_make_request(vcpu, ACRN_REQUEST_NMI);
			vcpu->arch.idt_vectoring_info = 0U;
		} else {
			/* No action on EXT_INT or SW exception. */
		}
	}

	/* Calculate basic exit reason (low 16-bits) */
	basic_exit_reason = (uint16_t)(vcpu->arch.exit_reason & 0xFFFFU);

	/* Log details for exit */
	pr_dbg("Exit Reason: 0x%016llx ", vcpu->arch.exit_reason);

	/* Ensure exit reason is within dispatch table */
	if (basic_exit_reason >= ARRAY_SIZE(dispatch_table)) {
		pr_err("Invalid Exit Reason: 0x%016llx ",
				vcpu->arch.exit_reason);
		return -EINVAL;
	}

	/* Calculate dispatch table entry */
	dispatch = (struct vm_exit_dispatch *)
		(dispatch_table + basic_exit_reason);

	/* See if an exit qualification is necessary for this exit
	 * handler
	 */
	if (dispatch->need_exit_qualification != 0U) {
		/* Get exit qualification */
		vcpu->arch.exit_qualification =
		    exec_vmread(VMX_EXIT_QUALIFICATION);
	}

	/* exit dispatch handling */
	if (basic_exit_reason == VMX_EXIT_REASON_EXTERNAL_INTERRUPT) {
		/* Handling external_interrupt
		 * should disable intr
		 */
		CPU_IRQ_DISABLE();
		ret = dispatch->handler(vcpu);
		CPU_IRQ_ENABLE();
	} else {
		ret = dispatch->handler(vcpu);
	}

	return ret;
}

static int unhandled_vmexit_handler(struct acrn_vcpu *vcpu)
{
	pr_fatal("Error: Unhandled VM exit condition from guest at 0x%016llx ",
			exec_vmread(VMX_GUEST_RIP));

	pr_fatal("Exit Reason: 0x%016llx ", vcpu->arch.exit_reason);

	pr_err("Exit qualification: 0x%016llx ",
			exec_vmread(VMX_EXIT_QUALIFICATION));

	TRACE_2L(TRACE_VMEXIT_UNHANDLED, vcpu->arch.exit_reason, 0UL);

	return 0;
}

int cpuid_vmexit_handler(struct acrn_vcpu *vcpu)
{
	uint64_t rax, rbx, rcx, rdx;

	rax = vcpu_get_gpreg(vcpu, CPU_REG_RAX);
	rbx = vcpu_get_gpreg(vcpu, CPU_REG_RBX);
	rcx = vcpu_get_gpreg(vcpu, CPU_REG_RCX);
	rdx = vcpu_get_gpreg(vcpu, CPU_REG_RDX);
	guest_cpuid(vcpu, (uint32_t *)&rax, (uint32_t *)&rbx,
		(uint32_t *)&rcx, (uint32_t *)&rdx);
	vcpu_set_gpreg(vcpu, CPU_REG_RAX, rax);
	vcpu_set_gpreg(vcpu, CPU_REG_RBX, rbx);
	vcpu_set_gpreg(vcpu, CPU_REG_RCX, rcx);
	vcpu_set_gpreg(vcpu, CPU_REG_RDX, rdx);

	TRACE_2L(TRACE_VMEXIT_CPUID, (uint64_t)vcpu->vcpu_id, 0UL);

	return 0;
}

int cr_access_vmexit_handler(struct acrn_vcpu *vcpu)
{
	uint64_t reg;
	uint32_t idx;
	uint64_t exit_qual;

	exit_qual = vcpu->arch.exit_qualification;
	idx = (uint32_t)vm_exit_cr_access_reg_idx(exit_qual);

	ASSERT((idx <= 15U), "index out of range");
	reg = vcpu_get_gpreg(vcpu, idx);

	switch ((vm_exit_cr_access_type(exit_qual) << 4U) |
			vm_exit_cr_access_cr_num(exit_qual)) {
	case 0x00UL:
		/* mov to cr0 */
		vcpu_set_cr0(vcpu, reg);
		break;
	case 0x04UL:
		/* mov to cr4 */
		vcpu_set_cr4(vcpu, reg);
		break;
	case 0x08UL:
		/* mov to cr8 */
		/* According to SDM 6.15 "Exception and interrupt Reference":
		 *
		 * set reserved bit in CR8 causes GP to guest
		 */
		if ((reg & ~0xFUL) != 0UL) {
			pr_dbg("Invalid cr8 write operation from guest");
			vcpu_inject_gp(vcpu, 0U);
			break;
		}
		vlapic_set_cr8(vcpu_vlapic(vcpu), reg);
		break;
	case 0x18UL:
		/* mov from cr8 */
		reg = vlapic_get_cr8(vcpu_vlapic(vcpu));
		vcpu_set_gpreg(vcpu, idx, reg);
		break;
	default:
		panic("Unhandled CR access");
		return -EINVAL;
	}

	TRACE_2L(TRACE_VMEXIT_CR_ACCESS, vm_exit_cr_access_type(exit_qual),
			vm_exit_cr_access_cr_num(exit_qual));

	return 0;
}

/*
 * XSETBV instruction set's the XCR0 that is used to tell for which
 * components states can be saved on a context switch using xsave.
 */
static int xsetbv_vmexit_handler(struct acrn_vcpu *vcpu)
{
	int idx;
	uint64_t val64;

	val64 = exec_vmread(VMX_GUEST_CR4);
	if ((val64 & CR4_OSXSAVE) == 0UL) {
		vcpu_inject_gp(vcpu, 0U);
		return 0;
	}

	idx = vcpu->arch.cur_context;
	if (idx >= NR_WORLD) {
		return -1;
	}

	/*to access XCR0,'rcx' should be 0*/
	if (vcpu_get_gpreg(vcpu, CPU_REG_RCX) != 0UL) {
		vcpu_inject_gp(vcpu, 0U);
		return 0;
	}

	val64 = (vcpu_get_gpreg(vcpu, CPU_REG_RAX) & 0xffffffffUL) |
			(vcpu_get_gpreg(vcpu, CPU_REG_RDX) << 32U);

	/*bit 0(x87 state) of XCR0 can't be cleared*/
	if ((val64 & 0x01UL) == 0UL) {
		vcpu_inject_gp(vcpu, 0U);
		return 0;
	}

	/*XCR0[2:1] (SSE state & AVX state) can't not be
	 *set to 10b as it is necessary to set both bits
	 *to use AVX instructions.
	 **/
	if (((val64 >> 1U) & 0x3UL) == 0x2UL) {
		vcpu_inject_gp(vcpu, 0U);
		return 0;
	}

	write_xcr(0, val64);
	return 0;
}

static int wbinvd_vmexit_handler(struct acrn_vcpu *vcpu)
{
	if (!iommu_snoop_supported(vcpu->vm)) {
		cache_flush_invalidate_all();
	}

	return 0;
}

#define HV_ARCH_X64_PREEMTION_TIMER_EXPIRY 40 /*timeout is 40ms*/
static int preemption_timeout_handler(struct acrn_vcpu *vcpu)
{
	uint64_t field;
	uint32_t value32;
	uint32_t preemtion_timer_freq;

	/* Get Preemption Timer Register */
	field = VMX_GUEST_TIMER;

	/* Calculate Preemption Timer Frequency */
	preemtion_timer_freq = ((tsc_khz * 1000U) >> (msr_read(MSR_IA32_VMX_MISC) & 0x1FUL));
	/* Calculate Number of ticks corresponding to expiry time */
	value32 = ((uint64_t) preemtion_timer_freq *
		   (uint64_t) HV_ARCH_X64_PREEMTION_TIMER_EXPIRY) / 1000;

	/* Write value into Preemption Timer Register */
	exec_vmwrite32(field, (uint64_t) value32);

	/*pr_fatal("Resetting VMX_GUEST_TIMER to Value: 0x%x ", value32);*/
	/* Kick HV-Shell and VirtIO-Console tasks */
	if (get_cpu_id() == BOOT_CPU_ID) {
		console_kick_handler();
	}

	/* Re-execute last instruction */

	vcpu_retain_rip(vcpu);

	return 0;
}
