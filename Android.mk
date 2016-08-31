# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libhidl
LOCAL_SHARED_LIBRARIES := libbase liblog libutils libhwbinder
LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := libbase libutils

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_CLANG := true
LOCAL_SANITIZE := integer
LOCAL_SRC_FILES := \
	HidlSupport.cpp \
	IServiceManager.cpp \
	Static.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_MULTILIB := both
include $(BUILD_SHARED_LIBRARY)
