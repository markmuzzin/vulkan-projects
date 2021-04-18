#include "bmpTools.h"


VkResult getBmpSize(char *path, VkExtent2D *size)
{
    uint8_t *data = NULL;
    FILE *pFile = NULL;
    errno_t err;

    err = fopen_s(&pFile, path, "rb");
    if (err != 0)
    {
       printf("Error opening BMP file\n");
       return VK_FALSE;
    }

    /* Read file */
    data = (uint8_t *)malloc( BMP_HEADER_SIZE );
    if (data == NULL)
    {
        return VK_FALSE;
    }

    /* Read BMP Header */
    fread(data, BMP_HEADER_SIZE, 1, pFile);

    /* Get width and height from header */
    size->width = *(uint32_t *)(&data[BMP_WIDTH_OFFSET]);
    size->height = *(uint32_t *)(&data[BMP_HEIGHT_OFFSET]);

    fclose(pFile);

    return VK_TRUE;
}

void loadBmpToBuffer(char *path, VkExtent2D *size, unsigned char **ptr)
{
    uint32_t i;
    FILE *pFile = NULL;
    errno_t err;

    err = fopen_s(&pFile, path, "rb");
    if (err != 0)
    {
        printf("Error opening BMP file\n");
        return;
    }

    /* Read BMP Header */
    fread(*ptr, BMP_HEADER_SIZE, 1, pFile);

    /* Read pixel data */
    for(i=0; i<size->width * size->height; i++)
    {
        /* Read a triple add an alpha byte */
        fread( *ptr+(i*4), 1, BMP_PIX_PER_COLOR, pFile );
    }

    fclose( pFile );
}
