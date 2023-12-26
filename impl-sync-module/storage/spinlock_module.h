#ifndef __SPINLOCKMOD_H
#define __SPINLOCKMOD_H
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <asm/current.h>
#include "calclock.h"

static struct task_struct* threads[4];
static int one = 1;
static int two = 2;
static int three = 3;
static int four = 4;
unsigned long long addCount, addTime, searchCount, searchTime, deleteCount, deleteTime;
spinlock_t list_lock;

struct my_node 
{
	int data;
	struct list_head list;
};

struct list_head root;

/*functions*/
void 	*add_to_list(int thread_id, int range_bound[]);
int 	search_list(int thread_id, void *data, int range_bound[]);
int	delete_from_list(int thread_id, int range_bound[]);

// thread fn
static int add_node_function(void* data);
int __init spinlock_module_init(void)
{
	printk("Entering Spinlock  Module!\n");
	INIT_LIST_HEAD(&root);
			
	threads[0] = kthread_run(add_node_function, (void *)&one, "list threads");
	threads[1] = kthread_run(add_node_function, (void *)&two, "list threads");
	threads[2] = kthread_run(add_node_function, (void *)&three, "list threads");
	threads[3] = kthread_run(add_node_function, (void *)&four, "list threads");

	return 0;
}

void __exit spinlock_module_cleanup(void)
{
	printk("Spinlock linked list insert time: %ld ns, count : %ld", addTime, addCount);
	printk("Exiting Compare and Swap module. bye 20194902 yechan !\, n");
	//temp node deletio
	struct my_node *pos, *next;
	list_for_each_entry_safe(pos, next, &root, list) {
		list_del(&pos->list);
		kfree(pos);
	}
}

module_init(spinlock_module_init);
module_exit(spinlock_module_cleanup);
MODULE_LICENSE("GPL");

#endif
