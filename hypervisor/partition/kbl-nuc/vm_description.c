/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hypervisor.h>

#define NUM_USER_VMS    2U

/* Number of CPUs in VM1 */
#define VM1_NUM_CPUS    2U

/* Logical CPU IDs assigned to this VM */
uint16_t VM1_CPUS[VM1_NUM_CPUS] = {0U, 2U};

/* Number of CPUs in VM2 */
#define VM2_NUM_CPUS    2U

/* Logical CPU IDs assigned with this VM */
uint16_t VM2_CPUS[VM2_NUM_CPUS] = {3U, 1U};

static struct vpci_vdev_array vpci_vdev_array1 = {
	.num_pci_vdev = 3,

	.vpci_vdev_list = {
	 {/*vdev 0: hostbridge */
	  .vbdf.bits = {.b = 0x00U, .d = 0x00U, .f = 0x0U},
	  .ops = &pci_ops_vdev_hostbridge,
	  .bar = {},
	  .pdev = {
		 .bdf.bits = {.b = 0x00U, .d = 0x00U, .f = 0x0U},
		}
	 },
	 {/*vdev 1: SATA controller*/
	  .vbdf.bits = {.b = 0x00U, .d = 0x01U, .f = 0x0U},
	  .ops = &pci_ops_vdev_pt,
	  .bar = {
			[0] = {
			.base = 0UL,
			.size = 0x2000UL,
			.type = PCIBAR_MEM32
			},
			[1] = {
			.base = 0UL,
			.size = 0x1000UL,
			.type = PCIBAR_MEM32
			},
			[5] = {
			.base = 0UL,
			.size = 0x1000UL,
			.type = PCIBAR_MEM32
			},
	  },
	 .pdev = {
		.bdf.bits = {.b = 0x00U, .d = 0x17U, .f = 0x0U},
		.bar = {
			[0] = {
			.base = 0xdf248000UL,
			.size = 0x2000UL,
			.type = PCIBAR_MEM32
			},
			[1] = {
			.base = 0xdf24c000UL,
			.size = 0x100UL,
			.type = PCIBAR_MEM32
			},
			[5] = {
			.base = 0xdf24b000UL,
			.size = 0x800UL,
			.type = PCIBAR_MEM32
			},
		 }
		}
	 },

	 {/*vdev 2: Ethernet*/
	  .vbdf.bits = {.b = 0x00U, .d = 0x02U, .f = 0x0U},
	  .ops = &pci_ops_vdev_pt,
	  .bar = {
			[0] = {
			.base = 0UL,
			.size = 0x20000UL,
			.type = PCIBAR_MEM32
			},
	  },
	 .pdev = {
		.bdf.bits = {.b = 0x00U, .d = 0x1fU, .f = 0x6U},
		.bar = {
			[0] = {
			.base = 0xdf200000UL,
			.size = 0x20000UL,
			.type = PCIBAR_MEM32
			},
		 }
		}
	 },
	}
};

static struct vpci_vdev_array vpci_vdev_array2 = {
	.num_pci_vdev = 2,

	.vpci_vdev_list = {
	 {/*vdev 0: hostbridge*/
	  .vbdf.bits = {.b = 0x00U, .d = 0x00U, .f = 0x0U},
	  .ops = &pci_ops_vdev_hostbridge,
	  .bar = {},
	  .pdev = {
			.bdf.bits = {.b = 0x00U, .d = 0x00U, .f = 0x0U},
		}
	 },

	 {/*vdev 1: USB controller*/
	  .vbdf.bits = {.b = 0x00U, .d = 0x01U, .f = 0x0U},
	  .ops = &pci_ops_vdev_pt,
	  .bar = {
			[0] = {
			.base = 0UL,
			.size = 0x40000UL,
			.type = PCIBAR_MEM32
		 },
	  },
	 .pdev = {
		.bdf.bits = {.b = 0x00U, .d = 0x14U, .f = 0x0U},
		.bar = {
			[0] = {
			.base = 0xdf230000UL,
			.size = 0x40000UL,
			.type = PCIBAR_MEM64
			},
		 }
		}
	 },
	}
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
				.bootargs = "root=PARTUUID=53aa36dd-91f5-4830-b4a5-eae53b680700 rw rootwait noxsave maxcpus=2 nohpet console=hvc0 \
						console=ttyS0 no_timer_check ignore_loglevel log_buf_len=16M \
						consoleblank=0 tsc=reliable xapic_phys apic_debug",
				.vpci_vdev_array = &vpci_vdev_array1,
				.mptable = &mptable_vm1,
#ifdef CONFIG_LAPIC_PT
				.lapic_pt = true,
#endif
			},

			{
				/* Internal variable, MUSTBE init to -1 */
				.vm_hw_num_cores = VM2_NUM_CPUS,
				.vm_pcpu_ids = &VM2_CPUS[0],
				.vm_id = 2U,
				.start_hpa = 0x120000000UL,
				.mem_size = 0x20000000UL, /* uses contiguous memory from host */
				.vm_vuart = true,
				.bootargs = "root=PARTUUID=a148bfdf-b924-4da8-b40f-27688ee97b64 rw rootwait noxsave maxcpus=2 nohpet console=hvc0 \
						console=ttyS0 no_timer_check ignore_loglevel log_buf_len=16M \
						consoleblank=0 tsc=reliable xapic_phys apic_debug",
				.vpci_vdev_array = &vpci_vdev_array2,
				.mptable = &mptable_vm2,
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
		.vm_desc_ptr = &vm_desc_partition.vm_desc_array[1],
		.is_bsp = false,
	},
	{
		.vm_desc_ptr = &vm_desc_partition.vm_desc_array[0],
		.is_bsp = false,
	},
	{
		.vm_desc_ptr = &vm_desc_partition.vm_desc_array[1],
		.is_bsp = true,
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
