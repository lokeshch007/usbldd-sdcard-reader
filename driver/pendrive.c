#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/usb.h>
#include<linux/uaccess.h>
#include<linux/errno.h>
#include<linux/slab.h>
#include<linux/kref.h>

/**************MACROS Needed*******************/
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))
#define PENDRIVE_MINOR_BASE 144
#define pendrive_dev(d) container_of(d,struct pendrive_local_struct,kref) 
#define MAXLEN (PAGE_SIZE - 512)
/**************MACROS Needed ENDS*******************/
 
/************Function Declarations starts*************************/
static int pendrive_open(struct inode *inode, struct file *file);
static int pendrive_close(struct inode *inode,struct file *file);
static ssize_t pendrive_read(struct file *file,char *buffer,size_t count,loff_t *fpos);
static ssize_t pendrive_write(struct file *file, const char *user_buffer,size_t count, loff_t *fpos);
static void pendrive_delete(struct kref *kref);
static int pendrive_probe(struct usb_interface *interface,const struct usb_device_id *id);
static void pendrive_disconnect(struct usb_interface *interface);
/************Function Declarations ends*******i******************/


/**************GOLBAL VARIABLES DECLARATION STARTS*****************/

/**file operation structure**/
static const struct file_operations file_ops = {
	.owner = THIS_MODULE,//owner of module
	.read = pendrive_read,/*read function pointer*/
	.write = pendrive_write,/*write function pointer*/
	.open = pendrive_open,/*open function pointer*/
	.release = pendrive_close/*close function pointer*/
};

/** usb class structure **/
static struct usb_class_driver pendrive_class = {
	.name = "pendrive_driver",//class name
	.fops = &file_ops,//file operation structure pointer
	.minor_base = PENDRIVE_MINOR_BASE,//minor number base limit
};


/**
 * vendor id, product id registration of USB
 */
static struct usb_device_id pendrive_table[]=
{
	{USB_DEVICE(0xABCD,0x2017)},
	{}
};

//It tell to user space which device can be configured
MODULE_DEVICE_TABLE(usb,pendrive_table);

/**driver call back functions,name,table name structer **/
static struct usb_driver pendrive_driver=
{
	.name="pendrive_driver",//driver name
	.id_table=pendrive_table,//driver table name
	.probe=pendrive_probe,//probe function pointer
	.disconnect=pendrive_disconnect//disconnect function pointer
};

/* Local structure for Device */
struct pendrive_local_struct
{
	struct usb_device *udev;//usb_device pointer
	struct usb_interface *interface;//usb_interface pointer
	unsigned char *bulk_in_buffer;//buffer for data transmission
	size_t bulk_in_size;//bulk in endpoint packet size max
	size_t bulk_in_filled;//NOT USED
	size_t bulk_in_copied;//NOT USED
	__u8 bulk_in_endpointAddr;//bulk in endpoint address
	__u8 bulk_out_endpointAddr;//bulk out endpoint address
	struct kref kref;//used for object counting
};


/************GLOBAL VARIABLES END DECLARATIONS******************/


/************************
* printk() template:
	printk(KERN_INFO "[ %s ] ",THIS_MODULE->name);
*************************/


/********Local structure deallocation function************/
/* this function will be called when any error occured in 'probe' function 
 * or at the end of disconnect function*/
static void pendrive_delete(struct kref *kref)
{
	//container of returns base address of local structure allocated in which kref is a member
	struct pendrive_local_struct *dev = container_of(kref,struct pendrive_local_struct,kref);
	
	//deallocates memory allocated to usb_device structure
	usb_put_dev(dev->udev);
	
	//deallocates memory allcoated to bulk_in_buffer
	kfree(dev->bulk_in_buffer);
	printk(KERN_INFO "[ %s ]: Bulk in buffer freed\n ",THIS_MODULE->name);
	
	//deallocates memory allcoated to local structure
	kfree(dev);
	printk(KERN_INFO "[ %s ]: Device local structure freed\n ",THIS_MODULE->name);
	
}
/********Local structure deallocation ends************/



