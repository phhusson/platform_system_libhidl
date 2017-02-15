/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "HidlInternal"

#include <hidl/HidlInternal.h>

#include <android-base/logging.h>

#ifdef LIBHIDL_TARGET_DEBUGGABLE
#include <cutils/properties.h>
#include <dirent.h>
#include <dlfcn.h>
#include <hidl-util/FQName.h>
#include <regex>
#endif

namespace android {
namespace hardware {
namespace details {

void hidl_log_base::logAlwaysFatal(const char *message) const {
    LOG(FATAL) << message;
}

// ----------------------------------------------------------------------
// HidlInstrumentor implementation.
HidlInstrumentor::HidlInstrumentor(
        const std::string &package,
        const std::string &interface)
        : mInstrumentationLibPackage(package), mInterfaceName(interface) {
    configureInstrumentation(false);
}

HidlInstrumentor:: ~HidlInstrumentor() {}

void HidlInstrumentor::configureInstrumentation(bool log) {
    bool enable_instrumentation = property_get_bool(
            "hal.instrumentation.enable",
            false);
    if (enable_instrumentation != mEnableInstrumentation) {
        mEnableInstrumentation = enable_instrumentation;
        if (mEnableInstrumentation) {
            if (log) {
                LOG(INFO) << "Enable instrumentation.";
            }
            registerInstrumentationCallbacks (&mInstrumentationCallbacks);
        } else {
            if (log) {
                LOG(INFO) << "Disable instrumentation.";
            }
            mInstrumentationCallbacks.clear();
        }
    }
}

void HidlInstrumentor::registerInstrumentationCallbacks(
        std::vector<InstrumentationCallback> *instrumentationCallbacks) {
#ifdef LIBHIDL_TARGET_DEBUGGABLE
    std::vector<std::string> instrumentationLibPaths;
    char instrumentation_lib_path[PROPERTY_VALUE_MAX];
    if (property_get("hal.instrumentation.lib.path",
                     instrumentation_lib_path,
                     "") > 0) {
        instrumentationLibPaths.push_back(instrumentation_lib_path);
    } else {
        instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_SYSTEM);
        instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_VENDOR);
        instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_ODM);
    }

    for (auto path : instrumentationLibPaths) {
        DIR *dir = opendir(path.c_str());
        if (dir == 0) {
            LOG(WARNING) << path << " does not exist. ";
            return;
        }

        struct dirent *file;
        while ((file = readdir(dir)) != NULL) {
            if (!isInstrumentationLib(file))
                continue;

            void *handle = dlopen((path + file->d_name).c_str(), RTLD_NOW);
            char *error;
            if (handle == nullptr) {
                LOG(WARNING) << "couldn't load file: " << file->d_name
                    << " error: " << dlerror();
                continue;
            }

            dlerror(); /* Clear any existing error */

            using cb_fun = void (*)(
                    const InstrumentationEvent,
                    const char *,
                    const char *,
                    const char *,
                    const char *,
                    std::vector<void *> *);
            FQName package_name = FQName(mInstrumentationLibPackage);
            auto cb = (cb_fun)dlsym(handle, ("HIDL_INSTRUMENTATION_FUNCTION_"
                        + package_name.tokenName() + "_"
                        + mInterfaceName).c_str());
            if ((error = dlerror()) != NULL) {
                LOG(WARNING)
                    << "couldn't find symbol: HIDL_INSTRUMENTATION_FUNCTION_"
                    << mInterfaceName << ", error: " << error;
                continue;
            }
            instrumentationCallbacks->push_back(cb);
            LOG(INFO) << "Register instrumentation callback from "
                << file->d_name;
        }
        closedir(dir);
    }
#else
    // No-op for user builds.
    return;
#endif
}

bool HidlInstrumentor::isInstrumentationLib(const dirent *file) {
#ifdef LIBHIDL_TARGET_DEBUGGABLE
    if (file->d_type != DT_REG) return false;
    std::cmatch cm;
    std::regex e("^" + mInstrumentationLibPackage + "(.*).profiler.so$");
    if (std::regex_match(file->d_name, cm, e)) return true;
#endif
    return false;
}

}  // namespace details
}  // namespace hardware
}  // namespace android
