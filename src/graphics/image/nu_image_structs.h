#pragma once

// Represents and image that can exist on the GPU
typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkImageLayout currentLayout;
    u16 width;
    u16 height;
} ImageResource;

typedef struct {
    unsigned char* buffer;
    u16 w, h;
    u16 x, y, shelfHeight;
    int imageResourceIndex;
} AtlasBuilder;

/* Represents a region of an image resource
 * An image resource may have 1 or many views (if the resource is an atlas)
 */
typedef struct {
    float u0, v0;
    float u1, v1;
    u16 w, h;
    int imageResourceIndex;
} ImageView;

typedef struct {
    LinearStringmap filepathToHandleMap;
    AtlasBuilder builders[3]; // Builders pack small images into image resources at startup
    int buildersCreated;
    Array groupDescriptorSets;
    Array imageViews;
    Array imageResources;

    // Shared Vulkan infrastructure for group descriptor sets
    VkDescriptorSetLayout setLayout;     // binding 0: sampler2D[8], fragment stage
    VkDescriptorPool      pool;          // sized for max group count × 8 descriptors
    VkSampler             sampler;       // shared; linear + clamp for now

    // 1x1 white, fills unused slots in a partial last group
    VkImage               dummyImage;
    VkDeviceMemory        dummyMemory;
    VkImageView           dummyView;
} ImageResourceManager;
