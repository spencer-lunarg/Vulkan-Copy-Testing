#include "factory.h"
#include <vulkan/vulkan_core.h>
#include "context.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vulkan/utility/vk_struct_helper.hpp>

namespace vct {

Buffer::Buffer(const Context& cx, uint32_t size) : cx_(cx), size_(size) {
    VkBufferCreateInfo buffer_ci = vku::InitStructHelper();
    buffer_ci.size = size;
    buffer_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    cx_.dt_.createBuffer(&buffer_ci, nullptr, &handle_);

    VkMemoryRequirements reqs;
    cx_.dt_.getBufferMemoryRequirements(handle_, &reqs);

    VkMemoryAllocateInfo allocate_info = vku::InitStructHelper();
    allocate_info.allocationSize = reqs.size;
    allocate_info.memoryTypeIndex = cx_.GetHostVisibleMemoryIndex(reqs.memoryTypeBits);
    cx_.dt_.allocateMemory(&allocate_info, nullptr, &memory_);

    cx_.dt_.bindBufferMemory(handle_, memory_, 0);
}

Buffer::~Buffer() {
    if (memory_ != VK_NULL_HANDLE) {
        cx_.dt_.freeMemory(memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
    if (handle_ != VK_NULL_HANDLE) {
        cx_.dt_.destroyBuffer(handle_, nullptr);
        handle_ = VK_NULL_HANDLE;
    }
}

void Buffer::Set(uint32_t pattern) {
    void* data = Map();
    memset(data, pattern, size_);
    Unmap();
}

void* Buffer::Map() {
    void* data;
    cx_.dt_.mapMemory(memory_, 0, VK_WHOLE_SIZE, 0, &data);
    return data;
}
void Buffer::Unmap() { cx_.dt_.unmapMemory(memory_); }

void Buffer::Print(uint32_t bytes) {
    if (bytes == 0) {
        bytes = size_;
    }
    const uint32_t dwords = bytes / 4;

    uint32_t* data = static_cast<uint32_t*>(Map());
    for (uint32_t i = 0; i < dwords; i++) {
        printf("[%u]\t(0x%x)\n", i, data[i]);
    }

    Unmap();
}

DescriptorSet::DescriptorSet(const Context& cx) : cx_(cx) {
    VkDescriptorPoolSize pool_sizes[2] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
    };

    VkDescriptorPoolCreateInfo pool_ci = vku::InitStructHelper();
    pool_ci.flags = 0;
    pool_ci.maxSets = 1;
    pool_ci.poolSizeCount = 2;
    pool_ci.pPoolSizes = pool_sizes;
    cx_.dt_.createDescriptorPool(&pool_ci, nullptr, &pool_);

    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_bindings = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_ALL, nullptr},
    };
    VkDescriptorSetLayoutCreateInfo dsl_ci = vku::InitStructHelper();
    dsl_ci.flags = 0;
    dsl_ci.bindingCount = static_cast<uint32_t>(descriptor_set_bindings.size());
    dsl_ci.pBindings = descriptor_set_bindings.data();
    cx_.dt_.createDescriptorSetLayout(&dsl_ci, nullptr, &layout_);

    VkDescriptorSetAllocateInfo ds_alloc_info = vku::InitStructHelper();
    ds_alloc_info.descriptorPool = pool_;
    ds_alloc_info.descriptorSetCount = 1;
    ds_alloc_info.pSetLayouts = &layout_;
    cx_.dt_.allocateDescriptorSets(&ds_alloc_info, &handle_);
}

DescriptorSet::~DescriptorSet() {
    if (pool_ != VK_NULL_HANDLE) {
        cx_.dt_.destroyDescriptorPool(pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
    }
    if (layout_ != VK_NULL_HANDLE) {
        cx_.dt_.destroyDescriptorSetLayout(layout_, nullptr);
        layout_ = VK_NULL_HANDLE;
    }
}

void DescriptorSet::Update(uint32_t binding, VkBuffer buffer, VkDeviceSize range) {
    VkDescriptorBufferInfo buffer_info = {buffer, 0, range};
    VkWriteDescriptorSet write_ds = vku::InitStructHelper();
    write_ds.dstSet = handle_;
    write_ds.dstBinding = binding;
    write_ds.dstArrayElement = 0;
    write_ds.descriptorCount = 1;
    write_ds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_ds.pBufferInfo = &buffer_info;
    cx_.dt_.updateDescriptorSets(1, &write_ds, 0, nullptr);
}

static void LoadSpirv(const std::string& shader, std::vector<uint32_t>& spirv) {
    std::filesystem::path filePath = std::filesystem::canonical("/proc/self/exe").parent_path().parent_path() / "shaders" / shader;
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    spirv.resize(size / sizeof(uint32_t));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(spirv.data()), size);
}

Pipeline::Pipeline(const Context& cx, VkDescriptorSetLayout dsl, const std::string& shader) : cx_(cx) {
    std::vector<uint32_t> spirv;
    LoadSpirv(shader, spirv);

    VkShaderModuleCreateInfo module_ci = vku::InitStructHelper();
    module_ci.pCode = spirv.data();
    module_ci.codeSize = spirv.size() * sizeof(uint32_t);

    VkPipelineShaderStageCreateInfo stage_ci = vku::InitStructHelper(&module_ci);
    stage_ci.flags = 0;
    stage_ci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_ci.module = VK_NULL_HANDLE;
    stage_ci.pName = "main";
    stage_ci.pSpecializationInfo = nullptr;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = vku::InitStructHelper();
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &dsl;
    pipeline_layout_ci.pushConstantRangeCount = 0;
    cx_.dt_.createPipelineLayout(&pipeline_layout_ci, nullptr, &layout_);

    VkComputePipelineCreateInfo pipe_ci = vku::InitStructHelper();
    pipe_ci.flags = 0;
    pipe_ci.stage = stage_ci;
    pipe_ci.layout = layout_;
    pipe_ci.basePipelineHandle = VK_NULL_HANDLE;

    if (cx.dt_.createComputePipelines(VK_NULL_HANDLE, 1, &pipe_ci, nullptr, &handle_) != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline\n";
    }
}

Pipeline::~Pipeline() {
    if (handle_ != VK_NULL_HANDLE) {
        cx_.dt_.destroyPipeline(handle_, nullptr);
        handle_ = VK_NULL_HANDLE;
    }
    if (layout_ != VK_NULL_HANDLE) {
        cx_.dt_.destroyPipelineLayout(layout_, nullptr);
        layout_ = VK_NULL_HANDLE;
    }
}

}  // namespace vct