/*
 * Copyright (C) 2013, The CyanogenMod Project
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

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <cutils/properties.h>

#include <system/audio.h>
#include <platform.h>
#include <audio_hw.h>
#include <sound/es310.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "libaudioamp"
#include <cutils/log.h>

#include "audio_amplifier.h"

static int mES310Fd = -1;
static pthread_mutex_t mES310Mutex;
static enum ES310_PathID dwOldPath = ES310_PATH_SUSPEND;
static unsigned int dwOldPreset = -1;

static audio_mode_t mMode = AUDIO_MODE_NORMAL;
static snd_device_t in_snd_device = SND_DEVICE_NONE;
static snd_device_t out_snd_device = SND_DEVICE_NONE;

static const char *es310_getNameByPresetID(int presetID)
{
	switch (presetID) {
	case ES310_PRESET_HANDSET_INCALL_NB:
		return "ES310_PRESET_HANDSET_INCALL_NB";
	case ES310_PRESET_HEADSET_INCALL_NB:
		return "ES310_PRESET_HEADSET_INCALL_NB";
	case ES310_PRESET_HANDSET_INCALL_NB_1MIC:
		return "ES310_PRESET_HANDSET_INCALL_NB_1MIC";
	case ES310_PRESET_HANDSFREE_INCALL_NB:
		return "ES310_PRESET_HANDSFREE_INCALL_NB";
	case ES310_PRESET_HANDSET_INCALL_WB:
		return "ES310_PRESET_HANDSET_INCALL_WB";
	case ES310_PRESET_HEADSET_INCALL_WB:
		return "ES310_PRESET_HEADSET_INCALL_WB";
	case ES310_PRESET_AUDIOPATH_DISABLE:
		return "ES310_PRESET_AUDIOPATH_DISABLE";
	case ES310_PRESET_HANDSFREE_INCALL_WB:
		return "ES310_PRESET_HANDSFREE_INCALL_WB";
	case ES310_PRESET_HANDSET_VOIP_WB:
		return "ES310_PRESET_HANDSET_VOIP_WB";
	case ES310_PRESET_HEADSET_VOIP_WB:
		return "ES310_PRESET_HEADSET_VOIP_WB";
	case ES310_PRESET_HANDSFREE_REC_WB:
		return "ES310_PRESET_HANDSFREE_REC_WB";
	case ES310_PRESET_HANDSFREE_VOIP_WB:
		return "ES310_PRESET_HANDSFREE_VOIP_WB";
	case ES310_PRESET_VOICE_RECOGNIZTION_WB:
		return "ES310_PRESET_VOICE_RECOGNIZTION_WB";
	case ES310_PRESET_HANDSET_INCALL_VOIP_WB_1MIC:
		return "ES310_PRESET_HANDSET_INCALL_VOIP_WB_1MIC";
	case ES310_PRESET_ANALOG_BYPASS:
		return "ES310_PRESET_ANALOG_BYPASS";
	case ES310_PRESET_HEADSET_MIC_ANALOG_BYPASS:
		return "ES310_PRESET_HEADSET_MIC_ANALOG_BYPASS";
	default:
		return "Unknown";
	}
	return "Unknown";
}

static const char *es310_getNameByPathID(int pathID)
{
	switch (pathID) {
	case ES310_PATH_SUSPEND:
		return "ES310_PATH_SUSPEND";
	case ES310_PATH_HANDSET:
		return "ES310_PATH_HANDSET";
	case ES310_PATH_HEADSET:
		return "ES310_PATH_HEADSET";
	case ES310_PATH_HANDSFREE:
		return "ES310_PATH_HANDSFREE";
	case ES310_PATH_BACKMIC:
		return "ES310_PATH_BACKMIC";
	default:
		return "Unknown";
	}
	return "Unknown";
}

static int es310_init(void)
{
	int rc = -1;

	ALOGD("%s\n", __func__);

	// open
	mES310Fd = open("/dev/audience_es310", O_RDWR | O_NONBLOCK, 0);
	if (mES310Fd < 0) {
		ALOGE("%s: unable to open es310 device!", __func__);
		return rc;
	}
	// reset
	rc = ioctl(mES310Fd, ES310_RESET_CMD);
	if (rc) {
		ALOGE("%s: ES310_RESET_CMD fail, rc=%d", __func__, rc);
		close(mES310Fd);
		mES310Fd = -1;
		return rc;
	}
	// sync
	rc = ioctl(mES310Fd, ES310_SYNC_CMD, NULL);
	if (rc) {
		ALOGE("%s: ES310_SYNC_CMD fail, rc=%d", __func__, rc);
		close(mES310Fd);
		mES310Fd = -1;
		return rc;
	}

	return rc;
}

static int es310_wakeup(void)
{
	int rc = -1;

	ALOGD("%s\n", __func__);

	if (mES310Fd < 0) {
		ALOGE("%s: codec not initialized.\n", __func__);
		return rc;
	}

	int retry = 4;
	do {
		ALOGV("%s: ioctl ES310_SET_CONFIG retry:%d", __func__,
		      4 - retry);
		rc = ioctl(mES310Fd, ES310_WAKEUP_CMD, NULL);
		if (!rc)
			break;
		else
			ALOGE("%s: ES310_SET_CONFIG fail rc=%d", __func__, rc);
	} while (--retry);

	return rc;
}

static int es310_do_route(void)
{
	int rc = -1;
	char cVNRMode[255] = "2";
	int VNRMode = 2;
	enum ES310_PathID dwNewPath = ES310_PATH_SUSPEND;
	unsigned int dwNewPreset = -1;

	if (mES310Fd < 0) {
		ALOGE("%s: codec not initialized.\n", __func__);
		return rc;
	}

	pthread_mutex_lock(&mES310Mutex);

	// suspend if we don't use a mic
	if ((in_snd_device == SND_DEVICE_NONE) &&
	    (out_snd_device != SND_DEVICE_NONE) &&
	    (mMode == AUDIO_MODE_NORMAL)) {
		ALOGD("%s: Normal mode, RX no routing\n", __func__);
		goto unlock;

		// we have no way to detect if mic routes are disabled right now
		// I don't know if this is even needed (power usage?)
		// dwNewPath = ES310_PATH_SUSPEND;
		// goto route;
	}
	// 1mic mode
	property_get("persist.audio.vns.mode", cVNRMode, "2");
	if (!strncmp("1", cVNRMode, 1))
		VNRMode = 1;
	else
		VNRMode = 2;

	if (mMode == AUDIO_MODE_IN_CALL || mMode == AUDIO_MODE_RINGTONE
	    || mMode == AUDIO_MODE_IN_COMMUNICATION) {
		bool is_voip = (mMode == AUDIO_MODE_IN_COMMUNICATION);
		switch (in_snd_device) {
		case SND_DEVICE_IN_SPEAKER_MIC:
		case SND_DEVICE_IN_SPEAKER_MIC_AEC:
		case SND_DEVICE_IN_VOICE_SPEAKER_MIC:
		case SND_DEVICE_IN_VOICE_SPEAKER_DMIC_EF:
		case SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BS:
		case SND_DEVICE_IN_HDMI_MIC:
			dwNewPath = ES310_PATH_HANDSFREE;
			dwNewPreset =
			    is_voip ? ES310_PRESET_HANDSFREE_VOIP_WB :
			    ES310_PRESET_HANDSFREE_INCALL_NB;
			break;

		case SND_DEVICE_IN_HEADSET_MIC:
		case SND_DEVICE_IN_HEADSET_MIC_AEC:
		case SND_DEVICE_IN_VOICE_HEADSET_MIC:
		case SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC:
		case SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC:
			dwNewPath = ES310_PATH_HEADSET;
			dwNewPreset =
			    is_voip ? ES310_PRESET_HEADSET_VOIP_WB :
			    ES310_PRESET_HEADSET_INCALL_NB;
			break;

		case SND_DEVICE_IN_HANDSET_MIC:
		case SND_DEVICE_IN_HANDSET_MIC_AEC:
		case SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC:
		case SND_DEVICE_IN_VOICE_DMIC_EF:
		case SND_DEVICE_IN_VOICE_DMIC_BS:
		case SND_DEVICE_IN_VOICE_DMIC_EF_TMUS:
		case SND_DEVICE_IN_VOICE_REC_MIC:
		case SND_DEVICE_IN_VOICE_REC_DMIC_EF:
		case SND_DEVICE_IN_VOICE_REC_DMIC_BS:
		case SND_DEVICE_IN_VOICE_REC_DMIC_EF_FLUENCE:
		case SND_DEVICE_IN_VOICE_REC_DMIC_BS_FLUENCE:
		case SND_DEVICE_IN_CAMCORDER_MIC:
		case SND_DEVICE_IN_BT_SCO_MIC:
		case SND_DEVICE_IN_BT_SCO_MIC_WB:
		default:
			dwNewPath = ES310_PATH_HANDSET;
			dwNewPreset =
			    is_voip ? ES310_PRESET_HANDSET_VOIP_WB :
			    ES310_PRESET_HANDSET_INCALL_NB;
			break;
		}
	} else {
		switch (in_snd_device) {
		case SND_DEVICE_IN_SPEAKER_MIC:
		case SND_DEVICE_IN_SPEAKER_MIC_AEC:
		case SND_DEVICE_IN_VOICE_SPEAKER_MIC:
		case SND_DEVICE_IN_VOICE_SPEAKER_DMIC_EF:
		case SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BS:
		case SND_DEVICE_IN_HDMI_MIC:
			dwNewPath = ES310_PATH_HANDSFREE;
			dwNewPreset = ES310_PRESET_HANDSFREE_REC_WB;
			break;

		case SND_DEVICE_IN_BT_SCO_MIC:
		case SND_DEVICE_IN_BT_SCO_MIC_WB:
			dwNewPath = ES310_PATH_HEADSET;
			dwNewPreset = ES310_PRESET_HANDSFREE_REC_WB;
			break;

		case SND_DEVICE_IN_HEADSET_MIC:
		case SND_DEVICE_IN_HEADSET_MIC_AEC:
		case SND_DEVICE_IN_VOICE_HEADSET_MIC:
		case SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC:
		case SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC:
			dwNewPath = ES310_PATH_HEADSET;
			// volume is better this way
			// dwNewPreset = ES310_PRESET_HEADSET_MIC_ANALOG_BYPASS;
			dwNewPreset = ES310_PRESET_HANDSFREE_REC_WB;
			break;

		case SND_DEVICE_IN_VOICE_REC_MIC:
		case SND_DEVICE_IN_VOICE_REC_DMIC_EF:
		case SND_DEVICE_IN_VOICE_REC_DMIC_BS:
		case SND_DEVICE_IN_VOICE_REC_DMIC_EF_FLUENCE:
		case SND_DEVICE_IN_VOICE_REC_DMIC_BS_FLUENCE:
			dwNewPath = ES310_PATH_HANDSET;
			// recognition is better this way
			// dwNewPreset = ES310_PRESET_VOICE_RECOGNIZTION_WB;
			dwNewPreset = ES310_PRESET_HANDSFREE_REC_WB;
			break;

		case SND_DEVICE_IN_CAMCORDER_MIC:
			dwNewPath = ES310_PATH_HANDSET;
			dwNewPreset = ES310_PRESET_ANALOG_BYPASS;
			break;

		case SND_DEVICE_IN_HANDSET_MIC:
		case SND_DEVICE_IN_HANDSET_MIC_AEC:
		case SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC:
		case SND_DEVICE_IN_VOICE_DMIC_EF:
		case SND_DEVICE_IN_VOICE_DMIC_BS:
		case SND_DEVICE_IN_VOICE_DMIC_EF_TMUS:
		default:
			dwNewPath = ES310_PATH_HANDSET;
			dwNewPreset = ES310_PRESET_HANDSFREE_REC_WB;
			break;
		}
	}

route:
	if (VNRMode == 1) {
		ALOGV("%s: Switch to 1-Mic Solution", __func__);
		if (dwNewPreset == ES310_PRESET_HANDSET_INCALL_NB) {
			dwNewPreset = ES310_PRESET_HANDSET_INCALL_NB_1MIC;
		}
		if (dwNewPreset == ES310_PRESET_HANDSET_VOIP_WB) {
			dwNewPreset = ES310_PRESET_HANDSET_INCALL_VOIP_WB_1MIC;
		}
	}
	// still the same path and preset
	if (dwOldPath == dwNewPath && dwOldPreset == dwNewPreset) {
		rc = 0;
		goto unlock;
	}

	ALOGD("%s: path=%s->%s, preset=%s->%s", __func__,
	      es310_getNameByPathID(dwOldPath),
	      es310_getNameByPathID(dwNewPath),
	      es310_getNameByPresetID(dwOldPreset),
	      es310_getNameByPresetID(dwNewPreset));

	// wakeup if suspended
	if (dwOldPath == ES310_PATH_SUSPEND) {
		rc = es310_wakeup();
		if (rc) {
			ALOGE("%s: es310_wakeup() failed rc=%d\n", __func__,
			      rc);
			goto recover;
		}
	}
	// path
	if (dwOldPath != dwNewPath) {
		int retry = 4;
		do {
			ALOGE("%s: ioctl ES310_SET_CONFIG newPath:%d, retry:%d",
			      __func__, dwNewPath, (4 - retry));
			rc = ioctl(mES310Fd, ES310_SET_CONFIG, &dwNewPath);

			if (!rc) {
				dwOldPath = dwNewPath;
				break;
			} else {
				ALOGE("%s: ES310_SET_CONFIG rc=%d", __func__,
				      rc);
			}
		} while (--retry);

		if (rc)
			goto recover;
	}
	// preset
	if (dwNewPath != ES310_PATH_SUSPEND && (dwOldPreset != dwNewPreset)) {
		int retry = 4;
		do {
			ALOGE
			    ("%s: ioctl ES310_SET_PRESET newPreset:0x%x, retry:%d",
			     __func__, dwNewPreset, (4 - retry));
			rc = ioctl(mES310Fd, ES310_SET_PRESET, &dwNewPreset);

			if (!rc) {
				dwOldPreset = dwNewPreset;
				break;
			} else {
				ALOGE("%s: ES310_SET_PRESET rc=%d", __func__,
				      rc);
			}
		} while (--retry);

		if (rc)
			goto recover;
	}

recover:
	if (rc < 0) {
		ALOGE("%s: ES310 do hard reset to recover from error!\n",
		      __func__);

		// close device first
		close(mES310Fd);
		mES310Fd = -1;

		// re-init
		rc = es310_init();
		if (!rc) {
			// set new config
			rc = ioctl(mES310Fd, ES310_SET_CONFIG, &dwNewPath);
			if (rc) {
				ALOGE("%s: RECOVERY: ES310_SET_CONFIG rc=%d",
				      __func__, rc);
				goto unlock;
			}

			dwOldPath = dwNewPath;
		} else {
			ALOGE("%s: RECOVERY: es310_init() failed rc=%d\n",
			      __func__, rc);
			goto unlock;
		}
	}

unlock:
	pthread_mutex_unlock(&mES310Mutex);
	return rc;
}

int amplifier_open(void)
{
	// mutex init
	pthread_mutex_init(&mES310Mutex, NULL);

	return es310_init();
}

void amplifier_set_devices(int snd_device)
{
	snd_device_t new_in_snd_device = SND_DEVICE_NONE;
	snd_device_t new_out_snd_device = SND_DEVICE_NONE;

	if (snd_device != 0) {
		if (snd_device >= SND_DEVICE_OUT_BEGIN
		    && snd_device < SND_DEVICE_OUT_END)
			new_out_snd_device = snd_device;
		else
			new_in_snd_device = snd_device;

		if (new_in_snd_device != in_snd_device
		    || new_out_snd_device != out_snd_device) {
			in_snd_device = new_in_snd_device;
			out_snd_device = new_out_snd_device;

			es310_do_route();
		}
	}
}

int amplifier_set_mode(audio_mode_t mode)
{
	int rc = -1;

	if ((mode < AUDIO_MODE_CURRENT) || (mode >= AUDIO_MODE_CNT)) {
		ALOGW("%s: invalid mode=%d\n", __func__, mode);
		return rc;
	}

	if (mMode != mode) {
		mMode = mode;
		es310_do_route();
	}

	return 0;
}

int amplifier_close(void)
{
	close(mES310Fd);
	mES310Fd = -1;

	return 0;
}
