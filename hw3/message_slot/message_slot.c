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
    minor = iminor(inode);
    device = findDeviceFromMinor(list, minor);
    if (device != NULL) {
        //device is already allocated, just need to open it
        device->isOpen = 1;
        currentHandledDevice = minor;
    }
    else {
        status = addDevice(list, minor);
        if (status < 0 ) {
            return -ENOMEM;
        }

        // New device created is already marked as open, but do need to set current handled device
        currentHandledDevice = minor;
    }
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file) {
    int minor;
    DEVICE *device;
    minor = iminor(inode);
    device = findDeviceFromMinor(list, minor);
    if (device == NULL) {
        // trying to close device that doesn't exist. don't do anything

    } else {
        if (device->isOpen == 0) {
            // again don't do anything the device is already closed
        } else {
            // we need to indicate the device is closed - the device is registered
            device->isOpen = 0;
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

    channelId = (unsigned long) file->private_data;

    // check for errors
    currentDevice = findDeviceFromMinor(list, currentHandledDevice);
    if (currentDevice == NULL || currentDevice->isOpen == 0){
        return -EINVAL;
    }

    currentChannel = findChannelInDevice(currentDevice, channelId);
    if (currentChannel == NULL){
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

    channelId = (unsigned long) file->private_data;

    // check for errors
    currentDevice = findDeviceFromMinor(list, currentHandledDevice);
    if (currentDevice == NULL || currentDevice->isOpen == 0){
        return -EINVAL;
    }

    currentChannel = findChannelInDevice(currentDevice, channelId);
    if (currentChannel == NULL){
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
    if( MSG_SLOT_CHANNEL == ioctl_command_id )
    {
        // Get the parameter given to ioctl by the process
        file->private_data = (void *) ioctl_param;


        // Verify that the channel exists. Crate a new channel if it doesn't

        currentDevice = findDeviceFromMinor(list, currentHandledDevice);
        if (currentDevice == NULL){
            return -EINVAL;
        }

        currentChannel = findChannelInDevice(currentDevice, ioctl_param);
        if (currentChannel != NULL){
        }
        else{

            if (currentDevice->channels == NULL){ // no channels list at all for device
                cList = cretaeEmptyChannelsList();
                if (cList == NULL){
                    return -ENOMEM;
                }
                currentDevice->channels = cList;
            }

            status = addChannel(currentDevice->channels, ioctl_param);
            if (status < 0){
                return -ENOMEM;
            }
        }
    }


    else{
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
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//==================== Helper Function =============================

// Implementing helper function to read, write, search and allocate memory


CHANNEL* createChannel(unsigned long channelId){
    CHANNEL* channel;
    channel = (CHANNEL* ) kmalloc(sizeof(CHANNEL), GFP_KERNEL);
    if (channel == NULL){
        return NULL;
    }

    channel->channelId = channelId;
    channel->messageExists = 0;
    channel->currentMessageLength = 0;
    return channel;
}

CHANNEL_NODE* createChannelNode(CHANNEL* channel){
    CHANNEL_NODE* cNode;
    cNode = (CHANNEL_NODE*) kmalloc(sizeof(CHANNEL_NODE), GFP_KERNEL);
    if (cNode == NULL){
        return NULL;
    }

    cNode->dataChannel = channel;
    cNode->next = NULL;
    return cNode;
}

DEVICE* createDevice(int minor){
    DEVICE* device;
    device = (DEVICE*) kmalloc(sizeof(DEVICE), GFP_KERNEL);
    if (device == NULL){
        return NULL;
    }

    device->minor = minor;
    device->isOpen = 1;
    device->channels = NULL;
    return device;
}

DEVICE_NODE* createDeviceNode(DEVICE* device){
    DEVICE_NODE* dNode;
    dNode = (DEVICE_NODE*) kmalloc(sizeof(DEVICE_NODE), GFP_KERNEL);
    if (dNode == NULL){
        return NULL;
    }

    dNode->device = device;
    dNode->next = NULL;
    return dNode;
}


CHANNEL_LINKED_LIST* cretaeEmptyChannelsList(){
    CHANNEL_LINKED_LIST* cList;
    cList = (CHANNEL_LINKED_LIST* ) kmalloc(sizeof(CHANNEL_LINKED_LIST), GFP_KERNEL);
    if (cList == NULL){
        return NULL;
    }

    cList->head = NULL;
    return cList;
}

DEVICE_LINKED_LIST* createEmptyDeviceList(){
    DEVICE_LINKED_LIST* dList;
    dList = (DEVICE_LINKED_LIST* ) kmalloc(sizeof(DEVICE_LINKED_LIST), GFP_KERNEL);
    if (dList == NULL){
        return NULL;
    }

    dList->head = NULL;
    return dList;
}

void destroyChannel(CHANNEL* channel){
    if (channel != NULL) {
        kfree(channel);
    }
}

void destroyChannelNode(CHANNEL_NODE* cNode){
    if (cNode != NULL){
        destroyChannel(cNode->dataChannel);
        kfree(cNode);
    }
}

void destroyChannelLinkedList(CHANNEL_LINKED_LIST* cList){
    CHANNEL_NODE* currentNode;
    CHANNEL_NODE* tmp;
    if (cList != NULL){
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
        destroyChannelLinkedList(device->channels);
        kfree(device);
    }
}

void destroyDeviceNode(DEVICE_NODE* dNode){
    if (dNode != NULL){
        destroyDevice(dNode->device);
        kfree(dNode);
    }
}

void destroyDeviceLinkedList(DEVICE_LINKED_LIST* dList){
    DEVICE_NODE* currentNode;
    DEVICE_NODE* tmp;
    if (dList != NULL){
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
    channel = createChannel(channelId);

    if (channel == NULL){
        return -1;
    }
    newChannelNode = createChannelNode(channel);
    if (newChannelNode == NULL){
        return -1;
    }

    currentNode = cList->head;
    if (currentNode == NULL){
        cList->head = newChannelNode;
    }
    else{
        while (currentNode->next != NULL){
            currentNode = currentNode->next;
        }
        currentNode->next = newChannelNode;
    }
    return 0;
}

int addDevice(DEVICE_LINKED_LIST* dList, int minor){
    DEVICE* device;
    DEVICE_NODE* newDeviceNode;
    DEVICE_NODE* currentNode;
    device = createDevice(minor);

    if (device == NULL){
        return -1;
    }

    newDeviceNode = createDeviceNode(device);
    if (newDeviceNode == NULL){
        return -1;
    }

    currentNode = dList->head;
    if (currentNode == NULL){
        dList->head = newDeviceNode;
    }
    else{
        while (currentNode->next != NULL){
            currentNode = currentNode->next;
        }
        currentNode->next = newDeviceNode;
    }
    return 0;
}

DEVICE* findDeviceFromMinor(DEVICE_LINKED_LIST* dList, int minor){
    DEVICE* device;
    DEVICE_NODE* currentNode;
    device = NULL;
    currentNode = dList->head;

    while (currentNode != NULL){
        if (currentNode->device->minor == minor){
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

    // The device doesn't have a channels linked list even yet.
    if (device->channels == NULL){
        return channel;
    }

    // The channels linked list exists, need to search for the desired channel id
    currentNode = device->channels->head;

    while (currentNode != NULL){
        if (currentNode->dataChannel->channelId == channelId){
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
    int bytesWrote;
    if (messageLength > BUF_LEN){
        return -1;
    }
    for (i=0; i < messageLength; i++){
        get_user(channel->channelBuffer[i], &message[i]);
    }

    //update the channel and return number of bytes written to channel
    bytesWrote = i;
    channel->messageExists = 1;
    channel->currentMessageLength = bytesWrote;
    return bytesWrote;
}

// Read a message from a channel, return num of bytes read from channel or -1 on error
int readMessageFromChannel(CHANNEL* channel, char* userBuffer, int bufferLength){
    int currentMsgLength;
    int i;
    int bytesRead;
    if (channel->messageExists == 0){ //there isn't a message on this channel
        return -1;
    }
    currentMsgLength = channel->currentMessageLength;
    if (bufferLength < currentMsgLength){ // user space buffer is too small for the message in channel
        return -2;
    }

    for (i=0; i < currentMsgLength; i++){
        put_user(channel->channelBuffer[i], &userBuffer[i]);
    }

    // return number of bytes read
    bytesRead = i;
    return bytesRead;
}
