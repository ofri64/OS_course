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

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"

// The message the device will give when asked
//static char the_message[BUF_LEN];

// Array to represent all the devices our driver handles (assume now it is constant)

static CHANNEL_DEVICE devices[MAX_DEVICES_FOR_DRIVER];


//ioctrl argument
static long arg = -1;

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    int minor;
    printk("Invoking device_open(%p)\n", file);
    minor = iminor(inode);
    printk("The minor number of the device is: %d\n", minor);

    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
    printk("Invoking message_device release(%p,%p)\n", inode, file);

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
    // Unregister the device
    // Should always succeed
    printk( "removing module from kernel! (Cleaning up message_slot module).\n");
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//==================== Helper Function =============================

// Implementing helper function to read, write, search and allocate memory

// Search for the device id within the devices in control by this driver - use minor number
CHANNEL_DEVICE* getDeviceFromMinor(int minor){
    printk( "Trying to return a device requested using minor num\n");
    CHANNEL_DEVICE* device;
    device = NULL;
    int i;
    for (i = 0; i < MAX_DEVICES_FOR_DRIVER; ++i){
        if (devices[i].minor == minor){
            device = &(devices[i]);
            break;
        }
    }
    return device;
}

// Search for the channel id within the device channels - use channel id
CHANNEL* getChannelFromDevice(CHANNEL_DEVICE* device, unsigned long channelId){
    printk( "Trying to return a channel object requested using its id\n");
    CHANNEL* channel;
    channel = NULL;
    int i;
    for (i=0; i < MAX_CHANNELS_FOR_DEVICE; ++i){
        CHANNEL* currentChannel = device->channels[i];
        if (currentChannel != NULL && currentChannel->channelId == channelId){
            channel = currentChannel;
            break;
        }
    }
    return channel;
}

// Write a message to a channel, return num of bytes written or -1 on error
int write_message_to_channel(CHANNEL* channel, const char* message, int messageLength){
    printk("Inside the write message to channel helper function\n");
    if (messageLength > BUF_LEN){
        return -1;
    }

    int i;
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
int read_message_from_channel(CHANNEL* channel, const char* userBuffer, int bufferLength){
    printk("Inside the read message from channel helper function\n");
    if (channel->messageExists == 0){ //there isn't a message on this channel
        return -1;
    }

    int currentMsgLength;
    currentMsgLength = channel->currentMessageLength;
    if (bufferLength < currentMsgLength){ // user space buffer is too small for the message in channel
        return -1;
    }

    int i;
    for (i=0; i < currentMsgLength; ++i){
        put_user(channel->channelBuffer[i], &userBuffer[i]);
    }

    // return number of bytes read
    printk("Read the message %s\n", channel->channelBuffer);
    return i;
}