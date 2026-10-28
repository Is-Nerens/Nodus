#pragma once

static VkBuffer quadBuffer  = VK_NULL_HANDLE;
static VkDeviceMemory quadMemory  = VK_NULL_HANDLE;

void NU_CreateQuadBuffer(VkPhysicalDevice physicalDevice, VkDevice device)
{
    static const float quadVerts[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    VkDeviceSize size = sizeof(quadVerts);

    VkBufferCreateInfo bi = {0};
    bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size        = size;
    bi.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bi, NULL, &quadBuffer) != VK_SUCCESS) {
        // error
        return;
    }

    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(device, quadBuffer, &mr);   // <-- device, not physicalDevice

    VkMemoryAllocateInfo ai = {0};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = mr.size;
    ai.memoryTypeIndex = FindMemoryType(physicalDevice,
                                        mr.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &ai, NULL, &quadMemory) != VK_SUCCESS) {
        // error
        return;
    }

    if (vkBindBufferMemory(device, quadBuffer, quadMemory, 0) != VK_SUCCESS) {
        // error
        return;
    }

    void* p = NULL;
    if (vkMapMemory(device, quadMemory, 0, size, 0, &p) != VK_SUCCESS) {
        // error
        return;
    }
    memcpy(p, quadVerts, (size_t)size);
    vkUnmapMemory(device, quadMemory);
}

void NU_DestroyQuadBuffer(VkDevice device)
{
    if (quadBuffer) vkDestroyBuffer(device, quadBuffer, NULL);
    if (quadMemory) vkFreeMemory(device, quadMemory, NULL);
    quadBuffer = VK_NULL_HANDLE;
    quadMemory = VK_NULL_HANDLE;
}

extern const unsigned char src_shaders_build_border_rect_vert_spv[];
extern const unsigned int  src_shaders_build_border_rect_vert_spv_len;
extern const unsigned char src_shaders_build_border_rect_frag_spv[];
extern const unsigned int  src_shaders_build_border_rect_frag_spv_len;

