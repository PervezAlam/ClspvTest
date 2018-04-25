//
// Created by Eric Berdahl on 10/31/17.
//

#ifndef CLSPVTEST_READLOCALSIZE_KERNEL_HPP
#define CLSPVTEST_READLOCALSIZE_KERNEL_HPP

#include "clspv_utils.hpp"
#include "test_utils.hpp"
#include "util.hpp"

#include <vulkan/vulkan.hpp>

namespace readlocalsize_kernel {

    enum idtype_t {
        idtype_globalsize_x = 1,
        idtype_globalsize_y = 2,
        idtype_globalsize_z = 3,

        idtype_localsize_x  = 4,
        idtype_localsize_y  = 5,
        idtype_localsize_z  = 6,

        idtype_globalid_x   = 7,
        idtype_globalid_y   = 8,
        idtype_globalid_z   = 9,

        idtype_groupid_x    = 10,
        idtype_groupid_y    = 11,
        idtype_groupid_z    = 12,

        idtype_localid_x    = 13,
        idtype_localid_y    = 14,
        idtype_localid_z    = 15
    };


    clspv_utils::execution_time_t
    invoke(const clspv_utils::kernel_module&    module,
           const clspv_utils::kernel&           kernel,
           const sample_info&                   info,
           vk::ArrayProxy<const vk::Sampler>    samplers,
           vk::Buffer                           outLocalSizes,
           int                                  inWidth,
           int                                  inHeight,
           int                                  inPitch,
           idtype_t                             inIdType);


    void test(const clspv_utils::kernel_module& module,
              const clspv_utils::kernel&        kernel,
              const sample_info&                info,
              vk::ArrayProxy<const vk::Sampler> samplers,
              const std::vector<std::string>&   args,
              bool                              verbose,
              test_utils::InvocationResultSet&  resultSet);

}


#endif //CLSPVTEST_READLOCALSIZE_KERNEL_HPP
