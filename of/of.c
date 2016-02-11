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

// leds {
//     compatible = "gpio-leds";
//     pinctrl-names = "default";
//     pinctrl-0 = <&leds_npi>, <&leds_r_npi>;

//     status {
//         label = "status_led";
//         gpios = <&pio 0 10 GPIO_ACTIVE_HIGH>;
//         linux,default-trigger = "heartbeat";
//     };

//     pwr {
//         label = "LED2";
//         gpios = <&r_pio 0 10 GPIO_ACTIVE_HIGH>;
//         default-state = "on";
//     };
// };

static int __init of_init(void)
{
    printk("In of_init\r\n");

    int ret = 0;
    
    /* 获取节点 */
    struct device_node* leds_nd = of_find_node_by_path("/leds");    // leds节点
    if (leds_nd == NULL)
    {
        printk("ERROR: of_find_node_by_path\r\n");
        return -1;
    }
    printk("SUCCESS: of_find_node_by_path\r\n");

    /* 获取属性值 通过 property 结构体 */
    struct property* compatible_pro = of_find_property(leds_nd, "compatible", NULL);
    if (compatible_pro == NULL)
    {
        printk("ERROR: of_find_property\r\n");
        return -1;
    }
    printk("SUCCESS: of_find_property\r\n");
    printk("The leds.compatible is %s\r\n", (char*)compatible_pro->value);

    /* 获取属性值 不通过 property 结构体 */
    const char* pinctrl_names_value;
    ret = of_property_read_string(leds_nd, "pinctrl-names", &pinctrl_names_value);
    if (ret < 0)
    {
        printk("ERROR: of_property_read_string\r\n");
        return -1;
    }
    printk("SUCCESS: of_find_property\r\n");
    printk("The leds.pinctrl-names is %s", pinctrl_names_value);

    return 0;
}

static void __exit of_exit(void)
{
    printk("In of_exit\r\n");
}

module_init(of_init);
module_exit(of_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yue");