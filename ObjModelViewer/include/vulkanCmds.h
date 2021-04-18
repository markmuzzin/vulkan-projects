#ifndef __VULKAN_CMDS_H__
#define __VULKAN_CMDS_H__

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <stddef.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include "objFileLoader.h"

#define GLOBAL_APP_NAME_W   "Wavefront Object Model Viewer"

#define MAX_TIMEOUT                 1000000000L

#define NUM_SWAP_CHAIN_IMAGES       2

#define MAX_DESCRIPTOR_SETS         32
#define MAX_IMAGE_TEXTURES          16

#define LAYOUT_BINDING_COUNT        3
#define INPUT_BINDING_COUNT         3

#define BINDING_VERT_POSITION       0
#define LOCATION_VERT_POSITION      0
#define LOCATION_VERT_NORMALS       1
#define LOCATION_VERT_TEXCOORDS     2


#define BINDING_FRAG_SAMPLER        0
#define BINDING_FRAG_TEXTURES       1
#define BINDING_FRAG_UNIFORM        2


typedef struct _vertexData_t {
    float vx, vy, vz, vw;
    float nx, ny, nz, nw;
    float s, t;
} vertexData_t;


typedef struct _buffer_t {
    VkBuffer        buffer;
    VkDeviceMemory  memory;
    void*           ptr;
} buffer_t;


typedef struct _texture_t {
    VkImage         image;
    VkImageView     view;
    VkDeviceMemory  memory;
} texture_t;


typedef struct VulkanObject
{
    VkDevice                device;
    VkInstance              instance;
    VkPhysicalDevice        physicalDevice;
    VkQueue                 queue;
    uint32_t                queueIndex;

    VkCommandPool           cmdPool;
    VkCommandBuffer         cmdBuffer;
    VkFence                 cmdBuffFence;

    VkDescriptorImageInfo   samplerInfo;
    VkDescriptorPool        descriptorPool;
    VkDescriptorImageInfo   dii[16];
    VkDescriptorBufferInfo  dbi;
    VkDescriptorSet         descriptorSet;
    VkWriteDescriptorSet    wds[MAX_DESCRIPTOR_SETS];

    VkExtent2D              windowSize;

    VkSemaphore             imageAcquiredSemaphore[NUM_SWAP_CHAIN_IMAGES];
    VkFence                 imageAcquiredFence;

    buffer_t                vertexBuffer;
    buffer_t                uniformBuffer;

    texture_t               textures[MAX_IMAGE_TEXTURES];
    uint32_t                numOfTextures;

    VkDescriptorSetLayout    dsl;
    VkPipelineLayout         pll;
    VkPipeline               texPipeline;

    VkRenderPass             renderPass;
    VkImage                  colorBuffer;
    VkImage                  depthBuffer;
    VkDeviceMemory           imageMemory[2];
    VkImageView              imageViews[2];
    VkFramebuffer            framebuffer;

    VkSwapchainKHR           swapChain;
    VkFormat                 displayFormat;
    VkExtent2D               displaySize;
    VkImage                  displayBuffers[NUM_SWAP_CHAIN_IMAGES];
    uint32_t                 imageIndex;
    uint32_t                 acquiredImages;
    VkSurfaceKHR             surface;

    VkSampler                sampler;
    uint32_t                 descSetCount;

} VulkanObject;

VkResult initDriver(VulkanObject* vulkanObj);
VkResult initRenderPass(VulkanObject* vulkanObj);
VkResult initImages(VulkanObject* vulkanObj);
VkResult initFrameBuffer(VulkanObject* vulkanObj);
VkResult initPlatformSurface(VulkanObject* vulkanObj);
VkResult initSwapChain(VulkanObject* vulkanObj);

buffer_t createBuffer(VulkanObject* vulkanObj, uint32_t size, uint32_t usageFlags, VkMemoryPropertyFlags memFlags, VkSharingMode sharing);
void createSampler(VulkanObject* vulkanObj);
void createUniformBufferDescriptorSet(VulkanObject* vulkanObj, uint32_t uniformStructSize);
void createImageDescriptorSet(VulkanObject* vulkanObj, texture_t textures[]);
void createTextureBufferDescriptorSet(VulkanObject* vulkanObj);
texture_t createTextureImage(VulkanObject* vulkanObj, VkExtent2D* size, buffer_t* staging);
void createPipelines(VulkanObject *vulkanObj);
VkResult createFence(VulkanObject *vulkanObj, VkFenceCreateInfo* info, VkFence* outFence, uint32_t count);
VkResult createSemaphore(VulkanObject *vulkanObj, VkSemaphore semaphore[], uint32_t count);
VkResult createCommandBuffer(VulkanObject* vulkanObj, VkCommandBuffer cmdBuffer[], uint32_t count);

void allocateDescriptorSet(VulkanObject *vulkanObj);
void transitionImage(VulkanObject *vulkanObj);
void draw(VulkanObject *vulkanObj, VkCommandBuffer cmdBuf, model_t model);
void swapFrontBuffer(VulkanObject *vulkanObj, VkCommandBuffer cmdBuf, VkFence cmdBufFence);

void waitForFence(VulkanObject *vulkanObj, VkFence fence);

#endif
