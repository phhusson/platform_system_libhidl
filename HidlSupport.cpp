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

#include <hidl/HidlSupport.h>

#ifndef DLIBHIDL_TARGET_BUILD_VARIANT_USER
#include <android-base/logging.h>
#include <cutils/properties.h>
#include <regex>
#endif

namespace android {
namespace hardware {

static const char *const kEmptyString = "";

hidl_string::hidl_string()
    : mBuffer(const_cast<char *>(kEmptyString)),
      mSize(0),
      mOwnsBuffer(true) {
}

hidl_string::~hidl_string() {
    clear();
}

hidl_string::hidl_string(const hidl_string &other)
    : mBuffer(const_cast<char *>(kEmptyString)),
      mSize(0),
      mOwnsBuffer(true) {
    setTo(other.c_str(), other.size());
}

hidl_string &hidl_string::operator=(const hidl_string &other) {
    if (this != &other) {
        setTo(other.c_str(), other.size());
    }

    return *this;
}

hidl_string &hidl_string::operator=(const char *s) {
    return setTo(s, strlen(s));
}

hidl_string &hidl_string::setTo(const char *data, size_t size) {
    clear();

    mBuffer = (char *)malloc(size + 1);
    memcpy(mBuffer, data, size);
    mBuffer[size] = '\0';

    mSize = size;
    mOwnsBuffer = true;

    return *this;
}

void hidl_string::clear() {
    if (mOwnsBuffer && (mBuffer != kEmptyString)) {
        free(mBuffer);
    }

    mBuffer = const_cast<char *>(kEmptyString);
    mSize = 0;
    mOwnsBuffer = true;
}

void hidl_string::setToExternal(const char *data, size_t size) {
    clear();

    mBuffer = const_cast<char *>(data);
    mSize = size;
    mOwnsBuffer = false;
}

const char *hidl_string::c_str() const {
    return mBuffer ? mBuffer : "";
}

size_t hidl_string::size() const {
    return mSize;
}

bool hidl_string::empty() const {
    return mSize == 0;
}

status_t hidl_string::readEmbeddedFromParcel(
        const Parcel &parcel, size_t parentHandle, size_t parentOffset) {
    const void *ptr = parcel.readEmbeddedBuffer(
            nullptr /* buffer_handle */,
            parentHandle,
            parentOffset + offsetof(hidl_string, mBuffer));

    return ptr != NULL ? OK : UNKNOWN_ERROR;
}

status_t hidl_string::writeEmbeddedToParcel(
        Parcel *parcel, size_t parentHandle, size_t parentOffset) const {
    return parcel->writeEmbeddedBuffer(
            mBuffer,
            mSize + 1,
            nullptr /* handle */,
            parentHandle,
            parentOffset + offsetof(hidl_string, mBuffer));
}

// static
const size_t hidl_string::kOffsetOfBuffer = offsetof(hidl_string, mBuffer);



void registerInstrumentationCallbacks(
        const std::string &profilerPrefix,
        std::vector<InstrumentationCallback> *instrumentationCallbacks) {
#ifndef DLIBHIDL_TARGET_BUILD_VARIANT_USER
    std::vector<std::string> instrumentationLibPaths;
    instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_SYSTEM);
    instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_VENDOR);
    instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_ODM);
    for (auto path : instrumentationLibPaths) {
        DIR *dir = opendir(path.c_str());
        if (dir == 0) {
            LOG(WARNING) << path << " does not exit. ";
            return;
        }

        struct dirent *file;
        while ((file = readdir(dir)) != NULL) {
            if (!isInstrumentationLib(profilerPrefix, file))
                continue;

            void *handle = dlopen((path + file->d_name).c_str(), RTLD_NOW);
            if (handle == nullptr) {
                LOG(WARNING) << "couldn't load file: " << file->d_name
                    << " error: " << dlerror();
                continue;
            }
            using cb_fun = void (*)(
                    const InstrumentationEvent,
                    const char *,
                    const char *,
                    const char *,
                    const char *,
                    std::vector<void *> *);
            auto cb = (cb_fun)dlsym(handle, "HIDL_INSTRUMENTATION_FUNCTION");
            if (cb == nullptr) {
                LOG(WARNING)
                    << "couldn't find symbol: HIDL_INSTRUMENTATION_FUNCTION, "
                       "error: "
                    << dlerror();
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

bool isInstrumentationLib(
        const std::string &profiler_prefix,
        const dirent *file) {
#ifndef DLIBHIDL_TARGET_BUILD_VARIANT_USER
    if (file->d_type != DT_REG) return false;
    std::cmatch cm;
    std::regex e("^" + profiler_prefix + "(.*).profiler.so$");
    if (std::regex_match(file->d_name, cm, e)) return true;
#else
#endif
    return false;
}

}  // namespace hardware
}  // namespace android


