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

#include <linux/timer.h>
#include <linux/semaphore.h>

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

#define CLOSE_CMD		(_IO(0xEF, 0x1))
#define OPEN_CMD		(_IO(0xEF, 0x2))
#define SETPERIOD_CMD	(_IO(0xEF, 0x3))

/* 设备结构体 */
struct timer_dev
{
	dev_t devid;			// 设备号
	int major;				// 主设备号
	int minor;				// 次设备号
	struct cdev cdev;		// cdev
	struct class *class;	// class
	struct device *device;	// class中的设备
	
	struct device_node *nd;	// 设备节点

	int led_gpio;			// LED GPIO 编号
	int time_period;		// 周期

	struct timer_list timer;		// 定时器
	spinlock_t lock;				// 自旋锁
};

struct timer_dev timery;

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int timery_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &timery; /* 设置私有数据 */

	timery.time_period = 100;	// 默认周期为 1s

	printk("timery open!\r\n");
	return 0;
}

/*
 * @description		: ioctl
 * @param – filp    : 要打开的设备文件(文件描述符)
 * @param - cmd     : 应用程序发送过来的命令
 * @param - arg     : 参数
 * @return 			: 0 成功;其他 失败
 */
static long timery_unlocked_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
	struct timer_dev* dev = (struct timer_dev*)filp->private_data;

	int time_period;
	unsigned long flags;

	switch (cmd)
	{
		case CLOSE_CMD:			// 关闭定时器
		{
			del_timer_sync(&dev->timer);
			break;
		}
		case OPEN_CMD:			// 开启定时器
		{
			spin_lock_irqsave(&dev->lock, flags);
			time_period = dev->time_period;
			spin_unlock_irqrestore(&dev->lock, flags);
			mod_timer(&dev->timer, jiffies+msecs_to_jiffies(time_period));
			break;
		}
		case SETPERIOD_CMD:		// 设置定时器周期
		{
			spin_lock_irqsave(&dev->lock, flags);
			dev->time_period = arg;
			spin_unlock_irqrestore(&dev->lock, flags);
			mod_timer(&dev->timer, jiffies+msecs_to_jiffies(arg));
			break;
		}
		default: break;
	}
}

/* 设备操作函数 */
static struct file_operations timery_fops =
{
	.owner = THIS_MODULE,
	.open  = timery_open,
	.unlocked_ioctl = timery_unlocked_ioctl,
};

/*
 * @description		: 定时器服务函数 用于按键消抖
 * @param - arg 	: 设备结构体
 * @return 			: 0 成功;其他 失败
 */
void timer_function(unsigned long arg)
{
	printk("DEBUG: this is timer_handler\r\n");
	struct timer_dev* dev = (struct timer_dev*)arg;
	static int sta = 1;
	int time_period;
	unsigned long flags;

	sta = !sta;
	gpio_set_value(dev->led_gpio, sta);

	/* 重启定时器 */
	spin_lock_irqsave(&dev->lock, flags);
	time_period = dev->time_period;
	spin_unlock_irqrestore(&dev->lock, flags);
	mod_timer(&dev->timer, jiffies+msecs_to_jiffies(dev->time_period));
}

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init timery_init(void)
{
	printk("DEBUG: in init!\r\n");

	int ret = 0;

	/* 获取设备节点 led_yellow */
	timery.nd = of_find_node_by_path("/led_yellow");
	if (timery.nd == NULL)
	{
		printk("ERROR: led_yellow can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: led_yellow has been found\r\n");
	}

	/* 获取 led_yellow 所使用的 gpio 编号 */
	timery.led_gpio = of_get_named_gpio(timery.nd, "gpios", 0);
	if (timery.led_gpio < 0)
	{
		printk("ERROR: led_yellow.timer_gpio can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: led_yellow.timer_gpio has been found is %d\r\n", timery.led_gpio);
	}

	/* 设置 GPIO 为输出 */
	gpio_request(timery.led_gpio, "led");
	ret = gpio_direction_output(timery.led_gpio, 1);
	if (ret < 0)
	{
		printk("ERROR: gpio_direction_output\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: gpio_direction_output\r\n");
	}
	
	/* 初始化自旋锁 */
	spin_lock_init(&timery.lock);

	/* 申请设备号 */
	alloc_chrdev_region(&timery.devid, 0, 1, "timery");	// 向内核申请设备号
	timery.major = MAJOR(timery.devid);	// 获取主设备号
	timery.minor = MINOR(timery.devid);	// 获取次设备号

	/* 初始化并向内核添加设备 cdev */
	timery.cdev.owner = THIS_MODULE;
	cdev_init(&timery.cdev, &timery_fops);
	cdev_add(&timery.cdev, timery.devid, 1);

	/* 创建 class 并添加设备 */
	timery.class = class_create(THIS_MODULE, "timery");
	if (IS_ERR(timery.class)) 
	{
		return PTR_ERR(timery.class);
	}
	timery.device = device_create(timery.class, NULL, timery.devid, NULL, "timery");
	if (IS_ERR(timery.device)) 
	{
		return PTR_ERR(timery.device);
	}

	/* 创建定时器 */
	init_timer(&timery.timer);
	timery.timer.function = timer_function;
	timery.timer.data     = (unsigned long)&timery;

	printk("timery init!\r\n");
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit timery_exit(void)
{
	del_timer_sync(&timery.timer);	// 删除定时器

	cdev_del(&timery.cdev);	// 删除字符设备 cdev
	unregister_chrdev_region(timery.devid, 1);	// 注销设备号

	device_destroy(timery.class, timery.devid);
	class_destroy(timery.class);

	printk("timery exit!\r\n");
}

module_init(timery_init);
module_exit(timery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");
