/* drivers/android/letama_bootsel.c
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/console.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/io.h>

struct bootsel_buffer {
	uint32_t    sig;
	uint32_t    value;
};

#define BOOTSEL_SIG (0x54414d41) /* TAMA */

static struct bootsel_buffer *bootsel_buffer;


static int bootsel_driver_probe(struct platform_device *pdev)
{
	struct resource *res = pdev->resource;
	size_t start;
	size_t buffer_size;
	void *buffer;
	printk("PL:bootsel_driver_probe()\n");

	if (res == NULL || pdev->num_resources != 1 ||
	    !(res->flags & IORESOURCE_MEM)) {
		printk(KERN_ERR "bootsel: invalid resource, %p %d flags "
		       "%lx\n", res, pdev->num_resources, res ? res->flags : 0);
		return -ENXIO;
	}
	buffer_size = res->end - res->start + 1;
	start = res->start;
	printk(KERN_INFO "bootsel: got buffer at %zx, size %zx\n",
	       start, buffer_size);
	buffer = ioremap(res->start, buffer_size);
	if (buffer == NULL) {
		printk(KERN_ERR "bootsel: failed to map memory\n");
		return -ENOMEM;
	}

	bootsel_buffer = buffer;
	if(bootsel_buffer->sig == BOOTSEL_SIG) {
	  printk("PL: bootsel sig was in memory, value=%X\n", bootsel_buffer->value);
	}
	bootsel_buffer->sig = BOOTSEL_SIG;
	return 0;
}

static struct platform_driver bootsel_driver = {
	.probe = bootsel_driver_probe,
	.driver		= {
		.name	= "bootsel",
	},
};

static int __init bootsel_module_init(void)
{
	int err;
	err = platform_driver_register(&bootsel_driver);
	return err;
}

static ssize_t bootsel_read(char *page, char **start,
			    off_t off, int count,
			    int *eof, void *data)
{
  printk("PL:bootsel_read\n");
  return sprintf(page, "%u\n", bootsel_buffer->value);
}

static ssize_t bootsel_write(struct file *file,
                             const char *buffer,
                             unsigned long count, 
                             void *data)
{
  unsigned char c;
  printk("PL:bootsel_write:\n");
  if(copy_from_user(&c, buffer,1)) {
    printk("PL: copy_from_user failed\n");
    return 0;
  }
  bootsel_buffer->value = (unsigned int)(c - '0');
  printk("PL: new value is %u\n", bootsel_buffer->value);
  return count;
}


static int __init bootsel_late_init(void)
{
	struct proc_dir_entry *entry;
	printk("PL:bootsel_late_init()\n");

	entry = create_proc_entry("bootsel", 666, NULL);
	if (!entry) {
		printk(KERN_ERR "bootsel: failed to create proc entry\n");
		return 0;
	}
	entry->read_proc = bootsel_read;
	entry->write_proc = bootsel_write;
	return 0;
}

postcore_initcall(bootsel_module_init);
late_initcall(bootsel_late_init);

