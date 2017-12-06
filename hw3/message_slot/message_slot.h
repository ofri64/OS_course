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

#endif //OS_COURSE_MESSAGE_SLOT_H
