// SPDX-License-Identifier: GPL-2.0
/*
 * 最小内核模块示例 —— 用于验证内核构建产物可以编译树外模块。
 */
#include <linux/module.h>
#include <linux/init.h>

static int __init hello_init(void)
{
	pr_info("hello: module loaded (built against running kernel)\n");
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WorkBuddy");
MODULE_DESCRIPTION("Minimal out-of-tree kernel module example");