/***device arrival detection methos****/
/* This function is called when device is connected by USB CORE since 
 * this function matches the interface of the connected device
 */
static int pendrive_probe(struct usb_interface *interface,const struct usb_device_id *id)
{
	struct pendrive_local_struct *dev;//local structure pointer
	struct usb_host_interface *iface_desc;//used for endpoint array traversing
	struct usb_endpoint_descriptor *endpoint;//endpoint descriptor pointer
	size_t buffer_size;
	int i;
	int retval = -ENOMEM;
	
	/*
	 * kzalloc():
	 * kzalloc() allocates the memory and zeros the whole memory and returns the pointer to the memory
	 */
	dev=kzalloc(sizeof(*dev),GFP_KERNEL);
	if(!dev)
	{
		dev_err(&interface->dev,"Out of memory\n");
	}

	//intializes the kref object counter
	kref_init(&dev->kref);

	/*
	 * interface_to_usbdev will convert interface structure to usb_device structure
	 * usb_get_dev will allocate memory to usb_device structure member presented inside our local structure
	 */
	dev->udev=usb_get_dev(interface_to_usbdev(interface));

	//stores interface pointer to local structure
	dev->interface=interface;

	/** in interface structure the member cur_altsetting holds the array of current interface endpoints*/
	iface_desc=interface->cur_altsetting;

	/*This loop loops to every endpoint that is present in the current interface*/
	for(i=0;i<iface_desc->desc.bNumEndpoints;++i)
	{
		//asssigning a local pointer to endpoint structure
		endpoint=&iface_desc->endpoint[i].desc;

		//check whether endpoint is IN or not then it will check whether endpoint type is bulk endpoint or not
		if(!dev->bulk_in_endpointAddr && usb_endpoint_is_bulk_in(endpoint) )	
		{
			/*if(!dev->bulk_in_endpointAddr && (endpoint->bEndpointAddress & USB_DIR_IN)&& 
			((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)==USB_ENDPOINT_XFERTYPE_BULK))*/
			printk(KERN_INFO "[ %s ]:In endpoint%d detected:\n",THIS_MODULE->name,i);//found a bulk in endpoint
			//gets maximum packet size of endpoint and store in buffer_size
			buffer_size = usb_endpoint_maxp(endpoint);
			
			//store buffer size to local structure
			dev->bulk_in_size=buffer_size;

			//store endpoint address to local structure
			dev->bulk_in_endpointAddr=endpoint->bEndpointAddress;

			//allocate buffer_size of memory to bulk_in_buffer present in local structure
			dev->bulk_in_buffer = kmalloc(buffer_size,GFP_KERNEL);

			if(!dev->bulk_in_buffer)//memeory allocation error check
			{
				//wont print anything anywhere just a prompt
				dev_err(&interface->dev,"Could not alloct bulk_in_buffer\n");
				goto error;
			}
		}

		//checks whether endpoint is OUT or not and bulk or not
		if(!dev->bulk_out_endpointAddr && usb_endpoint_is_bulk_out(endpoint))
		{
			//bulk out endpoint detected
			printk(KERN_INFO "[ %s ]:Out endpoint%d detected:\n",THIS_MODULE->name,i);

			//store address of found bulkout_endpoint to local structure
			dev->bulk_out_endpointAddr=endpoint->bEndpointAddress;
		}

		/****traverse whole array of endpoints******/
	}


	//no bulk_in or bulk_out endpoints found 
	if(!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr))
	{
		printk(KERN_INFO "[ %s ]:Could not found Bulk in and Bulk out endpoint\n",THIS_MODULE->name);
		goto error;
	}

	printk(KERN_INFO "[ %s ]:save data pointer in interface\n",THIS_MODULE->name);
	//storing local structure in current interface structure for future use
	usb_set_intfdata(interface,dev);

	
	printk(KERN_INFO "[ %s ]: Registration of device\n",THIS_MODULE->name);
	//registering the device with USB CORE with give interface and class
	retval=usb_register_dev(interface,&pendrive_class);
	if(retval)//error check of device registration
	{
		printk(KERN_INFO "[ %s ] : Not able to get Minor number for this device\n ",THIS_MODULE->name);
		//making local structure data in interface structure as NULL
		usb_set_intfdata(interface,NULL);
		goto error;
	}


	//prompt of minor number
	//this wont print anything anywhere
	dev_info(&interface->dev,"USB device not attached to PENDRIVE-%d",interface->minor);

	//printing vendor id and product id
	printk(KERN_INFO "[ %s ] : Pendrive (%04x:%04x) is plugged in\n",THIS_MODULE->name,id->idVendor,id->idProduct);
	return 0;

