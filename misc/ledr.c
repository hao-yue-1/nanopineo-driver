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

// led_blue {
// 		compatible = "led_blue";
// 		pinctrl-names = "default";
// 		pinctrl-0 = <&led_blue_pin>;

// 		status = "okay";
// 		gpios = <&pio 6 6 GPIO_ACTIVE_HIGH>;
// 	};

// led_blue_pin: led_blue_pin {
// 	pins = "PG6";
// 	function = "gpio_out";
// };

#define LED_ON 1
#define LED_OFF 0

/* led设备结构体 */
struct led_dev
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

struct led_dev ledb;

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int ledb_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &ledb; /* 设置私有数据 */

	printk("ledb open!\r\n");
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
static ssize_t ledb_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	struct led_dev* dev = (struct led_dev*)filp->private_data;
	
	printk("ledb read!\r\n");
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
static ssize_t ledb_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	struct led_dev* dev = (struct led_dev*)filp->private_data;

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
		printk("ledb is on\r\n");
	}
	else if (led_state == LED_OFF)
	{
		gpio_set_value(dev->led_gpio, 1);	// 关
		printk("ledb is off\r\n");
	}
	else
	{
		printk("ledb error is %d\r\n", led_state);
	}
	
	printk("ledb write!\r\n");
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int ledb_release(struct inode *inode, struct file *filp)
{
	printk("ledb release!\r\n");
	return 0;
}

/* 设备操作函数 */
static struct file_operations ledb_fops = 
{
	.owner = THIS_MODULE,	
	.open = ledb_open,
	.read = ledb_read,
	.write = ledb_write,
	.release = ledb_release,
};

/* MISC 设备结构体 */
static struct miscdevice ledb_miscdev = 
{
	.minor = 144,
	.name  = "led_blue"
	.fops  = &ledb_fops,
};

/*
 * @description	: probe 函数 驱动与设备匹配成功后首先执行
 * @param - dev	: platform 设备
 * @return 		: 0成功
 */
static int ledb_probe(struct platform_device* dev)
{
	int ret = 0;

	/* 获取设备节点 led_blue */
	ledb.nd = of_find_node_by_path("/led_blue");
	if (ledb.nd == NULL)
	{
		printk("ERROR: led_blue can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: led_blue has been found\r\n");
	}

	/* 获取 LED 所使用的 LED编号 */
	ledb.led_gpio = of_get_named_gpio(ledb.nd, "gpios", 0);
	if (ledb.led_gpio < 0)
	{
		printk("ERROR: led_blue.led_gpio can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: led_blue.led_gpio has been found is %d\r\n", ledb.led_gpio);
	}

	/* 设置 GPIO 为输出 并且输出高电平 默认关闭 LED 灯 */
	ret = gpio_direction_output(ledb.led_gpio, 1);
	if (ret < 0)
	{
		printk("ERROR: gpio_direction_output\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: gpio_direction_output\r\n");
	}

	ret = misc_register(&ledb_miscdev);
	if (ret < 0)
	{
		printk("ERROR: misc_register\r\n");
		return -EFAULT;
	}

	printk("ledb_probe!\r\n");
	return 0;
}

/*
 * @description	: remove 函数 移除驱动的时候会执行此函数
 * @param - dev	: platform 设备
 * @return 		: 0成功
 */
static int ledb_remove(struct platform_device* dev)
{
	misc_deregister(&ledb_miscdev);

	printk("ledb_remove!\r\n");
	return 0;
}

/* 匹配列表 用于驱动和设备树中的设备进行匹配（通过 compatible 属性） */
static const struct of_device_id ledb_of_match[] = 
{
	{.compatible = "led_blue"},
};

/* platform 驱动结构体 */
static struct platform_driver ledb_driver = 
{
	.driver = 
	{
		.name = "h3-led_blue",
		.of_match_table = ledb_of_match,
	},
	.probe = ledb_probe,
	.remove = ledb_remove,
};

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init ledb_init(void)
{
	int ret = 0;
	
	/* 向 Linux 内核注册一个 platform 驱动 */
	ret = platform_driver_register(&ledb_driver);
	if (ret != 0)
	{
		printk("ERROR: platform_driver_register!\r\n");
		return ret;
	}

	printk("ledb init!\r\n");
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit ledb_exit(void)
{
	/* 向 Linux 内核卸载 platform 驱动 */
	platform_driver_unregister(&ledb_driver);

	printk("ledb exit!\r\n");
}

module_init(ledb_init);
module_exit(ledb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");
