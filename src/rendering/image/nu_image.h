#pragma once

#include <datastructures/Linear_Stringmap.h>
#include <utils/nu_int.h>
#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

typedef struct AtlasImage {
    float u0, v0;
    float u1, v1;
} AtlasImage;

typedef struct StandaloneImageRenderData {
    ImageRenderData renderData;
    GLuint glImageHandle;
} StandaloneImageRenderData;

typedef struct Atlas {
    Array images;
    GLuint glImageHandle;
    Array renderDataArray;
} Atlas;

typedef struct ImageResourceManager {
    Array largeImageGlHandles;
    Array atlases;
    Array standaloneImageRenderDatas;
} ImageResourceManager;

typedef struct AtlasBuild {
    unsigned char* buffer;
    u16 w, h;
    u16 x, y, shelfHeight;
} AtlasBuild;

inline void AtlasBuild_Init(AtlasBuild* a, u16 w, u16 h)
{
    a->w = w;
    a->h = h;
    a->x = 0;
    a->y = 0;
    a->shelfHeight = 0;
    a->buffer = (unsigned char*)malloc(w * h * 4);
    memset(a->buffer, 0, w * h * 4);
}

bool AtlasBuild_CanFit(AtlasBuild* a, int w, int h)
{
    // new row if needed
    if (a->x + w > a->w) {
        if (a->y + a->shelfHeight + h > a->h) return 0;
    } else {
        if (a->y + h > a->h) return 0;
    }
    return 1;
}

static void AtlasBuild_AddImage(
    AtlasBuild* a,
    const unsigned char* src,
    int imgW, int imgH,
    int* outX, int* outY)
{
    // New row if needed
    if (a->x + imgW > a->w) {
        a->y += a->shelfHeight;
        a->x = 0;
        a->shelfHeight = 0;
    }

    // Write position
    int x = a->x;
    int y = a->y;

    // Blit
    for (int row = 0; row < imgH; row++) {
        unsigned char* dst = &a->buffer[((y + row) * a->w + x) * 4];
        const unsigned char* srcRow = &src[row * imgW * 4];
        memcpy(dst, srcRow, imgW * 4);
    }

    // Advance cursor
    a->x += imgW;
    if (imgH > a->shelfHeight) a->shelfHeight = imgH;
    *outX = x;
    *outY = y;
}

typedef struct ImageResourceLoader {
    ImageResourceManager* resourceManager;
    LinearStringmap imageFilepathToHandleMap;
    Array atlasBuilds;
} ImageResourceLoader;

void ImageResourceManager_Init(ImageResourceManager* resourceManager)
{
    ArrayInit(&resourceManager->atlases, sizeof(Atlas), 4);
    ArrayInit(&resourceManager->largeImageGlHandles, sizeof(GLuint), 16);
    ArrayInit(&resourceManager->standaloneImageRenderDatas, sizeof(StandaloneImageRenderData), 16);
}

void ImageResourceManager_Free(ImageResourceManager* resourceManager)
{
    // Free large image GL memory
    for (int i=0; i<resourceManager->largeImageGlHandles.size; i++) {
        GLuint handle = *(GLuint*)ArrayGet(&resourceManager->largeImageGlHandles, i);
        glDeleteTextures(1, &handle);
    }

    // Free atlas memory
    for (int i=0; i<resourceManager->atlases.size; i++) {
        Atlas* atlas = ArrayGet(&resourceManager->atlases, i);
        glDeleteTextures(1, &atlas->glImageHandle);
        ArrayFree(&atlas->images);
        ArrayFree(&atlas->renderDataArray);
    }

    // Free arrays
    ArrayFree(&resourceManager->largeImageGlHandles);
    ArrayFree(&resourceManager->atlases);
    ArrayFree(&resourceManager->standaloneImageRenderDatas);
}

void ImageResourceManager_AddImageRenderData(ImageResourceManager* resourceManager, int imageHandle, ImageRenderData* renderData)
{
    // Image is standalone
    if ((imageHandle & 0xFFFFu) == 0xFFFFu) {
        int imageIndex = (imageHandle >> 16) & 0xFFFFu;
        GLuint glHandle = *(GLuint*)ArrayGet(&resourceManager->largeImageGlHandles, imageIndex);
        StandaloneImageRenderData* sRenderData = ArrayPushEmpty(&resourceManager->standaloneImageRenderDatas);  
        sRenderData->glImageHandle = glHandle;
        sRenderData->renderData = *renderData;
    }
    // Image is atlased
    else {
        int atlasIndex = ((imageHandle >> 16) & 0xFFFFu) - 1;
        int imageIndex = (imageHandle & 0xFFFFu) - 1;
        Atlas* atlas = ArrayGet(&resourceManager->atlases, atlasIndex);
        AtlasImage* img = ArrayGet(&atlas->images, imageIndex);
        renderData->u0 = img->u0;
        renderData->v0 = img->v0;
        renderData->u1 = img->u1;
        renderData->v1 = img->v1;
        ArrayPush(&atlas->renderDataArray, renderData);
    }
}

