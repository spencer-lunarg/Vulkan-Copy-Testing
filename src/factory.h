#include <vulkan/vulkan_core.h>

class Context;

// Vulkan Copy Test
namespace vct {
class Buffer {
  public:
    Buffer(const Context& cx, uint32_t size);
    ~Buffer();
    operator VkBuffer() const { return handle_; }

    void Set(uint32_t pattern);
    void* Map();
    void Unmap();

    void Print(uint32_t bytes = 0);

    const Context& cx_;
    VkBuffer handle_;
    VkDeviceMemory memory_;
    uint32_t size_;
};
}  // namespace vct