error:
	if(dev)
		kref_put(&dev->kref,pendrive_delete);//internlly calls kref_sub which will decrement the object count and calls given function in 2nd arg
	return retval;

}

/***device removal detection methos****/
// This function is called by USB CORE when device is unplugged
static void pendrive_disconnect(struct usb_interface *interface)
{
	struct pendrive_local_struct *dev;
	int minor = interface->minor;//storing minor number to temporay variable

	//getting the local structure pointer stored in interface structure
	dev=usb_get_intfdata(interface);

	//set local structure pointer as null in interface structure
	usb_set_intfdata(interface,NULL);

	//deregister the device with this interface and class so that
	//USB core can use this minor number to other devices
	usb_deregister_dev(interface,&pendrive_class);

	//making interface pointer as null in local structure because that
	//interface no longer present and to avoid dangling pointer
	dev->interface=NULL;

	//decrement the kref object count and calls the function given in 2nd arg
	kref_put(&dev->kref,pendrive_delete);
	printk(KERN_INFO "[ %s ]: Pendrive is unplugged - MINOR : %d\n",THIS_MODULE->name,minor);

	printk(KERN_INFO "Pendrive unplugged\n");
}



static int __init pendrive_init(void)
{
	int ret;//return status variable

	//register usb_driver structure with USB CORE
	ret=usb_register(&pendrive_driver);
	if(ret!=0)//error check for registration of usb_driver
	{
		printk(KERN_INFO "[ %s ]: usb_register() error\n",THIS_MODULE->name);
		return ret;
	}
	printk(KERN_INFO "[ %s]: USB Pendrive driver module initiated\n",THIS_MODULE->name);
	return ret;
}

static int pendrive_open(struct inode *inode, struct file *file)
{
	struct pendrive_local_struct *dev;//local structure pointer
	struct usb_interface *interface;//interface pointer
	int subminor;//minor number variable
	int retval=0;
	printk(KERN_INFO "[ %s ]: pendrive_open() called\n",THIS_MODULE->name);

	//iminor internally calls MINOR(dev_t) 
	//which gives device minor number
	subminor = iminor(inode);

	//in '/dev' with given minor number and driver name find the interface of that driver and return it
	interface = usb_find_interface(&pendrive_driver,subminor);
	if(!interface)//error check
	{
		printk(KERN_INFO "[ %s ]: %s-error can't find device for minor %d\n",THIS_MODULE->name,__func__,subminor);
		retval = -ENODEV;
		goto exit;
	}

	//get local structure's pointer from interface structure
	dev=usb_get_intfdata(interface);
	if(!dev)//error check
	{
		printk(KERN_INFO "[ %s ]: usb_get_infdata() error\n",THIS_MODULE->name);
		retval = -ENODEV;
		goto exit;
	}

	//increment the kref object counter
	kref_get(&dev->kref);

	//store the local structure pointer in file strucuter to private_data pointer so we can
	//use this file structure and access the local structure in other functions
	//where this file structure is being passed
	file->private_data = dev;

exit:
	printk(KERN_INFO "[ %s ]: retval:\t%d\n ",THIS_MODULE->name,retval);
	return retval;
}

