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
	dev_t devid;			// è®¾å¤å·
	int major;				// ä¸»è®¾å¤å·
	int minor;				// æ¬¡è®¾å¤å·
	struct cdev cdev;		// cdev
	struct class *class;	// class
	struct device *device;	// è®¾å¤

	char buf[256];
};

struct hello_dev hello;

/*
 * @description		: æå¼è®¾å¤
 * @param - inode 	: ä¼ éç»é©±å¨çinode
 * @param - filp 	: è®¾å¤æä»¶ï¼fileç»æä½æä¸ªå«åprivate_dataçæååé
 * 					  ä¸è¬å¨opençæ¶åå°private_dataæåè®¾å¤ç»æä½ã
 * @return 			: 0 æå;å¶ä» å¤±è´¥
 */
static int hello_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &hello; /* è®¾ç½®ç§ææ°æ® */

	printk("hello open!\r\n");
	return 0;
}

/*
 * @description		: ä»è®¾å¤è¯»åæ°æ® 
 * @param - filp 	: è¦æå¼çè®¾å¤æä»¶(æä»¶æè¿°ç¬¦)
 * @param - buf 	: è¿åç»ç¨æ·ç©ºé´çæ°æ®ç¼å²åº
 * @param - cnt 	: è¦è¯»åçæ°æ®é¿åº¦
 * @param - offt 	: ç¸å¯¹äºæä»¶é¦å°åçåç§»
 * @return 			: è¯»åçå­èæ°ï¼å¦æä¸ºè´å¼ï¼è¡¨ç¤ºè¯»åå¤±è´¥
 */
static ssize_t hello_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	struct hello_dev* dev = (struct hello_dev*)filp->private_data;
	
	/* åç¨æ·ç©ºé´åéæ°æ® */
	int ret = copy_to_user(buf, dev->buf, cnt);
	if(ret != 0)
	{
		printk("kernel senddata failed!\r\n");
	}
	
	printk("hello read!\r\n");
	return 0;
}

/*
 * @description		: åè®¾å¤åæ°æ® 
 * @param - filp 	: è®¾å¤æä»¶ï¼è¡¨ç¤ºæå¼çæä»¶æè¿°ç¬¦
 * @param - buf 	: è¦åç»è®¾å¤åå¥çæ°æ®
 * @param - cnt 	: è¦åå¥çæ°æ®é¿åº¦
 * @param - offt 	: ç¸å¯¹äºæä»¶é¦å°åçåç§»
 * @return 			: åå¥çå­èæ°ï¼å¦æä¸ºè´å¼ï¼è¡¨ç¤ºåå¥å¤±è´¥
 */
static ssize_t hello_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	struct hello_dev* dev = (struct hello_dev*)filp->private_data;

	/* æ¥æ¶ç¨æ·ç©ºé´ä¼ éç»åæ ¸çæ°æ®å¹¶ä¸æå°åºæ¥ */
	int ret = copy_from_user(dev->buf, buf, cnt);
	if(ret != 0)
	{
		printk("kernel recevdata failed!\r\n");
	}
	
	printk("hello write!\r\n");
	return 0;
}

/*
 * @description		: å³é­/éæ¾è®¾å¤
 * @param - filp 	: è¦å³é­çè®¾å¤æä»¶(æä»¶æè¿°ç¬¦)
 * @return 			: 0 æå;å¶ä» å¤±è´¥
 */
static int hello_release(struct inode *inode, struct file *filp)
{
	printk("hello release!\r\n");
	return 0;
}

/*
 * è®¾å¤æä½å½æ°ç»æä½
 */
static struct file_operations hello_fops = 
{
	.owner = THIS_MODULE,	
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
	.release = hello_release,
};

/*
 * @description	: é©±å¨å¥å£å½æ° 
 * @param 		: æ 
 * @return 		: 0 æå;å¶ä» å¤±è´¥
 */
static int __init hello_init(void)
{
	/* å¨æç³è¯·è®¾å¤å· */
	alloc_chrdev_region(&hello.devid, 0, 1, "hello");	// ç³è¯·è®¾å¤å·
	hello.major = MAJOR(hello.devid);	// è·ååéå·çä¸»è®¾å¤å·
	hello.minor = MINOR(hello.devid);	// è·ååéå·çæ¬¡è®¾å¤å·

	/* åå§åå¹¶åLinuxåæ ¸æ·»å å­ç¬¦è®¾å¤cdev */
	hello.cdev.owner = THIS_MODULE;
	cdev_init(&hello.cdev, &hello_fops);
	cdev_add(&hello.cdev, hello.devid, 1);

	/* åå»ºclasså¹¶æ·»å è®¾å¤ */
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
 * @description	: é©±å¨åºå£å½æ°
 * @param 		: æ 
 * @return 		: æ 
 */
static void __exit hello_exit(void)
{
	cdev_del(&hello.cdev);	// å é¤cdev
	unregister_chrdev_region(hello.devid, 1);	// æ³¨éè®¾å¤å·

	device_destroy(hello.class, hello.devid);
	class_destroy(hello.class);

	printk("hello exit!\r\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");
