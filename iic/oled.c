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

#include <linux/semaphore.h>

#include <linux/time.h>

#include <linux/i2c.h>

const unsigned char OLED_init_cmd[25]=
{
    0xAE,//关闭显示
    0xD5,//设置时钟分频因子,震荡频率
    0x80,  //[3:0],分频因子;[7:4],震荡频率
    0xA8,//设置驱动路数
    0X3F,//默认0X3F(1/64)
    0xD3,//设置显示偏移
    0X00,//默认为0
    0x40,//设置显示开始行 [5:0],行数.                              
    0x8D,//电荷泵设置
    0x14,//bit2，开启/关闭
    0x20,//设置内存地址模式
    0x02,//[1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10;
    0xA1,//段重定义设置,bit0:0,0->0;1,0->127;
    0xC8,//设置COM扫描方向;bit3:0,普通模式;1,重定义模式 COM[N-1]->COM0;N:驱动路数
    0xDA,//设置COM硬件引脚配置
    0x12,//[5:4]配置            
    0x81,//对比度设置
    0xEF,//1~255;默认0X7F (亮度设置,越大越亮)
    0xD9,//设置预充电周期
    0xf1,//[3:0],PHASE 1;[7:4],PHASE 2;
    0xDB,//设置VCOMH 电压倍率
    0x30,//[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;
    0xA4,//全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
    0xA6,//设置显示方式;bit0:1,反相显示;0,正常显示        
    0xAF,//开启显示
};

/* oled 设备结构体 */
struct oled_dev
{
	dev_t devid;			// 设备号
	int major;				// 主设备号
	int minor;				// 次设备号

	struct cdev cdev;		// cdev
	struct class *class;	// class
	struct device *device;	// class中的设备
	
	struct device_node *nd;	// 设备节点
    void* private_data;     // 私有数据
};

struct oled_dev oled;

/*
 * @description		: 写命令
 * @param - cmd 	: 命令
 * @return 			: 无
 */
static void OLED_SendCmd(unsigned char cmd)
{

}

/*
 * @description		: 写数据
 * @param - data 	: 数据
 * @return 			: 无
 */
static void OLED_SendData(unsigned char data)
{

}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int oled_open(struct inode *inode, struct file *filp)
{
    printk("oled_open!\r\n");
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
static ssize_t oled_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	printk("oled_read!\r\n");
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
static ssize_t oled_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	printk("oled_write!\r\n");
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int oled_release(struct inode *inode, struct file *filp)
{
	printk("oled_release!\r\n");
	return 0;
}

/* 设备操作函数 */
static struct file_operations oled_fops = 
{
	.owner = THIS_MODULE,	
	.open = oled_open,
	.read = oled_read,
	.write = oled_write,
	.release = oled_release,
};

/*
 * @description	    : probe 函数 驱动与设备匹配成功后首先执行
 * @param - client	: IIC 设备
 * @param - id	    : IIC 设备 ID
 * @return 		    : 0成功
 */
static int oled_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    printk("oled_probe!\r\n");
    return 0;
}

/*
 * @description	    : remove 函数 移除驱动的时候会执行此函数
 * @param - client	: IIC 设备
 * @return 		    : 0成功
 */
static int oled_remove(struct i2c_client *client)
{
    printk("oled_remove!\r\n");
    return 0;
}

/* 匹配列表 用于驱动和设备树中的设备进行匹配（通过 compatible 属性） */
static const struct of_device_id oled_of_match[] = 
{
	{.compatible = "oled"},
};

/* i2c 驱动结构体 */
static struct i2c_driver oled_driver =
{
    .probe = oled_probe,
    .remove = oled_remove,
    .driver =
    {
        .owner = THIS_MODULE,
        .name  = "oled",
        .of_match_table = oled_of_match,
    },
};

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init oled_init(void)
{
	int ret = 0;
	
	/* 向 Linux 内核注册一个 IIC 驱动 */
	ret = i2c_add_driver(&oled_driver);
	if (ret != 0)
	{
		printk("ERROR: i2c_add_driver!\r\n");
		return ret;
	}

	printk("oled_init!\r\n");
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit oled_exit(void)
{
	/* 向 Linux 内核卸载 IIC 驱动 */
	i2c_del_driver(&oled_driver);

	printk("oled_exit!\r\n");
}

module_init(oled_init);
module_exit(oled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");