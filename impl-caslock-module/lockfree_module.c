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
#include <linux/atomic.h>

#define BILLION 1000000000UL


static struct task_struct* threads[4];
static int one = 1;
static int two = 2;
static int three = 3;
static int four = 4;
unsigned long long addCount, addTime, searchCount, searchTime, deleteCount, deleteTime;
// add_to_garbage definition
static void add_to_garbage_list(int thread_id, void* data);

// initialize list
struct animal
{
	int total;
	struct list_head entry;
	atomic_t removed;
	struct list_head gc_entry;
};

struct cat 
{
	int var;
	struct list_head entry;
	atomic_t removed;
	struct list_head gc_entry;
};

struct lock 
{
	int held;
};

// root struct : animal
static struct animal *head;
// cas lock
static struct lock *cas;

// way to initialize lock
void init_lock(struct lock *lock_v)
{
	lock_v->held = 0; // initial lock value is zero
}

//acquire & release routine
void acquire(struct lock *lock_v)
{
	while(__sync_val_compare_and_swap(&lock_v->held, 0, 1));
}
void release(struct lock *lock_v) 
{
	__sync_lock_test_and_set(&lock_v->held, 0);
}


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
	struct cat *new, *first = NULL;
	int i;

	for(i = range_bound[0]; i < range_bound[1]; i++) 
	{
	
		getrawmonotonic(&localclock[0]); // start calclock
		// allocate memory for new cat
		new = (struct cat*)kmalloc(sizeof(struct cat), GFP_KERNEL);
		new->var = i; 
		atomic_set(&new->removed, 0); // initialize remove field
		if(!first)
			first = new; // if first is NULL, assign new cat into first

		// initialize list entries
		INIT_LIST_HEAD(&new->entry);
		INIT_LIST_HEAD(&new->gc_entry);
		
		// compete to insert node to list
		acquire(cas);
		list_add(&new->entry, &head->entry);
		head->total++; // if success, increment total count
		release(cas);
		

		getrawmonotonic(&localclock[1]); // end of calclock
		calclock(localclock, &addTime, &addCount);
		
	}

	
	printk(KERN_INFO "thread #%d: inserted cat # %d ~ %d to the list, total: %u cats\n",thread_id-1, first->var, new->var, head->total);
	return first;
}


int search_list(int thread_id,  int range_bound[])
{
	struct list_head *entry, *iter = &head->entry;
      	struct cat *cur;
      	struct timespec localclock[2];
	int target_idx = range_bound[0] + 100000;

	int flag = 0;
	list_for_each_entry(cur, iter, entry) {
		if(atomic_read(&cur->removed) == 1) continue;
		getrawmonotonic(&localclock[0]);
		if(cur->var == target_idx)
		{
			// if found target data, set flag as one
			flag = 1;
			printk("found cat #%d", cur->var);	
		}
		getrawmonotonic(&localclock[1]);
		calclock(localclock, &searchTime, &searchCount);
	}

	if(flag == 0) {
		return ENODATA; // if target not existed
	}
	

	return -ENODATA;
}

int delete_from_list(int thread_id, int range_bound[])
{
      	int start = range_bound[0], end = range_bound[1];
	struct timespec localclock[2];
	struct list_head *entry, *iter = &head->entry;
       	/* This will point on the actual data structuries during the iteration */

	struct cat *cur;
	int target_idx = range_bound[0];

	list_for_each_entry(cur, iter, entry) {
		// check whether current cat is needed to remove.
		if(atomic_read(&cur->removed) == 1) continue;
		getrawmonotonic(&localclock[0]);
		// if a cat is within range bound
		if(cur->var >= start && cur->var <= end)
		{
			// add it to gc_entry
			add_to_garbage_list(thread_id, cur);	
		}
		getrawmonotonic(&localclock[1]);
		calclock(localclock, &deleteTime, &deleteCount);
	}
	
	printk(KERN_INFO "thread #%d: marked cat #%d~%d as deleted, total: %u cats\n", thread_id-1, range_bound[0], range_bound[1]-1, head->total);

	
	return 0;

}

