#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

import re

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/samsung/exynos7885-common',
    'hardware/samsung',
    'hardware/samsung_slsi-linaro/exynos',
    'hardware/samsung_slsi-linaro/graphics',
]


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None


lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    'libuuid': lib_fixup_vendor_suffix,
}

blob_fixups: blob_fixups_user_type = {
    'vendor/etc/init/init.gps.rc': blob_fixup()
        .regex_replace(
            r'vendor/bin/hw/gps\.sh',
            'vendor/bin/hw/gpsd -c /vendor/etc/gnss/gps.cfg',
        ),
    (
        'vendor/lib/libsensorlistener.so',
        'vendor/lib64/libsensorlistener.so',
    ): blob_fixup()
        .add_needed('libshim_sensorndkbridge.so'),
    (
        'vendor/lib/libaudio_soundtrigger.so',
        'vendor/lib/soundfx/libaudioeffectoffload.so',
        'vendor/lib/libaudioroute.exynos7885.so',
    ): blob_fixup()
        .replace_needed('libtinyalsa.so', 'libtinyalsa.exynos7885.so'),
    (
        'vendor/bin/hw/rild',
        'vendor/lib64/libsec-ril-dsds.so',
        'vendor/lib64/libsec-ril.so',
    ): blob_fixup()
        .replace_needed('libril.so', 'libril-samsung.so')
        .binary_regex_replace(
            re.escape(bytes.fromhex(
                '600e40f9820c805224008052e10315aae30314aa'
            )),
            bytes.fromhex(
                '600e40f9820c805224008052e10315aa030080d2'
            ),
        ),
    (
        'vendor/lib64/libskeymaster4device.so',
        'vendor/lib64/libkeymaster_helper.so',
    ): blob_fixup()
        .replace_needed('libcrypto.so', 'libcrypto-v33.so'),
    (
        'vendor/lib/libwvhidl.so',
        'vendor/lib/mediadrm/libwvdrmengine.so',
    ): blob_fixup()
        .add_needed('libcrypto_shim.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'exynos7885-common',
    'samsung',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
