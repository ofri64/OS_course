//
// Created by okleinfeld on 12/6/17.
//


// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE



#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>      /* for kmalloc and kfree */
#include <asm/errno.h>      /* for memory and other errors */

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"

// The message the device will give when asked
//static char the_message[BUF_LEN];

// Array to represent all the devices our driver handles (assume now it is constant)
static CHANNEL_DEVICE* devices[MAX_DEVICES_FOR_DRIVER];


//ioctrl argument
static long arg = -1;

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    int minor;
    CHANNEL_DEVICE* device;
    int index;
    printk("Invoking device_open(%p)\n", file);
    minor = iminor(inode);
    printk("The minor number of the device to open is: %d\n", minor);
    device = getExistingDeviceFromMinor(minor, &index);
    if (device == NULL) {
        // device is not allocated yet allocated;
        device = (CHANNEL_DEVICE* ) kmalloc(sizeof(CHANNEL_DEVICE), GFP_KERNEL);
        if (device == NULL) {
            return -ENOMEM;
        }

        printk("Allocated memory for new device object\n");
        index = findAvailableDeviceIndex();
        printk("Available index can be found in index %d\n", index);
        if (index == -1){
            printk("Didn't find an available slot to allocate. too many registered devices");
            return -ENOMEM;
        }
        device->isOpen = 1;
        devices[index] = device;
        printk("Created new device data structure, in index %d\n", index);

    }

    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
    int minor;
    int deviceIndex;
    CHANNEL_DEVICE* device;
    printk("Invoking message_device release(%p,%p)\n", inode, file);
    minor = iminor(inode);
    printk("The minor number of the device to release is: %d\n", minor);
    device = getExistingDeviceFromMinor(minor, &deviceIndex);
    printk("the device returned address is %x and index in array is %d", device, deviceIndex);

    if (device != NULL){
    // we need to indicate the device is closed - the device is registered
        device->isOpen = 0;
        printk("Indicated the device is closed\n", minor);
    }
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset){

    // read doesnt really do anything (for now)
    printk( "Invocing device_read(%p,%d) operation not supported yet\n", file, (int) length);
    printk( "But I can show you my argument value: %d\n", (int) arg);
    //invalid argument error
    return -EINVAL;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to i

static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset){

    // read doesnt really do anything (for now)
    printk( "Invocing write(%p,%d) operation not supported yet\n", file, (int) length);
    //invalid argument error
    return -EINVAL;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    // Switch according to the ioctl called
    if( MSG_SLOT_CHANNEL == ioctl_command_id )
    {
        // Get the parameter given to ioctl by the process
        printk( "Invoking ioctl: setting arg to %ld\n", ioctl_param );
        arg = ioctl_param;
        return SUCCESS;
    }

    else{
        return -EINVAL;
    }

}



//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .release        = device_release,
        };


//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void){
    int rc = -1;
    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 )
    {
        printk( KERN_ALERT "%s registration failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }

    printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);
    printk( "Registeration is successful. ");
    printk( "If you want to talk to the device driver,\n" );
    printk( "you have to create a device file:\n" );
    printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );
    printk( "You can echo/cat to/from the device file.\n" );
    printk( "Dont forget to rm the device file and "
                    "rmmod when you're done\n" );

    return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    // Unregiseter the device
    // Should always succeed
    int i;
    int j;
    CHANNEL_DEVICE* currentDevice;
    printk( "removing module from kernel! (Cleaning up message_slot module).\n");
    // free all memory allocations
    for (i=0; i < MAX_DEVICES_FOR_DRIVER; ++i){
        if (devices[i] != NULL){
            currentDevice = devices[i];
            for(j=0; j < MAX_CHANNELS_FOR_DEVICE; ++j){
                if (currentDevice->channels[j] != NULL){
                    kfree(currentDevice->channels[j]);
                }
            }
            kfree(currentDevice);
            printk( "Freed memory associated with device num %d", i);
        }
        printk("done with iteration %d", i);
    }
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//==================== Helper Function =============================

// Implementing helper function to read, write, search and allocate memory

// Search for the device id within the devices in control by this driver - use minor number
CHANNEL_DEVICE* getExistingDeviceFromMinor(int minor, int* index){
    CHANNEL_DEVICE* device;
    CHANNEL_DEVICE* currentDevice;
    int i;
    int deviceIndex;
    device = NULL;
    deviceIndex = -1;
    printk( "Trying to return a device requested using minor num\n");
    for (i = 0; i < MAX_DEVICES_FOR_DRIVER; ++i){
        currentDevice = devices[i];
        printk( "The address of the current memory is %x\n", devices[i]);
        if (currentDevice != NULL && currentDevice->minor == minor){
            printk( "Found the device with the minor number within the device at index %d\n", i);
            device = currentDevice;
            break;
        }
    }

    *index = deviceIndex;
    return device;
}

// Search for the channel id within the device channels - use channel id
CHANNEL* getChannelFromDevice(CHANNEL_DEVICE* device, unsigned long channelId){
    CHANNEL* channel;
    CHANNEL* currentChannel;
    int i;
    channel = NULL;
    printk( "Trying to return a channel object requested using its id\n");
    for (i=0; i < MAX_CHANNELS_FOR_DEVICE; ++i){
        currentChannel = device->channels[i];
        if (currentChannel != NULL && currentChannel->channelId == channelId){
            printk( "Found the channel with the id within the device at index %d\n", i);
            channel = currentChannel;
            break;
        }
    }
    return channel;
}

// Write a message to a channel, return num of bytes written or -1 on error
int write_message_to_channel(CHANNEL* channel, const char* message, int messageLength){
    int i;
    printk("Inside the write message to channel helper function\n");
    if (messageLength > BUF_LEN){
        return -1;
    }
    for (i=0; i < messageLength; ++i){
        get_user(channel->channelBuffer[i], &message[i]);
    }

    //update the channel and return number of bytes written to channel
    channel->messageExists = 1;
    channel->currentMessageLength = i;
    printk("Wrote the message %s\n", channel->channelBuffer);
    return i;
}

// Read a message from a channel, return num of bytes read from channel or -1 on error
int read_message_from_channel(CHANNEL* channel, char* userBuffer, int bufferLength){
    int currentMsgLength;
    int i;
    printk("Inside the read message from channel helper function\n");
    if (channel->messageExists == 0){ //there isn't a message on this channel
        return -1;
    }
    currentMsgLength = channel->currentMessageLength;
    if (bufferLength < currentMsgLength){ // user space buffer is too small for the message in channel
        return -1;
    }

    for (i=0; i < currentMsgLength; ++i){
        put_user(channel->channelBuffer[i], &userBuffer[i]);
    }

    // return number of bytes read
    printk("Read the message %s\n", channel->channelBuffer);
    return i;
}

int findAvailableDeviceIndex(){
    int i;
    int index;
    index = -1;
    printk("Looking of an available place to locate a device\n");
    for (i=0; i < MAX_DEVICES_FOR_DRIVER; ++i){
        if (devices[i] == NULL){
            index = i;
            printk("Found an available place for device in index %d\n", i);
            break;
        }
    }

    return index;
}

int findAvialableChannelIndex(CHANNEL_DEVICE* device){
    int i;
    int index;
    index = -1;
    printk("Looking of an available place to locate a new channel buffer\n");
    for (i=0; i < MAX_CHANNELS_FOR_DEVICE; ++i){
        if (device->channels[i] != NULL){
            index = i;
            printk("Found an available place for channel in index %d\n", i);
            break;
        }
    }

    return index;
}