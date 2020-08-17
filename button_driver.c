#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of_device.h>
#include <linux/input.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/sysfs.h>
#include <linux/sysfs.h>

#define HIGH_STATE 1
#define LOW_STATE 0

struct stm32device_params {
    ktime_t timeout_group1;       // usage ->  timeout_latch = ktime_set( 0, 1000 );
    ktime_t timeout_group2;
    char gain_c;
};

struct stm32device_key_data;

struct stm32device_key_data {
    struct i2c_client *client;
    struct device *dev;
    struct input_dev *dev_in;
    struct gpio_desc *irqpin;
    struct hrtimer hrt_group1;
    struct hrtimer hrt_group2;
    struct work_struct work_group1;
    struct work_struct work_group2;
    struct stm32device_params params;
};

static int stm32device_i2c_write(struct device *dev, unsigned int reg,
                                unsigned int val);
static int stm32device_i2c_read(struct device *dev);

/* --------------------------------------------------------------------------------------------*/
static irqreturn_t stm32device_irq_handler( int irq, void *dev_id )
{
    struct stm32device_key_data *keydata = dev_id;
    uint8_t input_status;
    bool active = 0;
    input_status = stm32device_i2c_read(keydata->dev);

    dev_printk(KERN_INFO, keydata->dev, "stm32device: Button Pressed! NR: %d\n", input_status);

    // (1 - 49 ;; 9 - 57) ACII TABLE
    if(input_status <= 57)
	input_status = input_status - 48; 
    else
	input_status = input_status - 87; 
	
    switch(input_status) {
    case 1:
        input_report_key(keydata->dev_in, KEY_F1, input_status);
        input_report_key(keydata->dev_in, KEY_F1, 0);
	active = 1;
	break;
    case 2:
        input_report_key(keydata->dev_in, KEY_F2, input_status);
        input_report_key(keydata->dev_in, KEY_F2, 0);
	active = 1;
	break;
    case 3:
        input_report_key(keydata->dev_in, KEY_F3, input_status);
        input_report_key(keydata->dev_in, KEY_F3, 0);
	active = 1;
	break;
    case 4:
        input_report_key(keydata->dev_in, KEY_F4, input_status);
        input_report_key(keydata->dev_in, KEY_F4, 0);
	active = 1;
	break;
    case 5:
        input_report_key(keydata->dev_in, KEY_F5, input_status);
        input_report_key(keydata->dev_in, KEY_F5, 0);
	active = 1;
	break;
    case 6:
        input_report_key(keydata->dev_in, KEY_F6, input_status);
        input_report_key(keydata->dev_in, KEY_F6, 0);
	active = 1;
	break;
    case 7:
        input_report_key(keydata->dev_in, KEY_F7, input_status);
        input_report_key(keydata->dev_in, KEY_F7, 0);
	active = 1;
	break;
    case 8:
        input_report_key(keydata->dev_in, KEY_F8, input_status);
        input_report_key(keydata->dev_in, KEY_F8, 0);
	active = 1;
	break;
    case 9:
        input_report_key(keydata->dev_in, KEY_F9, input_status);
        input_report_key(keydata->dev_in, KEY_F9, 0);
	active = 1;
	break;
    case 10:
        input_report_key(keydata->dev_in, KEY_F10, input_status);
        input_report_key(keydata->dev_in, KEY_F10, 0);
	active = 1;
	break;
    case 11:
        input_report_key(keydata->dev_in, KEY_F11, input_status);
        input_report_key(keydata->dev_in, KEY_F11, 0);
	active = 1;
	break;
    case 12:
        input_report_key(keydata->dev_in, KEY_F12, input_status);
        input_report_key(keydata->dev_in, KEY_F12, 0);
	active = 1;
	break;
    case 13:
        input_report_key(keydata->dev_in, KEY_F13, input_status);
        input_report_key(keydata->dev_in, KEY_F13, 0);
	active = 1;
	break;
    case 14:
        input_report_key(keydata->dev_in, KEY_F14, input_status);
        input_report_key(keydata->dev_in, KEY_F14, 0);
	active = 1;
	break;
    default:
	active = 0;
	break;
    }
    if (active) {
    	input_sync(keydata->dev_in);
    	dev_printk(KERN_INFO, keydata->dev, "stm32device: button pressed finish %d\n", input_status);
    }
    return IRQ_HANDLED;
}

/* --------------------------------------------------------------------------------------------*/

/* Init module printer driver */

/* Functions */
static int stm32device_i2c_write(struct device *dev, unsigned int reg,
                                unsigned int val) {
    struct i2c_client *client = to_i2c_client(dev);
    return i2c_smbus_write_byte_data(client, reg, val);
}

