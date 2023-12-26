#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>


static struct task_struct* threads[4];
static int shared_data = 0;


static int test_and_set_function(void *tid) 
{
	int original;

	while(!kthread_should_stop()) {
		original = __sync_lock_test_and_set(&shared_data, shared_data+1);
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


int __init testset_module_init(void)
{
	printk("Entering Test and Set  Module!\n");

	int i;
	for(i = 0; i < 4; i++) 
	{
		threads[i] = kthread_run(test_and_set_function, &i, "yechan-thread");
	}

	return 0;
}

void __exit testset_module_cleanup(void)
{
	
	printk("Exiting Test and Set module. bye 20194902 yechan !\n");

}

module_init(testset_module_init);
module_exit(testset_module_cleanup);
MODULE_LICENSE("GPL");
