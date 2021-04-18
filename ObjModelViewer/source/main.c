#include <stdio.h>
#include <string.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "objFileLoader.h"
#include "bmpTools.h"
#include "matrixMath.h"
#include "vulkanCmds.h"

#define WINDOW_WIDTH                1024
#define WINDOW_HEIGHT               768

#define DEFAULT_CAM_DIST            100.0f
#define DEFAULT_FOV                 55.0f

#define SCENE_NEAR                  0.1f
#define SCENE_FAR                   2000.0f

typedef struct _matrices_t
{
    float persepctiveProjMatrix[16];
    float rotationMatrixUp[16];
    float rotationMatrixRight[16];
    float viewMatrix[16];
} matrices_t;


static model_t s_model =
{
    .modelRotationUp    = 0.0f,
    .modelRotationRight = 0.0f,
    .cameraRight        = {0.0f, 0.0f, 0.0f},
    .cameraDirection    = {0.0f, 0.0f, 0.0f},
    .cameraTarget       = {0.0f, 0.0f, 0.0f},
    .cameraPosition     = {0.0f, 0.0f, DEFAULT_CAM_DIST},
    .cameraFront        = {0.0f, 0.0f, -1.0f},
    .cameraUp           = {0.0f, 1.0f,  0.0f}
};


void updateModelViewProjMatrix(matrices_t *matrices)
{
    /* Generate lookAt matrix for camera */
    generateLookAtMatrix(s_model.cameraPosition,
        (vec3_t) {
        s_model.cameraPosition.x + s_model.cameraFront.x,
            s_model.cameraPosition.y + s_model.cameraFront.y,
            s_model.cameraPosition.z + s_model.cameraFront.z
    },
        s_model.cameraUp,
            matrices->viewMatrix);

    /* Generate the rotation matrix */
    generateRotationMatrix(s_model.modelRotationUp, s_model.cameraUp, matrices->rotationMatrixUp);
    generateRotationMatrix(s_model.modelRotationRight, s_model.cameraRight, matrices->rotationMatrixRight);
}


static void initModelView(VulkanObject vulkanObj, matrices_t *matrices)
{
    /* Set Aspect ratio */
    float aspect = (float)vulkanObj.windowSize.width / (float)vulkanObj.windowSize.height;

    /* Generate perspective/projection matrix */
    generatePerspectiveProjectionMatrix(DEFAULT_FOV, aspect, SCENE_NEAR, SCENE_FAR, matrices->persepctiveProjMatrix);

    /* Set inital camera and model settings */
    s_model.cameraDirection = normalize(subProd(s_model.cameraPosition, s_model.cameraTarget));
    s_model.cameraRight = normalize(crossProd(s_model.cameraUp, s_model.cameraDirection));
    s_model.cameraUp = crossProd(s_model.cameraDirection, s_model.cameraRight);
}

static void initSceneDefaults(model_t *model)
{
    model->sp.ambientLight.x = 0.3f;
    model->sp.ambientLight.y = 0.3f;
    model->sp.ambientLight.z = 0.3f;

    model->sp.lightSourceIntensity.x = 0.8f;
    model->sp.lightSourceIntensity.y = 0.8f;
    model->sp.lightSourceIntensity.z = 0.8f;
}

static void updateUniformBuffer(VulkanObject vulkanObj, matrices_t *matrices)
{
    /* Update matrix data */
    memcpy(vulkanObj.uniformBuffer.ptr, (float*)matrices, sizeof(matrices_t));
}

static void updateVertexBuffer(VulkanObject vulkanObj)
{
    /* Update vertex data */
    memcpy(vulkanObj.vertexBuffer.ptr, (float*)s_model.vertArray, sizeof(vertexData_t) * 3 * s_model.numOfFaces);
}

