#include "factory.h"
#include <vulkan/vulkan_core.h>
#include "context.h"
#include <vulkan/utility/vk_struct_helper.hpp>

namespace vct {

Buffer::Buffer(const Context& cx, uint32_t size) : cx_(cx), size_(size) {
    VkBufferCreateInfo buffer_ci = vku::InitStructHelper();
    buffer_ci.size = size;
    buffer_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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
}  // namespace vct