#include "vulkanCmds.h"

extern const char *EnabledLayers[];
extern const char *EnabledInstanceExtensions[];
extern const char *EnabledDeviceExtensions[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const char* EnabledLayers[] =
{
    "VK_LAYER_LUNARG_standard_validation",
};

const char* EnabledInstanceExtensions[] =
{
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
};


VkResult initPlatformSurface(VulkanObject* vulkanObj)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = 
    { 
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = NULL,
        .hwnd = NULL,
    };

    SDL_Window* window = SDL_CreateWindow(
        GLOBAL_APP_NAME_W,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        vulkanObj->windowSize.width, vulkanObj->windowSize.height,
        0
    );

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(window, &info);

    surfaceCreateInfo.hinstance = GetModuleHandle(0);
    surfaceCreateInfo.hwnd = info.info.win.window;

    VkResult result = vkCreateWin32SurfaceKHR(vulkanObj->instance, &surfaceCreateInfo, NULL, &vulkanObj->surface);

    return result;
}


VkShaderModule createShaderModule(VkDevice device, const char* shaderFile)
{
    VkShaderModule shaderModule = VK_NULL_HANDLE;

    HANDLE hFile = CreateFile(shaderFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return VK_NULL_HANDLE;

    LARGE_INTEGER size;
    GetFileSizeEx(hFile, &size);

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hFile);
    if (!hMapping) return VK_NULL_HANDLE;

    const uint32_t* data = (const uint32_t*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size.LowPart,
        .pCode = data,
        .flags = 0,
        .pNext = NULL,
    };
    vkCreateShaderModule(device, &shaderModuleCreateInfo, 0, &shaderModule);

    if (data != NULL)
    {
        UnmapViewOfFile(data);
    }

    CloseHandle(hMapping);

    return shaderModule;
}

VkResult initGraphicsPipeline(VkDevice device, VkGraphicsPipelineCreateInfo *createInfo, VkPipeline *pipeline)
{
    VkResult result;

    VkShaderModule vertexShader = createShaderModule(device, "shaders\\modelobjviewer.vert.spv");
    VkShaderModule fragmentShader = createShaderModule(device, "shaders\\modelobjviewer.frag.spv");

    const VkPipelineShaderStageCreateInfo stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShader,
            .pName = "main",
        },
    };

    /* Set the stages structures and count */
    createInfo->pStages = stages;
    createInfo->stageCount = 2;

    /*Create a graphics pipeline object */
    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, createInfo, NULL, pipeline);

    if (result != VK_SUCCESS)
    {
        printf("Creating graphics pipeline");
    }

    /*P RETURN the results of creating the graphics pipeline */
    return result;
}


