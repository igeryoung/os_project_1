#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/uaccess.h>

asmlinkage int sys_print_ans(int pid, char __user *buff){
	struct timespec ts;
	getnstimeofday(&ts);
	char et[200];
	sprintf(et, "%lld.%.9ld", (long long)ts.tv_sec, ts.tv_nsec);
	printk("[Project1] %d %s %s\n" , pid , buff , et);

	return 0;
}
