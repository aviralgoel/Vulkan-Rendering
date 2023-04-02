#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include<GLFW/glfw3native.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <optional>
#include <set>
#include <limits> // Necessary for std::numeric_limits

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

// names of validation layers to enable
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
// names of required extensions that our physical device must have
const std::vector<const char*> deviceExtensions = {
    // capability of supporting swap chains
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
    // index of family which supports presentation to Windows
    std::optional<uint32_t> presentationFamily;
    bool isComplete()
    {
        return graphicsFamily.has_value() && presentationFamily.has_value();
    }
};

// properties we need our swap chain to support 
struct SwapChainSupportDetails {
    // types of properties
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentationModes;
};

class HelloTriangleApplication {
public:
    // progression of our application startup
    void run() 
    {   
        // a window is created
        initWindow();
        // everything related to vulkan is initialized
        initVulkan();
        // game loop
        mainLoop();
        // everything is closed/destroyed/freed
        cleanup();
    }

private:

    GLFWwindow* m_window;

    // handle to the Vulkan instance
    VkInstance m_instance;
    // handle to the debug callback function
    VkDebugUtilsMessengerEXT debugMessenger;
    // handle to store the select PHYSICAL graphic card for our vulkan instance
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    // a handle to store the logical (virtual) represenation of our selected PHYSICAL device
    VkDevice m_device;
    // a handle to the graphics queue from our logical device
    VkQueue graphicsQueue;
    // a handle to vulkan surface
    VkSurfaceKHR m_surface;
    // a handle to the presentation queue from our logical device
    VkQueue presentationQueue;
    // a handle to the swap chain associated with our vulkan instance
    VkSwapchainKHR m_swapChain;
    // images in the swap chain
    std::vector<VkImage> m_swapChainImages;
    // swap chain related values we will need in the future
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;


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

    // initialize vulkan related stuff
    void initVulkan() 
    {   
        // create a vulkan instance
        createInstance();
        // create a debug "thing"
        setupDebugMessenger();
        // create a surface to paint stuff on using Vulkan
        createSurface();
        // find and set a graphics card on the running machine as our device to do vulkan stuff
        pickPhysicalDevice();
        // create a logical version of the selected physical gpu/device we have selected
        createLogicalDevice();
        // create a swap chain with desiered properties for our vulkan instance
        createSwapChain();
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
    void createSurface()
    {   
        // creates a surface for us, specifically made for windows machine
        if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}

	}

    void pickPhysicalDevice() {
        // how many physical devices/GPUs we have on the system
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        // if none, then vulkan can't be run
        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        // store all available GPUs in the devices array
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        // for each device, whichever is suitable first, set it as our device for vulkan
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

        // if none is suitable, then can't do vulkan stuff
        if (physicalDevice == VK_NULL_HANDLE)
        {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

    }
    bool isDeviceSuitable(VkPhysicalDevice device)
    {   
        // does our physical device have required queue families?
        QueueFamilyIndices indices = findQueueFamilies(device);
        // does our physical device have required extensions available?
        bool extensionSupported = checkDeviceExtentionSupport(device);
        bool swapChainAdequate = false;
        if (extensionSupported)
        {  
            // what are all the swap chain functionalities our device has?
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            // we have atleast one supported image format and one supported presentation mode given the window surface we have
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentationModes.empty();
        }
        // does this device has a queue family which we require?
        return indices.isComplete() && extensionSupported && swapChainAdequate;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {   
        // out of all the available formats which are supported by our physical device (for our given surface)
        // we iterate to find if a specific format is available
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        // if our wish is not fulfilled, we just settle with th first one
        return availableFormats[0];
    }
    VkPresentModeKHR chooseSwapPresentationMode(const std::vector<VkPresentModeKHR>& availablePresentationModes)
    {   
        for (const auto& currentMode : availablePresentationModes)
        {
            if (currentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
                return currentMode;
        }
        // if no better mode is found
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    // ??? 
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
    {   
        if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) 
        {
            return capabilities.currentExtent;
        }
        else 
        {
            int width, height;
            glfwGetFramebufferSize(m_window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void createSwapChain() {
        // what is being supported by our physical device
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
        // configuring the values of the capabilties that our swap chain has
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentationMode(swapChainSupport.presentationModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
        // 
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

        if (indices.graphicsFamily != indices.presentationFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    bool checkDeviceExtentionSupport(VkPhysicalDevice device)
    {   
        // figure out how many and what all extentions are supported by our physical device
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        
        // list of extentions we ACTUALLY require
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // for every available extension
        for (const auto& extension : availableExtensions)
        {   
            // check if it is one of the required one, if yes, cross them off
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {   

        QueueFamilyIndices indices;
        // figure out how many different type of queue families are supported by our selected device
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        // list all of them in a vector
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());



        int i = 0;
        // iterate over every family
        for (const auto& queueFamily : queueFamilies)
        {   
            // if this queueu family supports graphic operations/commands
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
				indices.graphicsFamily = i;
			}
            VkBool32 presentationSupport = false;
            // if this queue family supports presentation to our surface (surface has info about it belonging to windows)
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentationSupport);
            // if yes, store this queueFamilies index
            if (presentationSupport)
            {
                indices.presentationFamily = i;
            }
            if (indices.isComplete()) {
                break;
            }
			i++;
        }
        return indices;
    }
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;
        // what are the basic surface capabilities that our device has?
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
        // what are the surface formars that our device has?
        uint32_t formatCounts;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCounts, nullptr);
        if(formatCounts!=0)
        {
            details.formats.resize(formatCounts);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCounts, details.formats.data());
        }
        // what are the presentation modes that our device has?
        uint32_t presentModeCounts;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCounts, nullptr);
        if (presentModeCounts != 0)
        {
            details.presentationModes.resize(presentModeCounts);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCounts, details.presentationModes.data());
        }
        return details;
    }
    void createLogicalDevice() {
        // queue family indices of the queues inside out physical device which as been selected
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // create multiple queue families in our logical device to support multiple commands
        // such as presentation, graphics, etc
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

        float queuePriority = 1.0f;
        // for every unique element in the set of queue families
        for (uint32_t queueFamily : uniqueQueueFamilies) 
        {
            // create a queue info struct
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1; // we only need 1 queue in this family
			queueCreateInfo.pQueuePriorities = &queuePriority; // default priority
			queueCreateInfos.push_back(queueCreateInfo);
		}
        // info about features we want our logical device to have (currently none)
        VkPhysicalDeviceFeatures deviceFeatures{};

        // creating our logical device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        // its queues
        createInfo.pQueueCreateInfos = queueCreateInfos.data(); // stored at
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); // size of

        // its features
        createInfo.pEnabledFeatures = &deviceFeatures;
        // Enabling the required extensions on the logical device (its extensions count and names)
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        // debugging
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }
        // actually actually creating the logical device
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }
        // retrieving a handle to the queues inside/associated with our logical device
        vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        // call to retrieve the queue handle of presentation queue (we already know the index)
        vkGetDeviceQueue(m_device, indices.presentationFamily.value(), 0, &presentationQueue);

    }

    void cleanup() 
    {   
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        vkDestroyDevice(m_device, nullptr);
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(m_instance, debugMessenger, nullptr);
        }
        // destroy the surface
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
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