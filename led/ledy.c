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

// / {
// 	model = "FriendlyElec NanoPi-NEO";
// 	compatible = "friendlyelec,nanopi-neo", "allwinner,sun8i-h3";

// 	led_yellow {
// 		compatible = "led_yellow";
// 		pinctrl-names = "default";
// 		pinctrl-0 = <&led_yellow_pin>;

// 		status = "okay";
// 		gpios = <&pio 0 6 GPIO_ACTIVE_HIGH>;
// 	};
// };

// &pio {
// 	led_yellow_pin: led_yellow_pin {
// 		pins = "PA6";
// 		function = "gpio_out";
// 	};
// };

#define LED_ON 1
#define LED_OFF 0

/* ledy设备结构体 */
struct ledy_dev
{
	dev_t devid;			// 设备号
	int major;				// 主设备号
	int minor;				// 次设备号
	struct cdev cdev;		// cdev
	struct class *class;	// class
	struct device *device;	// class中的设备
	
	struct device_node *nd;	// 设备节点
	int led_gpio;			// led所使用的GPIO编号
};

struct ledy_dev ledy;

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int ledy_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &ledy; /* 设置私有数据 */

	printk("ledy open!\r\n");
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
static ssize_t ledy_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	struct ledy_dev* dev = (struct ledy_dev*)filp->private_data;
	
	printk("ledy read!\r\n");
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
static ssize_t ledy_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	struct ledy_dev* dev = (struct ledy_dev*)filp->private_data;

	int ret = 0;
	unsigned char databuf[1];
	unsigned char led_state;
	
	ret = copy_from_user(databuf, buf, cnt);
	if (ret < 0)
	{
		printk("ERROR: copy_from_user\r\n");
		return -EFAULT;;
	}

	led_state = databuf[0];					// 获取写入的 LED 状态

	if (led_state == LED_ON)
	{
		gpio_set_value(dev->led_gpio, 0);	// 开
		printk("ledy is on\r\n");
	}
	else if (led_state == LED_OFF)
	{
		gpio_set_value(dev->led_gpio, 1);	// 关
		printk("ledy is off\r\n");
	}
	else
	{
		printk("ledy error is %d\r\n", led_state);
	}
	
	printk("ledy write!\r\n");
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int ledy_release(struct inode *inode, struct file *filp)
{
	printk("ledy release!\r\n");
	return 0;
}

/* 设备操作函数 */
static struct file_operations ledy_fops = 
{
	.owner = THIS_MODULE,	
	.open = ledy_open,
	.read = ledy_read,
	.write = ledy_write,
	.release = ledy_release,
};

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init ledy_init(void)
{
	int ret = 0;

	/* 获取设备节点 led_yellow */
	ledy.nd = of_find_node_by_path("/led_yellow");
	if (ledy.nd == NULL)
	{
		printk("ERROR: led_yellow can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: led_yellow has been found\r\n");
	}

	/* 获取 LED 所使用的 LED编号 */
	ledy.led_gpio = of_get_named_gpio(ledy.nd, "gpios", 0);
	if (ledy.led_gpio < 0)
	{
		printk("ERROR: led_yellow.led_gpio can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: led_yellow.led_gpio has been found is %d\r\n", ledy.led_gpio);
	}

	/* 设置 GPIO 为输出 并且输出高电平 默认关闭 LED 灯 */
	ret = gpio_direction_output(ledy.led_gpio, 1);
	if (ret < 0)
	{
		printk("ERROR: gpio_direction_output\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: gpio_direction_output\r\n");
	}

	/* 申请设备号 */
	alloc_chrdev_region(&ledy.devid, 0, 1, "ledy");	// 向内核申请设备号
	ledy.major = MAJOR(ledy.devid);	// 获取主设备号
	ledy.minor = MINOR(ledy.devid);	// 获取次设备号

	/* 初始化并向内核添加设备 cdev */
	ledy.cdev.owner = THIS_MODULE;
	cdev_init(&ledy.cdev, &ledy_fops);
	cdev_add(&ledy.cdev, ledy.devid, 1);

	/* 创建 class 并添加设备 */
	ledy.class = class_create(THIS_MODULE, "ledy");
	if (IS_ERR(ledy.class)) 
	{
		return PTR_ERR(ledy.class);
	}
	ledy.device = device_create(ledy.class, NULL, ledy.devid, NULL, "ledy");
	if (IS_ERR(ledy.device)) 
	{
		return PTR_ERR(ledy.device);
	}

	printk("ledy init!\r\n");
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit ledy_exit(void)
{
	cdev_del(&ledy.cdev);	// 删除字符设备 cdev
	unregister_chrdev_region(ledy.devid, 1);	// 注销设备号

	device_destroy(ledy.class, ledy.devid);
	class_destroy(ledy.class);

	printk("ledy exit!\r\n");
}

module_init(ledy_init);
module_exit(ledy_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");
