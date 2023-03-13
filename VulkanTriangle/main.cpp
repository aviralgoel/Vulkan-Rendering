#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif //NDEBUG


class HelloTriangleApplication {
public:
    void run() 
    {   
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    GLFWwindow* m_window;

    // handle to the Vulkan instance
    VkInstance m_instance;

    void initWindow()
    {   
        // initialize glfw library
        glfwInit();

        // let glfw know not to create window for an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // no support for resizing our window
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() 
    {
        createInstance();
    }

    void mainLoop() 
    {   
        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
        }
    }

    void createInstance()
    {   
        // error checking
        if (!checkRequiredValidationLayers() && enableValidationLayers)
        {
            throw std::runtime_error("failed to find required validation layers");
        }
        // Structure specifying application information
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        // must be the highest version of Vulkan that the application is designed to use
        // VK_API_VERSION_1_0 returns the API version number for Vulkan 1.0.0.
        appInfo.apiVersion = VK_API_VERSION_1_0;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        // returns an array of names of Vulkan instance extensions required by GLFW for creating Vulkan surfaces for GLFW windows
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::cout << "Extensions required for glfw\n";
        for (int i = 0; i < glfwExtensionCount; i++)
        {
            std::cout << '\t' << glfwExtensions[i] << '\n';
        }
        // struct to tell the Vulkan driver which global extensions and validation layers we want to use
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        // is NULL or a pointer to a VkApplicationInfo structure. 
        // If not NULL, this information helps implementations recognize behavior inherent to classes of applications
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;

        //VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

        //retrieve a list of supported extensions before creating an instance
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        // Each VkExtensionProperties struct contains the name and version of an extension.
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        // add the required validation layer we are going to use into the struct that will tell Vulkan
        // which validation layers are going to be used
        if(enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }
       
        std::cout << "\navailable extensions:\n";
        for (const VkExtensionProperties& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
        if (!checkRequiredExtentionPresent(extensions, glfwExtensions, glfwExtensionCount))
        {
            throw std::runtime_error("failed to find required extension");
        }

        std::cout << "Current status of validation layers: " << enableValidationLayers << '\n';
        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    bool checkRequiredValidationLayers()
    {   
        // number of available vulkan validation layers
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        // list of all the available layers
        std::vector<VkLayerProperties> availableVKLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableVKLayers.data());

        for (const char* requiredLayer : validationLayers)
        {   
            std::cout << "Now checking for layer: " << requiredLayer << ' ';
            bool layerFound = false;
            for (const VkLayerProperties& availableLayer : availableVKLayers)
            {   
               
                if (strcmp(requiredLayer, availableLayer.layerName)==0)
                {   
                    std::cout << "found\n";
                    layerFound = true; break;
                }
            }
            if (!layerFound)
            {
                return false;
            }
        }
        return true;

    }

    bool checkRequiredExtentionPresent(std::vector<VkExtensionProperties>& availabeExtentensions, const char** requiredExtentions, int requiredExtensionCount) const
    {
        for (int i = 0; i < requiredExtensionCount; i++)
        {
            bool found = false;
            for (const VkExtensionProperties& extension : availabeExtentensions) {
                if (strcmp(extension.extensionName, requiredExtentions[i])==0) found = true;
            }
            if (!found)
            {   
                return found;
                //throw std::runtime_error("missing vulkan extension");
            }
        }
        return true;
    }

    void cleanup() 
    {   
        vkDestroyInstance(m_instance, nullptr);
        glfwDestroyWindow(m_window);
        glfwTerminate();

    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}