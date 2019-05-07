/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hypervisor.h>
#include <multiboot.h>
#include <boot_context.h>

#define	GDT_ADDRESS	(2 * 1024 * 1024)
static const uint64_t unit_test_init_gdt[] = {
        0x0UL,
        0x00CF9B000000FFFFUL,   /* Linear Code */
        0x00CF93000000FFFFUL,   /* Linear Data */
};

static struct vm_io_range testdev_range = {
	.flags = IO_ATTR_RW,
	.base = 0xf4U,
	.len = 4U,
};

static uint32_t
testdev_io_read(__unused struct acrn_vm *vm, __unused uint16_t addr, __unused size_t bytes)
{
	return 0U;
}

static void
testdev_io_write(struct acrn_vm *vm, __unused uint16_t addr, __unused size_t bytes, __unused uint32_t val)
{
	uint16_t i;
	struct acrn_vcpu *vcpu = NULL;

	foreach_vcpu(i, vm, vcpu) {
		pause_vcpu(vcpu, VCPU_PAUSED);
	}
}

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
	//uint32_t kernel_entry_offset = 12U;  /* Size of the multiboot header */
	//change to 0x3c according to the realmode.elf  /* Size of the multiboot header */
	uint32_t kernel_entry_offset = 0x3c;  /* Size of the multiboot header */

	struct sw_kernel_info *sw_kernel = &(vm->sw.kernel_info);
	struct acrn_vcpu *vcpu = get_primary_vcpu(vm);
	struct acrn_vcpu_regs unit_test_context;

	pr_dbg("Loading guest to run-time location");

	prepare_bsp_gdt(vm);
	memset(&unit_test_context, 0, sizeof(unit_test_context));

	/* Hack: unit tests are always loaded at 4M, not 16M. */
	//sw_kernel->kernel_load_addr = (void *)(4 * 1024 * 1024UL);
	//load to 16K(0x4000) for realmode.elf
	sw_kernel->kernel_load_addr = (void *)(16 * 1024UL);

	sw_kernel->kernel_entry_addr =
		(void *)((uint64_t)sw_kernel->kernel_load_addr
			+ kernel_entry_offset);
	if (is_vcpu_bsp(vcpu)) {
		/* Set VCPU entry point to kernel entry */
		unit_test_context.rip = (uint64_t)sw_kernel->kernel_entry_addr;
		pr_info("%s, VM %hu VCPU %hu Entry: 0x%016llx ",
			__func__, vm->vm_id, vcpu->vcpu_id,
			sw_kernel->kernel_entry_addr);
	}

	/* Calculate the host-physical address where the guest will be loaded */
	hva = gpa2hva(vm, (uint64_t)sw_kernel->kernel_load_addr);

    memset((void *)hva, 0, 4 * 1024 * 1024);
	/* Copy the guest kernel image to its run-time location */
	(void)memcpy_s((void *)hva, sw_kernel->kernel_size,
				sw_kernel->kernel_src_addr,
				sw_kernel->kernel_size);

	hva = gpa2hva(vm, GDT_ADDRESS);
	(void)memcpy_s((void *)hva, sizeof(unit_test_init_gdt), &unit_test_init_gdt,
		sizeof(unit_test_init_gdt));

	unit_test_context.gdt.limit = sizeof(unit_test_init_gdt) - 1;
	unit_test_context.gdt.base = GDT_ADDRESS;

	/* CR0_ET | CR0_NE | CR0_PE */
	unit_test_context.cr0 = 0x31U;

	unit_test_context.cs_ar = 0xCF9BU;
	unit_test_context.cs_sel = 0x8U;
	unit_test_context.cs_limit = 0xFFFFFFFFU;

	unit_test_context.ds_sel = 0x10U;
	unit_test_context.ss_sel = 0x10U;
	unit_test_context.es_sel = 0x10U;
	unit_test_context.gs_sel = 0x10U;
	unit_test_context.fs_sel = 0x10U;

	set_vcpu_regs(vcpu, &unit_test_context);
	/* Documentation states:
	 *     eax = MULTIBOOT_INFO_MAGIC
	 *     ebx = physical address of multiboot info
	 */
	for (i = 0U; i < NUM_GPRS; i++) {
		vcpu_set_gpreg(vcpu, i, 0UL);
	}
	vcpu_set_gpreg(vcpu, CPU_REG_RAX, MULTIBOOT_INFO_MAGIC);
	vcpu_set_gpreg(vcpu, CPU_REG_RBX, create_multiboot_info(vm));

	register_io_emulation_handler(vm, TESTDEV_PIO_IDX, &testdev_range,
				      testdev_io_read, testdev_io_write);

	return ret;
}

void unit_test_init(void)
{
	vm_sw_loader = unit_test_sw_loader;
}
