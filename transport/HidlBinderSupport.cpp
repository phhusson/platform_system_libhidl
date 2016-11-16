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

#define LOG_TAG "HidlSupport"

#include <hidl/HidlBinderSupport.h>

#ifdef LIBHIDL_TARGET_DEBUGGABLE
#include <android-base/logging.h>
#endif

namespace android {
namespace hardware {

std::map<std::string, std::function<sp<IBinder>(void*)>> gBnConstructorMap{};

// static
const size_t hidl_string::kOffsetOfBuffer = offsetof(hidl_string, mBuffer);

status_t readEmbeddedFromParcel(hidl_string * /* string */,
        const Parcel &parcel, size_t parentHandle, size_t parentOffset) {
    const void *ptr = parcel.readEmbeddedBuffer(
            nullptr /* buffer_handle */,
            parentHandle,
            parentOffset + hidl_string::kOffsetOfBuffer);

    return ptr != NULL ? OK : UNKNOWN_ERROR;
}

status_t writeEmbeddedToParcel(const hidl_string &string,
        Parcel *parcel, size_t parentHandle, size_t parentOffset) {
    return parcel->writeEmbeddedBuffer(
            string.c_str(),
            string.size() + 1,
            nullptr /* handle */,
            parentHandle,
            parentOffset + hidl_string::kOffsetOfBuffer);
}

android::status_t writeToParcel(const hidl_version &version, android::hardware::Parcel& parcel) {
    return parcel.writeUint32(static_cast<uint32_t>(version.get_major()) << 16 | version.get_minor());
}

hidl_version* readFromParcel(const android::hardware::Parcel& parcel) {
    uint32_t version;
    android::status_t status = parcel.readUint32(&version);
    if (status != OK) {
        return nullptr;
    } else {
        return new hidl_version(version >> 16, version & 0xFFFF);
    }
}

status_t readFromParcel(Status *s, const Parcel& parcel) {
    int32_t exception;
    int32_t errorCode;
    status_t status = parcel.readInt32(&exception);
    if (status != OK) {
        s->setFromStatusT(status);
        return status;
    }

    // Skip over fat response headers.  Not used (or propagated) in native code.
    if (exception == Status::EX_HAS_REPLY_HEADER) {
        // Note that the header size includes the 4 byte size field.
        const int32_t header_start = parcel.dataPosition();
        int32_t header_size;
        status = parcel.readInt32(&header_size);
        if (status != OK) {
            s->setFromStatusT(status);
            return status;
        }
        parcel.setDataPosition(header_start + header_size);
        // And fat response headers are currently only used when there are no
        // exceptions, so act like there was no error.
        exception = Status::EX_NONE;
    }

    if (exception == Status::EX_NONE) {
        *s = Status::ok();
        return status;
    }

    // The remote threw an exception.  Get the message back.
    String16 message;
    status = parcel.readString16(&message);
    if (status != OK) {
        s->setFromStatusT(status);
        return status;
    }

    if (exception == Status::EX_SERVICE_SPECIFIC) {
        status = parcel.readInt32(&errorCode);
    }
    if (status != OK) {
        s->setFromStatusT(status);
        return status;
    }

    if (exception == Status::EX_SERVICE_SPECIFIC) {
        s->setServiceSpecificError(errorCode, String8(message));
    } else {
        s->setException(exception, String8(message));
    }

    return status;
}

status_t writeToParcel(const Status &s, Parcel* parcel) {
    // Something really bad has happened, and we're not going to even
    // try returning rich error data.
    if (s.exceptionCode() == Status::EX_TRANSACTION_FAILED) {
        return s.transactionError();
    }

    status_t status = parcel->writeInt32(s.exceptionCode());
    if (status != OK) { return status; }
    if (s.exceptionCode() == Status::EX_NONE) {
        // We have no more information to write.
        return status;
    }
    status = parcel->writeString16(String16(s.exceptionMessage()));
    if (s.exceptionCode() != Status::EX_SERVICE_SPECIFIC) {
        // We have no more information to write.
        return status;
    }
    status = parcel->writeInt32(s.serviceSpecificErrorCode());
    return status;
}

}  // namespace hardware
}  // namespace android
