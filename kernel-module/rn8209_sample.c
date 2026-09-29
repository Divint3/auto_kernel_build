#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
 

#include "meter_uart.h"
static nor_flash_t *meter_flash = NULL;

static char *uart_dev = "/dev/ttyAMA1";
module_param(uart_dev, charp, 0444);
MODULE_PARM_DESC(uart_dev, "rn8209 计量芯片所接的串口设备 (默认 /dev/ttyAMA1)");
/* NOR Flash 仿真器持久化文件（相对路径基于模块加载时的进程 cwd，QEMU 里即 /） */
static char *flash_data_file = "flash_data.bin";
static char *flash_meta_file = "flash_meta.bin";
module_param(flash_data_file, charp, 0444);
module_param(flash_meta_file, charp, 0444);
MODULE_PARM_DESC(flash_data_file, "NOR Flash 仿真数据文件 (默认 flash_data.bin)");
MODULE_PARM_DESC(flash_meta_file, "NOR Flash 仿真元数据文件 (默认 flash_meta.bin)");



typedef enum {
  RN8209_DEVICE_STATUS_DUMP = 0,
  RN8209_DEVICE_STATUS_VOLTAGE,
  RN8209_DEVICE_STATUS_CURRENT, 
}rn8209_device_status_e; 

static rn8209_device_status_e status = RN8209_DEVICE_STATUS_DUMP;


 
//cat命令时,将会调用该函数
static ssize_t cat_rn8209_device(struct device *dev, struct device_attribute *attr, char *buf)        
{
    return sprintf(buf, "%s\n", mybuf);
}
 
//echo命令时,将会调用该函数
static ssize_t echo_rn8209_device(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)        
{
    if (strcmp(buf, "voltage") == 0) {
        /* 相等 */
        status = RN8209_DEVICE_STATUS_VOLTAGE;
    }
    else if (strcmp(buf, "current") == 0) {
        /* 相等 */
        status = RN8209_DEVICE_STATUS_CURRENT;
    }   
    else {
        status = RN8209_DEVICE_STATUS_DUMP;
    }

    return len;
}

static DEVICE_ATTR(rn8209_device_test, S_IWUSR|S_IRUSR, cat_rn8209_device, echo_rn8209_device);
 
struct file_operations rn8209_device_ops={
    .owner  = THIS_MODULE,
};
 
static int major;
static struct class *cls;
static int rn8209_device_init(void)
{

    meter_flash = flash_init(flash_data_file, flash_meta_file);
    int ret = meter_uart_open(uart_dev);

    struct device *rn8209_device;
    major=register_chrdev(0,"rn8209", &rn8209_device_ops);
    cls=class_create(THIS_MODULE, "rn8209_class");
    //创建mytest_device设备
    rn8209_device = device_create(cls, 0, MKDEV(major,0),NULL,"rn8209_device");    
    
    //在mytest_device设备目录下创建一个my_device_test属性文件
    if(sysfs_create_file(&(rn8209_device->kobj), &dev_attr_rn8209_device_test.attr)) {
        return -1;
    }
    
    return 0;
}
 
static void rn8209_device_exit(void)
{
    meter_uart_close();
    if (meter_flash != NULL) {
      flash_deinit(meter_flash); // 自动 save_all 后释放
      meter_flash = NULL;
    }    
    device_destroy(cls, MKDEV(major,0));
    class_destroy(cls);
    unregister_chrdev(major, "rn8209");
}
 
module_init(rn8209_device_init);
module_exit(rn8209_device_exit);
MODULE_LICENSE("GPL");