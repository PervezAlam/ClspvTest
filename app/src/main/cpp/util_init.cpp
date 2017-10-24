/*
 * Vulkan Samples
 *
 * Copyright (C) 2015-2016 Valve Corporation
 * Copyright (C) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
VULKAN_SAMPLE_DESCRIPTION
samples "init" utility functions
*/

#include <cstdlib>
#include <assert.h>
#include <string.h>
#include "util_init.hpp"

using namespace std;

/*
 * TODO: function description here
 */
VkResult init_global_extension_properties(layer_properties &layer_props) {
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count;
    VkResult res;
    char *layer_name = NULL;

    layer_name = layer_props.properties.layerName;

    do {
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (res) return res;

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }

        layer_props.extensions.resize(instance_extension_count);
        instance_extensions = layer_props.extensions.data();
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, instance_extensions);
    } while (res == VK_INCOMPLETE);

    return res;
}

/*
 * TODO: function description here
 */
VkResult init_global_layer_properties(struct sample_info &info) {
    uint32_t instance_layer_count;
    VkLayerProperties *vk_props = NULL;
    VkResult res;
    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for VkLayerProperties. The loader indicates that
     * by returning a VK_INCOMPLETE status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
     */
    do {
        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        if (res) return res;

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }

        vk_props = (VkLayerProperties *)realloc(vk_props, instance_layer_count * sizeof(VkLayerProperties));

        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
    } while (res == VK_INCOMPLETE);

    /*
     * Now gather the extension list for each instance layer.
     */
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = vk_props[i];
        res = init_global_extension_properties(layer_props);
        if (res) return res;
        info.instance_layer_properties.push_back(layer_props);
    }
    free(vk_props);

    return res;
}

/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
VkBool32 demo_check_layers(const std::vector<layer_properties> &layer_props, const std::vector<const char *> &layer_names) {
    uint32_t check_count = layer_names.size();
    uint32_t layer_count = layer_props.size();
    for (uint32_t i = 0; i < check_count; i++) {
        VkBool32 found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(layer_names[i], layer_props[j].properties.layerName)) {
                found = 1;
            }
        }
        if (!found) {
            std::cout << "Cannot find layer: " << layer_names[i] << std::endl;
            return 0;
        }
    }
    return 1;
}

VkResult init_instance(struct sample_info &info, char const *const app_short_name) {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = app_short_name;
    app_info.applicationVersion = 1;
    app_info.pEngineName = app_short_name;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = info.instance_layer_names.size();
    inst_info.ppEnabledLayerNames = info.instance_layer_names.size() ? info.instance_layer_names.data() : NULL;
    inst_info.enabledExtensionCount = info.instance_extension_names.size();
    inst_info.ppEnabledExtensionNames = info.instance_extension_names.data();

    VkResult res = vkCreateInstance(&inst_info, NULL, &info.inst);
    assert(res == VK_SUCCESS);

    return res;
}

VkResult init_device(struct sample_info &info) {
    VkResult res;
    VkDeviceQueueCreateInfo queue_info = {};

    float queue_priorities[1] = {0.0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;
    queue_info.queueFamilyIndex = info.graphics_queue_family_index;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = info.device_extension_names.size();
    device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? info.device_extension_names.data() : NULL;
    device_info.pEnabledFeatures = NULL;

    res = vkCreateDevice(info.gpus[0], &device_info, NULL, &info.device);
    assert(res == VK_SUCCESS);

    return res;
}

VkResult init_enumerate_device(struct sample_info &info, uint32_t gpu_count) {
    uint32_t const U_ASSERT_ONLY req_count = gpu_count;
    VkResult res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, NULL);
    assert(gpu_count);
    info.gpus.resize(gpu_count);

    res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, info.gpus.data());
    assert(!res && gpu_count >= req_count);

    vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0], &info.queue_family_count, NULL);
    assert(info.queue_family_count >= 1);

    info.queue_props.resize(info.queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0], &info.queue_family_count, info.queue_props.data());
    assert(info.queue_family_count >= 1);

    /* This is as good a place as any to do this */
    vkGetPhysicalDeviceMemoryProperties(info.gpus[0], &info.memory_properties);
    vkGetPhysicalDeviceProperties(info.gpus[0], &info.gpu_props);

    return res;
}

