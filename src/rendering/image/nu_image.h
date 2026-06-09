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
    Array_Init(&resourceManager->atlases, sizeof(Atlas), 4);
    Array_Init(&resourceManager->largeImageGlHandles, sizeof(GLuint), 16);
    Array_Init(&resourceManager->standaloneImageRenderDatas, sizeof(StandaloneImageRenderData), 16);
}

void ImageResourceManager_Free(ImageResourceManager* resourceManager)
{
    // Free large image GL memory
    for (int i=0; i<resourceManager->largeImageGlHandles.size; i++) {
        GLuint handle = *(GLuint*)Array_Get(&resourceManager->largeImageGlHandles, i);
        glDeleteTextures(1, &handle);
    }

    // Free atlas memory
    for (int i=0; i<resourceManager->atlases.size; i++) {
        Atlas* atlas = Array_Get(&resourceManager->atlases, i);
        glDeleteTextures(1, &atlas->glImageHandle);
        Array_Free(&atlas->images);
        Array_Free(&atlas->renderDataArray);
    }

    // Free arrays
    Array_Free(&resourceManager->largeImageGlHandles);
    Array_Free(&resourceManager->atlases);
    Array_Free(&resourceManager->standaloneImageRenderDatas);
}

void ImageResourceManager_AddImageRenderData(ImageResourceManager* resourceManager, int imageHandle, ImageRenderData* renderData)
{
    // Image is standalone
    if ((imageHandle & 0xFFFFu) == 0xFFFFu) {
        int imageIndex = (imageHandle >> 16) & 0xFFFFu;
        GLuint glHandle = *(GLuint*)Array_Get(&resourceManager->largeImageGlHandles, imageIndex);
        StandaloneImageRenderData* sRenderData = Array_PushEmpty(&resourceManager->standaloneImageRenderDatas);  
        sRenderData->glImageHandle = glHandle;
        sRenderData->renderData = *renderData;
    }
    // Image is atlased
    else {
        int atlasIndex = ((imageHandle >> 16) & 0xFFFFu) - 1;
        int imageIndex = (imageHandle & 0xFFFFu) - 1;
        Atlas* atlas = Array_Get(&resourceManager->atlases, atlasIndex);
        AtlasImage* img = Array_Get(&atlas->images, imageIndex);
        renderData->u0 = img->u0;
        renderData->v0 = img->v0;
        renderData->u1 = img->u1;
        renderData->v1 = img->v1;
        Array_Push(&atlas->renderDataArray, renderData);
    }
}

void ImageResourceManager_ClearAllImageRenderData(ImageResourceManager* resourceManager)
{
    for (int i=0; i<resourceManager->atlases.size; i++) {
        Atlas* atlas = Array_Get(&resourceManager->atlases, i);
        Array_Clear(&atlas->renderDataArray);
    }
    Array_Clear(&resourceManager->standaloneImageRenderDatas);
}

void ImageResourceLoader_Init(ImageResourceLoader* loader, ImageResourceManager* resourceManager)
{
    loader->resourceManager = resourceManager;
    LinearStringmap_Init(&loader->imageFilepathToHandleMap, sizeof(int), 20, 512);
    Array_Init(&loader->atlasBuilds, sizeof(AtlasBuild), 8);
}

int ImageResourceLoader_GetLoadedImageHandle(ImageResourceLoader* loader, const char* filepath)
{
    void* found = LinearStringmap_Get(&loader->imageFilepathToHandleMap, filepath);
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
        Array_Push(&loader->resourceManager->largeImageGlHandles, &handle);
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
            AtlasBuild* atlasBuild = Array_Get(&loader->atlasBuilds, i);
            if (AtlasBuild_CanFit(atlasBuild, w, h)) {
                validAtlasBuild = atlasBuild;
                atlas = Array_Get(&loader->resourceManager->atlases, i);
                atlasIndex = i;
                break;
            }
        }

        // Did not find a valid atlas build -> create one
        if (!validAtlasBuild) {
            validAtlasBuild = Array_PushEmpty(&loader->atlasBuilds);
            AtlasBuild_Init(validAtlasBuild, atlasSize, atlasSize); // example size

            // Create a corresponding ImageResourceManager Atlas
            atlas = Array_PushEmpty(&loader->resourceManager->atlases);
            Array_Init(&atlas->images, sizeof(AtlasImage), 32);
            Array_Init(&atlas->renderDataArray, sizeof(ImageRenderData), 32);
            atlasIndex = loader->resourceManager->atlases.size - 1;
        }

        // Pack image into atlas build
        int x, y;
        AtlasBuild_AddImage(validAtlasBuild, buffer, w, h, &x, &y);
        stbi_image_free(buffer);

        // Add image to resource manager
        AtlasImage* newImage = Array_PushEmpty(&atlas->images);
        newImage->u0 = (float)x / (float)atlasSize;
        newImage->v0 = (float)y / (float)atlasSize;
        newImage->u1 = (float)(x + w) / (float)atlasSize;
        newImage->v1 = (float)(y + h) / (float)atlasSize;
        int imageIndex = atlas->images.size - 1;

        // Create handle
        imageHandle = (((atlasIndex + 1) & 0xFFFFu) << 16) | ((imageIndex + 1) & 0xFFFFu);
    }   

    LinearStringmap_Set(&loader->imageFilepathToHandleMap, filepath, &imageHandle);
    return imageHandle;
}

void ImageResourceLoader_UploadImagesAndFree(ImageResourceLoader* loader)
{
    for (int i=0; i<loader->atlasBuilds.size; i++) 
    {
        AtlasBuild* build = Array_Get(&loader->atlasBuilds, i);

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
        Atlas* atlas = Array_Get(&loader->resourceManager->atlases, i);
        atlas->glImageHandle = handle;

        // Free atlas memory
        free(build->buffer);
    }

    // Free memory
    LinearStringmap_Free(&loader->imageFilepathToHandleMap);
}