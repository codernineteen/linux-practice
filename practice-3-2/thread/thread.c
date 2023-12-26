#include <stdio.h>
#include <pthread.h>

void *hello_entry(void* name) {
	char *thread_name = (char*)name;
	printf("%s", thread_name);
}

int main() {
	pthread_t pthread;
	pthread_create(&pthread, NULL, hello_entry, (void*)"child_thread");

	hello_entry((void*)"parent_thread");
	pthread_join(pthread, (void**)0);

	return 0;
}
