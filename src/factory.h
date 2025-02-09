#include <vulkan/vulkan_core.h>
#include <string>

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

class Image {
  public:
    Image(const Context& cx, VkImageType image_type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage);
    ~Image();
    operator VkImage() const { return handle_; }

    const Context& cx_;
    VkImage handle_;
    VkImageView view_;
    VkDeviceMemory memory_;
    VkImageLayout layout_;
};

class DescriptorSet {
  public:
    DescriptorSet(const Context& cx);
    ~DescriptorSet();
    operator VkDescriptorSet() const { return handle_; }

    void Update(uint32_t binding, VkBuffer buffer, VkDeviceSize range = VK_WHOLE_SIZE);
    void Update(uint32_t binding, vct::Image& image);

    const Context& cx_;
    VkDescriptorSet handle_;
    VkDescriptorPool pool_;
    VkDescriptorSetLayout layout_;
};

// Compute pipeline
class Pipeline {
  public:
    Pipeline(const Context& cx, VkDescriptorSetLayout dsl, const std::string& shader);
    ~Pipeline();
    operator VkPipeline() const { return handle_; }

    const Context& cx_;
    VkPipeline handle_;
    VkPipelineLayout layout_;
};

}  // namespace vct