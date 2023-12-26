#include <linux/kernel.h>  // All modules
#include <linux/module.h>
#include <linux/init.h>    // For macros
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/slab.h>   //kmalloc()
#include <linux/wait.h>   //For wait queues
#include <linux/delay.h>

struct task_struct *waiters[8];
struct task_struct *waker;
struct wait_queue_head wait_queue;
int wait_queue_flag = 0;
int indexs[8] = {0,1,2,3,4,5,6,7};
int waker_types[2] = {1,2};

int waiter_type_1_fn(void *data) {
	int index = *(int*) data;
	DEFINE_WAIT(wait);
	add_wait_queue(&wait_queue, &wait);
	pr_info("Waiter %d is added to the wait queue\n", index);
	while(wait_queue_flag != 1) {
		prepare_to_wait(&wait_queue, &wait, TASK_INTERRUPTIBLE);
	        schedule();
	}
	finish_wait(&wait_queue, &wait);
	pr_info("Condition meets, so waiter %d of type 1 finishes waiting\n", index);
	return 0;
}

int waiter_type_2_fn(void* data) {
	int index =  *(int*) data;
	DEFINE_WAIT(wait);
	add_wait_queue(&wait_queue, &wait);
	pr_info("Waiter %d is added to the wait queue\n", index);
	while(wait_queue_flag != 2) {
		prepare_to_wait(&wait_queue, &wait, TASK_INTERRUPTIBLE);
	        schedule();
	}
	finish_wait(&wait_queue, &wait);
	pr_info("Condition meets, so waiter %d of type 2 finishes waiting\n", index);
	return 0;

}

int waker_fn(void *data) {
	int flag = *(int*) data;
	wait_queue_flag = flag;
	printk("Current wait_queue_flag is %d", wait_queue_flag);
	wake_up_interruptible(&wait_queue);
	return 0;
}

int __init wait_module_init(void) {
	printk("Initialize a wait queue!\n");
	// Initialize wait queue
	init_waitqueue_head(&wait_queue);
	// Create waiting threads
	int i;
	for (i = 0; i < 4; i++) {
		waiters[i] = kthread_run(waiter_type_1_fn, &indexs[i], "waiting threads");
		waiters[i+4] = kthread_run(waiter_type_2_fn, &indexs[i+4], "waiting threads");
	}
	return 0;
}

void __exit wait_module_exit(void) {
	if (waitqueue_active(&wait_queue)) {
		waker = kthread_run(waker_fn, &waker_types[0], "waker thread");
		waker = kthread_run(waker_fn, &waker_types[1], "waker thread");
	} 
		
}



module_init(wait_module_init);
module_exit(wait_module_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lan Anh Nguyen, SySLAB");
MODULE_DESCRIPTION("Simple example for waitqueue");
