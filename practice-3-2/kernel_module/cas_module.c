#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

// define spin lock here



// initialize list here



int __init hello_module_init(void) {
	printk("HELLO MODULE\n");
	return 0;
}


void __exit hello_module_cleanup(void) {
	printk("bye module\n");
}

module_init(hello_module_init);
module_exit(hello_module_cleanup);
MODULE_LICENSE("GPL");
