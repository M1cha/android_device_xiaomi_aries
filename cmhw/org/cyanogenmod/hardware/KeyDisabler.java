/*
 * Copyright (C) 2014 The CyanogenMod Project
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

package org.cyanogenmod.hardware;

import org.cyanogenmod.hardware.util.FileUtils;

import java.io.File;

/*
 * Disable capacitive keys
 *
 * This is intended for use on devices in which the capacitive keys
 * can be fully disabled for replacement with a soft navbar. You
 * really should not be using this on a device with mechanical or
 * otherwise visible-when-inactive keys
 */

public class KeyDisabler {

    private static String KEY_CONTROL_PATH = "/sys/devices/i2c-3/3-004b/disable_keys";

    public static boolean isSupported() {
        return new File(KEY_CONTROL_PATH).exists();
    }

    public static boolean isActive() {
        return Integer.parseInt(FileUtils.readOneLine(KEY_CONTROL_PATH)) == 1;
    }

    public static boolean setActive(boolean state) {
        return FileUtils.writeLine(KEY_CONTROL_PATH, state ? "1" : "0");
    }

}
