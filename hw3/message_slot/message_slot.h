//
// Created by okleinfeld on 12/6/17.
//

#ifndef OS_COURSE_MESSAGE_SLOT_H
#define OS_COURSE_MESSAGE_SLOT_H


#include <linux/ioctl.h>

// The major device number.
// We don't rely on dynamic registration
// any more. We want ioctls to know this
// number at compile time.
#define MAJOR_NUM 244

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot_char_dev"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_slot_dev"
#define SUCCESS 0

// struct to represent communication channel
typedef struct channel{
    unsigned long channelId;
    int messageExists;
    int currentMessageLength;
    char channelBuffer[BUF_LEN];
} CHANNEL;

// Channel Node object of a Linked List
typedef struct channel_node{
    CHANNEL* dataChannel;
    struct channel_node* next;
} CHANNEL_NODE;

// Channels Linked List object
typedef struct channel_linked_list{
    CHANNEL_NODE* head;
} CHANNEL_LINKED_LIST;

// struct to represent a specific device - identified by it's minor number
typedef struct channel_device{
    int minor;
    int isOpen;
    CHANNEL_LINKED_LIST* channels;
} DEVICE;

// Device Node object of a Linked List
typedef struct device_node{
    DEVICE* device;
    struct device_node* next;
} DEVICE_NODE;

// Devices Linked List object
typedef struct device_linked_list{
    DEVICE_NODE* head;
} DEVICE_LINKED_LIST;

CHANNEL* createChannel(unsigned long channelId);
CHANNEL_NODE* createChannelNode(CHANNEL* channel);
DEVICE* createDevice(int minor);
DEVICE_NODE* createDeviceNode(DEVICE* device);
CHANNEL_LINKED_LIST* cretaeEmptyChannelsList(void);
DEVICE_LINKED_LIST* createEmptyDeviceList(void);

void destroyChannel(CHANNEL*);
void destroyChannelNode(CHANNEL_NODE*);
void destroyChannelLinkedList(CHANNEL_LINKED_LIST*);
void destroyDevice(DEVICE*);
void destroyDeviceNode(DEVICE_NODE*);
void destroyDeviceLinkedList(DEVICE_LINKED_LIST*);

int addChannel(CHANNEL_LINKED_LIST* cList, unsigned long channelId);
int addDevice(DEVICE_LINKED_LIST* dList, int minor);
DEVICE* findDeviceFromMinor(DEVICE_LINKED_LIST* dList, int minor);
CHANNEL* findChannelInDevice(DEVICE* device, unsigned long channelId);

//int write_message_to_channel(CHANNEL* channel, const char* message, int messageLength);
//int read_message_from_channel(CHANNEL* channel, char* userBuffer, int bufferLength);


#endif //OS_COURSE_MESSAGE_SLOT_H
