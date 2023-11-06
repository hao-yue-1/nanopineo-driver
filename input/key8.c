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

#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/timer.h>

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

/* 中断 IO 描述结构体 */
struct irq_key
{
	int gpio;				// GPIO 编号
	int irq_num;			// 中断号
	unsigned char value;	// 按键值
	char name[10];			// 名称
	irqreturn_t (*handler)(int, void*);	// 中断服务函数
};

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

	struct input_dev* input_dev;	// input设备
	atomic_t key_value;				// 有效的按键值
	atomic_t release_key;			// 是否完成一次按键
	unsigned char cur_key_num;		// 当前按键号

	struct irq_key irq_key;			// 中断 IO
	struct timer_list timer;		// 定时器
};

struct key_dev key8;

/*
 * @description		: 中断服务函数 开启定时器 延时 10ms 用于按键消抖
 * @param - irq 	: 中断号
 * @param - dev_id	: 设备结构体
 * @return 			: 0 成功;其他 失败
 */
static irqreturn_t key8_handler(int irq, void* dev_id)
{
	printk("DEBUG: this is irq_handler\r\n");
	struct key_dev* dev = (struct key_dev*)dev_id;

	dev->cur_key_num = 0;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies+msecs_to_jiffies(10));	// 10ms
	
	return IRQ_RETVAL(IRQ_HANDLED);
}

/*
 * @description		: 定时器服务函数 用于按键消抖
 * @param - arg 	: 设备结构体
 * @return 			: 0 成功;其他 失败
 */
void timer_function(unsigned long arg)
{
	printk("DEBUG: this is timer_handler\r\n");
	struct key_dev* dev = (struct key_dev*)arg;
	struct irq_key* key = &dev->irq_key;

	/* 上报按键值 */
	unsigned char value = gpio_get_value(key->gpio);	// 读取 IO 值
	if (value == 0)	// 按键按下
	{
		input_report_key(dev->input_dev, key->value, 1);
		input_sync(dev->input_dev);
		printk("DEBUG: release_key is 1\r\n");
	}
	else			// 按键松开
	{
		input_report_key(dev->input_dev, key->value, 0);
		input_sync(dev->input_dev);
		printk("DEBUG: release_key is 0\r\n");
	}
}

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init key8_init(void)
{
	printk("DEBUG: in init!\r\n");

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
	key8.irq_key.gpio = of_get_named_gpio(key8.nd, "gpios", 0);
	if (key8.irq_key.gpio < 0)
	{
		printk("ERROR: key_g8.key_gpio can't found\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: key_g8.key_gpio has been found is %d\r\n", key8.irq_key.gpio);
	}

	/* 设置 GPIO 为输入 */
	ret = gpio_direction_input(key8.irq_key.gpio);
	if (ret < 0)
	{
		printk("ERROR: gpio_direction_input\r\n");
		return -EINVAL;
	}
	else
	{
		printk("SUCCESS: gpio_direction_input\r\n");
	}

	/* 获取中断号 */
	key8.irq_key.irq_num = irq_of_parse_and_map(key8.nd, 0);	// 从设备树节点 interrupts 属性获取中断号
	// key8.irq_key.irq_num = gpio_to_irq(key8.irq_key.gpio);		// 通过 GPIO 编号获取中断号
	
	printk("DEBUG: irq_of_parse_and_map irq_num is %d\r\n", key8.irq_key.irq_num);
	
	/* 申请中断 */
	sprintf(key8.irq_key.name, "key_g8");
	key8.irq_key.handler = key8_handler;
	key8.irq_key.value   = 1;

	ret = request_irq(key8.irq_key.irq_num, key8.irq_key.handler, 
					IRQ_TYPE_EDGE_BOTH, key8.irq_key.name, &key8);
	if (ret < 0)
	{
		printk("ERROR: request_irq is %d\r\n", ret);
		return -EFAULT;
	}
	else
	{
		printk("SUCCESS: request_irq\r\n");
	}
	
	/* 创建定时器 */
	init_timer(&key8.timer);
	key8.timer.function = timer_function;

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
	del_timer_sync(&key8.timer);	// 删除定时器
	free_irq(key8.irq_key.irq_num, &key8);	// 释放中断
	gpio_free(key8.irq_key.gpio);	// 释放 IO

	input_unregister_device(key8.input_dev);
	input_free_device(key8.input_dev);

	printk("key8 exit!\r\n");
}

module_init(key8_init);
module_exit(key8_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");
