#include <linux/kernel.h>

asmlinkage long __sys_yechan(void)
{
	printk("Hello Yechan 20194902");
	return 0;
}
