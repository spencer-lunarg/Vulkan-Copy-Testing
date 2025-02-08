add_executable(VulkanCopyTesting)

target_sources(VulkanCopyTesting PRIVATE
    context.h
    context.cpp
    factory.h
    factory.cpp
    main.cpp)

target_link_libraries(VulkanCopyTesting PRIVATE vk-bootstrap::vk-bootstrap Vulkan::UtilityHeaders Vulkan::Vulkan)