VkResult initDriver(VulkanObject *vulkanObj)
{
    uint32_t count = 10;
    VkPhysicalDevice deviceArray[10];
    VkPhysicalDeviceProperties properties = { 0 };

    VkApplicationInfo appInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "GLOBAL_APP_NAME",
        .applicationVersion = 0,
        .pEngineName = NULL,
        .engineVersion = 0,
        .apiVersion = VK_MAKE_VERSION(1,0,0)
    };

    VkInstanceCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = (EnabledLayers[0] == NULL) ? 0 : 1,
        .ppEnabledLayerNames = EnabledLayers,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = EnabledInstanceExtensions
    };

    /* Get Vulkan Instance */
    VkResult result = vkCreateInstance(&createInfo, NULL, &vulkanObj->instance);
    if(VK_SUCCESS != result)
    {
        printf("Failed to create instance\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    /* Get physical deivces */
    result = vkEnumeratePhysicalDevices(vulkanObj->instance, &count, deviceArray);
    if(VK_SUCCESS != result)
    {
        printf("Failed to get physical devices\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    printf("Found %d physical devices\n", count);

    /* Get physical device properties */
    for(uint32_t i=0; i<count; ++i)
    {
        vkGetPhysicalDeviceProperties(deviceArray[0], &properties);
        printf("Physical Device [%d]: %s\n", i, properties.deviceName);
    }

    vulkanObj->physicalDevice = deviceArray[0];

    /* Determine which queues to use */
    count = 10;
    VkQueueFamilyProperties propertiesArray[10];
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanObj->physicalDevice, &count, propertiesArray);

    /* Find queue family index that can support graphics */
    for(uint32_t i=0; i< count; ++i)
    {
        if(propertiesArray[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            vulkanObj->queueIndex = i;
            printf("Found queue family index %d\n", vulkanObj->queueIndex);
            break;
        }
    }

    VkPhysicalDeviceFeatures pdfeatures;
    vkGetPhysicalDeviceFeatures(vulkanObj->physicalDevice, &pdfeatures);

    float priorities[1] = {1.0};
    VkDeviceQueueCreateInfo dqci[] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = vulkanObj->queueIndex,
            .queueCount = 1,
            .pQueuePriorities = priorities,
        }
    };

    VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = dqci,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = sizeof(EnabledDeviceExtensions)/sizeof(EnabledDeviceExtensions[0]),
        .ppEnabledExtensionNames = EnabledDeviceExtensions,
        .pEnabledFeatures = &pdfeatures
    };

    /* Create device */
    result = vkCreateDevice(vulkanObj->physicalDevice, &dci, NULL, &vulkanObj->device);
    if (VK_SUCCESS != result)
    {
        printf("Could not create device\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    /* Create command pools for each queue and thread */
    VkCommandPoolCreateInfo cpci = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        NULL,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        vulkanObj->queueIndex,
    };

    /* Create command buffer for initialization usage */
    result = vkCreateCommandPool(vulkanObj->device, &cpci, NULL, &vulkanObj->cmdPool);
    if(VK_SUCCESS != result)
    {
        printf("Could not create command pool\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    /* Create command buffers */
    VkCommandBufferAllocateInfo cbai = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        NULL,
        vulkanObj->cmdPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };

    /* Allocate command buffers */
    result = vkAllocateCommandBuffers(vulkanObj->device, &cbai, &vulkanObj->cmdBuffer);
    if(VK_SUCCESS != result)
    {
        printf("Could not allocate command buffer\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    else
    {
        vkGetDeviceQueue(vulkanObj->device, vulkanObj->queueIndex, 0, &vulkanObj->queue);
        printf("The instance has been created\n");
    }

    return VK_SUCCESS;
}


VkResult initRenderPass(VulkanObject *vulkanObj)
{
    VkAttachmentDescription attachments[] =
    {
        {
            0,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
            0,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };

    VkAttachmentReference amr[2] =
    {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        }
    };

    VkSubpassDescription subpass[] =
    {
        {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = NULL,
            .colorAttachmentCount = 1,
            .pColorAttachments = &amr[0],
            .pResolveAttachments = NULL,
            .pDepthStencilAttachment = &amr[1],
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = NULL,
        }
    };

    VkRenderPassCreateInfo rpci =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = subpass,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };

    /* Create render pass */
    VkResult result = vkCreateRenderPass(vulkanObj->device, &rpci, NULL, &vulkanObj->renderPass);
    if (VK_SUCCESS != result)
    {
        printf("Failed to create render pass\n");
    }
    return VK_SUCCESS;
}


int32_t memoryTypeIndex(VulkanObject *vulkanObj, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties)
{
    static  VkPhysicalDeviceMemoryProperties g_memoryProperties = {0};
    if(0 == g_memoryProperties.memoryHeapCount)
    {
        vkGetPhysicalDeviceMemoryProperties(vulkanObj->physicalDevice, &g_memoryProperties);
    }

    /* Find a memory in `memoryTypeBitsRequirement` that includes all of `requiredProperties` */
    const uint32_t memoryCount = g_memoryProperties.memoryTypeCount;
    for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex) 
    {
        const uint32_t memoryTypeBits = (1 << memoryIndex);
        const uint32_t isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

        const VkMemoryPropertyFlags properties = g_memoryProperties.memoryTypes[memoryIndex].propertyFlags;
        const uint32_t hasRequiredProperties = (properties & requiredProperties) == requiredProperties;

        if (isRequiredMemoryType && hasRequiredProperties)
        {
            return (int32_t)(memoryIndex);
        }
    }

    return -1;
}


VkResult initImages(VulkanObject *vulkanObj)
{
    uint32_t qfi[1] =
    {
        vulkanObj->queueIndex
    };

    VkImageCreateInfo ici =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {vulkanObj->windowSize.width, vulkanObj->windowSize.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = qfi,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };


    /* Create color buffer image */
    VkResult result = vkCreateImage(vulkanObj->device, &ici, NULL, &vulkanObj->colorBuffer);
    if (VK_SUCCESS != result)
    {
        printf("Failed to create color buffer image\n");
    }
    else
    {
        /* Create depth/stencil image */
        ici.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
        ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        result = vkCreateImage(vulkanObj->device, &ici, NULL, &vulkanObj->depthBuffer);
        if(VK_SUCCESS != result)
        {
            printf("Failed to create depth buffer image\n");
        }
        else
        {
            /* Determine how much memory is needed for both images */
            VkMemoryRequirements memRequirements[2];
            VkMemoryAllocateInfo memAllocInfo =
            {
                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, 0, 0
            };

            vkGetImageMemoryRequirements(vulkanObj->device, vulkanObj->colorBuffer, &memRequirements[0]);
            vkGetImageMemoryRequirements(vulkanObj->device, vulkanObj->depthBuffer, &memRequirements[1]);

            for(uint32_t i=0; i<2; ++i)
            {
                /* Sum the sizes and add any alignment needed of the second image to the size of the first image */
                memAllocInfo.allocationSize = memRequirements[i].size;
                memAllocInfo.memoryTypeIndex =
                        (uint32_t)memoryTypeIndex(vulkanObj, memRequirements[i].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                if((int32_t)memAllocInfo.memoryTypeIndex == -1)
                {
                    memAllocInfo.memoryTypeIndex =
                            (uint32_t)memoryTypeIndex(vulkanObj, memRequirements[i].memoryTypeBits, 0);
                }

                /* Allocate memory for the images */
                result = vkAllocateMemory(vulkanObj->device, &memAllocInfo, NULL, &vulkanObj->imageMemory[i]);
                if(VK_SUCCESS != result)
                {
                    printf("Failed to allocate memory for framebuffer images\n");
                    return VK_ERROR_INITIALIZATION_FAILED;
                }
            }

            /* Bind memory to the depth/stencil image */
            result = vkBindImageMemory(vulkanObj->device, vulkanObj->colorBuffer, vulkanObj->imageMemory[0], 0);
            if(VK_SUCCESS != result)
            {
                printf("Failed to bind memory for color buffer image\n");
            }
            else
            {
                /* Bind memory to the color buffer image */
                result = vkBindImageMemory(vulkanObj->device, vulkanObj->depthBuffer, vulkanObj->imageMemory[1], 0);
                if(VK_SUCCESS != result)
                {
                    printf("Failed to bind memory for depth buffer image\n");
                }
                else
                {
                    /* Create the image view for the color buffer */
                    VkImageViewCreateInfo ivci =
                    {
                        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        NULL,
                        0,
                        vulkanObj->colorBuffer,
                        VK_IMAGE_VIEW_TYPE_2D,
                        VK_FORMAT_R8G8B8A8_UNORM,
                        {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
                        {
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            0,
                            1,
                            0,
                            1,
                        }
                    };

                    result = vkCreateImageView(vulkanObj->device, &ivci, NULL, &vulkanObj->imageViews[0]);
                    if(VK_SUCCESS != result)
                    {
                        printf("Failed to create view for color buffer\n");
                    } else
                    {
                        /* Create the image view for the depth buffer */
                        ivci.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                        ivci.image = vulkanObj->depthBuffer;
                        result = vkCreateImageView(vulkanObj->device, &ivci, NULL, &vulkanObj->imageViews[1]);
                        if(VK_SUCCESS != result)
                        {
                            printf("Failed to create image view for depth buffer\n");
                        }
                    }
                }
            }
        }
    }
    return result;
}


void transitionImage(VulkanObject *vulkanObj)
{
    VkImageMemoryBarrier barriers[] =
    {
        /* Transition color buffer to color attachment */
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 
            .pNext = NULL,
            .srcAccessMask = 0, 
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = vulkanObj->colorBuffer,
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        },
        /* Transition depth buffer to depth attachement */
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 
            .pNext = NULL,
            .srcAccessMask = 0, 
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = 0, 
            .dstQueueFamilyIndex = 0,
            .image = vulkanObj->depthBuffer,
            .subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 }
        },
        /* Transition surface image 0 to transfer source */
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 
            .pNext = NULL,
            .srcAccessMask = 0, 
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = 0, 
            .dstQueueFamilyIndex = 0,
            .image = vulkanObj->displayBuffers[0],
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        },
        /* Transition surface image 1 to transfer source */
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 
            .pNext = NULL,
            .srcAccessMask = 0, 
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = 0, 
            .dstQueueFamilyIndex = 0,
            .image = vulkanObj->displayBuffers[1],
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        },
    };

    /* Create pipeline barrier */
    vkCmdPipelineBarrier(vulkanObj->cmdBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                         VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT |
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         sizeof(barriers)/sizeof(barriers[0]), barriers);
}


