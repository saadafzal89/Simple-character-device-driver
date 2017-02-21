#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

#define ASP_MAGIC_NUM 'Z'
#define ASP_CHGACCDIR  _IOWR(ASP_MAGIC_NUM, 1,int)

#endif