static int pendrive_close(struct inode *inode,struct file *file)
{
	struct pendrive_local_struct *dev;//local structure pointer
	printk(KERN_INFO "[ %s ]: pendrive_close() called\n",THIS_MODULE->name);

	//get local structure pointer from file structure private data
	dev=file->private_data;
	if(dev == NULL)//error check
		return -ENODEV;

	
	if(dev->interface)
		usb_autopm_put_interface(dev->interface);//suspending the device so that DEV_CLK and AHB_CLK will be off for device to save power

	//decrement kref object and calls pendrive_delete
	kref_put(&dev->kref,pendrive_delete);

	return 0;
}

static ssize_t pendrive_read(struct file *file,char *buffer,size_t count,loff_t *fpos)
{
	struct pendrive_local_struct *dev;//local structure pointer
	int retval;//return status variable
	int read_count;//no of bytes to read successfully from device
	dev=file->private_data;//getting local structure from file structure private data

	/* Read data from Bulk end point */
	//usb_bulk_msg will create an URB and send it to the specified device in its arg and wait
	//to complete before it returnng to the caller
	retval=usb_bulk_msg(dev->udev,//pointer to usb device to send bulk message
			usb_rcvbulkpipe(dev->udev,dev->bulk_in_endpointAddr),//receives data from bulk_in_endpoint 
			dev->bulk_in_buffer,//buffer where data is stored after reading from endpoint
			MIN(dev->bulk_in_size,count),//no of bytes to be read
			&read_count,//address of count variable
			HZ*10);//timeout
	
	if(retval)//error check if retval==0 means no data read
	{
		printk(KERN_INFO "[ %s ]: Bulk message returned %d\n",THIS_MODULE->name,retval);
		return retval;
	}

	printk(KERN_INFO "[ %s ]: Pendrive read data\n",THIS_MODULE->name);

	/*  copy data to user buffer
	 *  copy the read data to user buffer and do error check
	 */
	if(copy_to_user(buffer,dev->bulk_in_buffer,MIN(count,read_count)))
	{
		return -EFAULT;
	}

	printk(KERN_INFO "[ %s ]: copy_to_user() done\n",THIS_MODULE->name);

	//return successfull read bytes
	return MIN(count,read_count);
}

static ssize_t pendrive_write(struct file *file, const char *user_buffer,size_t count, loff_t *fpos)
{
	static struct pendrive_local_struct *dev;//local structure pointer
	int retval,write_count;//return status variable,no of bytes to read successfully from device
	char *write_buf=NULL;//buffer used for writing data
	dev=file->private_data;//getting local structure from file structure private data
	write_count=MIN(count,(size_t)MAXLEN);//find valid byte size to be write

	//allocate memory to write buffer of write_count bytes
	write_buf=kmalloc(write_count,GFP_KERNEL);
	if(write_buf == NULL)//error check
	{
		printk(KERN_INFO "[ %s ]: pendrive_write() failed to kmalloc() \n",THIS_MODULE->name);
		return -ENOMEM;
	}

	/* copy data from user buffer to write buffer and do error check*/
	if(copy_from_user(write_buf,user_buffer,write_count))
	{
		printk(KERN_INFO "[ %s ]: copy_from_user() failed\n",THIS_MODULE->name);
		return -EFAULT;
	}

	/* Write data to bulk end point */
	retval=usb_bulk_msg(dev->udev,//pointer to usb device to send bulk message
		usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),//receives data from bulk_out_endpoint 
		write_buf, //write buffer
		write_count,//no of bytes to write
		&write_count,//address of count variable
		HZ*10);//timeout
	
	//error check,if retval==0 means no data written		
	if(retval)
	{
		printk(KERN_INFO "[ %s ]: Bulk message returned %d\n",THIS_MODULE->name,retval);
		return retval;
	}

	//deallocate memeory to write buffer
	kfree(write_buf);
	
	//return successful written bytes count
	return write_count;
}




static void __exit pendrive_exit(void)
{
	//deregister driver structure from USB core
	usb_deregister(&pendrive_driver);
	printk(KERN_INFO "[ %s ]: Exiting from USB pendrive driver module\n",THIS_MODULE->name);
} 



module_init(pendrive_init);
module_exit(pendrive_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Lokesh");
MODULE_DESCRIPTION("pendrive_driver");
