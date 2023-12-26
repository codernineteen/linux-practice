#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>


static struct task_struct* threads[4];
static int shared_data = 0;


static int compare_and_swap_function(void *tid) 
{
	int original;

	while(!kthread_should_stop()) {
		original = __sync_val_compare_and_swap(&shared_data, shared_data, shared_data+1);
		if(original > 30) {
			int idx = *(int *)tid;
			if(idx > -1 && threads[idx]) 
			{
				threads[idx] = NULL;
			}
			break;
		}

		printk(KERN_INFO "pid[%u] %s: counter: %d\n", current->pid, __func__, original);
		msleep(50);
	}
	
	do_exit(0);
}


int __init cas_module_init(void)
{
	printk("Entering Compare and Swap  Module!\n");

	int i;
	for(i = 0; i < 4; i++) 
	{
		threads[i] = kthread_run(compare_and_swap_function, &i, "yechan-thread");
	}

	return 0;
}

void __exit cas_module_cleanup(void)
{
	printk("Exiting Compare and Swap module. bye 20194902 yechan !\n");
}

module_init(cas_module_init);
module_exit(cas_module_cleanup);
MODULE_LICENSE("GPL");
