#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/uaccess.h>

asmlinkage int sys_get_time(char __user *buff){
	struct timespec ts;
	getnstimeofday(&ts);
	char tmp[200];
	sprintf(tmp, "%lld.%.9ld", (long long)ts.tv_sec, ts.tv_nsec);
	copy_to_user( buff , tmp , sizeof(tmp));
	return 0;
}
