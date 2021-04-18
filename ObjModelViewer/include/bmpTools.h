#ifndef __BMP_TOOLS_H__
#define __BMP_TOOLS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>
#include "vulkanCmds.h"

#define BMP_WIDTH_OFFSET            18
#define BMP_HEIGHT_OFFSET           22
#define BMP_PIX_PER_COLOR           3
#define BMP_HEADER_SIZE             54


VkResult getBmpSize(char *path, VkExtent2D *size);
void loadBmpToBuffer(char *path, VkExtent2D *size, unsigned char **ptr);

#endif
