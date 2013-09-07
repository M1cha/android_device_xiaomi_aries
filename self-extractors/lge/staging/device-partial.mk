# Copyright 2013 The Android Open Source Project
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

# Xiaomi blob(s) necessary for Aries hardware
PRODUCT_COPY_FILES := \
    vendor/xiaomi/aries/proprietary/DxHDCP.cfg:system/etc/DxHDCP.cfg:xiaomi \
    vendor/xiaomi/aries/proprietary/libAKM.so:system/lib/libAKM.so:xiaomi \
    vendor/xiaomi/aries/proprietary/libDxHdcp.so:system/lib/libDxHdcp.so:xiaomi \
    vendor/xiaomi/aries/proprietary/libcamera_fast_af.so:system/lib/libcamera_fast_af.so:xiaomi \
    vendor/xiaomi/aries/proprietary/libmorpho_noise_reduction.so:system/lib/libmorpho_noise_reduction.so:xiaomi \
    vendor/xiaomi/aries/proprietary/dxhdcp2.b00:system/vendor/firmware/discretix/dxhdcp2.b00:xiaomi \
    vendor/xiaomi/aries/proprietary/dxhdcp2.b01:system/vendor/firmware/discretix/dxhdcp2.b01:xiaomi \
    vendor/xiaomi/aries/proprietary/dxhdcp2.b02:system/vendor/firmware/discretix/dxhdcp2.b02:xiaomi \
    vendor/xiaomi/aries/proprietary/dxhdcp2.b03:system/vendor/firmware/discretix/dxhdcp2.b03:xiaomi \
    vendor/xiaomi/aries/proprietary/dxhdcp2.mdt:system/vendor/firmware/discretix/dxhdcp2.mdt:xiaomi \
    vendor/xiaomi/aries/proprietary/keymaster.b00:system/vendor/firmware/keymaster/keymaster.b00:xiaomi \
    vendor/xiaomi/aries/proprietary/keymaster.b01:system/vendor/firmware/keymaster/keymaster.b01:xiaomi \
    vendor/xiaomi/aries/proprietary/keymaster.b02:system/vendor/firmware/keymaster/keymaster.b02:xiaomi \
    vendor/xiaomi/aries/proprietary/keymaster.b03:system/vendor/firmware/keymaster/keymaster.b03:xiaomi \
    vendor/xiaomi/aries/proprietary/keymaster.mdt:system/vendor/firmware/keymaster/keymaster.mdt:xiaomi \

