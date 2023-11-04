#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <linux/input.h>

// key_g8 {
// 	compatible = "key_g8";
// 	pinctrl-names = "default";
// 	pinctrl-0 = <&key_g8_pin>;

// 	status = "okay";
// 	gpios = <&pio 6 8 GPIO_ACTIVE_HIGH>;
// };

// key_g8_pin: key_g8_pin {
// 	pins = "PG8";
// 	function = "gpio_in";
// };

/* key设备结构体 */
struct key_dev
{
	dev_t devid;			// 设备号
	int major;				// 主设备号
	int minor;				// 次设备号
	struct cdev cdev;		// cdev
	struct class *class;	// class
	struct device *device;	// class中的设备
	
	struct device_node *nd;	// 设备节点
	int key_gpio;			// key所使用的GPIO编号

	struct input_dev* input_dev;	// input设备
};

struct key_dev key8;

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int key8_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &key8; /* 设置私有数据 */

	printk("key8 open!\r\n");
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
static ssize_t key8_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	struct key_dev* dev = (struct key_dev*)filp->private_data;
	
	printk("key8 read!\r\n");
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
static ssize_t key8_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	struct key_dev* dev = (struct key_dev*)filp->private_data;
	
	printk("key8 write!\r\n");
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int key8_release(struct inode *inode, struct file *filp)
{
	printk("key8 release!\r\n");
	return 0;
}

/* 设备操作函数 */
static struct file_operations key8_fops = 
{
	.owner = THIS_MODULE,	
	.open = key8_open,
	.read = key8_read,
	.write = key8_write,
	.release = key8_release,
};

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init key8_init(void)
{
	int ret = 0;

	/* 获取设备节点 key_g8 */
	key8.nd = of_find_node_by_path("/key_g8");
	if (key8.nd == NULL)
	{
		printk("ERROR: key_g8 can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: key_g8 has been found\r\n");
	}

	/* 获取 key 所使用的 key编号 */
	key8.key_gpio = of_get_named_gpio(key8.nd, "gpios", 0);
	if (key8.key_gpio < 0)
	{
		printk("ERROR: key_g8.key_gpio can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: key_g8.key_gpio has been found is %d\r\n", key8.key_gpio);
	}

	/* 设置 GPIO 为输入 */
	ret = gpio_direction_input(key8.key_gpio);
	if (ret < 0)
	{
		printk("ERROR: gpio_direction_input\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: gpio_direction_input\r\n");
	}

	/* 申请 input_dev */
	key8.input_dev = input_allocate_device();
	key8.input_dev->name = "key8";
	key8.input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	input_set_capability(key8.input_dev, EV_KEY, KEY_0);

	/* 注册 input 设备 */
	ret = input_register_device(key8.input_dev);
	if (ret != 0)
	{
		printk("ERROR: input_register_device\r\n");
		return ret;
	}

	printk("key8 init!\r\n");
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit key8_exit(void)
{
	input_unregister_device(key8.input_dev);
	input_free_device(key8.input_dev);

	printk("key8 exit!\r\n");
}

module_init(key8_init);
module_exit(key8_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");