VkResult initFrameBuffer(VulkanObject *vulkanObj)
{
    VkFramebufferCreateInfo fbci =
    {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderPass = vulkanObj->renderPass,
        .attachmentCount = 2,
        .pAttachments = vulkanObj->imageViews,
        .width = vulkanObj->windowSize.width, 
        .height = vulkanObj->windowSize.height, 
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(vulkanObj->device, &fbci, NULL, &vulkanObj->framebuffer);
    if(VK_SUCCESS != result)
    {
        printf("Failed to create framebuffer\n");
    }

    return result;
}


VkResult initSwapChain(VulkanObject *vulkanObj)
{
    /* Get device surface format */
    uint32_t count = 10;
    VkSurfaceFormatKHR surFormat[10];
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanObj->physicalDevice, vulkanObj->surface, &count, surFormat);
    if(VK_SUCCESS != result && VK_INCOMPLETE != result)
    {
        printf("Failed to get physical device surface formats: %d\n", result);
    }
    else
    {
        /* Get device surface support */
        VkBool32 isSupported;
        result = vkGetPhysicalDeviceSurfaceSupportKHR(vulkanObj->physicalDevice, vulkanObj->queueIndex, vulkanObj->surface, &isSupported);
        if(VK_SUCCESS != result)
        {
            printf("Failed to get physical device surface support: %d\n", result);
        }
        else if (!isSupported)
        {
            printf("Physical device doesn't support presentation\n");
        }
        else
        {
            /* Get device surface capabilities */
            VkSurfaceCapabilitiesKHR surCap;
            result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanObj->physicalDevice, vulkanObj->surface, &surCap);
            if(VK_SUCCESS != result)
            {
                printf("Failed to get physical device surface capabilities: %d\n", result);
            }
            else
            {
                /* Create swap chain */
                VkSwapchainCreateInfoKHR sci =
                {
                    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                    .pNext = NULL,
                    .flags = 0,
                    .surface = vulkanObj->surface,
                    .minImageCount = NUM_SWAP_CHAIN_IMAGES,
                    .imageFormat = surFormat[0].format,
                    .imageColorSpace = surFormat[0].colorSpace,
                    .imageExtent = {0,0},
                    .imageArrayLayers = 1,
                    .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = 1,
                    .pQueueFamilyIndices = &vulkanObj->queueIndex,
                    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
                    .clipped = VK_TRUE,
                    .oldSwapchain = NULL
                };

                sci.imageExtent = surCap.currentExtent;
                vulkanObj->displayFormat = surFormat[0].format;
                vulkanObj->displaySize = surCap.currentExtent;

                /* Get swap chain images (NumDisplayBuffers) */
                result = vkCreateSwapchainKHR(vulkanObj->device, &sci, NULL, &vulkanObj->swapChain);
                if(VK_SUCCESS != result)
                {
                    printf("Failed to create swapchain\n");
                }
                else
                {
                    /* Get swapchain images (NumDispBuffers) */
                    result = vkGetSwapchainImagesKHR(vulkanObj->device, vulkanObj->swapChain, &vulkanObj->acquiredImages, vulkanObj->displayBuffers);
                    if(VK_SUCCESS != result)
                    {
                        printf("Failed to get swapchain images\n");
                    }
                }
            }
        }
    }
    return result;
}