VkResult init_debug_report_callback(struct sample_info &info, PFN_vkDebugReportCallbackEXT dbgFunc) {
    VkResult res;
    VkDebugReportCallbackEXT debug_report_callback;

    info.dbgCreateDebugReportCallback =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(info.inst, "vkCreateDebugReportCallbackEXT");
    if (!info.dbgCreateDebugReportCallback) {
        std::cout << "GetInstanceProcAddr: Unable to find "
                     "vkCreateDebugReportCallbackEXT function."
                  << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    std::cout << "Got dbgCreateDebugReportCallback function\n";

    info.dbgDestroyDebugReportCallback =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(info.inst, "vkDestroyDebugReportCallbackEXT");
    if (!info.dbgDestroyDebugReportCallback) {
        std::cout << "GetInstanceProcAddr: Unable to find "
                     "vkDestroyDebugReportCallbackEXT function."
                  << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    std::cout << "Got dbgDestroyDebugReportCallback function\n";

    VkDebugReportCallbackCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    create_info.pNext = NULL;
    create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    create_info.pfnCallback = dbgFunc;
    create_info.pUserData = NULL;

    res = info.dbgCreateDebugReportCallback(info.inst, &create_info, NULL, &debug_report_callback);
    switch (res) {
        case VK_SUCCESS:
            std::cout << "Successfully created debug report callback object\n";
            info.debug_report_callbacks.push_back(debug_report_callback);
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            std::cout << "dbgCreateDebugReportCallback: out of host memory pointer\n" << std::endl;
            return VK_ERROR_INITIALIZATION_FAILED;
            break;
        default:
            std::cout << "dbgCreateDebugReportCallback: unknown failure\n" << std::endl;
            return VK_ERROR_INITIALIZATION_FAILED;
            break;
    }
    return res;
}

void destroy_debug_report_callback(struct sample_info &info) {
    while (info.debug_report_callbacks.size() > 0) {
        info.dbgDestroyDebugReportCallback(info.inst, info.debug_report_callbacks.back(), NULL);
        info.debug_report_callbacks.pop_back();
    }
}

void init_command_pool(struct sample_info &info) {
    /* DEPENDS on init_swapchain_extension() */
    VkResult U_ASSERT_ONLY res;

    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = NULL;
    cmd_pool_info.queueFamilyIndex = info.graphics_queue_family_index;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    res = vkCreateCommandPool(info.device, &cmd_pool_info, NULL, &info.cmd_pool);
    assert(res == VK_SUCCESS);
}

void init_device_queue(struct sample_info &info) {
    /* DEPENDS on init_swapchain_extension() */

    vkGetDeviceQueue(info.device, info.graphics_queue_family_index, 0, &info.graphics_queue);
    if (info.graphics_queue_family_index == info.present_queue_family_index) {
        info.present_queue = info.graphics_queue;
    } else {
        vkGetDeviceQueue(info.device, info.present_queue_family_index, 0, &info.present_queue);
    }
}

void destroy_descriptor_pool(struct sample_info &info) { vkDestroyDescriptorPool(info.device, info.desc_pool, NULL); }

void destroy_command_pool(struct sample_info &info) { vkDestroyCommandPool(info.device, info.cmd_pool, NULL); }

void destroy_device(struct sample_info &info) {
    vkDeviceWaitIdle(info.device);
    vkDestroyDevice(info.device, NULL);
}

void destroy_instance(struct sample_info &info) { vkDestroyInstance(info.inst, NULL); }


