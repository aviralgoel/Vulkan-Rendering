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
#include <fstream> // shader files i/o
#include <glm/glm.hpp> // vec3 and vec2
#include <array> // std::array
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

// names of validation layers to enable
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
// names of required extensions that our physical device must have
const std::vector<const char*> deviceExtensions = {
	// capability of supporting swap chains
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
// the names of the states that we want to dynamically change in the rendering pipeline
const std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
};

// enable validation layers only when in debug mode
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif //NDEBUG

// input function for reading shader files
static std::vector<char> readFile(const std::string& filename) {
	// read the file in binary mode and start at the end of the file
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	// exception if file is not open
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	// how many bytes are in the file
	size_t fileSize = (size_t)file.tellg();
	// allocate appropriate size of buffer
	std::vector<char> buffer(fileSize);

	// read the file into the buffer from the beginning
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

// functions to create and destroy debug messenger
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

// struct to hold the queue family indices
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	// index of family which supports presentation to Windows
	std::optional<uint32_t> presentationFamily;
	bool isComplete()
	{
		return graphicsFamily.has_value() && presentationFamily.has_value();
	}
};
// properties of the swap chain that is being supported by our physical device
struct SwapChainSupportDetails {
	// types of properties
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentationModes;
};

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoords;

	// populating VkVertexInputBindingDescription struct
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0; // index of the binding in the array of bindings
		bindingDescription.stride = sizeof(Vertex); // number of bytes from one entry to the next
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // move to the next data entry after each vertex
		return bindingDescription;
	}

	// returns an array of 2 elements of type VkVertexInputAttributeDescription
	// one for color and one for position
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		// POSITION ATTRIBUTE
		attributeDescriptions[0].binding = 0; // index of the binding in the array of bindings
		attributeDescriptions[0].location = 0; // location of the attribute in the vertex shader
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // format of the data
		attributeDescriptions[0].offset = offsetof(Vertex, pos); // number of bytes since the start of the per-vertex data to read from

		// COLOR ATTRIBUTE
		attributeDescriptions[1].binding = 0; // index of the binding in the array of bindings
		attributeDescriptions[1].location = 1; // location of the attribute in the vertex shader
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // format of the data
		attributeDescriptions[1].offset = offsetof(Vertex, color); // number of bytes since the start of the per-vertex data to read from

		// TEXTURE ATTRIBUTE
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoords);

		return attributeDescriptions;
	}
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