int NU_CreateBorderRectPipeline(DisplayManager* dm, NuPipeline* pipeline, bool MSAA_4X)
{
    // Shader module create info
    VkShaderModuleCreateInfo smci = {0};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    // Create vertex shader module
    VkShaderModule vertModule;
    smci.codeSize = src_shaders_build_border_rect_vert_spv_len;   // works because it's a real array
    smci.pCode = (const u32*)(src_shaders_build_border_rect_vert_spv);
    if (vkCreateShaderModule(dm->device, &smci, NULL, &vertModule) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vertex shader module failed");
        return 0;
    }

    // Create fragment shader module
    VkShaderModule fragModule;
    smci.codeSize = src_shaders_build_border_rect_frag_spv_len;
    smci.pCode = (const u32*)(src_shaders_build_border_rect_frag_spv);
    if (vkCreateShaderModule(dm->device, &smci, NULL, &fragModule) != VK_SUCCESS) {
        vkDestroyShaderModule(dm->device, vertModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> fragment shader module failed");
        return 0;
    }

    // Create shader stages
    VkPipelineShaderStageCreateInfo stages[2] = {0};

    // Vertex shader stage
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName  = "main";

    // Fragment shader stage
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName  = "main";

    // Vertex attribute layout
    VkVertexInputBindingDescription bindings[2] = {0};
    bindings[0].binding   = 0;
    bindings[0].stride    = sizeof(float) * 2;
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindings[1].binding   = 1;
    bindings[1].stride    = sizeof(BorderRectRenderData);
    bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    VkVertexInputAttributeDescription attrs[8] = {0};
    attrs[0].location = 0;   // aQuad
    attrs[0].binding  = 0;
    attrs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset   = 0;
    attrs[1].location = 1;   // iPos
    attrs[1].binding  = 1;
    attrs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset   = offsetof(BorderRectRenderData, x);
    attrs[2].location = 2;   // iSize
    attrs[2].binding  = 1;
    attrs[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset   = offsetof(BorderRectRenderData, w);
    attrs[3].location = 3;   // iBgColor
    attrs[3].binding  = 1;
    attrs[3].format   = VK_FORMAT_R32_UINT;
    attrs[3].offset   = offsetof(BorderRectRenderData, backgroundRGBA);
    attrs[4].location = 4;   // iBorderColor
    attrs[4].binding  = 1;
    attrs[4].format   = VK_FORMAT_R32_UINT;
    attrs[4].offset   = offsetof(BorderRectRenderData, borderRGBA);
    attrs[5].location = 5;   // iRadius (vec4: Tl, Tr, Bl, Br)
    attrs[5].binding  = 1;
    attrs[5].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[5].offset   = offsetof(BorderRectRenderData, radiusTl);
    attrs[6].location = 6;   // iBorder (vec4: Top, Bottom, Left, Right)
    attrs[6].binding  = 1;
    attrs[6].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[6].offset   = offsetof(BorderRectRenderData, borderTop);
    attrs[7].location = 7;   // iScissor (vec4: Top, Bottom, Left, Right)
    attrs[7].binding  = 1;
    attrs[7].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[7].offset   = offsetof(BorderRectRenderData, scissorTop);
    VkPipelineVertexInputStateCreateInfo vis = {0};
    vis.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount   = 2;
    vis.pVertexBindingDescriptions      = bindings;
    vis.vertexAttributeDescriptionCount = 8;
    vis.pVertexAttributeDescriptions    = attrs;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo ia = {0};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport and scissor
    VkPipelineViewportStateCreateInfo vp = {0};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;
    VkDynamicState dynStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dyn = {0};
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates    = dynStates;

    // Rasteriser settings
    VkPipelineRasterizationStateCreateInfo rast = {0};
    rast.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.depthClampEnable        = VK_FALSE;
    rast.rasterizerDiscardEnable = VK_FALSE;
    rast.polygonMode             = VK_POLYGON_MODE_FILL;
    rast.cullMode                = VK_CULL_MODE_NONE;
    rast.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rast.depthBiasEnable         = VK_FALSE;
    rast.lineWidth               = 1.0f;

    // Multisample
    VkPipelineMultisampleStateCreateInfo ms = {0};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = MSAA_4X ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;
    ms.sampleShadingEnable  = VK_FALSE;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo ds = {0};
    ds.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable       = VK_FALSE;
    ds.depthWriteEnable      = VK_FALSE;
    ds.depthCompareOp        = VK_COMPARE_OP_ALWAYS;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.stencilTestEnable     = VK_FALSE;

    // Colour blend
    VkPipelineColorBlendAttachmentState cba = {0};
    cba.blendEnable         = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp        = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.alphaBlendOp        = VK_BLEND_OP_ADD;
    cba.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb = {0};
    cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.logicOpEnable   = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments    = &cba;

    // Pipeline layout
    VkPushConstantRange pcr = {0};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcr.offset     = 0;
    pcr.size       = sizeof(float) * 2;
    VkPipelineLayoutCreateInfo plci = {0};
    plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount         = 0;
    plci.pSetLayouts            = NULL;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges    = &pcr;
    if (vkCreatePipelineLayout(dm->device, &plci, NULL, &pipeline->layout) != VK_SUCCESS) {
        vkDestroyShaderModule(dm->device, vertModule, NULL);
        vkDestroyShaderModule(dm->device, fragModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> border rect pipeline layout failed");
        return 0;
    }

    // Create pipeline
    VkGraphicsPipelineCreateInfo gp = {0};
    gp.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp.stageCount          = 2;
    gp.pStages             = stages;
    gp.pVertexInputState   = &vis;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState      = &vp;
    gp.pRasterizationState = &rast;
    gp.pMultisampleState   = &ms;
    gp.pDepthStencilState  = &ds;
    gp.pColorBlendState    = &cb;
    gp.pDynamicState       = &dyn;
    gp.layout              = pipeline->layout;
    gp.renderPass          = dm->sharedRenderPass;
    gp.subpass             = 0;
    if (vkCreateGraphicsPipelines(dm->device, VK_NULL_HANDLE, 1, &gp, NULL, &pipeline->handle) != VK_SUCCESS) {
        vkDestroyPipelineLayout(dm->device, pipeline->layout, NULL);
        pipeline->layout = VK_NULL_HANDLE;
        vkDestroyShaderModule(dm->device, vertModule, NULL);
        vkDestroyShaderModule(dm->device, fragModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> border rect graphics pipeline failed");
        return 0;
    }
    vkDestroyShaderModule(dm->device, vertModule, NULL);
    vkDestroyShaderModule(dm->device, fragModule, NULL);
    return 1;
}

extern const unsigned char src_shaders_build_image_vert_spv[];
extern const unsigned int  src_shaders_build_image_vert_spv_len;
extern const unsigned char src_shaders_build_image_frag_spv[];
extern const unsigned int  src_shaders_build_image_frag_spv_len;

int NU_CreateImagePipeline(NuPipeline* pipeline)
{
    return 1;
}

extern const unsigned char src_shaders_build_mono_text_vert_spv[];
extern const unsigned int  src_shaders_build_mono_text_vert_spv_len;
extern const unsigned char src_shaders_build_mono_text_frag_spv[];
extern const unsigned int  src_shaders_build_mono_text_frag_spv_len;

int NU_CreateMonoTextPipeline(DisplayManager* displayManager, NuPipeline* pipeline)
{
    // ---- Device + render pass come from the DisplayManager value ----
    VkDevice device = displayManager->device;
    VkRenderPass renderPass = displayManager->sharedRenderPass;

    // Descriptor set layout is local: it only needs to outlive
    // vkCreatePipelineLayout. (If you later need to allocate a descriptor
    // set from it, add `VkDescriptorSetLayout setLayout;` to NuPipeline.)
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;

    // ---- Shader modules ----
    VkShaderModuleCreateInfo smci = {0};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    VkShaderModule vertModule;
    smci.codeSize = src_shaders_build_mono_text_vert_spv_len;
    smci.pCode    = (const u32*)(src_shaders_build_mono_text_vert_spv);
    if (vkCreateShaderModule(device, &smci, NULL, &vertModule) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> mono text vertex shader module failed");
        return 0;
    }

    VkShaderModule fragModule;
    smci.codeSize = src_shaders_build_mono_text_frag_spv_len;
    smci.pCode    = (const u32*)(src_shaders_build_mono_text_frag_spv);
    if (vkCreateShaderModule(device, &smci, NULL, &fragModule) != VK_SUCCESS) {
        vkDestroyShaderModule(device, vertModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> mono text fragment shader module failed");
        return 0;
    }

    // ---- Shader stages ----
    VkPipelineShaderStageCreateInfo stages[2] = {0};

    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName  = "main";

    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName  = "main";

    // ---- Vertex input ----
    VkVertexInputBindingDescription bindings[2] = {0};
    bindings[0].binding   = 0;
    bindings[0].stride    = sizeof(float) * 2;      // aQuad (vec2)
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindings[1].binding   = 1;
    bindings[1].stride    = sizeof(GlyphRenderData);
    bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    VkVertexInputAttributeDescription attrs[6] = {0};

    attrs[0].location = 0;   // aQuad
    attrs[0].binding  = 0;
    attrs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset   = 0;

    attrs[1].location = 1;   // iPos (vec3)
    attrs[1].binding  = 1;
    attrs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset   = offsetof(GlyphRenderData, x);

    attrs[2].location = 2;   // iSize (vec2)
    attrs[2].binding  = 1;
    attrs[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset   = offsetof(GlyphRenderData, w);

    attrs[3].location = 3;   // iUV (vec4: u0,v0,u1,v1)
    attrs[3].binding  = 1;
    attrs[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[3].offset   = offsetof(GlyphRenderData, u0);

    attrs[4].location = 4;   // iColor (u32 packed RGBA)
    attrs[4].binding  = 1;
    attrs[4].format   = VK_FORMAT_R32_UINT;
    attrs[4].offset   = offsetof(GlyphRenderData, colourRGBA);

    attrs[5].location = 5;   // iTextureIndex (u32)
    attrs[5].binding  = 1;
    attrs[5].format   = VK_FORMAT_R32_UINT;
    attrs[5].offset   = offsetof(GlyphRenderData, textureIndex);

    VkPipelineVertexInputStateCreateInfo vis = {0};
    vis.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount   = 2;
    vis.pVertexBindingDescriptions      = bindings;
    vis.vertexAttributeDescriptionCount = 6;
    vis.pVertexAttributeDescriptions    = attrs;

    // ---- Input assembly ----
    VkPipelineInputAssemblyStateCreateInfo ia = {0};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // ---- Viewport / scissor (dynamic) ----
    VkPipelineViewportStateCreateInfo vp = {0};
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    VkDynamicState dynStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dyn = {0};
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates    = dynStates;

    // ---- Rasteriser ----
    VkPipelineRasterizationStateCreateInfo rast = {0};
    rast.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.depthClampEnable        = VK_FALSE;
    rast.rasterizerDiscardEnable = VK_FALSE;
    rast.polygonMode             = VK_POLYGON_MODE_FILL;
    rast.cullMode                = VK_CULL_MODE_NONE;
    rast.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rast.depthBiasEnable         = VK_FALSE;
    rast.lineWidth               = 1.0f;

    // ---- Multisample (match render pass; border-rect used MSAA_4X flag) ----
    VkPipelineMultisampleStateCreateInfo ms = {0};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;   // <-- set to 4x if sharedRenderPass is 4x MSAA
    ms.sampleShadingEnable  = VK_FALSE;

    // ---- Depth stencil (disabled) ----
    VkPipelineDepthStencilStateCreateInfo ds = {0};
    ds.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable       = VK_FALSE;
    ds.depthWriteEnable      = VK_FALSE;
    ds.depthCompareOp        = VK_COMPARE_OP_ALWAYS;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.stencilTestEnable     = VK_FALSE;

    // ---- Colour blend ----
    VkPipelineColorBlendAttachmentState cba = {0};
    cba.blendEnable         = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp        = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.alphaBlendOp        = VK_BLEND_OP_ADD;
    cba.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb = {0};
    cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.logicOpEnable   = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments    = &cba;

    // ---- Descriptor set layout: set 0, binding 0 = sampler2D uFontTextures[8] ----
    VkDescriptorSetLayoutBinding fontBinding = {0};
    fontBinding.binding            = 0;
    fontBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    fontBinding.descriptorCount    = 8;
    fontBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    fontBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo dslci = {0};
    dslci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslci.bindingCount = 1;
    dslci.pBindings    = &fontBinding;

    if (vkCreateDescriptorSetLayout(device, &dslci, NULL, &setLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(device, vertModule, NULL);
        vkDestroyShaderModule(device, fragModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> mono text descriptor set layout failed");
        return 0;
    }

    // ---- Pipeline layout: 1 set + push constant (4 floats) ----
    VkPushConstantRange pcr = {0};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcr.offset     = 0;
    pcr.size       = sizeof(float) * 4;   // uScreenWidth, uScreenHeight, uOffsetX, uOffsetY

    VkPipelineLayoutCreateInfo plci = {0};
    plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount         = 1;
    plci.pSetLayouts            = &setLayout;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges    = &pcr;

    if (vkCreatePipelineLayout(device, &plci, NULL, &pipeline->layout) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device, setLayout, NULL);
        vkDestroyShaderModule(device, vertModule, NULL);
        vkDestroyShaderModule(device, fragModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> mono text pipeline layout failed");
        return 0;
    }

    // ---- Graphics pipeline ----
    VkGraphicsPipelineCreateInfo gp = {0};
    gp.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp.stageCount          = 2;
    gp.pStages             = stages;
    gp.pVertexInputState   = &vis;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState      = &vp;
    gp.pRasterizationState = &rast;
    gp.pMultisampleState   = &ms;
    gp.pDepthStencilState  = &ds;
    gp.pColorBlendState    = &cb;
    gp.pDynamicState       = &dyn;
    gp.layout              = pipeline->layout;
    gp.renderPass          = renderPass;
    gp.subpass             = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp, NULL, &pipeline->handle) != VK_SUCCESS) {
        vkDestroyPipelineLayout(device, pipeline->layout, NULL);
        pipeline->layout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(device, setLayout, NULL);
        vkDestroyShaderModule(device, vertModule, NULL);
        vkDestroyShaderModule(device, fragModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> mono text graphics pipeline failed");
        return 0;
    }

    vkDestroyShaderModule(device, vertModule, NULL);
    vkDestroyShaderModule(device, fragModule, NULL);

    // The pipeline is fully created; the descriptor set layout is no longer
    // needed for the pipeline itself. Destroy it here unless the caller needs
    // it to allocate/write the font-texture descriptor set (see note below).
    vkDestroyDescriptorSetLayout(device, setLayout, NULL);

    return 1;
}

extern const unsigned char src_shaders_build_clipped_mono_text_vert_spv[];
extern const unsigned int  src_shaders_build_clipped_mono_text_vert_spv_len;
extern const unsigned char src_shaders_build_clipped_mono_text_frag_spv[];
extern const unsigned int  src_shaders_build_clipped_mono_text_frag_spv_len;

int NU_CreateClippedMonoTextPipeline(NuPipeline* pipeline)
{
    return 1;
}

extern const unsigned char src_shaders_build_subpixel_text_vert_spv[];
extern const unsigned int  src_shaders_build_subpixel_text_vert_spv_len;
extern const unsigned char src_shaders_build_subpixel_text_frag_spv[];
extern const unsigned int  src_shaders_build_subpixel_text_frag_spv_len;

int NU_CreateSubpixelTextPipeline(NuPipeline* pipeline)
{
    return 1;
}

extern const unsigned char src_shaders_build_clipped_subpixel_text_vert_spv[];
extern const unsigned int  src_shaders_build_clipped_subpixel_text_vert_spv_len;
extern const unsigned char src_shaders_build_clipped_subpixel_text_frag_spv[];
extern const unsigned int  src_shaders_build_clipped_subpixel_text_frag_spv_len;

int NU_CreateClippedSubpixelTextPipeline(NuPipeline* pipeline)
{
    return 1;
}

extern const unsigned char src_shaders_build_vertex_rgb_vert_spv[];
extern const unsigned int  src_shaders_build_vertex_rgb_vert_spv_len;
extern const unsigned char src_shaders_build_vertex_rgb_frag_spv[];
extern const unsigned int  src_shaders_build_vertex_rgb_frag_spv_len;

int NU_CreateVertexRGBPipeline(DisplayManager* dm, NuPipeline* pipeline, bool MSAA_4X)
{
    // Shader module create info
    VkShaderModuleCreateInfo smci = {0};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    // Create vertex shader module
    VkShaderModule vertModule;
    smci.codeSize = src_shaders_build_vertex_rgb_vert_spv_len;   // works because it's a real array
    smci.pCode = (const u32*)(src_shaders_build_vertex_rgb_vert_spv);
    if (vkCreateShaderModule(dm->device, &smci, NULL, &vertModule) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vertex shader module failed");
        return 0;
    }

    // Create fragment shader module
    VkShaderModule fragModule;
    smci.codeSize = src_shaders_build_vertex_rgb_frag_spv_len;
    smci.pCode = (const u32*)(src_shaders_build_vertex_rgb_frag_spv);
    if (vkCreateShaderModule(dm->device, &smci, NULL, &fragModule) != VK_SUCCESS) {
        vkDestroyShaderModule(dm->device, vertModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> fragment shader module failed");
        return 0;
    }

    // Create shader stages
    VkPipelineShaderStageCreateInfo stages[2] = {0};

    // Vertex shader stage
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName  = "main";

    // Fragment shader stage
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName  = "main";

    // Vertex attribute layout
    VkVertexInputBindingDescription binding = {0};
    binding.binding   = 0;                              // which binding slot
    binding.stride    = sizeof(vertex_rgb);             // 24 bytes per vertex
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;    // not instanced
    VkVertexInputAttributeDescription attrs[2] = {0};
    attrs[0].location = 0;                              // matches layout(location=0) in shader
    attrs[0].binding  = 0;
    attrs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;     // vec3
    attrs[0].offset   = offsetof(vertex_rgb, x);        // 0
    attrs[1].location = 1;                              // matches layout(location=1)
    attrs[1].binding  = 0;
    attrs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;     // vec3
    attrs[1].offset   = offsetof(vertex_rgb, r);        // 12
    VkPipelineVertexInputStateCreateInfo vis = {0};
    vis.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount   = 1;
    vis.pVertexBindingDescriptions      = &binding;
    vis.vertexAttributeDescriptionCount = 2;
    vis.pVertexAttributeDescriptions    = attrs;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo ia = {0};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    ia.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkPipelineViewportStateCreateInfo vps = {0};
    vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vps.viewportCount = 1;
    vps.scissorCount  = 1;
    VkDynamicState dynStates[2] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dyn = {0};
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates    = dynStates;

    // Rasteriser settings
    VkPipelineRasterizationStateCreateInfo rast = {0};
    rast.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.depthClampEnable        = VK_FALSE;
    rast.rasterizerDiscardEnable = VK_FALSE;
    rast.polygonMode             = VK_POLYGON_MODE_FILL;
    rast.cullMode                = VK_CULL_MODE_NONE;
    rast.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rast.depthBiasEnable         = VK_FALSE;
    rast.lineWidth               = 1.0f;

    // Multisample
    VkPipelineMultisampleStateCreateInfo ms = {0};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = MSAA_4X ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;
    ms.sampleShadingEnable  = VK_FALSE;

    // Colour blend
    VkPipelineColorBlendAttachmentState blend = {0};
    blend.blendEnable         = VK_TRUE;
    blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend.colorBlendOp        = VK_BLEND_OP_ADD;
    blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend.alphaBlendOp        = VK_BLEND_OP_ADD;
    blend.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
                                VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT |
                                VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb = {0};
    cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments    = &blend;

    // Pipeline layout
    VkPushConstantRange pcRange = {0};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(float) * 4;    // screenWidth, screenHeight, offsetX, offsetY
    VkPipelineLayoutCreateInfo plci = {0};
    plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount         = 0;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges    = &pcRange;
    if (vkCreatePipelineLayout(dm->device, &plci, NULL, &pipeline->layout) != VK_SUCCESS) {
        vkDestroyShaderModule(dm->device, vertModule, NULL);
        vkDestroyShaderModule(dm->device, fragModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> pipeline layout failed");
        return 0;
    }

    // Create pipeline
    VkGraphicsPipelineCreateInfo gpci = {0};
    gpci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.stageCount          = 2;
    gpci.pStages             = stages;
    gpci.pVertexInputState   = &vis;
    gpci.pInputAssemblyState = &ia;
    gpci.pViewportState      = &vps;
    gpci.pRasterizationState = &rast;
    gpci.pMultisampleState   = &ms;
    gpci.pColorBlendState    = &cb;
    gpci.pDynamicState       = &dyn;
    gpci.layout              = pipeline->layout;
    gpci.renderPass          = dm->sharedRenderPass;
    gpci.subpass             = 0;
    if (vkCreateGraphicsPipelines(dm->device, VK_NULL_HANDLE, 1, &gpci, NULL, &pipeline->handle) != VK_SUCCESS) {
        vkDestroyPipelineLayout(dm->device, pipeline->layout, NULL);
        vkDestroyShaderModule(dm->device, vertModule, NULL);
        vkDestroyShaderModule(dm->device, fragModule, NULL);
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateGraphicsPipelines failed");
        return 0;
    }
    vkDestroyShaderModule(dm->device, vertModule, NULL);
    vkDestroyShaderModule(dm->device, fragModule, NULL);
    return 1;
}





int NU_DestroyPipeline(NuPipeline* pipeline)
{
    return 1;
}