buffer_t createBuffer(VulkanObject *vulkanObj, uint32_t size, uint32_t usageFlags, VkMemoryPropertyFlags memFlags, VkSharingMode sharing)
{
    buffer_t buffer;
    VkBufferCreateInfo bci =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = size,
        .usage = usageFlags,
        .sharingMode = sharing,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL
    };
    VkResult result = vkCreateBuffer(vulkanObj->device, &bci, NULL, &buffer.buffer);
    if(VK_SUCCESS != result)
    {
        printf("Failed to create buffer\n");
    }
    else
    {
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(vulkanObj->device, buffer.buffer, &memReq);
        VkMemoryAllocateInfo memAllocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 
            .memoryTypeIndex = 0, 
            .memoryTypeIndex =  0, 
            .allocationSize =  0
        };

        /* Sum the sizes, and add any alignment needed of the second buffer to the size of the first buffer */
        memAllocInfo.allocationSize = memReq.size;
        int32_t index = memoryTypeIndex(vulkanObj, memReq.memoryTypeBits, memFlags);
        if(-1 == index)
        {
            index = memoryTypeIndex(vulkanObj, memReq.memoryTypeBits, 0);
        }

        /* Allocate memory for the images */
        memAllocInfo.memoryTypeIndex = (uint32_t)index;        
        result = vkAllocateMemory(vulkanObj->device, &memAllocInfo, NULL, &buffer.memory);
        if(VK_SUCCESS != result)
        {
            printf("Failed to allocate memory for buffer\n");
        }
        else
        {
            /* Bind buffer memory */
            result = vkBindBufferMemory(vulkanObj->device, buffer.buffer, buffer.memory, 0);
            if(VK_SUCCESS != result)
            {
                printf("Failed to bind buffer\n");
            }
            else
            {
                /* Map memory */
                result = vkMapMemory(vulkanObj->device, buffer.memory, 0, memAllocInfo.allocationSize, 0, &buffer.ptr);
                if(VK_SUCCESS != result)
                {
                    printf("Unable to map buffer memory\n");
                }
            }
        }
    }

    return buffer;
}


void createSampler(VulkanObject *vulkanObj)
{
    VkSamplerCreateInfo sci =
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0,
        .maxLod = 0.25,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    };

    VkResult result = vkCreateSampler(vulkanObj->device, &sci, NULL, &vulkanObj->sampler);
    if(VK_SUCCESS != result)
    {
        printf("Unable to create sampler\n");
    }
    else
    {
        vulkanObj->samplerInfo.sampler = vulkanObj->sampler;
    }
}


void allocateDescriptorSet(VulkanObject *vulkanObj)
{
    VkDescriptorPoolSize dps[] =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 4
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 5
        },
        {
            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 5
        },
        {
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 5
        }
    };

    VkDescriptorPoolCreateInfo dpci =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 6,
        .poolSizeCount = sizeof (dps)/sizeof(dps[0]),
        .pPoolSizes = dps
    };

    VkResult result = vkCreateDescriptorPool(vulkanObj->device, &dpci, NULL, &vulkanObj->descriptorPool);
    if(VK_SUCCESS != result)
    {
        printf("Failed to create descriptor pool\n");
    }
    else
    {
        VkDescriptorSetAllocateInfo dsai =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = NULL,
            .descriptorPool = vulkanObj->descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &vulkanObj->dsl
        };
        result = vkAllocateDescriptorSets(vulkanObj->device, &dsai, &vulkanObj->descriptorSet);

        if(VK_SUCCESS != result)
        {
            printf("Failed to allocate descriptor sets\n");
        }
    }
    printf("Descriptor Set allocated\n");
}


