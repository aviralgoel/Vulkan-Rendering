#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <optional>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

// names of validation layers to enable
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// enable validation layers only when in debug mode 
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif //NDEBUG


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    
    // returns a function pointer to the specified function for the given instance
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    
    // success if function pointer is not null
    if (func != nullptr) {
        // vkCreateDebugUtilsMessengerEXT(instance, pcreateinfo, pAllocator, pMessenger)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    bool isComplete()
    {
        return graphicsFamily.has_value();
    }
};
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
    // handle to the debug callback function
    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;


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
        setupDebugMessenger();
        pickPhysicalDevice();
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
        // if required validation layers are not found
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

        std::vector<const char*> glfwExtensions = getRequiredExtensions();

        // struct to tell the Vulkan driver which global extensions and validation layers we want to use
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        // is NULL or a pointer to a VkApplicationInfo structure. 
        // If not NULL, this information helps implementations recognize behavior inherent to classes of applications
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
        createInfo.ppEnabledExtensionNames = glfwExtensions.data();

        //VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

        //retrieve a list of supported extensions before creating an instance
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        // Each VkExtensionProperties struct contains the name and version of an extension.
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};


        // add the required validation layer we are going to use into the struct that will tell Vulkan
        // which validation layers are going to be used
        if(enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;

        }
       
        std::cout << "\navailable extensions:\n";
        for (const VkExtensionProperties& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
        if (!checkRequiredExtentionPresent(extensions, glfwExtensions, glfwExtensions.size()))
        {
            throw std::runtime_error("failed to find required extension");
        }

        std::cout << "Current status of validation layers: " << enableValidationLayers << '\n';
        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }


    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    bool checkRequiredValidationLayers()
    {   
        // number of available vulkan validation layers
        uint32_t layerCount;
        // return how many layers are available
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        // list of all the available layers
        std::vector<VkLayerProperties> availableVKLayers(layerCount);
        // return the list of all the available layers
        vkEnumerateInstanceLayerProperties(&layerCount, availableVKLayers.data());

        // for evert required layer, check if it is in the list of available layers
        for (const char* requiredLayer : validationLayers)
        {   
            std::cout << "Now checking for layer: " << requiredLayer << '\n';
            bool layerFound = false;
            for (const VkLayerProperties& availableLayer : availableVKLayers)
            {   
               
                if (strcmp(requiredLayer, availableLayer.layerName)==0)
                {   
                    std::cout << "found: " << requiredLayer << '\n';
                    layerFound = true; 
                    break;
                }
            }
            if (!layerFound)
            {   
                std::cout << "did not find the layer\n";
                return false;
            }
        }
        return true;

    }

    bool checkRequiredExtentionPresent(std::vector<VkExtensionProperties>& availabeExtentensions, std::vector<const char*> requiredExtentions, int requiredExtensionCount) const
    {
        for (int i = 0; i < requiredExtensionCount; i++)
        {
            std::cout << "checking for extension:  " << requiredExtentions[i];
            bool found = false;
            for (const VkExtensionProperties& extension : availabeExtentensions) {
                if (strcmp(extension.extensionName, requiredExtentions[i]) == 0)
                {   
                    std::cout << "found\n";
                    found = true;
                    break;
                }
            }
            if (!found)
            {   
                return found;
                //throw std::runtime_error("missing vulkan extension");
            }
        }
        return true;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        // pointer to a char* (string) 
        const char** glfwExtensions;
        // returns an array of names of Vulkan instance extensions required by GLFW for creating Vulkan surfaces for GLFW windows
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        // initialize a vector which picks elements from the array of strings (clever pointer arithematic)
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        std::cout << "Extensions required for glfw\n";
        for (int i = 0; i < glfwExtensionCount; i++)
        {
            std::cout << '\t' << glfwExtensions[i] << '\n';
        }
        if (enableValidationLayers)
        {   
            std::cout << "Added an additional extension required for validation\n";
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            std::cout << '\t' << extensions[extensions.size()-1] << '\n';
        }
        return extensions;

    }
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void setupDebugMessenger()
    {   
        // function is not in use if validation layers are not enabled
        if (!enableValidationLayers) return;

        // create a struct that contains information about the debug messenger
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }

    }

    void pickPhysicalDevice() {

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}
        if (physicalDevice == VK_NULL_HANDLE)
        {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

    }
    bool isDeviceSuitable(VkPhysicalDevice device)
    {   
        QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete();
    }
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
				indices.graphicsFamily = i;
			}
            if (indices.isComplete()) {
                break;
            }
			i++;
        }
        return indices;
    }

    void cleanup() 
    {   
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(m_instance, debugMessenger, nullptr);
        }
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