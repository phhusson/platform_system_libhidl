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

}  // namespace hardware
}  // namespace android