void createImageDescriptorSet(VulkanObject *vulkanObj, texture_t textures[])
{
    uint32_t i;

    for(i=0; i<vulkanObj->numOfTextures ; i++)
    {
        vulkanObj->dii[i].sampler = NULL;
        vulkanObj->dii[i].imageView = textures[i].view;
        vulkanObj->dii[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    vulkanObj->wds[vulkanObj->descSetCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vulkanObj->wds[vulkanObj->descSetCount].pNext = NULL;
    vulkanObj->wds[vulkanObj->descSetCount].dstSet = vulkanObj->descriptorSet;
    vulkanObj->wds[vulkanObj->descSetCount].dstBinding = BINDING_FRAG_TEXTURES;
    vulkanObj->wds[vulkanObj->descSetCount].dstArrayElement = 0;
    vulkanObj->wds[vulkanObj->descSetCount].descriptorCount = vulkanObj->numOfTextures;
    vulkanObj->wds[vulkanObj->descSetCount].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    vulkanObj->wds[vulkanObj->descSetCount].pImageInfo = vulkanObj->dii;
    vulkanObj->wds[vulkanObj->descSetCount].pBufferInfo = NULL;
    vulkanObj->wds[vulkanObj->descSetCount].pTexelBufferView = NULL;

    vulkanObj->descSetCount++;

    vkUpdateDescriptorSets(vulkanObj->device, vulkanObj->descSetCount, vulkanObj->wds, 0, NULL);
}


void createUniformBufferDescriptorSet(VulkanObject *vulkanObj, uint32_t uniformStructSize)
{

    vulkanObj->dbi.buffer = vulkanObj->uniformBuffer.buffer;
    vulkanObj->dbi.offset = 0;
    vulkanObj->dbi.range = uniformStructSize;

    vulkanObj->wds[vulkanObj->descSetCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vulkanObj->wds[vulkanObj->descSetCount].pNext = NULL;
    vulkanObj->wds[vulkanObj->descSetCount].dstSet = vulkanObj->descriptorSet;
    vulkanObj->wds[vulkanObj->descSetCount].dstBinding = BINDING_FRAG_UNIFORM;
    vulkanObj->wds[vulkanObj->descSetCount].dstArrayElement = 0;
    vulkanObj->wds[vulkanObj->descSetCount].descriptorCount = 1;
    vulkanObj->wds[vulkanObj->descSetCount].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vulkanObj->wds[vulkanObj->descSetCount].pImageInfo = NULL;
    vulkanObj->wds[vulkanObj->descSetCount].pBufferInfo = &vulkanObj->dbi;
    vulkanObj->wds[vulkanObj->descSetCount].pTexelBufferView = NULL;

    vulkanObj->descSetCount++;

    vkUpdateDescriptorSets(vulkanObj->device, vulkanObj->descSetCount, vulkanObj->wds, 0, NULL);
}


void createTextureBufferDescriptorSet(VulkanObject *vulkanObj)
{

    vulkanObj->wds[vulkanObj->descSetCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vulkanObj->wds[vulkanObj->descSetCount].pNext = NULL;
    vulkanObj->wds[vulkanObj->descSetCount].dstSet = vulkanObj->descriptorSet;
    vulkanObj->wds[vulkanObj->descSetCount].dstBinding = BINDING_FRAG_SAMPLER;
    vulkanObj->wds[vulkanObj->descSetCount].dstArrayElement = 0;
    vulkanObj->wds[vulkanObj->descSetCount].descriptorCount = 1;
    vulkanObj->wds[vulkanObj->descSetCount].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    vulkanObj->wds[vulkanObj->descSetCount].pImageInfo = &vulkanObj->samplerInfo;
    vulkanObj->wds[vulkanObj->descSetCount].pBufferInfo = NULL;
    vulkanObj->wds[vulkanObj->descSetCount].pTexelBufferView = NULL;

    vulkanObj->descSetCount++;

    vkUpdateDescriptorSets(vulkanObj->device, vulkanObj->descSetCount, vulkanObj->wds, 0, NULL);
}


VkResult createFence(VulkanObject *vulkanObj, VkFenceCreateInfo *info, VkFence *outFence, uint32_t count)
{
    VkResult result;
    uint32_t i;

    VkFenceCreateInfo fenceInfo = {0};

    for(i=0;i<count;i++)
    {
        if ( info == NULL )
        {
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.pNext = NULL;
            fenceInfo.flags = 0;

            result = vkCreateFence(vulkanObj->device, &fenceInfo, NULL, outFence+i);
        }
        else
        {
            result = vkCreateFence(vulkanObj->device, info, NULL, outFence+i);
        }
    }
    return result;
}


void waitForFence(VulkanObject *vulkanObj, VkFence fence)
{
    if (fence)
    {
        if (vkGetFenceStatus(vulkanObj->device, fence) == VK_SUCCESS)
        {
            vkWaitForFences(vulkanObj->device, 1, &fence, VK_TRUE, 0);
        }
    }
}


void transitionImageLayout(VulkanObject *vulkanObj, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkFence fence;

    createFence(vulkanObj, NULL, &fence, 1);

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    //these are used to transfer queue ownership, which we aren't doing
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = { 0 };
    VkPipelineStageFlags destinationStage = { 0 };

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        /* Unsupported layout transition */
        printf("Attempting an unsupported image layout transition\n");
    }

    vkCmdPipelineBarrier(
        vulkanObj->cmdBuffer,
        sourceStage, destinationStage,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
}


texture_t createTextureImage(VulkanObject *vulkanObj, VkExtent2D *size, buffer_t *staging)
{
    texture_t texture;
    uint32_t qfi[1] =
    {
        0
    };

    VkImageCreateInfo ici =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {size->width, size->height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = qfi,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    /* Create texture image */
    VkResult result = vkCreateImage(vulkanObj->device, &ici, NULL, &texture.image);
    if(VK_SUCCESS != result)
    {
        printf("Failed to create color buffer image\n");
    }
    else
    {
        /* Determine how much memory is needed for the texture */
        VkMemoryRequirements memRequirements;
        VkMemoryAllocateInfo memAllocInfo =
        {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, 0, 0
        };
        int32_t index = -1;

        vkGetImageMemoryRequirements(vulkanObj->device, texture.image, &memRequirements);


        index = memoryTypeIndex(vulkanObj, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if(-1 == index){
            index = memoryTypeIndex(vulkanObj, memRequirements.memoryTypeBits, 0);
        }

        memAllocInfo.memoryTypeIndex = (uint32_t)index;
        memAllocInfo.allocationSize = memRequirements.size;

        /* Allocate memory for the images */
        result = vkAllocateMemory(vulkanObj->device, &memAllocInfo, NULL, &texture.memory);
        if(VK_SUCCESS != result)
        {
            printf("Failed to allocate memory for framebuffer images\n");
        }
        else
        {

            /*  Bind memory to the color buffer image */
            result = vkBindImageMemory(vulkanObj->device, texture.image, texture.memory, 0);
            if(VK_SUCCESS != result)
            {
                printf("Failed to bind memory for depth buffer image\n");
            }
            else
            {
                /* Transition image */
                transitionImageLayout(vulkanObj, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                /* Copy texture to buffer */
                VkBufferImageCopy bic = {
                    .bufferOffset = 0,
                    .bufferRowLength = size->width,
                    .bufferImageHeight = size->height,
                    .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                    .imageOffset = { 0, 0, 0 },
                    .imageExtent = {size->width, size->height, 1},

                };

                vkCmdCopyBufferToImage(vulkanObj->cmdBuffer, staging->buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bic);

                transitionImageLayout(vulkanObj, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                /* Create the image view for the color buffer */
                VkImageViewCreateInfo ivci =
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = NULL,
                    .flags = 0,
                    .image = texture.image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = VK_FORMAT_R8G8B8A8_UNORM,
                    .components = {VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_IDENTITY},
                    .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
                };
                result = vkCreateImageView(vulkanObj->device, &ivci, NULL, &texture.view);
                if(VK_SUCCESS != result)
                {
                    printf("Failed to create image view for color buffer\n");
                }
            }
        }
    }
    return texture;
}

void createPipelines(VulkanObject *vulkanObj)
{
    VkRenderPass renderPass = vulkanObj->renderPass;

    /* Define descriptor set layout */
    VkDescriptorSetLayoutBinding dslb[LAYOUT_BINDING_COUNT] =
    {
        {
            .binding = BINDING_FRAG_SAMPLER,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL,
        },
        {
            .binding = BINDING_FRAG_TEXTURES,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = vulkanObj->numOfTextures,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL,
        },
        {
            .binding = BINDING_FRAG_UNIFORM,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = NULL,
        }
    };

    VkDescriptorSetLayoutCreateInfo dslci =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = LAYOUT_BINDING_COUNT,
        .pBindings = dslb
    };

    VkResult result = vkCreateDescriptorSetLayout(vulkanObj->device, &dslci, NULL, &vulkanObj->dsl);
    if(VK_SUCCESS != result)
    {
        printf("Failed to create descriptor set layout\n");
    }
    else
    {
        VkPushConstantRange pushConst =
        {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(pushConstants_t)
        };

        /* Define the pipeline layout */
        VkPipelineLayoutCreateInfo plci =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &vulkanObj->dsl,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConst,
        };

        result = vkCreatePipelineLayout(vulkanObj->device, &plci, NULL, &vulkanObj->pll);
        if(VK_SUCCESS != result)
        {
            printf("Failed to create pipeline layout\n");
        }
        else
        {
            VkVertexInputBindingDescription vibd[] =
            {
                {
                    .binding = BINDING_VERT_POSITION,
                    .stride = sizeof(vertexData_t),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            };

            VkVertexInputAttributeDescription viads[] =
            {
                {
                    .location = LOCATION_VERT_POSITION,
                    .binding = BINDING_VERT_POSITION,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = 0,
                },
                {
                    .location = LOCATION_VERT_NORMALS,
                    .binding = BINDING_VERT_POSITION,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = 4 * sizeof(float)
                },
                {
                    .location = LOCATION_VERT_TEXCOORDS,
                    .binding = BINDING_VERT_POSITION,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = 8 * sizeof(float)
                }
            };
            
            VkPipelineVertexInputStateCreateInfo plvisci =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = vibd,
                .vertexAttributeDescriptionCount = INPUT_BINDING_COUNT,
                .pVertexAttributeDescriptions = viads
            };

            VkPipelineInputAssemblyStateCreateInfo pliasci =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE
            };

            VkViewport viewport =
            {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float)vulkanObj->windowSize.width,
                .height = (float)vulkanObj->windowSize.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            };

            VkRect2D rect =
            {
                .offset = {0,0},
                .extent = {vulkanObj->windowSize.width, vulkanObj->windowSize.height}
            };

            VkPipelineViewportStateCreateInfo plvpsci =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .viewportCount = 1,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &rect
            };

            VkPipelineRasterizationStateCreateInfo plrsci =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_NONE,
                .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                .depthBiasConstantFactor = 0,
                .depthBiasClamp = 0,
                .depthBiasSlopeFactor = 0,
                .lineWidth = 4
            };

            VkSampleMask sampMask =
            {
                0xFFFFFFFF
            };

            VkPipelineMultisampleStateCreateInfo plmssci =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = VK_FALSE,
                .minSampleShading = 0,
                .pSampleMask = &sampMask,
                .alphaToCoverageEnable = VK_FALSE,
                .alphaToOneEnable = VK_FALSE
            };

            VkPipelineDepthStencilStateCreateInfo pldssci =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .depthTestEnable = VK_TRUE,
                .depthWriteEnable = VK_TRUE,
                .depthCompareOp = VK_COMPARE_OP_GREATER,
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable = VK_FALSE,
                .front = { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
                  VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS,
                  0xFF, 0xFF, 0xFF
                },
                .back = { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
                  VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS,
                  0xFF, 0xFF, 0xFF
                },
                .minDepthBounds = 0,
                .maxDepthBounds = 1
            };

            VkPipelineColorBlendAttachmentState pcbas =
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR,
                .dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
            };

            VkPipelineColorBlendStateCreateInfo plcbsci =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_SET,
                .attachmentCount = 1,
                .pAttachments = &pcbas,
                .blendConstants = {0,0,0,0}
            };

            VkGraphicsPipelineCreateInfo gpci =
            {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .stageCount = 0,
                .pStages = NULL,
                .pVertexInputState = &plvisci,
                .pInputAssemblyState = &pliasci,
                .pTessellationState = NULL,
                .pViewportState = &plvpsci,
                .pRasterizationState = &plrsci,
                .pMultisampleState = &plmssci,
                .pDepthStencilState = &pldssci,
                .pColorBlendState = &plcbsci,
                .pDynamicState = NULL,
                .layout = vulkanObj->pll,
                .renderPass = renderPass,
                .subpass = 0,
                .basePipelineHandle = NULL,
                .basePipelineIndex = 0
            };

            /* Create the pipeline for texturing */
            plvisci.vertexAttributeDescriptionCount = 3;
            result = initGraphicsPipeline(vulkanObj->device, &gpci, &vulkanObj->texPipeline);
            if (VK_SUCCESS != result)
            {
                printf("Error creating texture pipeline %d\n", result);
            }
        }
    }
}