int main(int argc, char *argv[])
{
    VkPipelineStageFlags stages         = 0;
    VkSemaphore sems                    = { 0 };
    
    VkCommandBuffer cmdBuffer[2]        = { 0 };
    VkFence fences[2]                   = { 0 };


    VkExtent2D textureSize              = { 0 };
    VkResult result                     = VK_SUCCESS;

    matrices_t matrices                 = { { 0 } };
    buffer_t stagingBuffer              = { 0 };
    material_t materials[MAX_MATERIALS] = { { 0 } };

    char *objFilePath                   = NULL;
    uint64_t objFilePathSize            = 0;
    uint32_t i                          = 0;
    int frame                           = 0;

    /* Create the vulkan object, init window size */
    VulkanObject vulkanObj =
    {
        .windowSize.width = WINDOW_WIDTH,
        .windowSize.height = WINDOW_HEIGHT,
        .numOfTextures = 0,
        .acquiredImages = NUM_SWAP_CHAIN_IMAGES,
    };

    /* Initialize the model view */
    initModelView(vulkanObj, &matrices);

    /* Initialize */
    if (VK_SUCCESS == initDriver(&vulkanObj) &&
        VK_SUCCESS == initRenderPass(&vulkanObj) &&
        VK_SUCCESS == initImages(&vulkanObj) &&
        VK_SUCCESS == initFrameBuffer(&vulkanObj) &&
        VK_SUCCESS == initPlatformSurface(&vulkanObj) &&
        VK_SUCCESS == initSwapChain(&vulkanObj)
        )
    {
        /* Create semaphores */
        result = createSemaphore(&vulkanObj, vulkanObj.imageAcquiredSemaphore, 2);
        if (VK_SUCCESS != result)
        {
            printf("Failed to create semaphore %d\n", i);
        }

        /* Create image fences */
        result = createFence(&vulkanObj, NULL, &vulkanObj.imageAcquiredFence, 1);
        if (VK_SUCCESS != result)
        {
            printf("Error creating fences\n");
        }

        /* Create command buffers */
        result = createCommandBuffer(&vulkanObj, cmdBuffer, 2);
        if (VK_SUCCESS != result)
        {
            printf("Failed to allocate command buffers\n");
        }
        else
        {
            VkFenceCreateInfo signaledFci =
            {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
                .pNext = NULL
            };

            /* Create signaled fences */
            createFence(&vulkanObj, &signaledFci, fences, 2);

            /* Get the path of the object file */
            char* path = getPath(argv[1]);

            /* Load the model file */
            if (VK_FALSE == loadModel(&s_model, materials, argv[1]))
            {
                printf("Error Loading OBJ file\n");
            }

            /* Init scene defaults */
            initSceneDefaults(&s_model);

            VkCommandBufferBeginInfo bi =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pInheritanceInfo = NULL,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pNext = NULL
            };

            VkSubmitInfo si =
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = NULL,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = &sems,
                .pWaitDstStageMask = &stages,
                .commandBufferCount = 1,
                .pCommandBuffers = &vulkanObj.cmdBuffer,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores = &sems
            };

            /* Load texture files */
            for (i = 0;i < s_model.materialCount;i++)
            {
                if (materials[i].fileName != NULL && i < MAX_IMAGE_TEXTURES)
                {
                    /* Set the texture id to the corresponding image file */
                    materials[i].mp.imageIndex = i;

                    objFilePathSize = strlen(path) + strlen(materials[i].fileName) + 1;
                    objFilePath = (char*)malloc(objFilePathSize);
                    memset(objFilePath, 0, objFilePathSize);
                    strcpy_s(objFilePath, objFilePathSize, path);
                    strcat_s(objFilePath, objFilePathSize, materials[i].fileName);

                    /* Get the BMP Size */
                    getBmpSize(objFilePath, &textureSize);

                    /* Begin command buffer */
                    vkBeginCommandBuffer(vulkanObj.cmdBuffer, &bi);

                    /* Create the staging buffer for the image */
                    stagingBuffer = createBuffer(&vulkanObj,
                        textureSize.width * textureSize.height * (BMP_PIX_PER_COLOR + 1),
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                        VK_SHARING_MODE_EXCLUSIVE);

                    /* Load the image data to the staging pointer */
                    loadBmpToBuffer(objFilePath, &textureSize, (unsigned char**)&stagingBuffer.ptr);

                    /* Create image */
                    vulkanObj.textures[i] = createTextureImage(&vulkanObj, &textureSize, &stagingBuffer);

                    /* Unmap memory */
                    vkUnmapMemory(vulkanObj.device, stagingBuffer.memory);

                    /* End command buffer */
                    vkEndCommandBuffer(vulkanObj.cmdBuffer);

                    /* Submit command buffer to GPU */
                    vkQueueSubmit(vulkanObj.queue, 1, &si, NULL);
                    vkQueueWaitIdle(vulkanObj.queue);

                    /* Update the number of textures */
                    vulkanObj.numOfTextures++;

                    /* Free path memory */
                    free(objFilePath);
                    objFilePath = NULL;
                }
            }

            /* Create pipelines */
            createPipelines(&vulkanObj);

            /* Begin command buffer */
            vkBeginCommandBuffer(vulkanObj.cmdBuffer, &bi);

            /* Prepare object arrays */
            prepareObjectArrays(&s_model);

            /* Create the vertex buffer */
            vulkanObj.vertexBuffer = createBuffer(&vulkanObj,
                sizeof(vertexData_t) * ELEMENTS_PER_VERTEX * s_model.numOfFaces,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_SHARING_MODE_EXCLUSIVE);

            /* Create the uniform buffer */
            vulkanObj.uniformBuffer = createBuffer(&vulkanObj,
                sizeof(matrices_t),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_SHARING_MODE_EXCLUSIVE);

            /* Create 2D texture sampler */
            createSampler(&vulkanObj);

            /* Allocate the descriptor set */
            allocateDescriptorSet(&vulkanObj);

            /* Create the image descriptor set */
            createImageDescriptorSet(&vulkanObj, vulkanObj.textures);

            /* Create the uniform buffer descriptor set */
            createUniformBufferDescriptorSet(&vulkanObj, sizeof(matrices_t));

            /* Create the texture buffer descriptor set */
            createTextureBufferDescriptorSet(&vulkanObj);

            /* Transition Images */
            transitionImage(&vulkanObj);

            /* End command buffer */
            vkEndCommandBuffer(vulkanObj.cmdBuffer);

            /* Submit command buffer to GPU */
            vkQueueSubmit(vulkanObj.queue, 1, &si, NULL);

            /* Update vertex buffer */
            updateVertexBuffer(vulkanObj);

            /* Setup command buffer structure */
            VkCommandBufferBeginInfo cbbi =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = NULL,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = NULL
            };

            for (i = 0; frame < 1000; ++frame, ++i)
            {
                VkFence fence = fences[i & 1];
                VkCommandBuffer cmdBuf = cmdBuffer[i & 1];

                /* Wait for the command buffer to useable */
                result = vkWaitForFences(vulkanObj.device, 1, &fence, VK_TRUE, MAX_TIMEOUT);
                if (VK_SUCCESS != result)
                {
                    printf("Timeout waiting for fence\n");
                }
                else
                {
                    /* Reset fence */
                    vkResetFences(vulkanObj.device, 1, &fence);

                    /* Begin the command buffer */
                    result = vkBeginCommandBuffer(cmdBuf, &cbbi);
                    if (VK_SUCCESS != result)
                    {
                        printf("Failed to begin command buffer\n");
                    }
                    else
                    {
                        /* Rotate the object */
                        s_model.modelRotationUp -= 0.5f;
                        if (s_model.modelRotationUp > 360.0f)
                        {
                            s_model.modelRotationUp = s_model.modelRotationUp - 360.0f;
                        }

                        /* Update the matrix */
                        updateModelViewProjMatrix(&matrices);

                        /* Update uniform buffer */
                        updateUniformBuffer(vulkanObj, &matrices);

                        /* Draw */
                        draw(&vulkanObj, cmdBuf, s_model);

                        /* Swap buffers */
                        swapFrontBuffer(&vulkanObj, cmdBuf, fence);
                    }
                }
            }
        }
    }
    return 0;
}
