#include <vulkan/vulkan.h>
#include <stdio.h>

int main() {
    printf("Testing bare minimum Vulkan instance creation...\n");
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    
    printf("Result: %d\n", result);
    
    if (result == VK_SUCCESS) {
        printf("Success! Destroying instance...\n");
        vkDestroyInstance(instance, nullptr);
        return 0;
    }
    
    printf("Failed!\n");
    return 1;
}