void draw(VulkanObject *vulkanObj, VkCommandBuffer cmdBuf, model_t model)
{
    uint32_t endFace;
    uint32_t faceCount;
    uint32_t i;
    pushConstants_t pc;

    /* Begin renderpass */
    static const VkClearValue clearVal[2] =
    {
        {{{0.1f, 0.1f, 0.1f, 1.0f}}},
        {{{0.0f, 0.0f, 0.0f, 0.0f}}}
    };

    VkRenderPassBeginInfo rpbi =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = vulkanObj->renderPass,
        .framebuffer = vulkanObj->framebuffer,
        .renderArea = {{0,0},{vulkanObj->windowSize.width, vulkanObj->windowSize.height}},
        .clearValueCount = 2, 
        .pClearValues = clearVal
    };

    /* Begin renderpass */
    vkCmdBeginRenderPass(cmdBuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    /* Bind buffer */
    VkDeviceSize offsets = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &vulkanObj->vertexBuffer.buffer, &offsets);

    /* Bind texture pipeline */
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanObj->texPipeline);

    /* Bind texture descriptor */
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanObj->pll, 0, 1, &vulkanObj->descriptorSet, 0, NULL);

    for(i=0;i<model.materialChangeCount;i++)
    {
        endFace = (i < model.materialChangeCount - 1) ? model.materialChange[i+1].startFace : model.numOfFaces;

        faceCount = (endFace - model.materialChange[i].startFace);

        /* Set up the push constants */
        memcpy(&pc.mp, &model.materialChange[i].material->mp, sizeof(pc.mp));
        memcpy(&pc.sp, &model.sp, sizeof(model.sp));

        /* Send the push constants to the shader */
        vkCmdPushConstants(cmdBuf, vulkanObj->pll, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants_t), &pc);

        /* Draw texture */
        vkCmdDraw(cmdBuf, faceCount*ELEMENTS_PER_FACE, 1, model.materialChange[i].startFace*ELEMENTS_PER_FACE, 0);
    }

    /* End renderpass */
    vkCmdEndRenderPass(cmdBuf);
}

