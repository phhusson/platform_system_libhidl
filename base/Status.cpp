/*
 * Copyright (C) 2015 The Android Open Source Project
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
#define LOG_TAG "HidlStatus"

#include <hidl/Status.h>

#include <android-base/logging.h>

namespace android {
namespace hardware {

Status Status::ok() {
    return Status();
}

Status Status::fromExceptionCode(int32_t exceptionCode) {
    return Status(exceptionCode, OK);
}

Status Status::fromExceptionCode(int32_t exceptionCode,
                                 const char *message) {
    return Status(exceptionCode, OK, message);
}

Status Status::fromServiceSpecificError(int32_t serviceSpecificErrorCode) {
    return Status(EX_SERVICE_SPECIFIC, serviceSpecificErrorCode);
}

Status Status::fromServiceSpecificError(int32_t serviceSpecificErrorCode,
                                        const char *message) {
    return Status(EX_SERVICE_SPECIFIC, serviceSpecificErrorCode, message);
}

Status Status::fromStatusT(status_t status) {
    Status ret;
    ret.setFromStatusT(status);
    return ret;
}

Status::Status(int32_t exceptionCode, int32_t errorCode)
    : mException(exceptionCode),
      mErrorCode(errorCode) {}

Status::Status(int32_t exceptionCode, int32_t errorCode, const char *message)
    : mException(exceptionCode),
      mErrorCode(errorCode),
      mMessage(message) {}

void Status::setException(int32_t ex, const char *message) {
    mException = ex;
    mErrorCode = NO_ERROR;  // an exception, not a transaction failure.
    mMessage = message;
}

void Status::setServiceSpecificError(int32_t errorCode, const char *message) {
    setException(EX_SERVICE_SPECIFIC, message);
    mErrorCode = errorCode;
}

void Status::setFromStatusT(status_t status) {
    mException = (status == NO_ERROR) ? EX_NONE : EX_TRANSACTION_FAILED;
    mErrorCode = status;
    mMessage.clear();
}

std::string Status::description() const {
    std::ostringstream oss;
    oss << (*this);
    return oss.str();
}

std::ostream& operator<< (std::ostream& stream, const Status& s) {
    if (s.exceptionCode() == Status::EX_NONE) {
        stream << "No error";
    } else {
        stream << "Status(" << s.exceptionCode() << "): '";
        if (s.exceptionCode() == Status::EX_SERVICE_SPECIFIC) {
            stream << s.serviceSpecificErrorCode() << ": ";
        } else if (s.exceptionCode() == Status::EX_TRANSACTION_FAILED) {
            stream << s.transactionError() << ": ";
        }
        stream << s.exceptionMessage() << "'";
    }
    return stream;
}

namespace details {
    void return_status::checkStatus() const {
        if (!isOk()) {
            LOG(FATAL) << "Attempted to retrieve value from failed HIDL call: " << description();
        }
    }

    return_status::~return_status() {
        // mCheckedStatus must be checked before isOk since isOk modifies mCheckedStatus
        if (!mCheckedStatus && !isOk()) {
            LOG(FATAL) << "Failed HIDL return status not checked: " << description();
        }
    }
}  // namespace details

}  // namespace hardware
}  // namespace android