// vertex information
const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2,
	2, 3, 0
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
	// current frame index
	uint32_t currentFrame = 0;

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
	// image views for the swap chain images
	std::vector<VkImageView> m_swapChainImageViews;
	// handle to render pass object
	VkRenderPass m_renderPass;
	// handle to the descriptor set layout
	VkDescriptorSetLayout m_descriptorSetLayout;
	// handle to uniform values
	VkPipelineLayout m_pipelineLayout;
	// final graphics pipeline
	VkPipeline m_graphicsPipeline;
	// handle to the framebuffers for each swap chain image
	std::vector<VkFramebuffer> m_swapChainFramebuffers;
	// handle to the command pool, bigger object that manages the memory for the command buffers
	VkCommandPool m_commandPool;
	// handle to the command buffer
	//VkCommandBuffer m_commandBuffer;
	std::vector<VkCommandBuffer> m_commandBuffers;
	///////////////////////////
	// semaphores and fences //
	///////////////////////////
	//VkSemaphore m_imageAvailableSemaphore; // an image has been acquired and is ready for rendering
	//VkSemaphore m_renderFinishedSemaphore; // rendering has finished and presentation can happen
	//VkFence m_inFlightFence; // fence to syncronize the cpu and gpu

	std::vector<VkFence> m_inFlightFences; // fences for each frame in flight
	std::vector<VkSemaphore> m_imageAvailableSemaphores; // semaphores for each frame in flight
	std::vector<VkSemaphore> m_renderFinishedSemaphores; // semaphores for each frame in flight

	// flag to check if the window has been resized
	bool framebufferResized = false;

	// handle to store the vertex buffer
	VkBuffer m_vertexBuffer;
	// handle to store the memory for the vertex buffer
	VkDeviceMemory m_vertexBufferMemory;
	// handle to store the index buffer
	VkBuffer m_indexBuffer;
	// handle to store the memory for the index buffer
	VkDeviceMemory m_indexBufferMemory;
	// uniform buffer for every frame in flight
	std::vector<VkBuffer> m_uniformBuffers;
	// memory for the uniform buffer for every frame in flight
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	// mapped memory for the uniform buffer for every frame in flight
	std::vector<void*> m_uniformBuffersData; // pointer to the mapped memory for the uniform buffer for every frame in flight
	VkDescriptorPool m_descriptorPool; // pool of memory for descriptors
	std::vector<VkDescriptorSet> m_descriptorSets;

	// texturing
	VkImage m_textureImage; // handle for the image
	VkDeviceMemory m_textureImageMemory; // memory for the texture image
	VkImageView m_textureImageView; // image view for the texture image
	VkSampler m_textureSampler; // sampler for the texture image

	void initWindow()
	{
		// initialize glfw library
		glfwInit();

		// let glfw know not to create window for an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// no support for resizing our window
		// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// create a window
		m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;

		// Commenting the below code will reproduce the problem
		int _width = 0, _height = 0;
		glfwGetFramebufferSize(app->m_window, &_width, &_height);
		//if (_width == 0 || _height == 0) {
		//	return;
		//}
		//
		//
		//// GLFW halts the event loop while resizing is in progress, but you get a call to the resize callback for every mouse movement.
		//// Two calls because the first after each resize will only recreate the swap chain, need a second call to get the actual render.
		//app->drawFrame();
		//app->drawFrame();
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
		// create image views for the swap chain images
		createImageViews();
		// create a render pass object
		createRenderPass();
		// create descriptor set layout
		createDescriptorSetLayout();
		// create graphics pipeline
		createGraphicsPipeline();
		// create framebuffers
		createFramebuffers();
		// create command pool to manage memory for future command buffers
		createCommandPool();
		// create texture image
		createTextureImage();
		// create texture image view
		createTextureImageView();
		// create texture sampler
		createTextureSampler();
		// create vertex buffer
		createVertexBuffer();
		// create index buffer
		createIndexBuffer();
		// create uniform buffers
		createUniformBuffers();
		// create descriptor pools
		createDescriptorPool();
		// create descriptor sets
		createDescriptorSets();
		// create command buffer
		createCommandBuffers();
		// creating semaphores and fences
		createSyncObjects();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
			drawFrame();
		}
		// wait for the device to finish all operations before exiting
		vkDeviceWaitIdle(m_device);
	}
	/////////////////////////////
	// create a vulkan instance
	/////////////////////////////
	void createInstance()
	{
		//////////////////////////////////////
		//              APPLICATION INFO
		//////////////////////////////////////
		// Structure specifying application information
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle"; // name of the application
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // version of the application (developer supplied)
		appInfo.pEngineName = "No Engine"; // name of the engine used to create the application
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); // version of the engine used to create the application
		appInfo.apiVersion = VK_API_VERSION_1_0; // must be the highest version of Vulkan that the application is designed to use

		// struct to tell the Vulkan driver which global EXTENSIONS and validation LAYERS we want to use
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		// is NULL or a pointer to a VkApplicationInfo structure.
		// If not NULL, this information helps implementations recognize behavior inherent to classes of applications
		createInfo.pApplicationInfo = &appInfo; // pointer to the VkApplicationInfo struct
		//////////////////////////////////////
		//              EXTENSIONS
		//////////////////////////////////////
		// Extentions for the Vulkan instance
		std::vector<const char*> glfwExtensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
		createInfo.ppEnabledExtensionNames = glfwExtensions.data();
		//retrieve a list of supported extensions before creating an instance
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		// Each VkExtensionProperties struct contains the name and version of an extension.
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		std::cout << "\navailable extensions:\n";
		for (const VkExtensionProperties& extension : extensions) {
			std::cout << '\t' << extension.extensionName << '\n';
		}
		if (!checkRequiredExtentionPresent(extensions, glfwExtensions, glfwExtensions.size()))
		{
			throw std::runtime_error("failed to find required extension");
		}
		//////////////////////////////////////
		//              LAYERS
		//////////////////////////////////////

		// add the required validation layer we are going to use into the struct that will tell Vulkan
		// which validation layers are going to be used
		// do we have all the required VALIDATION LAYERS available in this developement environment?
		if (!checkRequiredValidationLayers() && enableValidationLayers)
		{
			throw std::runtime_error("failed to find required validation layers");
		}
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); // number of validation layers
			createInfo.ppEnabledLayerNames = validationLayers.data(); // pointer to the array of validation layers

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}
		std::cout << "Current status of validation layers: " << enableValidationLayers << '\n';

		//////////////////////////////////////
		//              INSTANCE CREATION
		//////////////////////////////////////
		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}
	// lets the vulkan validation layers know what function to call when things go wrong
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		// The debugCallback function will be called by Vulkan whenever a debug message is generated,
		// and the application can implement this function to handle the debug messages according to its specific requirements.
		createInfo.pfnUserCallback = debugCallback;
	}
	// check if the required Validation Layers are present
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

		// for every required layer, check if it is in the list of available layers
		for (const char* requiredLayer : validationLayers)
		{
			std::cout << "Now checking for layer: " << requiredLayer << '\n';
			bool layerFound = false;
			for (const VkLayerProperties& availableLayer : availableVKLayers)
			{
				if (strcmp(requiredLayer, availableLayer.layerName) == 0)
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
	// checks if all the required extensions are present in the list of available extensions
	bool checkRequiredExtentionPresent(std::vector<VkExtensionProperties>& availabeExtentensions,
		std::vector<const char*> requiredExtentions, int requiredExtensionCount) const
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
	// returns a vector of strings (const char*) which are the names of the extensions required by GLFW
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
		// more layers are required for validation
		if (enableValidationLayers)
		{
			std::cout << "Added an additional extension required for validation\n";
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			std::cout << '\t' << extensions[extensions.size() - 1] << '\n';
		}
		return extensions;
	}
	// this function is called by Vulkan whenever a debug message is generated
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		std::cerr << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
	/////////////////////////////////////
	// PREPARE DEBUG MESSENGER FUNCTION
	/////////////////////////////////////
	void setupDebugMessenger()
	{
		// function is not in use if validation layers are not enabled
		if (!enableValidationLayers) return;

		// create a struct that contains information about the debug messenger
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		// links the instance with the debug messenger, when to call = createInfo, what to call = debugMessenger
		if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}
	/////////////////////////////////////
	// CREATE A VULKAN SURFACE
	/////////////////////////////////////
	void createSurface()
	{
		// creates a surface for us, specifically made for windows machine
		// needs: instance to create the surface IN, window to create the surface FOR
		// returns handle to the surface
		if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}
	/////////////////////////////////////
	// PHYSICAL DEVICE (Search, Select, Suitability, Compatability)
	/////////////////////////////////////
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
		// does our physical device have required queue families (graphics and presentation)?
		QueueFamilyIndices indices = findQueueFamilies(device);
		// does our physical device have required extensions available?
		bool extensionSupported = checkDeviceExtentionSupport(device); //  swap chain support
		bool swapChainAdequate = false;
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		// if we have swap chain support then
		if (extensionSupported)
		{
			// at this point we know that swap chain extention is supported by our physical device
			// what are all the swap chain functionalities our device has?
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			// we have atleast one supported image format and one supported presentation mode compatible with the window surface we have
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentationModes.empty();
		}

		// does this device has a queue family which we require, and does it have the required extensions available
		// and does it have a swap chain which is compatible with our surface and window
		return indices.isComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	// does a physical device support the extentions we require?
	bool checkDeviceExtentionSupport(VkPhysicalDevice device)
	{
		// figure out how many and what all extentions are supported by our physical device
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		// list of extentions we ACTUALLY require
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		// iterate over all the available extentions by our physical device
		for (const auto& extension : availableExtensions)
		{
			// check if it is one of the required one, if yes, cross them off
			requiredExtensions.erase(extension.extensionName);
		}

		// true or false
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

	/////////////////////////////////////
	// Swap Chain (Search,
	/////////////////////////////////////

	// create swap chain
	void createSwapChain() {
		// what is being supported by our physical device
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
		// best possible settings for our swap chain
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentationMode(swapChainSupport.presentationModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// recommended to have atleast one more than minimum
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// struct for creating swap chain
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface; // surface we are creating swap chain for
		createInfo.minImageCount = imageCount; // minimum number of images in swap chain
		createInfo.imageFormat = surfaceFormat.format; // format of images in swap chain
		createInfo.imageColorSpace = surfaceFormat.colorSpace; // color space of images in swap chain
		createInfo.imageExtent = extent; // resolution of images in swap chain
		createInfo.imageArrayLayers = 1; // number of layers for each image in swap chain
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // what kind of operations we will use the images in swap chain for
		createInfo.presentMode = presentMode;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

		// is graphic queue different than presentation queue
		if (indices.graphicsFamily != indices.presentationFamily) {
			// don't enforce too strict rules for sharing
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			// be strict about sharing (anyway they are the same queueu) [This is easier to implement]
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // transformation to perform on swap chain images
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // how to handle blending with other windows in window system

		// topics to be learned later
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		// creation!!
		if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		// handle to images in the swap chain
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
		m_swapChainImageFormat = surfaceFormat.format;
		// handle to the extent of swap chain
		m_swapChainExtent = extent;
	}
	// what are the capabilities of the swap chain that our physical device has
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		// what are the basic surface capabilities that we need
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
		// what are the surface formats that we need
		uint32_t formatCounts;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCounts, nullptr);
		if (formatCounts != 0)
		{
			details.formats.resize(formatCounts);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCounts, details.formats.data());
		}
		// what are the presentation modes that we need
		uint32_t presentModeCounts;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCounts, nullptr);
		if (presentModeCounts != 0)
		{
			details.presentationModes.resize(presentModeCounts);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCounts, details.presentationModes.data());
		}
		// this struct has the required settings our swap chain must have to work
		return details;
	}
	// what is the best format for our swap chain images
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
	// what is the best presentation mode for our swap chain images
	VkPresentModeKHR chooseSwapPresentationMode(const std::vector<VkPresentModeKHR>& availablePresentationModes)
	{
		// out of all the available presentation modes which are supported by our physical device (for our given surface)
		// we iterate to find if a specific presentation mode is available
		for (const auto& currentMode : availablePresentationModes)
		{
			if (currentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return currentMode;
		}
		// if no better mode is found
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	// what is the best extent for our swap chain images
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		// if the current extent is not the maximum value, then we can set the extent to whatever we want
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

			// make sure the values from glfw are within the bounds of the capabilities
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}
	void createImageViews() {
		m_swapChainImageViews.resize(m_swapChainImages.size());

		// iterate over every swap chain image
		for (size_t i = 0; i < m_swapChainImages.size(); i++) {
			m_swapChainImageViews[i] = createImageView(m_swapChainImages[i], m_swapChainImageFormat);
		}
	}
	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_swapChainImageFormat; // same as the swap chain image format
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // no multisampling
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear the values to a constant at the start
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // we want to store the rendered contents for displaying it later
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we don't care about the stencil buffer
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // we don't care about the stencil buffer
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // we don't care about the previous layout
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // we want to present the image in the swap chain

		// subpasses and attachment reference
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; // the index of the attachment in the attachment description array
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // the layout the attachment will have during the subpass

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // we are using this subpass in a graphics pipeline
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}
	// how are the descriptors going to be layed out (binding no, total count etc.)
	void createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0; // binding = 0 in the shader
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // type of descriptor
		uboLayoutBinding.descriptorCount = 1; // number of values in the array of descriptors
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // shader stage to bind to
		uboLayoutBinding.pImmutableSamplers = nullptr; // only relevant for image sampling

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		//std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
	void createGraphicsPipeline()
	{
		// piece of code to run for every vertex
		auto vertShaderCode = readFile("shaders/vert.spv");
		// piece of code to run for every fragment (pixel)
		auto fragShaderCode = readFile("shaders/frag.spv");
		//std::cout << "Size of vert shader code: " << vertShaderCode.size() << std::endl;
		//std::cout << "Size of frag shader code: " << fragShaderCode.size() << std::endl;
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode); // create a shader module from the code
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode); // create a shader module from the code

		// create vertex shader stage info
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main"; // entry point

		// create fragment shader stage info
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main"; // entry point

		// array to hold both the stages
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// describe how our vertex data is structured
		VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();

		// tell our graphics pipeline how the vertex data is structured
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

		// settings to configure the assembly of geometry from vertices
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_swapChainExtent.width;
		viewport.height = (float)m_swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChainExtent;

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;

		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// anti aliasing (currently disabled)
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		// color blending first method (disabled)
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		// color blending second method (disabled as well)
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		// pipeline layout (information about descriptor layouts)
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1; // uniform layout
		pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout; // Optional

		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

		if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// graphics pipeline creation
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState; // Optional
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState; // viewport and scissor
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		// destroy shader modules linked to the logical device, since we now havd them in an array already
		vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
		vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
	}
	void createFramebuffers()
	{
		// resize the framebuffer array to fit the number of swapchain images
		m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

		// create a framebuffer for each swapchain image
		for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
			// create a framebuffer for each swapchain image
			VkImageView attachments[] = { m_swapChainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPass; // render pass to use with this framebuffer
			framebufferInfo.attachmentCount = 1; // number of attachments
			framebufferInfo.pAttachments = attachments; // list of attachments
			framebufferInfo.width = m_swapChainExtent.width; // width of the framebuffer
			framebufferInfo.height = m_swapChainExtent.height; // height of the framebuffer
			framebufferInfo.layers = 1; // number of layers

			// create ith framebuffer
			if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}
	void createCommandPool()
	{
		/*	a command pool is a memory pool that is used to manage the memory resources for command buffers.
		Think of a command buffer as a list of instructions that tell the GPU what to do, such as drawing objects or updating buffers.
		The command pool is responsible for allocating and managing the memory used by these command buffers.
		When you create a command pool, you specify the type of command buffer that it will manage, such as graphics or compute.
		This allows the command pool to optimize the memory allocation and management specifically for that type of command buffer.
		By using a command pool, you can efficiently manage the creation and destruction of command buffers without having to manually allocate
		and deallocate memory each time. This can improve the performance of your application by reducing the overhead of memory management
		and allowing the GPU to process commands more efficiently.
		*/
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow command buffers to be reset
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(); // command pool is for graphics queue
		if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}
	// load an image from file and upload it to Vulkan Image Object
	void createTextureImage()
	{
		// store texture image width, height and number of color channels
		int texWidth, texHeight, texChannels;
		// load the image data from file
		stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		// calculate image size in bytes
		VkDeviceSize imageSize = texWidth * texHeight * 4; // 4 bytes per pixel

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		// VkBuffer is a logical abstraction for data storage, whereas VkDeviceMemory is a physical representation of memory on the device.

		// create a staging buffer (on CPU ) to copy the image data to before copying to the device local buffer (on GPU)
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		// create the staging buffer (required size of the buffer, will be used as SOURCE of a transfer command,
		// can be used for mapping and flushing not necceary, output results)
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		// map staging buffer memory to a pointer so we can copy image data to it
		// The reason for mapping and unmapping memory is to ensure that the CPU and GPU can access the same memory regions without conflicts. 
		void* data;
		vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_device, stagingBufferMemory);

		// free the image data
		stbi_image_free(pixels);

		// create the image
		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_textureImage, m_textureImageMemory);

		// change the layout of the image from old to a new one which is better for GPU
		transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		// copy image data to VkImage object
		copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		// change the layout of this image object for opitmal GPU access
		transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);
	}
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1, };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer);
	}
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling imageTiling, VkImageUsageFlags usageFlags,
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{	
		// create an image object 
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D; // 2D image
		imageInfo.extent.width = width; // width of image
		imageInfo.extent.height = height; // height of image
		imageInfo.extent.depth = 1; // depth of image (1 for 2D image)
		imageInfo.mipLevels = 1; // number of mipmap levels (1 = currently no mipmapping)
		imageInfo.arrayLayers = 1; // number of layers in image array (1 = currently texture is not an array)
		imageInfo.format = format; // format of texels (pixels)
		imageInfo.tiling = imageTiling; // tiling of image data
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout of image data on creation
		imageInfo.usage = usageFlags; // how image will be used
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// create the image
		if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		// actually creating internal memory for the image object
		// settings for allocate memory for the image
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size; // size of memory allocation
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties); // index of memory type to use

		// allocate memory
		if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		// bind the image to the allocated memory
		vkBindImageMemory(m_device, image, imageMemory, 0);
	}
	void createTextureImageView()
	{
		m_textureImageView = createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB);

	}
	VkImageView createImageView(VkImage image, VkFormat format)
	{	
		VkImageView imageView;
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // color aspect of image
		viewInfo.subresourceRange.baseMipLevel = 0; // start at mip level 0
		viewInfo.subresourceRange.levelCount = 1; // 1 mip level
		viewInfo.subresourceRange.baseArrayLayer = 0; // start at array layer 0
		viewInfo.subresourceRange.layerCount = 1; // 1 layer

		if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
		return imageView;
	}
	void createTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR; // how to interpolate texels that are magnified on screen
		samplerInfo.minFilter = VK_FILTER_LINEAR; // how to interpolate texels that are minified on screen

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // how to handle texture coordinates outside of [0,1] range in U direction
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // how to handle texture coordinates outside of [0,1] range in V direction
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // how to handle texture coordinates outside of [0,1] range in W direction

		samplerInfo.anisotropyEnable = VK_TRUE; // enable anisotropic filtering

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // max amount of texel samples used to calculate final color
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // border beyond texture (only works for border clamp)
		samplerInfo.unnormalizedCoordinates = VK_FALSE; // whether to use normalized texture coordinates
		samplerInfo.compareEnable = VK_FALSE; // whether to compare texel before final sampling
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS; // comparison operation to use

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		// create sampler
		if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

	}
	void createVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		// createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//	m_vertexBuffer, m_vertexBufferMemory);
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		// create a staging buffer to copy vertex data to before copying to the device local buffer
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		// map staging buffer memory to a pointer so we can copy vertex data to it
		void* data;
		vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_device, stagingBufferMemory);

		// create a vertex buffer on the device local memory
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

		// copy the staging buffer to the vertex buffer
		copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);
	}
	void createIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		// create a staging buffer to copy index data to before copying to the device local buffer
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		// map staging buffer memory to a pointer so we can copy index data to it
		void* data;
		vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_device, stagingBufferMemory);

		// create a index buffer on the device local memory
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

		// copy the staging buffer to the vertex buffer
		copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);


	}
	void createUniformBuffers() {
		VkDeviceSize size = sizeof(UniformBufferObject);
		m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBuffersData.resize(MAX_FRAMES_IN_FLIGHT);

		// create a uniform buffer for each frame in flight
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				m_uniformBuffers[i], m_uniformBuffersMemory[i]);
			vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, size, 0, &m_uniformBuffersData[i]);
		}
	}
	void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
	{
		// make a one time command buffer to submit copy command
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // soruce buffer offset
		copyRegion.dstOffset = 0; // destination buffer offset
		copyRegion.size = size; // size of data to copy

		// command to copy data from one buffer to other buffer
		vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

		// end the command buffer
		endSingleTimeCommands(commandBuffer);
	}
	// create a single use command buffer from the command pool 
	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		// the command buffer is ready to accept commands, and any subsequent Vulkan commands issued will be recorded into the buffer 
		// until vkEndCommandBuffer is called to finalize the buffer recording.
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}
	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {

		// end accepting more commands into the command buffer
		vkEndCommandBuffer(commandBuffer);


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// submit whatever commands are in the commandBuffer to the queue
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		// destroy the command buffer
		vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
	}
	// converts an VkImage object with a particular format and a layout to another VkImage object with a different layout
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		
		// start a command buffer
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // used for transfer queue ownership
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // not applicable in our case
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0; 
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		// will happen when we copy data from CPU to VkImage
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; 
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		// will happen when we copy data from VkImage to GPU memory
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer, // 
			sourceStage, // in which pipeline stage the operations occur that should happen before the barrier
			destinationStage,  // specifies the pipeline stage in which operations will wait on the barrier. 
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}
	// it creates a buffer based on the size required, the intended use of the buffer, and the properties of the memory
	// last two parameters are the buffer and the memory object which are returned
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		// creating a buffer requires us to create VkBufferCreateInfo struct
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size; // size of the buffer
		bufferInfo.usage = usage; // buffer is used as a vertex buffer / staging buffer
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue (graphics) family at a time

		// create vertex/staging buffer
		if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		// actually assign memory to it
		VkMemoryRequirements memRequirements;
		// give our buffer what kind of memory it needs
		vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size; // size of required memory in bytes
		// bitmask, Bit i is set iff the memory type i is supported
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		// device that owns the memory, allocation info like memory type and size, handle to store the memory
		if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		// link the buffer with actual raw memory
		vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
	}
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		// what kinds of memory does our GPU offer
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// find a memory type that is suitable for the buffer
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			// is the memory type i suitable for the buffer and does it have the properties we want
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
	}
	void createDescriptorPool() {

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		// create the pool
		if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}
	void createDescriptorSets()
	{
		// array of descriptor set layout
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

		// allocate the descriptor sets
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool; // pool to allocate from
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data(); // array of layouts

		// allocate the descriptor sets
		m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		// POOL -> LAYOUT -> SETS -> BINDING -> BUFFER -> MEMORY
		// CONFIGURE EACH DESCRIPTOR SET
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_textureImageView;
			imageInfo.sampler = m_textureSampler;

			// VkWriteDescriptorSet descriptorWrite{};
			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data() , 0, nullptr);
		}
	}
	void createCommandBuffers()
	{
		// resize the command buffer array to number of inflight frames allowed
		m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		// we have two command buffers now beacause MAX_FRAMES_IN_FLIGHT = 2
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

		if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
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
		// info about features we want our logical device to have (currently only anistropy filtering)
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		// creating our logical device
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		// its queues
		createInfo.pQueueCreateInfos = queueCreateInfos.data(); // stored at
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); // size of

		// its features
		createInfo.pEnabledFeatures = &deviceFeatures;

		// Enabling the required extensions on the logical device (its extensions count and names)
		// isDeviceSuitable() already makes sure that these extensions are supported by our physical device
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
	// take raw shader bytecode and create a shader module (basically wrap it)
	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		// reinterpret cast basically changes the type of the pointer
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}
	// writes the commands we want to execute into a command buffer
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		//  begin recording the command buffer by calling vkBeginCommandBuffer
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		// start a render pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainExtent;

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// begin the render pass
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		// The vkCmdBindVertexBuffers function is used to bind vertex buffers to bindings,
		// like the one we set up in the previous chapter. The first two parameters, besides the command buffer,
		// specify the offset and number of bindings we're going to specify vertex buffers for.
		// The last two parameters specify the array of vertex buffers to bind and
		// the byte offsets to start reading vertex data from.
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		// set dynamic viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapChainExtent.width);
		viewport.height = static_cast<float>(m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// bind the descriptor sets
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[currentFrame], 0, nullptr);

		// actual draw call
		// vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		// end the render pass
		vkCmdEndRenderPass(commandBuffer);

		// end recording the command buffer
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void createSyncObjects() {
		// resize the vectors to hold the semaphores and fences
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		// create a semaphore info struct
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// create a fence info struct
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // set the fence to be signaled at creation [Green Light]

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	void drawFrame()
	{
		// wait for the fence to be signaled [Green light that previous frame has finished and new frame rendering can begin]
		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		// acquire an image from the swap chain, when done, signal the semaphore ON
		// has the swapchain requirements changed?
		VkResult swapChainScore = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[currentFrame],
			VK_NULL_HANDLE, &imageIndex);

		// YES, make a new swap chain
		if (swapChainScore == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain(); return;
		}
		// No but some other kinds of errors
		else if (swapChainScore != VK_SUCCESS && swapChainScore != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		// Switch on Red light for the fence
		vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);

		// now let us render to this image
		// we need to specify which command buffer to use for this image
		// we can use the image index to select the command buffer
		vkResetCommandBuffer(m_commandBuffers[currentFrame], 0);
		// begin recording the command buffer
		recordCommandBuffer(m_commandBuffers[currentFrame], imageIndex);
		// update the uniform buffer (MVP matrix)
		updateUniformBuffer(currentFrame);

		// submit the command buffer to the graphics queue
		// we need to specify which semaphores to wait on before execution and which to signal when execution is done
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		// wait on the image available semaphore
		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame] }; // wait for these to be signaled
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		// submit the command buffer
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[currentFrame];

		// signal the render finished semaphore when done
		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] }; // signal them when done
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// submit the command buffer to the graphics queue
		// m_inFlightFence will be GREEN when all submitted command buffers have finished execution
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// now we need to present the rendered image to the screen
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		// wait on the render finished semaphore
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores; // specifies the semaphores to wait for before issuing the present request.

		// specify the swap chain to present to
		VkSwapchainKHR swapChains[] = { m_swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		// specify the image index to present
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // optional
		// present the image
		// present the image
		VkResult result = vkQueuePresentKHR(presentationQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	/// <summary>
	///  Clean up the Vulkan objects
	/// </summary>
	void cleanup()
	{
		// destroy the swap chain
		cleanupSwapChain();

		vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
		// delete the pipeline layout (uniforms)
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		// delete the render pass object
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);

		// delete the vertex buffer and its memory
		vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
		vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

		// delete the vertex buffer and its memory
		vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
		vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

		// delete all uniform buffers stuff
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
			vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
		}
		vkDestroySampler(m_device, m_textureSampler, nullptr);
		vkDestroyImageView(m_device, m_textureImageView, nullptr);
		vkDestroyImage(m_device, m_textureImage, nullptr);
		vkFreeMemory(m_device, m_textureImageMemory, nullptr);


		vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

		// delete the semaphores and fence
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
		}

		// delete the graphics command pool
		vkDestroyCommandPool(m_device, m_commandPool, nullptr);
		// delete the logical device
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

	void recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(m_window, &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(m_device);
		cleanupSwapChain();
		createSwapChain();
		createImageViews();
		createFramebuffers();
	}
	void cleanupSwapChain()
	{
		// delete all the frame buffers associated with every swpachain image
		for (auto framebuffer : m_swapChainFramebuffers)
		{
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}

		// delete all swap chain image views
		for (auto imageView : m_swapChainImageViews) {
			vkDestroyImageView(m_device, imageView, nullptr);
		}
		// delete the swap chain itself
		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
	}

	void updateUniformBuffer(uint32_t currentImage)
	{
		// update happen independent of frame rate
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float timeElapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), timeElapsed * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // rotate 90 degrees per second around z axis
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), // camera position
			glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
			glm::vec3(0.0f, 0.0f, 1.0f)); // up vector
		ubo.proj = glm::perspective(glm::radians(45.0f), // field of view
			m_swapChainExtent.width / (float)m_swapChainExtent.height, // aspect ratio
			0.1f, // near plane
			10.0f); // far plane
		ubo.proj[1][1] *= -1;

		// copy the updated MVP matrix to the uniform buffer memory
		memcpy(m_uniformBuffersData[currentImage], &ubo, sizeof(ubo));
	}
};

int main() {
	// to adhere to RAII principle
	HelloTriangleApplication app;

	try {
		app.run();
	}
	// stop execution as soon as something goes wrong and print the error message
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}