void swapFrontBuffer(VulkanObject *vulkanObj, VkCommandBuffer cmdBuf, VkFence cmdBufFence)
{
    /* Get next image */
    vkResetFences(vulkanObj->device, 1, &vulkanObj->imageAcquiredFence);
    VkResult result = vkAcquireNextImageKHR(vulkanObj->device, vulkanObj->swapChain, MAX_TIMEOUT, NULL, vulkanObj->imageAcquiredFence, &vulkanObj->imageIndex);
    if(VK_SUCCESS != result)
    {
        printf("Failed to acquire next image\n");
    }
    else
    {
        vkWaitForFences(vulkanObj->device, 1, &vulkanObj->imageAcquiredFence, VK_TRUE, MAX_TIMEOUT);

        VkImageMemoryBarrier toTransfer[2] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 
                .pNext = NULL,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = 0,
                .dstQueueFamilyIndex = 0,
                .image = vulkanObj->colorBuffer,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            },
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = NULL,
                .srcAccessMask = VK_ACCESS_HOST_READ_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = 0,
                .dstQueueFamilyIndex = 0,
                .image = vulkanObj->displayBuffers[vulkanObj->imageIndex],
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            }
        };

        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                             0, NULL, 0, NULL, 2, toTransfer);

        /* Copy frame buffer to display */
        VkImageBlit imblit =
        {
            .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
            .srcOffsets = {{0,0,0}, {vulkanObj->windowSize.width, vulkanObj->windowSize.height, 1}},
            .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
            .dstOffsets = {{0,0,0},{vulkanObj->displaySize.width, vulkanObj->displaySize.height, 1}}
        };

        vkCmdBlitImage(cmdBuf, vulkanObj->colorBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vulkanObj->displayBuffers[vulkanObj->imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imblit, VK_FILTER_LINEAR);

        /* Transfer images to presentation and color attachment state */
        VkImageMemoryBarrier toPresent[2] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 
                .pNext = NULL,
                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = 0,
                .dstQueueFamilyIndex = 0,
                .image = vulkanObj->colorBuffer,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            },
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 
                .pNext = NULL,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = 0,
                .dstQueueFamilyIndex = 0,
                .image = vulkanObj->displayBuffers[vulkanObj->imageIndex],
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            }
        };

        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                             0, NULL, 0, NULL, 2, toPresent);

        /* End the command buffer */
        result = vkEndCommandBuffer(cmdBuf);
        if(VK_SUCCESS != result)
        {
            printf("Failed to end command buffer\n");
        }
        else
        {
            /* Submit the command buffer */
            VkSemaphore noSemaphores;
            VkPipelineStageFlags noDstStageMask;
            VkSubmitInfo subInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = NULL,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = &noSemaphores,
                .pWaitDstStageMask = &noDstStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &cmdBuf,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &vulkanObj->imageAcquiredSemaphore[vulkanObj->imageIndex]
            };
            result = vkQueueSubmit(vulkanObj->queue, 1, &subInfo, cmdBufFence);
            if(VK_SUCCESS != result)
            {
                printf("Failed to submit command buffer\n");
            }
            else
            {
                VkPresentInfoKHR presInfo =
                {
                    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                    .pNext = NULL,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = &vulkanObj->imageAcquiredSemaphore[vulkanObj->imageIndex],
                    .swapchainCount = 1,
                    .pSwapchains = &vulkanObj->swapChain,
                    .pImageIndices = &vulkanObj->imageIndex,
                    .pResults = NULL
                };
                result = vkQueuePresentKHR(vulkanObj->queue, &presInfo);
                if(VK_SUCCESS != result)
                {
                    printf("Failed to queue presentation\n");
                }
            }
        }
    }
}

VkResult createSemaphore(VulkanObject *vulkanObj, VkSemaphore semaphore[], uint32_t count)
{
    VkResult result = VK_SUCCESS;
    uint32_t i;

    VkSemaphoreCreateInfo sci =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    for (i = 0; i < count; i++)
    {
        result = vkCreateSemaphore(vulkanObj->device, &sci, NULL, &semaphore[i]);
        if (VK_SUCCESS != result)
        {
            break;
        }
    }

    return result;
}

VkResult createCommandBuffer(VulkanObject *vulkanObj, VkCommandBuffer cmdBuffer[], uint32_t count)
{
    VkResult result;

    VkCommandBufferAllocateInfo cbai =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = vulkanObj->cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count
    };

    result = vkAllocateCommandBuffers(vulkanObj->device, &cbai, cmdBuffer);
    
    return result;

}