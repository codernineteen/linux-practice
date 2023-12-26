#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <linux/time.h>

#define BILLION 1000000000UL


static struct task_struct* threads[4];
static int one = 1;
static int two = 2;
static int three = 3;
static int four = 4;
unsigned long long addCount, addTime, searchCount, searchTime, deleteCount, deleteTime;
static int found = 0;

// define mutex
spinlock_t list_lock;


// initialize list
struct animal
{
	int total;
	struct list_head entry;
};

struct cat
{
	int var;
	struct list_head entry;
};

static struct animal *head;

// calclock code
unsigned long long calclock(struct timespec *myclock,
		unsigned long long *total_time, unsigned long long *total_count)
{
	unsigned long long timedelay = 0, temp = 0, temp_n = 0;
	if (myclock[1].tv_nsec >= myclock[0].tv_nsec) {
		temp = myclock[1].tv_sec - myclock[0].tv_sec;
		temp_n = myclock[1].tv_nsec - myclock[0].tv_nsec;
		timedelay = BILLION * temp + temp_n;
	} else {
		temp = myclock[1].tv_sec - myclock[0].tv_sec - 1;
		temp_n = BILLION + myclock[1].tv_nsec - myclock[0].tv_nsec;
		timedelay = BILLION * temp + temp_n;
	}
	__sync_fetch_and_add(total_time, timedelay);
	__sync_fetch_and_add(total_count, 1);
	return timedelay;
}

// add a node to list
void *add_to_list(int thread_id, int range_bound[])
{
	struct timespec localclock[2];
	// Add node to list logic
	struct cat *new, *first = NULL;

	unsigned int i;
	for(i = range_bound[0]; i < range_bound[1]; i++) 
	{
	
		getrawmonotonic(&localclock[0]); // start calclock
		new = (struct cat*)kmalloc(sizeof(struct cat), GFP_KERNEL);
		
		if(!first)
			first = new;

		// critical section starts (start to add list node)
		spin_lock(&list_lock);

		new->var = i;
		list_add(&new->entry, &head->entry);
		head->total++;
		spin_unlock(&list_lock);
		// critical section ends

		getrawmonotonic(&localclock[1]); // end of calclock
		calclock(localclock, &addTime, &addCount);
		
	}
	
	printk(KERN_INFO "thread#%d inserted cat :  #%d ~ %d to the list, total: %u cats\n",thread_id-1, first->var, new->var, head->total);
	return first;
}

// search nodes in list
int search_list(int thread_id, void *data, int range_bound[])
{
      	struct timespec localclock[2];
       	/* This will point on the actual data structures during the iteration */
      	struct cat *cur = (struct cat *) data, *tmp;
	int target_idx = range_bound[0] + 100000;

	spin_lock(&list_lock);
	list_for_each_entry(cur, &head->entry, entry) {
		getrawmonotonic(&localclock[0]);
		if(cur->var == target_idx)
		{
			found++;
			printk("found cat #%d", cur->var);
			getrawmonotonic(&localclock[1]);
			calclock(localclock, &searchTime, &searchCount);
			break;
		}
		getrawmonotonic(&localclock[1]);
		calclock(localclock, &searchTime, &searchCount);
	}	
	spin_unlock(&list_lock);

	return 0;
}


//delete a node from list
int delete_from_list(int thread_id, int range_bound[])
{
      	struct cat *cur, *tmp;
      	struct timespec localclock[2];

	list_for_each_entry_safe(cur, tmp, &head->entry, entry) {
		spin_lock(&list_lock);
		if(head->total > 0) 
		{
			getrawmonotonic(&localclock[0]);
			list_del(&cur->entry);
			kfree(cur);
			head->total--;
			getrawmonotonic(&localclock[1]);
			calclock(localclock, &deleteTime, &deleteCount);
		}
		spin_unlock(&list_lock);
	}	
	
	printk(KERN_INFO "thread #%d deleted range: %d ~ %d, total: %u cats\n",thread_id-1, range_bound[0], range_bound[1]-1, head->total);

	
	return 0;

}

static int work_function(void* data) {
	int idx = *(int *)data;
	int range_bound[2] = {(idx-1)*250000, (idx)*250000};

	//start insert
	struct cat *first_node = (struct cat *)add_to_list(idx, range_bound);
	//after insert
	
	//start search
	search_list(idx, (void *)first_node, range_bound);
	//after search
	
	while(found < 4) 
	{
		msleep(500);
	}

	//start delete
	delete_from_list(idx, range_bound);	
	//after delete


	while(!kthread_should_stop())
		msleep(500);

	return 0;
}


int __init spinlock_module_init(void)
{
	printk("spinlock_module_init : Entering Spinlock  Module!\n");
	head = (struct animal*)kmalloc(sizeof(struct animal), GFP_KERNEL);
	
	head->total = 0;
	INIT_LIST_HEAD(&head->entry);
	
	threads[0] = kthread_run(work_function, (void *)&one, "list threads");
	threads[1] = kthread_run(work_function, (void *)&two, "list threads");
	threads[2] = kthread_run(work_function, (void *)&three, "list threads");
	threads[3] = kthread_run(work_function, (void *)&four, "list threads");


	return 0;
}

void __exit spinlock_module_cleanup(void)
{
	
	printk("spinlock_module_cleanup : Spinlock linked list insert time: %lld ns, count : %lld \n", addTime, addCount);	
	printk("spinlock_module_cleanup : Spinlock linked list search time: %lld ns, count : %lld \n", searchTime, searchCount);
	printk("spinlock_module_cleanup : Spinlock linked list delete time: %lld ns, count : %lld \n", deleteTime, deleteCount);

	int i;
	for(i = 0; i<4; i++)
	{
		kthread_stop(threads[i]);
		threads[i] = NULL;
	}

	printk("spinlock_module_cleanup : Exiting Spinlock module. bye 20194902 yechan !\n");
}


module_init(spinlock_module_init);
module_exit(spinlock_module_cleanup);
MODULE_LICENSE("GPL");