void ImageResourceManager_ClearAllImageRenderData(ImageResourceManager* resourceManager)
{
    for (int i=0; i<resourceManager->atlases.size; i++) {
        Atlas* atlas = ArrayGet(&resourceManager->atlases, i);
        ArrayClear(&atlas->renderDataArray);
    }
    ArrayClear(&resourceManager->standaloneImageRenderDatas);
}

void ImageResourceLoader_Init(ImageResourceLoader* loader, ImageResourceManager* resourceManager)
{
    loader->resourceManager = resourceManager;
    LinearStringmapInit(&loader->imageFilepathToHandleMap, sizeof(int), 20, 512);
    ArrayInit(&loader->atlasBuilds, sizeof(AtlasBuild), 8);
}

int ImageResourceLoader_GetLoadedImageHandle(ImageResourceLoader* loader, const char* filepath)
{
    void* found = LinearStringmapGet(&loader->imageFilepathToHandleMap, filepath);
    if (found == NULL) {
        return 0;
    }
    return *(int*)found;
}

int ImageResourceLoader_LoadImage(ImageResourceLoader* loader, const char* filepath)
{
    int w, h, n;
    unsigned char* buffer = stbi_load(filepath, &w, &h, &n, 4); // Force RGBA
    if (!buffer) {
        return 0;
    }

    int imageHandle;

    // Too large to be added to an atlas -> keep as individual image
    if (w > 128 || h > 128) 
    {
        // Upload to GPU
        GLuint handle;
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        stbi_image_free(buffer); // Free CPU memory

        // Add handle to large image handles array
        ArrayPush(&loader->resourceManager->largeImageGlHandles, &handle);
        int index = loader->resourceManager->largeImageGlHandles.size - 1;

        // Create handle
        imageHandle = (index << 16) | 0xFFFFu;
    }
    // Pack image into an atlas
    else
    {
        u16 atlasSize = 512;

        // Search the last 4 atlases to find one that the image can fit into
        AtlasBuild* validAtlasBuild = NULL;
        Atlas* atlas = NULL;
        int atlasIndex = 0;
        for (int i = 0; i < min(loader->atlasBuilds.size, 4); i++) {
            AtlasBuild* atlasBuild = ArrayGet(&loader->atlasBuilds, i);
            if (AtlasBuild_CanFit(atlasBuild, w, h)) {
                validAtlasBuild = atlasBuild;
                atlas = ArrayGet(&loader->resourceManager->atlases, i);
                atlasIndex = i;
                break;
            }
        }

        // Did not find a valid atlas build -> create one
        if (!validAtlasBuild) {
            validAtlasBuild = ArrayPushEmpty(&loader->atlasBuilds);
            AtlasBuild_Init(validAtlasBuild, atlasSize, atlasSize); // example size

            // Create a corresponding ImageResourceManager Atlas
            atlas = ArrayPushEmpty(&loader->resourceManager->atlases);
            ArrayInit(&atlas->images, sizeof(AtlasImage), 32);
            ArrayInit(&atlas->renderDataArray, sizeof(ImageRenderData), 32);
            atlasIndex = loader->resourceManager->atlases.size - 1;
        }

        // Pack image into atlas build
        int x, y;
        AtlasBuild_AddImage(validAtlasBuild, buffer, w, h, &x, &y);
        stbi_image_free(buffer);

        // Add image to resource manager
        AtlasImage* newImage = ArrayPushEmpty(&atlas->images);
        newImage->u0 = (float)x / (float)atlasSize;
        newImage->v0 = (float)y / (float)atlasSize;
        newImage->u1 = (float)(x + w) / (float)atlasSize;
        newImage->v1 = (float)(y + h) / (float)atlasSize;
        int imageIndex = atlas->images.size - 1;

        // Create handle
        imageHandle = (((atlasIndex + 1) & 0xFFFFu) << 16) | ((imageIndex + 1) & 0xFFFFu);
    }   

    LinearStringmapSet(&loader->imageFilepathToHandleMap, filepath, &imageHandle);
    return imageHandle;
}

void ImageResourceLoader_UploadImagesAndFree(ImageResourceLoader* loader)
{
    for (int i=0; i<loader->atlasBuilds.size; i++) 
    {
        AtlasBuild* build = ArrayGet(&loader->atlasBuilds, i);

        // Upload to GPU
        GLuint handle;
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, build->w, build->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, build->buffer);

        // Set the handle of the corresponding ImageResourceManager Atlas
        Atlas* atlas = ArrayGet(&loader->resourceManager->atlases, i);
        atlas->glImageHandle = handle;

        // Free atlas memory
        free(build->buffer);
    }

    // Free memory
    LinearStringmapFree(&loader->imageFilepathToHandleMap);
}