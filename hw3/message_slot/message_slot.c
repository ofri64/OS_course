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

// Devices Linked list to represent all the devices our driver handles (assume now it is constant)
static DEVICE_LINKED_LIST* list;
static int currentHandledDevice = -1;

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    int minor;
    int status;
    DEVICE* device;
    printk("Invoking device_open(%p)\n", file);
    minor = iminor(inode);
    printk("The minor number of the device to open is: %d\n", minor);
    device = findDeviceFromMinor(list, minor);
    if (device != NULL) {
        //device is already allocated, just need to open it
        device->isOpen = 1;
        currentHandledDevice = minor;
        printk("The device was found, and marked as open and as current handled device\n");
    }
    else {
        printk("The device was not found, trying to create a new device\n");
        status = addDevice(list, minor);
        if (status < 0 ) {
            printk("Didn't managed to add new device. memory error\n");
            return -ENOMEM;
        }

        // New device created is already marked as open, but do need to set current handled device
        currentHandledDevice = minor;
        printk("Successfully created a new device and marked it as current handled device\n");
    }
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file) {
    int minor;
    DEVICE *device;
    printk("Invoking message_device release(%p,%p)\n", inode, file);
    minor = iminor(inode);
    printk("The minor number of the device to release is: %d\n", minor);
    device = findDeviceFromMinor(list, minor);
    if (device == NULL) {
        printk("Trying to close the device but could not find such one\n");
        // trying to close device that doesn't exist. don't do anything

    } else {
        if (device->isOpen == 0) {
            // again don't do anything the device is already closed
            printk("Trying to close the device. the device was found but already closed\n");
        } else {
            // we need to indicate the device is closed - the device is registered
            device->isOpen = 0;
            printk("Closed the device\n");
        }
    }
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset){

    unsigned long channelId;
    DEVICE* currentDevice;
    CHANNEL* currentChannel;
    int readStatus;
    printk( "Invocing device_read with file descriptor and message length: (%p,%d) \n", file, (int) length);

    channelId = (unsigned long) file->private_data;
    printk("The current handeled device number is %d\n",currentHandledDevice);
    printk( "The desired channel id to read from is %ld\n", channelId);

    // check for errors
    currentDevice = findDeviceFromMinor(list, currentHandledDevice);
    if (currentDevice == NULL || currentDevice->isOpen == 0){
        printk("Problem with current device - it doens't exist or it did not opened prior to read request\n");
        return -EINVAL;
    }

    currentChannel = findChannelInDevice(currentDevice, channelId);
    if (currentChannel == NULL){
        printk("The desired channel hasn't been set yet %ld\n", channelId);
        return -EINVAL;
    }

    readStatus = readMessageFromChannel(currentChannel, buffer, length);
    if (readStatus == -1){
        return -EWOULDBLOCK;
    }

    if (readStatus == -2){
        return -ENOSPC;
    }


    // Read status is 0 - read the message successfully
    return SUCCESS;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to i

static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset){

    unsigned long channelId;
    DEVICE* currentDevice;
    CHANNEL* currentChannel;
    int writeStatus;
    printk( "Invocing device_write with file descriptor and message length: (%p,%d) \n", file, (int) length);

    channelId = (unsigned long) file->private_data;
    printk("The current handeled device number is %d\n",currentHandledDevice);
    printk( "The desired channel id to read from is %ld\n", channelId);

    // check for errors
    currentDevice = findDeviceFromMinor(list, currentHandledDevice);
    if (currentDevice == NULL || currentDevice->isOpen == 0){
        printk("Problem with current device - it doens't exist or it did not opened prior to read request\n");
        return -EINVAL;
    }

    currentChannel = findChannelInDevice(currentDevice, channelId);
    if (currentChannel == NULL){
        printk("The desired channel hasn't been set yet %ld\n", channelId);
        return -EINVAL;
    }

    writeStatus = writeMessageToChannel(currentChannel, buffer, length);
    if (writeStatus < 0){
        return -EINVAL;
    }

    // Write was successful
    return SUCCESS;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    DEVICE* currentDevice;
    CHANNEL* currentChannel;
    CHANNEL_LINKED_LIST* cList;
    int status;
    // Switch according to the ioctl called
    printk("Entering ioctl command\n");
    if( MSG_SLOT_CHANNEL == ioctl_command_id )
    {
        // Get the parameter given to ioctl by the process
        printk( "Invoking ioctl: associating the channel id given to the file descriptor %ld\n", ioctl_param );
        file->private_data = (void *) ioctl_param;

        printk("The current device handled by the driver is %d", currentHandledDevice);

        // Verify that the channel exists. Crate a new channel if it doesn't

        currentDevice = findDeviceFromMinor(list, currentHandledDevice);
        if (currentDevice == NULL){
            printk("Wrong ioctl command. the device is not defined yet\n");
            return -EINVAL;
        }

        currentChannel = findChannelInDevice(currentDevice, ioctl_param);
        if (currentChannel != NULL){
            printk("The channel already exists - ready for read or write operations\n");
        }
        else{
            printk("The channel doesn't exists - trying to allocate a new channel\n");

            if (currentDevice->channels == NULL){ // no channels list at all for device
                printk("Needs to allocate also channels linked list for the device\n");
                cList = cretaeEmptyChannelsList();
                if (cList == NULL){
                    printk("Channels linked list allocation failed. returning indicating status\n");
                    return -ENOMEM;
                }
                currentDevice->channels = cList;
            }

            status = addChannel(currentDevice->channels, ioctl_param);
            if (status < 0){
                printk("New channel allocation failed. returning indicating status\n");
                return -ENOMEM;
            }
        }
    }


    else{
        printk("The ioctl command given is not in the right format. error is returned\n");
        return -EINVAL;
    }

    return SUCCESS;

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
    DEVICE_LINKED_LIST* dList;
    int rc = -1;
    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 )
    {
        printk( KERN_ALERT "%s registration failed for %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }

    dList = createEmptyDeviceList();
    if (dList == NULL){
        printk( KERN_ALERT "%s registration failed for %d, memory allocation error\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return -1;
    }

    list = dList;

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
    printk( "removing module from kernel! (Cleaning up message_slot module).\n");
    // free all memory allocations
    destroyDeviceLinkedList(list);
    printk( "Removed all memory allocations of the module.\n");
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//==================== Helper Function =============================

// Implementing helper function to read, write, search and allocate memory


CHANNEL* createChannel(unsigned long channelId){
    CHANNEL* channel;
    printk("Creating new channel with id %d\n", (int) channelId);
    channel = (CHANNEL* ) kmalloc(sizeof(CHANNEL), GFP_KERNEL);
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
    CHANNEL_NODE* cNode;
    printk("Creating new node associated with channel with id  %d\n", (int) channel->channelId);
    cNode = (CHANNEL_NODE*) kmalloc(sizeof(CHANNEL_NODE), GFP_KERNEL);
    if (cNode == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Channel Node\n");
    cNode->dataChannel = channel;
    cNode->next = NULL;
    return cNode;
}

DEVICE* createDevice(int minor){
    DEVICE* device;
    printk("Creating new device with minor number %d\n", minor);
    device = (DEVICE*) kmalloc(sizeof(DEVICE), GFP_KERNEL);
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
    DEVICE_NODE* dNode;
    printk("Creating new node associated with device with minor  %d\n", device->minor);
    dNode = (DEVICE_NODE*) kmalloc(sizeof(DEVICE_NODE), GFP_KERNEL);
    if (dNode == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Device Node\n");
    dNode->device = device;
    dNode->next = NULL;
    return dNode;
}


CHANNEL_LINKED_LIST* cretaeEmptyChannelsList(){
    CHANNEL_LINKED_LIST* cList;
    printk("Creating new empty channels list\n");
    cList = (CHANNEL_LINKED_LIST* ) kmalloc(sizeof(CHANNEL_LINKED_LIST), GFP_KERNEL);
    if (cList == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Channels List\n");
    cList->head = NULL;
    return cList;
}

DEVICE_LINKED_LIST* createEmptyDeviceList(){
    DEVICE_LINKED_LIST* dList;
    printk("Creating new empty devices list\n");
    dList = (DEVICE_LINKED_LIST* ) kmalloc(sizeof(DEVICE_LINKED_LIST), GFP_KERNEL);
    if (dList == NULL){
        return NULL;
    }

    printk("Allocation Successfully for Devices List\n");
    dList->head = NULL;
    return dList;
}

void destroyChannel(CHANNEL* channel){
    if (channel != NULL) {
        printk("Destroying channel with id %d\n", (int) channel->channelId);
        kfree(channel);
    }
}

void destroyChannelNode(CHANNEL_NODE* cNode){
    if (cNode != NULL){
        printk("Destroying channel node associated with channel id %d\n", (int) cNode->dataChannel->channelId);
        destroyChannel(cNode->dataChannel);
        kfree(cNode);
    }
}

void destroyChannelLinkedList(CHANNEL_LINKED_LIST* cList){
    CHANNEL_NODE* currentNode;
    CHANNEL_NODE* tmp;
    if (cList != NULL){
        printk("Destroying channels List\n");
        currentNode = cList->head;
        while (currentNode != NULL){
            tmp = currentNode;
            currentNode = currentNode->next;
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
    DEVICE_NODE* currentNode;
    DEVICE_NODE* tmp;
    if (dList != NULL){
        printk("Destroying devices List\n");
        currentNode = dList->head;
        while (currentNode != NULL){
            tmp = currentNode;
            currentNode = currentNode->next;
            destroyDeviceNode(tmp);
        }
    }
}

int addChannel(CHANNEL_LINKED_LIST* cList, unsigned long channelId){
    CHANNEL* channel;
    CHANNEL_NODE* newChannelNode;
    CHANNEL_NODE* currentNode;
    printk("Adding new channel with id %d to channel linked list\n", (int) channelId);
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
    printk("Adding new device with minor number %d to device linked list\n", minor);
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
    DEVICE* device;
    DEVICE_NODE* currentNode;
    device = NULL;
    currentNode = dList->head;
    printk("Searching for device with minor %d\n", minor);

    while (currentNode != NULL){
        if (currentNode->device->minor == minor){
            printk("Found the searched device\n");
            device = currentNode->device;
            break;
        }
        currentNode = currentNode->next;
    }
    return device;
}

CHANNEL* findChannelInDevice(DEVICE* device, unsigned long channelId){
    CHANNEL* channel;
    CHANNEL_NODE* currentNode;
    channel = NULL;
    printk("Searching for channel with id %d in device %d\n", (int) channelId, device->minor);

    // The device doesn't have a channels linked list even yet.
    if (device->channels == NULL){
        printk("The given device doesn't have any known channels at all yet\n");
        return channel;
    }

    // The channels linked list exists, need to search for the desired channel id
    currentNode = device->channels->head;

    while (currentNode != NULL){
        if (currentNode->dataChannel->channelId == channelId){
            printk("Found the searched channel\n");
            channel = currentNode->dataChannel;
            break;
        }
        currentNode = currentNode->next;
    }
    return channel;
}


// Write a message to a channel, return num of bytes written or -1 on error
int writeMessageToChannel(CHANNEL* channel, const char* message, int messageLength){
    int i;
    printk("Inside the write message to channel helper function\n");
    if (messageLength > BUF_LEN){
        printk("Writing failed. Message length is greater than channel buffer size\n");
        return -1;
    }
    for (i=0; i < messageLength; i++){
        get_user(channel->channelBuffer[i], &message[i]);
    }

    //update the channel and return number of bytes written to channel
    channel->messageExists = 1;
    channel->currentMessageLength = i;
    printk("Wrote to channel %ld a message with length %d\n", channel->channelId, i);
    return i;
}

// Read a message from a channel, return num of bytes read from channel or -1 on error
int readMessageFromChannel(CHANNEL* channel, char* userBuffer, int bufferLength){
    int currentMsgLength;
    int i;
    printk("Inside the read message from channel helper function\n");
    if (channel->messageExists == 0){ //there isn't a message on this channel
        printk("Read failed. the channel exist but there isn't a message yet on the channel\n");
        return -1;
    }
    currentMsgLength = channel->currentMessageLength;
    if (bufferLength < currentMsgLength){ // user space buffer is too small for the message in channel
        printk("Read failed. the provided buffer is too small to hold the message in the channel\n");
        return -2;
    }

    for (i=0; i < currentMsgLength; i++){
        put_user(channel->channelBuffer[i], &userBuffer[i]);
    }

    // return number of bytes read
    printk("Read from channel %ld a message with length %d\n", channel->channelId, i);
    return i;
}
