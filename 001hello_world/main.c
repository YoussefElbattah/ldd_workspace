#include <linux/module.h>

static int __init my_function_entry(void){
	pr_info("hello it's my simple module\n");
	return 0;
}

static void __exit my_function_exit(void){
	pr_info("Good Bye from my simple module\n");
}

module_init(my_function_entry);
module_exit(my_function_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Youssef ELBATTAH");
MODULE_DESCRIPTION("A simple hello worlk kernel module");
MODULE_INFO(board, "BBB with linux rev_5.168.1");