static int stm32device_i2c_read(struct device *dev) {
    struct i2c_client *client = to_i2c_client(dev);
    return i2c_smbus_read_byte(client);
}
/* ------------------------------------------------------------------------------------------- */

static int stm32device_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct stm32device_key_data *keydata;
    int error;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
            dev_printk(KERN_ERR, &client->dev, "I2C check functionality failed.\n");
            return -ENXIO;
    }

    keydata = devm_kzalloc(&client->dev, sizeof(struct stm32device_key_data), GFP_KERNEL);
    if(!keydata) return -ENOMEM;

    keydata->client = client;
    /* Set client to struct keydata */
    i2c_set_clientdata(client, keydata);
    keydata->dev = &client->dev;

    dev_printk(KERN_INFO, keydata->dev,"I2C Address: 0x%02x\n", client->addr);

    /* Get the interrupt GPIO pin number */
    keydata->irqpin = devm_gpiod_get_optional(keydata->dev, "irqpin", GPIOD_IN);
        if (IS_ERR(keydata->irqpin)) {
            error = PTR_ERR(keydata->irqpin);
        if (error != -EPROBE_DEFER)
            dev_printk(KERN_ERR, keydata->dev, "Failed to get %s GPIO: %d\n", "irqpin", error);
        else
            dev_printk(KERN_ERR, keydata->dev, "probe defer for GPIO irqpin\n");
        return error;
    }
    if(!keydata->irqpin)
        dev_printk(KERN_ERR, keydata->dev, "unable to get irqpin pin handle");

    /* Init input device */
    keydata->dev_in = devm_input_allocate_device(keydata->dev);
    if (!keydata->dev_in) {
            printk(KERN_ERR "button.c: Not enough memory\n");
            error = -ENOMEM;
            //goto err_free_irq;
    }
    set_bit(EV_KEY, keydata->dev_in->evbit);
    set_bit(KEY_F1, keydata->dev_in->keybit);
    set_bit(KEY_F2, keydata->dev_in->keybit);
    set_bit(KEY_F3, keydata->dev_in->keybit);
    set_bit(KEY_F4, keydata->dev_in->keybit);
    set_bit(KEY_F5, keydata->dev_in->keybit);
    set_bit(KEY_F6, keydata->dev_in->keybit);
    set_bit(KEY_F7, keydata->dev_in->keybit);
    set_bit(KEY_F8, keydata->dev_in->keybit);
    set_bit(KEY_F9, keydata->dev_in->keybit);
    set_bit(KEY_F10, keydata->dev_in->keybit);
    set_bit(KEY_F11, keydata->dev_in->keybit);
    set_bit(KEY_F12, keydata->dev_in->keybit);
    set_bit(KEY_F13, keydata->dev_in->keybit);
    set_bit(KEY_F14, keydata->dev_in->keybit);
    keydata->dev_in->name = "STM32_Keyboard";
    keydata->dev_in->phys = "stm32/input0";
    error = input_register_device(keydata->dev_in);
    if (error) {
        printk(KERN_ERR "Failed to register device input\n");
        //goto err_free_dev;
    }

    /* INT */
    error =  devm_request_threaded_irq(keydata->dev, client->irq, NULL,
                                       stm32device_irq_handler,
                                       IRQ_TYPE_LEVEL_HIGH | IRQF_ONESHOT, client->name, keydata);
    /* Init IRQ */
    if(error) {
        dev_printk(KERN_ERR, keydata->dev, "request IRQ failed: %d\n", error);
            return error;
    }
    
    dev_printk(KERN_INFO, keydata->dev, "stm32device: probe completed\n");
    return 0;
}

static int stm32device_remove(struct i2c_client *client) {

    struct stm32device_key_data *keydata = i2c_get_clientdata(client);
    input_unregister_device(keydata->dev_in);
    
    pr_info("stm32device: Remove keyboard device \n");
    return 0;
}
static const struct i2c_device_id stm32device_i2c_ids[] = {
        { "stm32device", 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, stm32device_i2c_ids);

static const struct of_device_id stm32device_of_match[] = {
        { .compatible = "st,stm32device" },
        { }
};
MODULE_DEVICE_TABLE(of, stm32device_of_match);
/* 
name 		-	Name of the device driver.
of_match_table 	-  	The open firmware table.
probe 		-	Called to query the existence of a specific device,
whether this driver can work with it, and bind the driver to a specific device.
*/
static struct i2c_driver stm32device_i2c_driver = {
	.driver = {
                .name = "stm32device",
                .of_match_table = stm32device_of_match,
                .owner = THIS_MODULE,
	},
        .id_table = stm32device_i2c_ids,
        .probe = stm32device_probe,
        .remove = stm32device_remove
};
module_i2c_driver(stm32device_i2c_driver);

MODULE_AUTHOR("STRING");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("STM32 button driver");

