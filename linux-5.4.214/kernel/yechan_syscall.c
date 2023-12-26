#include <linux/syscalls.h>

SYSCALL_DEFINE0(yechan_call)
{
	
	printk("20194902 yechan jun system call test\n");

	return 0;
}
