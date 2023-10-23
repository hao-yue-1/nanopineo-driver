#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/cdev.h>
#include <linux/device.h>

/* hello设备结构体 */
struct hello_dev
{
	dev_t devid;			/* 设备号 	 */
	int major;				/* 主设备号	  */
	int minor;				/* 次设备号   */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */

	char buf[256];
};

struct hello_dev hello;

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int hello_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &hello; /* 设置私有数据 */

	printk("hello open!\r\n");
	return 0;
}

/*
 * @description		: 从设备读取数据 
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t hello_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	struct hello_dev* dev = (struct hello_dev*)filp->private_data;
	
	/* 向用户空间发送数据 */
	int ret = copy_to_user(buf, dev->buf, cnt);
	if(ret != 0)
	{
		printk("kernel senddata failed!\r\n");
	}
	
	printk("hello read!\r\n");
	return 0;
}

/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t hello_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	struct hello_dev* dev = (struct hello_dev*)filp->private_data;

	/* 接收用户空间传递给内核的数据并且打印出来 */
	int ret = copy_from_user(dev->buf, buf, cnt);
	if(ret != 0)
	{
		printk("kernel recevdata failed!\r\n");
	}
	
	printk("hello write!\r\n");
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int hello_release(struct inode *inode, struct file *filp)
{
	printk("hello release!\r\n");
	return 0;
}

/*
 * 设备操作函数结构体
 */
static struct file_operations hello_fops = {
	.owner = THIS_MODULE,	
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
	.release = hello_release,
};

/*
 * @description	: 驱动入口函数 
 * @param 		: 无
 * @return 		: 0 成功;其他 失败
 */
static int __init hello_init(void)
{
	/* 动态申请设备号 */
	alloc_chrdev_region(&hello.devid, 0, 1, "hello");	/* 申请设备号 */
	hello.major = MAJOR(hello.devid);	/* 获取分配号的主设备号 */
	hello.minor = MINOR(hello.devid);	/* 获取分配号的次设备号 */

	/* 初始化并向Linux内核添加字符设备cdev */
	hello.cdev.owner = THIS_MODULE;
	cdev_init(&hello.cdev, &hello_fops);
	cdev_add(&hello.cdev, hello.devid, 1);

	/* 创建class并添加设备 */
	hello.class = class_create(THIS_MODULE, "hello");
	if (IS_ERR(hello.class)) 
	{
		return PTR_ERR(hello.class);
	}
	hello.device = device_create(hello.class, NULL, hello.devid, NULL, "hello");
	if (IS_ERR(hello.device)) 
	{
		return PTR_ERR(hello.device);
	}

	printk("hello init!\r\n");
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit hello_exit(void)
{
	cdev_del(&hello.cdev);/*  删除cdev */
	unregister_chrdev_region(hello.devid, 1); /* 注销设备号 */

	device_destroy(hello.class, hello.devid);
	class_destroy(hello.class);

	printk("hello exit!\r\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");