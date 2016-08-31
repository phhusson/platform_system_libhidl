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

#define LOG_TAG "HwServiceManager"

#include <hidl/IServiceManager.h>
#include <hidl/Static.h>
#include <hidl/Status.h>

#include <utils/Log.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/Parcel.h>
#include <hwbinder/Static.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>

#include <unistd.h>


namespace android {
namespace hardware {
sp<IServiceManager> defaultServiceManager()
{
    if (gDefaultServiceManager != NULL) return gDefaultServiceManager;

    {
        AutoMutex _l(gDefaultServiceManagerLock);
        while (gDefaultServiceManager == NULL) {
            gDefaultServiceManager = interface_cast<IHwServiceManager>(
                ProcessState::self()->getContextObject(NULL));
            if (gDefaultServiceManager == NULL)
                sleep(1);
        }
    }

    return gDefaultServiceManager;
}

// ----------------------------------------------------------------------

class BpServiceManager : public BpInterface<IHwServiceManager>
{
public:
    explicit BpServiceManager(const sp<IBinder>& impl)
        : BpInterface<IHwServiceManager>(impl)
    {
    }

    virtual sp<IBinder> getService(const String16& name, const hidl_version& version) const
    {
        unsigned n;
        for (n = 0; n < 5; n++){
            sp<IBinder> svc = checkService(name, version);
            if (svc != NULL) return svc;
            ALOGI("Waiting for service %s...\n", String8(name).string());
            sleep(1);
        }
        return NULL;
    }

    virtual sp<IBinder> checkService( const String16& name, const hidl_version& version) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(getInterfaceDescriptor());
        data.writeString16(name);
        version.writeToParcel(data);
        remote()->transact(CHECK_SERVICE_TRANSACTION, data, &reply);
        return reply.readStrongBinder();
    }

    virtual status_t addService(const String16& name,
            const sp<IBinder>& service, const hidl_version& version,
            bool allowIsolated)
    {
        Parcel data, reply;
        data.writeInterfaceToken(getInterfaceDescriptor());
        data.writeString16(name);
        data.writeStrongBinder(service);
        version.writeToParcel(data);
        data.writeInt32(allowIsolated ? 1 : 0);
        status_t err = remote()->transact(ADD_SERVICE_TRANSACTION, data, &reply);
        if (err == NO_ERROR) {
            Status status;
            status.readFromParcel(reply);
            return status.exceptionCode();
        } else {
            return err;
        }
    }

    virtual Vector<String16> listServices()
    {
        Vector<String16> res;
        int n = 0;

        for (;;) {
            Parcel data, reply;
            data.writeInterfaceToken(getInterfaceDescriptor());
            data.writeInt32(n++);
            status_t err = remote()->transact(LIST_SERVICES_TRANSACTION, data, &reply);
            if (err != NO_ERROR)
                break;
            res.add(reply.readString16());
        }
        return res;
    }
};

IMPLEMENT_HWBINDER_META_INTERFACE(ServiceManager, "android.hardware.IServiceManager");

}; // namespace hardware
}; // namespace android
