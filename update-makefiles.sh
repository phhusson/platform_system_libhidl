#!/bin/bash

# TODO(b/33276472)
if [ ! -d system/libhidl/transport ] ; then
  echo "Where is system/libhidl/transport?";
  exit 1;
fi

#base
hidl-gen -Lmakefile -r android.hidl:system/libhidl/transport android.hidl.base@1.0
hidl-gen -Landroidbp -r android.hidl:system/libhidl/transport android.hidl.base@1.0

#manager
hidl-gen -Lmakefile -r android.hidl:system/libhidl/transport android.hidl.manager@1.0
hidl-gen -Landroidbp -r android.hidl:system/libhidl/transport android.hidl.manager@1.0

#memory
hidl-gen -Lmakefile -r android.hidl:system/libhidl/transport android.hidl.memory@1.0
hidl-gen -Landroidbp -r android.hidl:system/libhidl/transport android.hidl.memory@1.0
