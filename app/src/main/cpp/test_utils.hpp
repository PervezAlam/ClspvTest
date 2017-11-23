//
// Created by Eric Berdahl on 10/31/17.
//

#ifndef CLSPVTEST_TEST_UTILS_HPP
#define CLSPVTEST_TEST_UTILS_HPP

#include "clspv_utils.hpp"
#include "fp_utils.hpp"
#include "gpu_types.hpp"
#include "pixels.hpp"
#include "util.hpp"

#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace test_utils {

    namespace details {
        template<typename ExpectedPixelType, typename ObservedPixelType>
        struct pixel_promotion {
            static constexpr const int expected_vec_size = pixels::traits<ExpectedPixelType>::num_components;
            static constexpr const int observed_vec_size = pixels::traits<ObservedPixelType>::num_components;
            static constexpr const int vec_size = (expected_vec_size > observed_vec_size
                                                   ? observed_vec_size : expected_vec_size);

            typedef typename pixels::traits<ExpectedPixelType>::component_t expected_comp_type;
            typedef typename pixels::traits<ObservedPixelType>::component_t observed_comp_type;

            static constexpr const bool expected_is_smaller =
                    sizeof(expected_comp_type) < sizeof(observed_comp_type);
            typedef typename std::conditional<expected_is_smaller, expected_comp_type, observed_comp_type>::type smaller_comp_type;
            typedef typename std::conditional<!expected_is_smaller, expected_comp_type, observed_comp_type>::type larger_comp_type;

            static constexpr const bool smaller_is_floating = std::is_floating_point<smaller_comp_type>::value;
            typedef typename std::conditional<smaller_is_floating, smaller_comp_type, larger_comp_type>::type comp_type;

            typedef typename pixels::vector<comp_type, vec_size>::type promotion_type;
        };

        template<typename T>
        struct pixel_comparator {
        };

        template<>
        struct pixel_comparator<float> {
            static bool is_equal(float l, float r) {
                const int ulp = 2;
                return fp_utils::almost_equal(l, r, ulp);
            }
        };

        template<>
        struct pixel_comparator<gpu_types::half> {
            static bool is_equal(gpu_types::half l, gpu_types::half r) {
                const int ulp = 2;
                return fp_utils::almost_equal(l, r, ulp);
            }
        };

        template<>
        struct pixel_comparator<gpu_types::uchar> {
            static bool is_equal(gpu_types::uchar l, gpu_types::uchar r) {
                return pixel_comparator<float>::is_equal(pixels::traits<float>::translate(l),
                                                         pixels::traits<float>::translate(r));
            }
        };

        template<typename T>
        struct pixel_comparator<gpu_types::vec2<T> > {
            static bool is_equal(const gpu_types::vec2<T> &l, const gpu_types::vec2<T> &r) {
                return pixel_comparator<T>::is_equal(l.x, r.x)
                       && pixel_comparator<T>::is_equal(l.y, r.y);
            }
        };

        template<typename T>
        struct pixel_comparator<gpu_types::vec4<T> > {
            static bool is_equal(const gpu_types::vec4<T> &l, const gpu_types::vec4<T> &r) {
                return pixel_comparator<T>::is_equal(l.x, r.x)
                       && pixel_comparator<T>::is_equal(l.y, r.y)
                       && pixel_comparator<T>::is_equal(l.z, r.z)
                       && pixel_comparator<T>::is_equal(l.w, r.w);
            }
        };
    }

    struct Results {
        Results(unsigned int testSuccess,
                unsigned int testFailure,
                unsigned int loadSuccess,
                unsigned int loadSkip,
                unsigned int loadFail) : mNumTestSuccess(testSuccess),
                                         mNumTestFail(testFailure),
                                         mNumKernelLoadSuccess(loadSuccess),
                                         mNumKernelLoadSkip(loadSkip),
                                         mNumKernelLoadFail(loadFail) {}

        Results() : Results(0, 0, 0, 0, 0) {}

        static const Results sTestSuccess;
        static const Results sTestFailure;
        static const Results sKernelLoadSuccess;
        static const Results sKernelLoadSkip;
        static const Results sKernelLoadFail;

        Results &operator+=(const Results &other) {
            mNumTestSuccess += other.mNumTestSuccess;
            mNumTestFail += other.mNumTestFail;
            mNumKernelLoadSuccess += other.mNumKernelLoadSuccess;
            mNumKernelLoadSkip += other.mNumKernelLoadSkip;
            mNumKernelLoadFail += other.mNumKernelLoadFail;
            return *this;
        }

        unsigned int mNumTestSuccess;
        unsigned int mNumTestFail;

        unsigned int mNumKernelLoadSuccess;
        unsigned int mNumKernelLoadSkip;
        unsigned int mNumKernelLoadFail;
    };
    
    struct options {
        bool logVerbose;
        bool logIncorrect;
        bool logCorrect;
    };

    typedef Results (*test_kernel_fn)(const clspv_utils::kernel_module& module,
                                      const clspv_utils::kernel&        kernel,
                                      const sample_info&                info,
                                      vk::ArrayProxy<const vk::Sampler> samplers,
                                      const options&                    opts);

    struct kernel_test_map {
        std::string entry;
        test_kernel_fn test;
        clspv_utils::WorkgroupDimensions workgroupSize;
    };

    struct module_test_bundle {
        std::string name;
        std::vector<kernel_test_map> kernelTests;
    };

    template<typename T>
    bool pixel_compare(const T &l, const T &r) {
        return details::pixel_comparator<T>::is_equal(l, r);
    }

    template<typename ExpectedPixelType, typename ObservedPixelType>
    bool check_result(ExpectedPixelType expected_pixel,
                      ObservedPixelType observed_pixel,
                      const char *label,
                      int row,
                      int column,
                      const options &opts) {
        typedef typename details::pixel_promotion<ExpectedPixelType, ObservedPixelType>::promotion_type promotion_type;

        auto expected = pixels::traits<promotion_type>::translate(expected_pixel);
        auto observed = pixels::traits<promotion_type>::translate(observed_pixel);

        auto t_expected = pixels::traits<gpu_types::float4>::translate(expected);
        auto t_observed = pixels::traits<gpu_types::float4>::translate(observed);

        const bool pixel_is_correct = pixel_compare(observed, expected);
        if (opts.logVerbose &&
            ((pixel_is_correct && opts.logCorrect) || (!pixel_is_correct && opts.logIncorrect))) {
            const gpu_types::float4 log_expected = pixels::traits<gpu_types::float4>::translate(
                    expected_pixel);
            const gpu_types::float4 log_observed = pixels::traits<gpu_types::float4>::translate(
                    observed_pixel);

            LOGE("%s: %s pixel{row:%d, col%d} expected{x=%f, y=%f, z=%f, w=%f} observed{x=%f, y=%f, z=%f, w=%f}",
                 pixel_is_correct ? "CORRECT" : "INCORRECT",
                 label, row, column,
                 log_expected.x, log_expected.y, log_expected.z, log_expected.w,
                 log_observed.x, log_observed.y, log_observed.z, log_observed.w);
        }

        return pixel_is_correct;
    }

    template<typename ObservedPixelType, typename ExpectedPixelType>
    bool check_results(const ObservedPixelType *observed_pixels,
                       int width,
                       int height,
                       int pitch,
                       ExpectedPixelType expected,
                       const char *label,
                       const options &opts) {
        unsigned int num_correct_pixels = 0;
        unsigned int num_incorrect_pixels = 0;

        auto row = observed_pixels;
        for (int r = 0; r < height; ++r, row += pitch) {
            auto p = row;
            for (int c = 0; c < width; ++c, ++p) {
                if (check_result(expected, *p, label, r, c, opts)) {
                    ++num_correct_pixels;
                } else {
                    ++num_incorrect_pixels;
                }
            }
        }

        if (opts.logVerbose) {
            LOGE("%s: Correct pixels=%d; Incorrect pixels=%d",
                 label, num_correct_pixels, num_incorrect_pixels);
        }

        return (0 == num_incorrect_pixels && 0 < num_correct_pixels);
    }

    template<typename ExpectedPixelType, typename ObservedPixelType>
    bool check_results(const ExpectedPixelType *expected_pixels,
                       const ObservedPixelType *observed_pixels,
                       int width,
                       int height,
                       int pitch,
                       const char *label,
                       const options &opts) {
        unsigned int num_correct_pixels = 0;
        unsigned int num_incorrect_pixels = 0;

        auto expected_row = expected_pixels;
        auto observed_row = observed_pixels;
        for (int r = 0; r < height; ++r, expected_row += pitch, observed_row += pitch) {
            auto expected_p = expected_row;
            auto observed_p = observed_row;
            for (int c = 0; c < width; ++c, ++expected_p, ++observed_p) {
                if (check_result(*expected_p, *observed_p, label, r, c, opts)) {
                    ++num_correct_pixels;
                } else {
                    ++num_incorrect_pixels;
                }
            }
        }

        if (opts.logVerbose) {
            LOGE("%s: Correct pixels=%d; Incorrect pixels=%d", label, num_correct_pixels,
                 num_incorrect_pixels);
        }

        return (0 == num_incorrect_pixels && 0 < num_correct_pixels);
    }

    template<typename ExpectedPixelType, typename ObservedPixelType>
    bool check_results(const vulkan_utils::device_memory &expected,
                       const vulkan_utils::device_memory &observed,
                       int width,
                       int height,
                       int pitch,
                       const char *label,
                       const options &opts) {
        vulkan_utils::memory_map src_map(expected);
        vulkan_utils::memory_map dst_map(observed);
        auto src_pixels = static_cast<const ExpectedPixelType *>(src_map.data);
        auto dst_pixels = static_cast<const ObservedPixelType *>(dst_map.data);

        return check_results(src_pixels, dst_pixels, width, height, pitch, label, opts);
    }

    template<typename Fn>
    Results runInExceptionContext(const std::string &label,
                                  const std::string &stage,
                                  Fn f,
                                  Results failureResult = Results::sTestFailure) {
        Results result = failureResult;

        try {
            result = f();
        }
        catch (const vk::SystemError &e) {
            std::ostringstream os;
            os << label << '/' << stage << ": vk::SystemError : " << e.code() << " ("
               << e.code().message() << ')';
            LOGE("%s", os.str().c_str());
        }
        catch (const std::system_error &e) {
            std::ostringstream os;
            os << label << '/' << stage << ": std::system_error : " << e.code() << " ("
               << e.code().message() << ')';
            LOGE("%s", os.str().c_str());
        }
        catch (const std::exception &e) {
            std::ostringstream os;
            os << label << '/' << stage << ": std::exception : " << e.what();
            LOGE("%s", os.str().c_str());
        }
        catch (...) {
            std::ostringstream os;
            os << label << '/' << stage << ": unknonwn error";
            LOGE("%s", os.str().c_str());
        }

        return result;
    }

    template<typename ObservedPixelType>
    bool check_results(const vulkan_utils::device_memory &observed,
                       int width,
                       int height,
                       int pitch,
                       const gpu_types::float4 &expected,
                       const char *label,
                       const options &opts) {
        vulkan_utils::memory_map map(observed);
        auto pixels = static_cast<const ObservedPixelType *>(map.data);
        return check_results(pixels, width, height, pitch, expected, label, opts);
    }

    Results test_kernel_invocation(const clspv_utils::kernel_module&    module,
                                   const clspv_utils::kernel&           kernel,
                                   test_kernel_fn                       testFn,
                                   const sample_info&                   info,
                                   vk::ArrayProxy<const vk::Sampler>    samplers,
                                   const options&                       opts);

    Results test_kernel_invocation(const clspv_utils::kernel_module&    module,
                                   const clspv_utils::kernel&           kernel,
                                   const test_kernel_fn*                first,
                                   const test_kernel_fn*                last,
                                   const sample_info&                   info,
                                   vk::ArrayProxy<const vk::Sampler>    samplers,
                                   const options&                       opts);

    Results test_kernel(const clspv_utils::kernel_module&       module,
                        const std::string&                      entryPoint,
                        test_kernel_fn                          testFn,
                        const clspv_utils::WorkgroupDimensions& numWorkgroups,
                        const sample_info&                      info,
                        vk::ArrayProxy<const vk::Sampler>       samplers,
                        const options&                          opts);

    Results test_module(const std::string&                  moduleName,
                        const std::vector<kernel_test_map>& kernelTests,
                        const sample_info&                  info,
                        vk::ArrayProxy<const vk::Sampler>   samplers,
                        const options&                      opts);

}

#endif //CLSPVTEST_TEST_UTILS_HPP
