#include <linux/xarray.h>
#include <asm/current.h>

struct ds_monitoring {
	struct xarray *elements;
	unsigned long long total_counts;
	const struct ds_monitoring_operations *dm_ops;
};

struct ds_monitoring_operations {
	unsigned long (*get_index)(void *elem);
	const char *(*get_name)(void *elem);
	void (*print_elem)(
			unsigned long index,
			const char *name,
			unsigned long long count,
			int percentage
			);
};

struct ds_monitoring_elem {
	unsigned long key;
	char *name;
	unsigned long long count;
};

#define DEFINE_DS_MONITORING_OPS(name, get_idx_fn, get_name_fn, print_fn) \
	static const struct ds_monitoring_operations name##_dm_ops = { \
		.get_index = get_idx_fn, \
		.get_name = get_name_fn, \
		.print_elem = print_fn, \
	}

#define DS_MONITORING_INIT(xarray, _dm_ops) \
{ \
	.elements = &xarray, \
	.total_counts = 0, \
	.dm_ops = &_dm_ops, \
}

#define DEFINE_DS_MONITORING(name, get_idx_fn, get_name_fn, print_fn) \
	DEFINE_XARRAY(name##xarray); \
	DEFINE_DS_MONITORING_OPS(name, get_idx_fn, get_name_fn, print_fn); \
	struct ds_monitoring name = DS_MONITORING_INIT(name##xarray, name##_dm_ops);


#define DECLARE_DS_MONITORING(name) \
	extern struct ds_monitoring name;

static void insert_ds_monitoring(struct ds_monitoring *dm, unsigned long index, void *elem)
{
	char *name;
	struct ds_monitoring_elem *new = kmalloc(sizeof(struct ds_monitoring_elem),
			GFP_KERNEL);
	new->key = index;
	new->count = 1;
	if (dm->dm_ops->get_name) {
		name = dm->dm_ops->get_name(elem);
		new->name = kmalloc(strlen(name)+1, GFP_KERNEL);
		strcpy(new->name, name);
	} else {
		new->name = NULL;
	}
	xa_store(dm->elements, new->key, (void*) new, GFP_KERNEL);
}

static void find_ds_monitoring(struct ds_monitoring *dm, void *elem)
{
	struct ds_monitoring_elem *cur;
	unsigned long xa_index;
	if (dm->dm_ops->get_index) {
		xa_index = dm->dm_ops->get_index(elem);
		// search ds_monitoring_elem at index of xa_index from xarray root
		cur = (struct ds_monitoring_elem *) xa_load(dm->elements, xa_index);
		if (cur) {
			__sync_fetch_and_add(&cur->count, 1);
		 } else {
		 insert_ds_monitoring(dm, xa_index, elem);
		 }
		 __sync_fetch_and_add(&dm->total_counts, 1);
		 }
		 }


static void print_ds_monitoring(struct ds_monitoring* dm)
{
	unsigned long cur_idx;
	void *cur;
	char *cur_name;
	unsigned long long cur_count;
	int percentage;
	if (!dm->total_counts)
		return;
	
	xa_for_each(dm->elements, cur_idx, cur) {
		cur_name = ((struct ds_monitoring_elem *)cur)->name;
		cur_count = ((struct ds_monitoring_elem *)cur)->count;
		percentage = cur_count * 100 / dm->total_counts;
		dm->dm_ops->print_elem(cur_idx, cur_name, cur_count, percentage);
	}
}

static void delete_ds_monitoring(struct ds_monitoring *dm)
{
	unsigned long cur_idx;
	void *cur;
	char *cur_name;
	xa_for_each(dm->elements, cur_idx, cur) {
		cur_name = ((struct ds_monitoring_elem *)cur)->name;
		kfree(cur_name);
		kfree(cur);
	}
	xa_destroy(dm->elements);
}


