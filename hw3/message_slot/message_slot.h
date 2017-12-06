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
#define MAX_CHANNELS_FOR_DEVICE 4
#define MAX_DEVICES_FOR_DRIVER 4

// struct to represent communication channel
typedef struct channel{
    unsigned long channelId;
    int messageExists;
    int currentMessageLength;
    char channelBuffer[BUF_LEN];
} CHANNEL;

// struct to represent a specific device - identified by it's minor number
typedef struct channel_device{
    int minor;
    int isOpen;
    CHANNEL* channels[MAX_CHANNELS_FOR_DEVICE];
} CHANNEL_DEVICE;

CHANNEL_DEVICE* getExistingDeviceFromMinor(int minor, int* index);
CHANNEL* getChannelFromDevice(CHANNEL_DEVICE* device, unsigned long channelId);
int findAvailableDeviceIndex(void);
int findAvialableChannelIndex(CHANNEL_DEVICE* device);
int write_message_to_channel(CHANNEL* channel, const char* message, int messageLength);
int read_message_from_channel(CHANNEL* channel, char* userBuffer, int bufferLength);


#endif //OS_COURSE_MESSAGE_SLOT_H
