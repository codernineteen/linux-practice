#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <linux/time.h>

#define BILLION 1000000000UL


static struct task_struct *threads[4];

static int one = 1;
static int two = 2;
static int three = 3;
static int four = 4;

unsigned long long addCount, addTime, searchCount, searchTime, deleteCount, deleteTime;
static int count = 0;

// define rw sem
struct rw_semaphore rwsem;

// initialize list
struct my_node 
{
	int data;
	struct list_head list;
};

struct list_head root;

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
	printk(KERN_INFO "thread #%d range: %d ~ %d\n",thread_id, range_bound[0], range_bound[1]-1);
	// Add node to list logic
	struct my_node *first;

	unsigned int i;
	for(i = range_bound[0]; i < range_bound[1]; i++) 
	{
	
		getrawmonotonic(&localclock[0]); // start calclock
		struct my_node *temp = (struct my_node*)kmalloc(sizeof(struct my_node), GFP_KERNEL);
		if(!temp)
	       	{
			printk(KERN_ERR "Failed to allocate memory in add_to_list while looping");
			return NULL;
		}

		if(i == range_bound[0])
		{
			first = temp;
		}

		// critical section starts (start to add list node)
		down_write(&rwsem);

		temp->data = count++;
		list_add(&temp->list, &root);

		up_write(&rwsem);
		// critical section ends

		getrawmonotonic(&localclock[1]); // end of calclock
		calclock(localclock, &addTime, &addCount);
		
	}
	return first;
}

// search nodes in list
int search_list(int thread_id, void *data, int range_bound[])
{
      	struct timespec localclock[2];
       	/* This will point on the actual data structures during the iteration */
      	struct my_node *cur = (struct my_node *) data, *tmp;
      	// put your code here

	int search_count = 0;
	
	while(search_count < 250000)
	{
		
		getrawmonotonic(&localclock[0]);
		
		down_read(&rwsem);
			tmp = list_next_entry(cur, list);
			if(tmp)
			{
				search_count++;
				cur->data; //access data
				cur = tmp;
			}
		up_read(&rwsem);
		
		getrawmonotonic(&localclock[1]);
		calclock(localclock, &searchTime, &searchCount);
	}
	printk("thread #%d searched range: %d ~ %d",thread_id, range_bound[0], range_bound[1]-1);

	return 0;
}


//delete a node from list
int delete_from_list(int thread_id,int range_bound[])
{
      	struct my_node *cur, *tmp;
      	struct timespec localclock[2];

	list_for_each_entry_safe(cur, tmp, &root, list) {
		getrawmonotonic(&localclock[0]);
		
		down_write(&rwsem);
			list_del(&cur->list);
			kfree(cur);
			count--;
		up_write(&rwsem);
		
		getrawmonotonic(&localclock[1]);
		calclock(localclock, &deleteTime, &deleteCount);
	}		
	printk("thread #%d deleted range: %d ~ %d",thread_id, range_bound[0], range_bound[1]-1);

	
	return 0;

}

static int work_function(void* data) {
	int idx = *(int *)data;
	int range_bound[2] = {(idx-1)*250000, (idx)*250000};

	//start insert
	struct my_node *first_node = (struct my_node *)add_to_list(idx, range_bound);
	//after insert
	
	// start search
	search_list(idx, (void *)first_node, range_bound);
	// after search 

	//start delete after searching
	while(searchCount < 1000000)
	{
		msleep(500);
	}
	
		
		
	delete_from_list(idx,  range_bound);	
	//after delete
	do_exit(0);
}


int __init rwsem_module_init(void)
{
	printk("rwsem_moduel_init : Entering RWsemaphore  Module!\n");
	INIT_LIST_HEAD(&root);
	init_rwsem(&rwsem);
	
	threads[0] = kthread_run(work_function, (void *)&one, "list threads");	
	threads[1] = kthread_run(work_function, (void *)&two, "list threads");	
	threads[2] = kthread_run(work_function, (void *)&three, "list threads");	
	threads[3] = kthread_run(work_function, (void *)&four, "list threads");	

	return 0;
}

void __exit rwsem_module_cleanup(void)
{
	
	printk("rwsem_moduel_cleanup : RWsemaphore linked list insert time: %lld ns, count : %lld \n", addTime, addCount);	
	printk("rwsem_moduel_cleanup : RWsemaphore linked list search time: %lld ns, count : %lld \n", searchTime, searchCount);
	printk("rwsem_moduel_cleanup : RWsemaphore linked list delete time: %lld ns, count : %lld \n", deleteTime, deleteCount);

	
	printk("thread #0 stopped");
	printk("thread #1 stopped");
	printk("thread #2 stopped");
	printk("thread #3 stopped");


	printk("rwsem_module_cleanup : Exiting RWsemaphore module. bye 20194902 yechan !\n");
}


module_init(rwsem_module_init);
module_exit(rwsem_module_cleanup);
MODULE_LICENSE("GPL");


