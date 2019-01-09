/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hypervisor.h>
#include <multiboot.h>
#include <boot_context.h>

static void prepare_bsp_gdt(struct acrn_vm *vm)
{
	size_t gdt_len;
	uint64_t gdt_base_hpa;
	void *gdt_base_hva;

	gdt_base_hpa = gpa2hpa(vm, boot_context.gdt.base);
	if (boot_context.gdt.base == gdt_base_hpa) {
		return;
	} else {
		gdt_base_hva = hpa2hva(gdt_base_hpa);
		gdt_len = ((size_t)boot_context.gdt.limit + 1U) / sizeof(uint8_t);

		(void )memcpy_s(gdt_base_hva, gdt_len, hpa2hva(boot_context.gdt.base), gdt_len);
	}

	return;
}

static uint64_t create_multiboot_info(struct acrn_vm *vm)
{
	struct multiboot_info *boot_info;
	char *cmdline;
	struct sw_linux *sw_linux = &(vm->sw.linux_info);
	struct sw_kernel_info *sw_kernel = &(vm->sw.kernel_info);
	struct vm_description *vm_desc = pcpu_vm_desc_map[get_cpu_id()].vm_desc_ptr;
	uint64_t gpa_boot_info, gpa_cmdline;

	gpa_boot_info = (uint64_t)sw_kernel->kernel_load_addr - MEM_4K;
	boot_info = (struct multiboot_info *)gpa2hva(vm, gpa_boot_info);
	gpa_cmdline = gpa_boot_info + MEM_2K;
	cmdline = (char *)gpa2hva(vm, gpa_cmdline);

	boot_info->mi_flags = MULTIBOOT_INFO_HAS_MEMORY | MULTIBOOT_INFO_HAS_CMDLINE | MULTIBOOT_INFO_HAS_MODS;

	boot_info->mi_mem_lower = 0U;
	boot_info->mi_mem_upper = vm_desc->mem_size / MEM_1K;

	boot_info->mi_cmdline = (uint32_t)gpa_cmdline;
	strcpy_s(cmdline, MEM_2K, sw_linux->bootargs_src_addr);

	boot_info->mi_mods_count = 0U;

	return gpa_boot_info;
}

int32_t unit_test_sw_loader(struct acrn_vm *vm)
{
	int32_t ret = 0;
	uint32_t i;
	void *hva;
	uint32_t kernel_entry_offset = 12U;  /* Size of the multiboot header */
	struct sw_kernel_info *sw_kernel = &(vm->sw.kernel_info);
	struct acrn_vcpu *vcpu = get_primary_vcpu(vm);

	pr_dbg("Loading guest to run-time location");

	prepare_bsp_gdt(vm);
	set_vcpu_regs(vcpu, &boot_context);

	/* Hack: unit tests are always loaded at 4M, not 16M. */
	sw_kernel->kernel_load_addr = (void *)(4 * 1024 * 1024UL);

	sw_kernel->kernel_entry_addr =
		(void *)((uint64_t)sw_kernel->kernel_load_addr
			+ kernel_entry_offset);
	if (is_vcpu_bsp(vcpu)) {
		/* Set VCPU entry point to kernel entry */
		vcpu_set_rip(vcpu, (uint64_t)sw_kernel->kernel_entry_addr);
		pr_info("%s, VM %hu VCPU %hu Entry: 0x%016llx ",
			__func__, vm->vm_id, vcpu->vcpu_id,
			sw_kernel->kernel_entry_addr);
	}

	/* Calculate the host-physical address where the guest will be loaded */
	hva = gpa2hva(vm, (uint64_t)sw_kernel->kernel_load_addr);

	/* Copy the guest kernel image to its run-time location */
	(void)memcpy_s((void *)hva, sw_kernel->kernel_size,
				sw_kernel->kernel_src_addr,
				sw_kernel->kernel_size);

	/* Documentation states:
	 *     eax = MULTIBOOT_INFO_MAGIC
	 *     ebx = physical address of multiboot info
	 */
	for (i = 0U; i < NUM_GPRS; i++) {
		vcpu_set_gpreg(vcpu, i, 0UL);
	}
	vcpu_set_gpreg(vcpu, CPU_REG_RAX, MULTIBOOT_INFO_MAGIC);
	vcpu_set_gpreg(vcpu, CPU_REG_RBX, create_multiboot_info(vm));

	return ret;
}

void unit_test_init(void)
{
	vm_sw_loader = unit_test_sw_loader;
}