static void add_to_garbage_list(int thread_id, void* data)
{
	struct cat *target = (struct cat*) data;
	
	// when only removed is zero at this moment, go inside condition scope and swap it with one
	if(atomic_cmpxchg(&target->removed, 0, 1)==0) {
		acquire(cas);
			list_add(&target->gc_entry, &head->gc_entry);
			head->total--; // decrement total count
		release(cas);
	}
}

static int work_function(void* data) {
	int err, thread_id = *(int *)data;
	int range_bound[2] = {(thread_id-1)*250000, (thread_id)*250000};

	add_to_list(thread_id, range_bound);
	err = search_list(thread_id, range_bound);
	if(err != ENODATA) {
		delete_from_list(thread_id, range_bound);
	}

	
	// wait  module removal.
	while(!kthread_should_stop()) {
		msleep(500);
	}


	printk(KERN_INFO "thread #%d stopped!\n", thread_id);
	
	
	return 0;
}


int __init lockfree_module_init(void)
{
	printk("lockfree_module_init : Entering Lockfree  Module!\n");

	// allocate memory for head and cas lock
	head = (struct animal*)kmalloc(sizeof(struct animal), GFP_KERNEL);
	cas = (struct lock*)kmalloc(sizeof(struct lock), GFP_KERNEL);
	
	// set total and removed as zero
	head->total = 0;
	atomic_set(&head->removed, 0);
	// initialize cas lock
	init_lock(cas);

	// init animal's list head & gc_list head	
	INIT_LIST_HEAD(&head->entry);
	INIT_LIST_HEAD(&head->gc_entry);

	// Run four threads.
	threads[0] = kthread_run(work_function, (void *)&one, "list threads");
	threads[1] = kthread_run(work_function, (void *)&two, "list threads");
	threads[2] = kthread_run(work_function, (void *)&three, "list threads");
	threads[3] = kthread_run(work_function, (void *)&four, "list threads");


	return 0;
}

int empty_garbage_list(void)
{
	struct cat *cur, *tmp;
	struct list_head *iter = &head->gc_entry;
	unsigned int counter = 0;


	// iter garbage list
	list_for_each_entry_safe(cur, tmp, iter, gc_entry) {
		list_del(&cur->gc_entry); // delete gc_entry
		counter++; // increment count
	}
	
	printk(KERN_INFO "%s: freed %u cats\n", __func__, counter);

	return 0;
}

void destroy_list(void) 
{
	struct cat *cur, *tmp;
	struct list_head *iter = &head->entry;

	// iter list
	list_for_each_entry_safe(cur, tmp, iter, entry) {
		list_del(&cur->entry); // delete entry
		//head->total--;
		kfree(cur); // free memory
	}
}

void __exit lockfree_module_cleanup(void)
{
	//print calclock
	printk("lockfree_module_cleanup : lockfree linked list insert time: %lld ns, count : %lld \n", addTime, addCount);	
	printk("lockfree_module_cleanup : lockfree linked list search time: %lld ns, count : %lld \n", searchTime, searchCount);
	printk("lockfree_module_cleanup : lockfree linked list delete time: %lld ns, count : %lld \n", deleteTime, deleteCount);

	// stop every threads
	int i;
	for(i = 0; i < 4; i++)
	{
		kthread_stop(threads[i]);
		threads[i] = NULL;
	}

	// clear list and deallocate dynamically allocated memory.
	empty_garbage_list();
	printk("After gc: total %d cats\n", head->total);

	destroy_list();
	printk("After destroyed list: total %d cats\n", head->total);
	kfree(head);
	kfree(cas);

	printk("%s: Exiting lockfree module.! Bye 20194902 Yechan Jun\n", __func__);
}


module_init(lockfree_module_init);
module_exit(lockfree_module_cleanup);
MODULE_LICENSE("GPL");


