/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hypervisor.h>

#define NUM_USER_VMS    1U

/* Number of CPUs in VM1 */
#define VM1_NUM_CPUS    2U

/* Logical CPU IDs assigned to this VM */
uint16_t VM1_CPUS[VM1_NUM_CPUS] = {0U, 2U};

static struct vpci_vdev_array vpci_vdev_array1 = {
	.num_pci_vdev = 0,

	.vpci_vdev_list = {
	 },
};

/*******************************/
/* User Defined VM definitions */
/*******************************/
struct vm_description_array vm_desc_partition = {
		/* Number of user virtual machines */
		.num_vm_desc = NUM_USER_VMS,

		/* Virtual Machine descriptions */
		.vm_desc_array = {
			{
				/* Internal variable, MUSTBE init to -1 */
				.vm_hw_num_cores = VM1_NUM_CPUS,
				.vm_pcpu_ids = &VM1_CPUS[0],
				.vm_id = 1U,
				.start_hpa = 0x100000000UL,
				.mem_size = 0x20000000UL, /* uses contiguous memory from host */
				.vm_vuart = true,
				.bootargs = "",
				.vpci_vdev_array = &vpci_vdev_array1,
				.mptable = &mptable_vm1,
#ifdef CONFIG_LAPIC_PT
				.lapic_pt = true,
#endif
			},
		}
};

const struct pcpu_vm_desc_mapping pcpu_vm_desc_map[] = {
	{
		.vm_desc_ptr = &vm_desc_partition.vm_desc_array[0],
		.is_bsp = true,
	},
	{
		.vm_desc_ptr = NULL,
		.is_bsp = false,
	},
	{
		.vm_desc_ptr = &vm_desc_partition.vm_desc_array[1],
		.is_bsp = false,
	},
	{
		.vm_desc_ptr = NULL,
		.is_bsp = false,
	},
};

const struct e820_entry e820_default_entries[NUM_E820_ENTRIES] = {
	{	/* 0 to mptable */
		.baseaddr =  0x0U,
		.length   =  0xEFFFFU,
		.type     =  E820_TYPE_RAM
	},

	{	/* mptable 65536U */
		.baseaddr =  0xF0000U,
		.length   =  0x10000U,
		.type     =  E820_TYPE_RESERVED
	},

	{	/* mptable to lowmem */
		.baseaddr =  0x100000U,
		.length   =  0x1FF00000U,
		.type     =  E820_TYPE_RAM
	},

	{	/* lowmem to PCI hole */
		.baseaddr =  0x20000000U,
		.length   =  0xa0000000U,
		.type     =  E820_TYPE_RESERVED
	},

	{	/* PCI hole to 4G */
		.baseaddr =  0xe0000000U,
		.length   =  0x20000000U,
		.type     =  E820_TYPE_RESERVED
	},
};
