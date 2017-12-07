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
static DEVICE* devices[MAX_DEVICES_FOR_DRIVER];


//ioctrl argument
static long arg = -1;

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    int minor;
    DEVICE* device;
    int index;
    printk("Invoking device_open(%p)\n", file);
    minor = iminor(inode);
    printk("The minor number of the device to open is: %d\n", minor);
    device = getExistingDeviceFromMinor(minor, &index);
    if (device == NULL) {
        // device is not allocated yet allocated;
        device = allocateDevice(minor);
        if (device == NULL) {
            return -ENOMEM;
        }
        index = findAvailableDeviceIndex();
        printk("Available index for the new can be found in index %d\n", index);
        if (index == -1){
            printk("Didn't find an available slot to allocate. too many registered devices");
            return -ENOMEM;
        }
        devices[index] = device;
        printk("Created new device data structure, in index %d\n", index);

        return SUCCESS;

    } else{
        // device is already allocated just mark it as open
        device->isOpen = 1;
        return SUCCESS;

    }

}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
    int minor;
    int deviceIndex;
    DEVICE* device;
    printk("Invoking message_device release(%p,%p)\n", inode, file);
    minor = iminor(inode);
    printk("The minor number of the device to release is: %d\n", minor);
    device = getExistingDeviceFromMinor(minor, &deviceIndex);
    printk("the device index returned is %d", deviceIndex);

    if (device != NULL){
    // we need to indicate the device is closed - the device is registered
        device->isOpen = 0;
        printk("Indicated that device num %d is closed\n", minor);
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
    DEVICE* currentDevice;
    printk( "removing module from kernel! (Cleaning up message_slot module).\n");
    // free all memory allocations
    for (i=0; i < MAX_DEVICES_FOR_DRIVER; i++){
        if (devices[i] != NULL){
            currentDevice = devices[i];
            for(j=0; j < MAX_CHANNELS_FOR_DEVICE; j++){
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


CHANNEL* createChannel(unsigned long channelId){
    printk("Creating new channel with id %d\n", channelId);
    CHANNEL* channel = (CHANNEL* ) kmalloc(sizeof(CHANNEL), GFP_KERNEL);
    if (channel == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Channel\n");
    channel->channelId = channelId;
    channel->messageExists = 0;
    channel->currentMessageLength = 0;
    return channel;
}

CHANNEL_NODE* createChannelNode(CHANNEL* channel){
    printk("Creating new node associated with channel with id  %d\n", channel->channelId);
    CHANNEL_NODE* cNode = (CHANNEL_NODE*) kmalloc(sizeof(CHANNEL_NODE), GFP_KERNEL);
    if (cNode == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Channel Node\n");
    cNode->dataChannel = channel;
    cNode->next = NULL;
    return cNode;
}

DEVICE* createDevice(int minor){
    printk("Creating new device with minor number %d\n", minor);
    DEVICE* device = (DEVICE*) kmalloc(sizeof(DEVICE), GFP_KERNEL);
    if (device == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Device\n");
    device->minor = minor;
    device->isOpen = 1;
    device->channels = NULL;
    return device;
}

DEVICE_NODE* createDeviceNode(DEVICE* device){
    printk("Creating new node associated with device with minor  %d\n", device->minor);
    DEVICE_NODE* dNode = (DEVICE_NODE*) kmalloc(sizeof(DEVICE_NODE), GFP_KERNEL);
    if (dNode == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Device Node\n");
    dNode->device = device;
    dNode->next = NULL;
    return dNode;
}


CHANNEL_LINKED_LIST* cretaeEmptyChannelsList(){
    printk("Creating new empty channels list\n");
    CHANNEL_LINKED_LIST* cList = (CHANNEL_LINKED_LIST* ) kmalloc(sizeof(CHANNEL_LINKED_LIST), GFP_KERNEL);
    if (cList == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Channels List\n");
    cList->head = NULL;
}

DEVICE_LINKED_LIST* createEmptyDeviceList(){
    printk("Creating new empty devices list\n");
    DEVICE_LINKED_LIST* dList = (DEVICE_LINKED_LIST* ) kmalloc(sizeof(DEVICE_LINKED_LIST), GFP_KERNEL);
    if (dList == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Devices List\n");
    dList->head = NULL;
}

void destroyChannel(CHANNEL* channel){
    if (channel != NULL) {
        printk("Destroying channel with id %d\n", channel->channelId);
        kfree(channel);
    }
}

void destroyChannelNode(CHANNEL_NODE* cNode){
    if (cNode != NULL){
        printk("Destroying channel node associated with channel id %d\n", cNode->dataChannel->channelId);
        destroyChannel(cNode->dataChannel);
        kfree(cNode);
    }
}

void destroyChannelLinkedList(CHANNEL_LINKED_LIST* cList){
    CHANNEL_NODE* current;
    CHANNEL_NODE* tmp;
    if (cList != NULL){
        printk("Destroying channels List\n");
        current = cList->head;
        while (current != NULL){
            tmp = current;
            current = current->next;
            destroyChannelNode(tmp);
        }
    }
}

void destroyDevice(DEVICE* device){
    if (device != NULL){
        printk("Destroying device with minor number %d\n", device->minor);
        destroyChannelLinkedList(device->channels);
        kfree(device);
    }
}

void destroyDeviceNode(DEVICE_NODE* dNode){
    if (dNode != NULL){
        printk("Destroying device node associated with device with minor number %d\n", dNode->device->minor);
        destroyDevice(dNode->device);
        kfree(dNode);
    }
}

void destroyDeviceLinkedList(DEVICE_LINKED_LIST* dList){
    DEVICE_NODE* current;
    DEVICE_NODE* tmp;
    if (dList != NULL){
        printk("Destroying devices List\n");
        current = dList->head;
        while (current != NULL){
            tmp = current;
            current = current->next;
            destroyDeviceNode(tmp);
        }
    }
}

int addChannel(CHANNEL_LINKED_LIST* cList, unsigned long channelId){
    CHANNEL* channel;
    CHANNEL_NODE* newChannelNode;
    CHANNEL_NODE* currentNode;
    printk("Adding new channel with id %d to channel linked list\n", channelId);
    channel = createChannel(channelId);

    if (channel == NULL){
        printk("Failed adding, memory error in channel\n");
        return -1;
    }
    newChannelNode = createChannelNode(channel);
    if (newChannelNode == NULL){
        printk("Failed adding, memory error in channel node\n");
        return -1;
    }

    currentNode = cList->head;
    if (currentNode == NULL){
        printk("Adding node channel to head of list\n");
        cList->head = newChannelNode;
    }
    else{
        while (currentNode->next != NULL){
            currentNode = currentNode->next;
        }
        printk("Adding node channel to end of list\n");
        currentNode->next = newChannelNode;
    }
    return 0;
}

int addDevice(DEVICE_LINKED_LIST* dList, int minor){
    DEVICE* device;
    DEVICE_NODE* newDeviceNode;
    DEVICE_NODE* currentNode;
    printk("Adding new channel with minor number %d to device linked list\n", minor);
    device = createDevice(minor);

    if (device == NULL){
        printk("Failed adding, memory error in device\n");
        return -1;
    }

    newDeviceNode = createDeviceNode(device);
    if (newDeviceNode == NULL){
        printk("Failed adding, memory error in device node\n");
        return -1;
    }

    currentNode = dList->head;
    if (currentNode == NULL){
        printk("Adding node device to head of list\n");
        dList->head = newDeviceNode;
    }
    else{
        while (currentNode->next != NULL){
            currentNode = currentNode->next;
        }
        printk("Adding node device to end of list\n");
        currentNode->next = newDeviceNode;
    }
    return 0;
}

DEVICE* findDeviceFromMinor(DEVICE_LINKED_LIST* dList, int minor){
    printk("Searching for device with minor %d\n", minor);
    DEVICE* device;
    DEVICE_NODE* current;
    device = NULL;
    current = dList->head;

    while (current != NULL){
        if (current->device->minor == minor){
            device = current->device;
            break;
        }
    }
    return device;
}




























// Allocate memory and initialize device object

//DEVICE* allocateDevice(int minor){
//    DEVICE* device;
//    int i;
//    printk("Trying to allocated memory for new device object\n");
//    device = (DEVICE* ) kmalloc(sizeof(DEVICE), GFP_KERNEL);
//    if (device == NULL) {
//        return NULL;
//    }
//
//    printk("Allocated memory successfully\n");
//    // update device data
//    device->minor = minor;
//    device->isOpen = 1;
//    for (i=0; i < MAX_CHANNELS_FOR_DEVICE; i++){
//        device->channels[i] = NULL;
//    }
//    printk("Initiated device values\n");
//    return device;
//}
//
//// Search for the device id within the devices in control by this driver - use minor number
//DEVICE* getExistingDeviceFromMinor(int minor, int* index){
//    DEVICE* device;
//    DEVICE* currentDevice;
//    int i;
//    int deviceIndex;
//    device = NULL;
//    deviceIndex = -1;
//    printk( "Trying to return a device requested using minor num\n");
//    for (i = 0; i < MAX_DEVICES_FOR_DRIVER; i++){
//        currentDevice = devices[i];
//        if (currentDevice !=NULL){
//            printk("The current device is not null and is minor number is %d\n", currentDevice->minor);
//        }
//        if (currentDevice != NULL && currentDevice->minor == minor){
//            printk( "Found the device with the minor number within the device at index %d\n", i);
//            device = currentDevice;
//            deviceIndex = i;
//            break;
//        }
//    }
//
//    *index = deviceIndex;
//    return device;
//}
//
//// Search for the channel id within the device channels - use channel id
//CHANNEL* getChannelFromDevice(DEVICE* device, unsigned long channelId){
//    CHANNEL* channel;
//    CHANNEL* currentChannel;
//    int i;
//    channel = NULL;
//    printk( "Trying to return a channel object requested using its id\n");
//    for (i=0; i < MAX_CHANNELS_FOR_DEVICE; i++){
//        currentChannel = device->channels[i];
//        if (currentChannel != NULL && currentChannel->channelId == channelId){
//            printk( "Found the channel with the id within the device at index %d\n", i);
//            channel = currentChannel;
//            break;
//        }
//    }
//    return channel;
//}
//
//int findAvailableDeviceIndex(){
//    int i;
//    int index;
//    index = -1;
//    printk("Looking of an available place to locate a device\n");
//    for (i=0; i < MAX_DEVICES_FOR_DRIVER; i++){
//        if (devices[i] == NULL){
//            index = i;
//            printk("Found an available place for device in index %d\n", i);
//            break;
//        }
//    }
//
//    return index;
//}
//
//int findAvialableChannelIndex(DEVICE* device){
//    int i;
//    int index;
//    index = -1;
//    printk("Looking of an available place to locate a new channel buffer\n");
//    for (i=0; i < MAX_CHANNELS_FOR_DEVICE; i++){
//        if (device->channels[i] != NULL){
//            index = i;
//            printk("Found an available place for channel in index %d\n", i);
//            break;
//        }
//    }
//
//    return index;
//}
//
//// Write a message to a channel, return num of bytes written or -1 on error
//int write_message_to_channel(CHANNEL* channel, const char* message, int messageLength){
//    int i;
//    printk("Inside the write message to channel helper function\n");
//    if (messageLength > BUF_LEN){
//        return -1;
//    }
//    for (i=0; i < messageLength; i++){
//        get_user(channel->channelBuffer[i], &message[i]);
//    }
//
//    //update the channel and return number of bytes written to channel
//    channel->messageExists = 1;
//    channel->currentMessageLength = i;
//    printk("Wrote the message %s\n", channel->channelBuffer);
//    return i;
//}
//
//// Read a message from a channel, return num of bytes read from channel or -1 on error
//int read_message_from_channel(CHANNEL* channel, char* userBuffer, int bufferLength){
//    int currentMsgLength;
//    int i;
//    printk("Inside the read message from channel helper function\n");
//    if (channel->messageExists == 0){ //there isn't a message on this channel
//        return -1;
//    }
//    currentMsgLength = channel->currentMessageLength;
//    if (bufferLength < currentMsgLength){ // user space buffer is too small for the message in channel
//        return -1;
//    }
//
//    for (i=0; i < currentMsgLength; i++){
//        put_user(channel->channelBuffer[i], &userBuffer[i]);
//    }
//
//    // return number of bytes read
//    printk("Read the message %s\n", channel->channelBuffer);
//    return i;
//}
