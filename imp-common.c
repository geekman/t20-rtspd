/*
 * sample-common.c
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <imp/imp_log.h>
#include <imp/imp_common.h>
#include <imp/imp_system.h>
#include <imp/imp_framesource.h>
#include <imp/imp_isp.h>
#ifdef PLATFORM_T31
  #include <imp/imp_encoder_t31.h>

  /* compat */
  #define IMPEncoderCHNAttr IMPEncoderChnAttr
  #define IMPEncoderCHNStat IMPEncoderChnStat
#else
  #include <imp/imp_encoder.h>
#endif
#include <imp/imp_osd.h>

#include "logodata_100x100_bgra.h"

#include "imp-common.h"
#include "pwm.h"

#define TAG "imp-Common"

#ifdef PLATFORM_T31
static const int S_RC_METHOD = IMP_ENC_RC_MODE_VBR;
#else
static const int S_RC_METHOD = ENC_RC_MODE_SMART;
#endif

struct chn_conf chn[FS_CHN_NUM] = {
	{
		.index = CH0_INDEX,
		.enable = CHN0_EN,
		.fs_chn_attr = {
			.pixFmt = PIX_FMT_NV12,
			.outFrmRateNum = SENSOR_FRAME_RATE_NUM,
			.outFrmRateDen = SENSOR_FRAME_RATE_DEN,
			.nrVBs = 2,
			.type = FS_PHY_CHANNEL,

			.crop.enable = CROP_EN,
			.crop.top = 0,
			.crop.left = 0,
			.crop.width = SENSOR_WIDTH,
			.crop.height = SENSOR_HEIGHT,

			.scaler.enable = 0,

			.picWidth = SENSOR_WIDTH,
			.picHeight = SENSOR_HEIGHT,
		   },
		.framesource_chn =	{ DEV_ID_FS, 0, 0},
		.imp_encoder = { DEV_ID_ENC, 0, 0},
	},
	{
		.index = CH1_INDEX,
		.enable = CHN1_EN,
		.fs_chn_attr = {
			.pixFmt = PIX_FMT_NV12,
			.outFrmRateNum = SENSOR_FRAME_RATE_NUM,
			.outFrmRateDen = SENSOR_FRAME_RATE_DEN,
			.nrVBs = 2,
			.type = FS_PHY_CHANNEL,

			.crop.enable = CROP_EN,
			.crop.top = 0,
			.crop.left = 0,
			.crop.width = SENSOR_WIDTH,
			.crop.height = SENSOR_HEIGHT,

			.scaler.enable = 1,
			.scaler.outwidth = SENSOR_WIDTH_SECOND,
			.scaler.outheight = SENSOR_HEIGHT_SECOND,

			.picWidth = SENSOR_WIDTH_SECOND,
			.picHeight = SENSOR_HEIGHT_SECOND,
		   },
		.framesource_chn =	{ DEV_ID_FS, 1, 0},
		.imp_encoder = { DEV_ID_ENC, 1, 0},
	},
};

struct chn_conf chn_ext_hsv[1] = {
	{
		.fs_chn_attr = {
			.pixFmt = PIX_FMT_HSV,
			.outFrmRateNum = SENSOR_FRAME_RATE_NUM,
			.outFrmRateDen = SENSOR_FRAME_RATE_DEN,
			.nrVBs = 2,
			.type = FS_EXT_CHANNEL,

			.crop.enable = 0,
			.crop.top = 0,
			.crop.left = 0,
			.crop.width = SENSOR_WIDTH_SECOND,
			.crop.height = SENSOR_HEIGHT_SECOND,

			.scaler.enable = 0,

			.picWidth = SENSOR_WIDTH_SECOND,
			.picHeight = SENSOR_HEIGHT_SECOND,
		},
	},
};

struct chn_conf chn_ext_bgra[1] = {
	{
		.fs_chn_attr = {
			.pixFmt = PIX_FMT_BGRA,
			.outFrmRateNum = SENSOR_FRAME_RATE_NUM,
			.outFrmRateDen = SENSOR_FRAME_RATE_DEN,
			.nrVBs = 2,
			.type = FS_EXT_CHANNEL,

			.crop.enable = 0,
			.crop.top = 0,
			.crop.left = 0,
			.crop.width = SENSOR_WIDTH_SECOND,
			.crop.height = SENSOR_HEIGHT_SECOND,

			.scaler.enable = 0,

			.picWidth = SENSOR_WIDTH_SECOND,
			.picHeight = SENSOR_HEIGHT_SECOND,
		},
	},
};

IMPSensorInfo sensor_info;
int sample_system_init()
{
	int ret = 0;

	memset(&sensor_info, 0, sizeof(IMPSensorInfo));
	memcpy(sensor_info.name, SENSOR_NAME, sizeof(SENSOR_NAME));
	sensor_info.cbus_type = SENSOR_CUBS_TYPE;
	memcpy(sensor_info.i2c.type, SENSOR_NAME, sizeof(SENSOR_NAME));
	sensor_info.i2c.addr = SENSOR_I2C_ADDR;

	IMP_LOG_DBG(TAG, "sample_system_init start\n");

	ret = IMP_ISP_Open();
	if(ret < 0){
		IMP_LOG_ERR(TAG, "failed to open ISP\n");
		return -1;
	}

	ret = IMP_ISP_AddSensor(&sensor_info);
	if(ret < 0){
		IMP_LOG_ERR(TAG, "failed to AddSensor\n");
		return -1;
	}

	ret = IMP_ISP_EnableSensor();
	if(ret < 0){
		IMP_LOG_ERR(TAG, "failed to EnableSensor\n");
		return -1;
	}

	ret = IMP_System_Init();
	if(ret < 0){
		IMP_LOG_ERR(TAG, "IMP_System_Init failed\n");
		return -1;
	}

	/* enable turning, to debug graphics */
	ret = IMP_ISP_EnableTuning();
	if(ret < 0){
		IMP_LOG_ERR(TAG, "IMP_ISP_EnableTuning failed\n");
		return -1;
	}

	// set mode to day by default
	ret = IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_DAY);
	if(ret < 0){
		IMP_LOG_ERR(TAG, "unable to set ISP running mode\n");
		return -1;
	}

    ret = IMP_ISP_Tuning_SetSensorFPS(SENSOR_FRAME_RATE_NUM, SENSOR_FRAME_RATE_DEN);
    if (ret < 0){
        IMP_LOG_ERR(TAG, "failed to set sensor fps\n");
        return -1;
    }

	// try not to blow out the highlights
	// note that increasing this will decrease overall exposure
	ret = IMP_ISP_Tuning_SetHiLightDepress(10);
	if (ret < 0){
		IMP_LOG_ERR(TAG, "failed to set hilight depress\n");
		return -1;
	}

	IMP_LOG_INFO(TAG, "ImpSystemInit success\n");

	return 0;
}

int sample_system_exit()
{
	int ret = 0;

	IMP_LOG_DBG(TAG, "sample_system_exit start\n");


	IMP_System_Exit();

	ret = IMP_ISP_DisableSensor();
	if(ret < 0){
		IMP_LOG_ERR(TAG, "failed to EnableSensor\n");
		return -1;
	}

	ret = IMP_ISP_DelSensor(&sensor_info);
	if(ret < 0){
		IMP_LOG_ERR(TAG, "failed to AddSensor\n");
		return -1;
	}

	ret = IMP_ISP_DisableTuning();
	if(ret < 0){
		IMP_LOG_ERR(TAG, "IMP_ISP_DisableTuning failed\n");
		return -1;
	}

	if(IMP_ISP_Close()){
		IMP_LOG_ERR(TAG, "failed to open ISP\n");
		return -1;
	}

	IMP_LOG_DBG(TAG, " sample_system_exit success\n");

	return 0;
}

int sample_framesource_streamon()
{
	int ret = 0, i = 0;
	/* Enable channels */
	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = IMP_FrameSource_EnableChn(chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_FrameSource_EnableChn(%d) error: %d\n", ret, chn[i].index);
				return -1;
			}
		}
	}
	return 0;
}

int sample_framesource_ext_hsv_streamon()
{
	int ret = 0;
	/* Enable channels */
	ret = IMP_FrameSource_EnableChn(2);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_EnableChn(%d) error: %d\n", ret, 2);
		return -1;
	}
	return 0;
}

int sample_framesource_ext_bgra_streamon()
{
	int ret = 0;
	/* Enable channels */
	ret = IMP_FrameSource_EnableChn(2);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_EnableChn(%d) error: %d\n", ret, 2);
		return -1;
	}
	return 0;
}

int sample_framesource_streamoff()
{
	int ret = 0, i = 0;
	/* Enable channels */
	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable){
			ret = IMP_FrameSource_DisableChn(chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_FrameSource_DisableChn(%d) error: %d\n", ret, chn[i].index);
				return -1;
			}
		}
	}
	return 0;
}

int sample_framesource_ext_hsv_streamoff()
{
	int ret = 0;
	/* Enable channels */
	ret = IMP_FrameSource_DisableChn(2);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_DisableChn(%d) error: %d\n", ret, 2);
		return -1;
	}
	return 0;
}

int sample_framesource_ext_bgra_streamoff()
{
	int ret = 0;
	/* Enable channels */
	ret = IMP_FrameSource_DisableChn(2);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_DisableChn(%d) error: %d\n", ret, 2);
		return -1;
	}
	return 0;
}

int sample_framesource_init()
{
	int i, ret;

	for (i = 0; i <  FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = IMP_FrameSource_CreateChn(chn[i].index, &chn[i].fs_chn_attr);
			if(ret < 0){
				IMP_LOG_ERR(TAG, "IMP_FrameSource_CreateChn(chn%d) error !\n", chn[i].index);
				return -1;
			}

			ret = IMP_FrameSource_SetChnAttr(chn[i].index, &chn[i].fs_chn_attr);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_FrameSource_SetChnAttr(chn%d) error !\n",  chn[i].index);
				return -1;
			}
		}
	}

	return 0;
}

int sample_framesource_ext_hsv_init()
{
	int ret;
	ret = IMP_FrameSource_CreateChn(2, &chn_ext_hsv[0].fs_chn_attr);
	if(ret < 0){
		IMP_LOG_ERR(TAG, "IMP_FrameSource_CreateChn(chn%d) error !\n", 2);
		return -1;
	}

	ret = IMP_FrameSource_SetChnAttr(2, &chn_ext_hsv[0].fs_chn_attr);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_SetChnAttr(chn%d) error !\n",  2);
		return -1;
	}
	return 0;
}

int sample_framesource_ext_bgra_init()
{
	int ret;
	ret = IMP_FrameSource_CreateChn(2, &chn_ext_bgra[0].fs_chn_attr);
	if(ret < 0){
		IMP_LOG_ERR(TAG, "IMP_FrameSource_CreateChn(chn%d) error !\n", 2);
		return -1;
	}

	ret = IMP_FrameSource_SetChnAttr(2, &chn_ext_bgra[0].fs_chn_attr);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_SetChnAttr(chn%d) error !\n",  2);
		return -1;
	}
	return 0;
}

int sample_framesource_exit()
{
	int ret,i;

	for (i = 0; i <  FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			/*Destroy channel i*/
			ret = IMP_FrameSource_DestroyChn(i);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_FrameSource_DestroyChn() error: %d\n", ret);
				return -1;
			}
		}
	}
	return 0;
}

int sample_framesource_ext_hsv_exit()
{
	int ret;

	ret = IMP_FrameSource_DestroyChn(2);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_DestroyChn() error: %d\n", ret);
		return -1;
	}
	return 0;
}

int sample_framesource_ext_bgra_exit()
{
	int ret;

	ret = IMP_FrameSource_DestroyChn(2);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_DestroyChn() error: %d\n", ret);
		return -1;
	}
	return 0;
}

#if 0
// unused code
int sample_jpeg_init()
{
	int i, ret;
	IMPEncoderAttr *enc_attr;
	IMPEncoderCHNAttr channel_attr;
	IMPFSChnAttr *imp_chn_attr_tmp;

	for (i = 0; i <  FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			imp_chn_attr_tmp = &chn[i].fs_chn_attr;
			memset(&channel_attr, 0, sizeof(IMPEncoderCHNAttr));
			enc_attr = &channel_attr.encAttr;
			enc_attr->enType = PT_JPEG;
			enc_attr->bufSize = 0;
			enc_attr->profile = 0;
			enc_attr->picWidth = imp_chn_attr_tmp->picWidth;
			enc_attr->picHeight = imp_chn_attr_tmp->picHeight;

			/* Create Channel */
			ret = IMP_Encoder_CreateChn(2 + chn[i].index, &channel_attr);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error: %d\n",
							chn[i].index, ret);
				return -1;
			}

			/* Resigter Channel */
			ret = IMP_Encoder_RegisterChn(i, 2 + chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_RegisterChn(0, %d) error: %d\n",
							chn[i].index, ret);
				return -1;
			}
		}
	}

	return 0;
}
#endif

int sample_encoder_init()
{
	int i, ret;
	IMPFSChnAttr *imp_chn_attr_tmp;
	IMPEncoderCHNAttr channel_attr;

	for (i = 0; i <  FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			imp_chn_attr_tmp = &chn[i].fs_chn_attr;
			memset(&channel_attr, 0, sizeof(IMPEncoderCHNAttr));

#ifdef PLATFORM_T31
			unsigned int uTargetBitRate = (double)2000.0 * (imp_chn_attr_tmp->picWidth * imp_chn_attr_tmp->picHeight) / (1280 * 720);
			ret = IMP_Encoder_SetDefaultParam(&channel_attr, IMP_ENC_PROFILE_AVC_HIGH, S_RC_METHOD,
					imp_chn_attr_tmp->picWidth, imp_chn_attr_tmp->picHeight,
					imp_chn_attr_tmp->outFrmRateNum, imp_chn_attr_tmp->outFrmRateDen,
					imp_chn_attr_tmp->outFrmRateNum * 2 / imp_chn_attr_tmp->outFrmRateDen, 2,
					(S_RC_METHOD == IMP_ENC_RC_MODE_FIXQP) ? 35 : -1,
					uTargetBitRate);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_SetDefaultParam(%d) error !\n", chn[i].index);
				return -1;
			}

#else
			IMPEncoderRcAttr *rc_attr;
			IMPEncoderAttr *enc_attr;

			enc_attr = &channel_attr.encAttr;
			enc_attr->enType = PT_H264;
			enc_attr->bufSize = 0;
			enc_attr->profile = 1;
			enc_attr->picWidth = imp_chn_attr_tmp->picWidth;
			enc_attr->picHeight = imp_chn_attr_tmp->picHeight;
			rc_attr = &channel_attr.rcAttr;
            rc_attr->outFrmRate.frmRateNum = imp_chn_attr_tmp->outFrmRateNum;
            rc_attr->outFrmRate.frmRateDen = imp_chn_attr_tmp->outFrmRateDen;
            rc_attr->maxGop = 2 * rc_attr->outFrmRate.frmRateNum / rc_attr->outFrmRate.frmRateDen;
            if (S_RC_METHOD == ENC_RC_MODE_CBR) {
                rc_attr->attrRcMode.rcMode = ENC_RC_MODE_CBR;
                rc_attr->attrRcMode.attrH264Cbr.outBitRate = (double)2000.0 * (imp_chn_attr_tmp->picWidth * imp_chn_attr_tmp->picHeight) / (1280 * 720);
                rc_attr->attrRcMode.attrH264Cbr.maxQp = 45;
                rc_attr->attrRcMode.attrH264Cbr.minQp = 15;
                rc_attr->attrRcMode.attrH264Cbr.iBiasLvl = 0;
                rc_attr->attrRcMode.attrH264Cbr.frmQPStep = 3;
                rc_attr->attrRcMode.attrH264Cbr.gopQPStep = 15;
                rc_attr->attrRcMode.attrH264Cbr.adaptiveMode = false;
                rc_attr->attrRcMode.attrH264Cbr.gopRelation = false;

                rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
                rc_attr->attrHSkip.hSkipAttr.m = 0;
                rc_attr->attrHSkip.hSkipAttr.n = 0;
                rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
                rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
                rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
                rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
            } else if (S_RC_METHOD == ENC_RC_MODE_VBR) {
                rc_attr->attrRcMode.rcMode = ENC_RC_MODE_VBR;
                rc_attr->attrRcMode.attrH264Vbr.maxQp = 45;
                rc_attr->attrRcMode.attrH264Vbr.minQp = 15;
                rc_attr->attrRcMode.attrH264Vbr.staticTime = 2;
                rc_attr->attrRcMode.attrH264Vbr.maxBitRate = (double)2000.0 * (imp_chn_attr_tmp->picWidth * imp_chn_attr_tmp->picHeight) / (1280 * 720);
                rc_attr->attrRcMode.attrH264Vbr.iBiasLvl = 0;
                rc_attr->attrRcMode.attrH264Vbr.changePos = 80;
                rc_attr->attrRcMode.attrH264Vbr.qualityLvl = 2;
                rc_attr->attrRcMode.attrH264Vbr.frmQPStep = 3;
                rc_attr->attrRcMode.attrH264Vbr.gopQPStep = 15;
                rc_attr->attrRcMode.attrH264Vbr.gopRelation = false;

                rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
                rc_attr->attrHSkip.hSkipAttr.m = 0;
                rc_attr->attrHSkip.hSkipAttr.n = 0;
                rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
                rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
                rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
                rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
            } else if (S_RC_METHOD == ENC_RC_MODE_SMART) {
                rc_attr->attrRcMode.rcMode = ENC_RC_MODE_SMART;
                rc_attr->attrRcMode.attrH264Smart.maxQp = 45;
                rc_attr->attrRcMode.attrH264Smart.minQp = 15;
                rc_attr->attrRcMode.attrH264Smart.staticTime = 2;
                rc_attr->attrRcMode.attrH264Smart.maxBitRate = (double)2000.0 * (imp_chn_attr_tmp->picWidth * imp_chn_attr_tmp->picHeight) / (1280 * 720);
                rc_attr->attrRcMode.attrH264Smart.iBiasLvl = 0;
                rc_attr->attrRcMode.attrH264Smart.changePos = 80;
                rc_attr->attrRcMode.attrH264Smart.qualityLvl = 2;
                rc_attr->attrRcMode.attrH264Smart.frmQPStep = 3;
                rc_attr->attrRcMode.attrH264Smart.gopQPStep = 15;
                rc_attr->attrRcMode.attrH264Smart.gopRelation = false;

                rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
                rc_attr->attrHSkip.hSkipAttr.m = rc_attr->maxGop - 1;
                rc_attr->attrHSkip.hSkipAttr.n = 1;
                rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 6;
                rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
                rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
                rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
            } else { /* fixQp */
                rc_attr->attrRcMode.rcMode = ENC_RC_MODE_FIXQP;
                rc_attr->attrRcMode.attrH264FixQp.qp = 42;

                rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
                rc_attr->attrHSkip.hSkipAttr.m = 0;
                rc_attr->attrHSkip.hSkipAttr.n = 0;
                rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
                rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
                rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
                rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
            }
#endif

			ret = IMP_Encoder_CreateChn(chn[i].index, &channel_attr);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error !\n", i);
				return -1;
			}

			ret = IMP_Encoder_RegisterChn(chn[i].index, chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_RegisterChn(%d, %d) error: %d\n",
						chn[i].index, chn[i].index, ret);
				return -1;
			}
		}
	}

	return 0;
}

static int encoder_chn_exit(int encChn)
{
	int ret;
	IMPEncoderCHNStat chn_stat;
	ret = IMP_Encoder_Query(encChn, &chn_stat);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_Query(%d) error: %d\n",
					encChn, ret);
		return -1;
	}

	if (chn_stat.registered) {
		ret = IMP_Encoder_UnRegisterChn(encChn);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "IMP_Encoder_UnRegisterChn(%d) error: %d\n",
						encChn, ret);
			return -1;
		}

		ret = IMP_Encoder_DestroyChn(encChn);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "IMP_Encoder_DestroyChn(%d) error: %d\n",
						encChn, ret);
			return -1;
		}
	}

	return 0;
}

int sample_encoder_exit(void)
{
	int ret;

	ret = encoder_chn_exit(ENC_H264_CHANNEL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Encoder Channel %d exit  error: %d\n",
					ENC_H264_CHANNEL, ret);
		return -1;
	}

	ret = encoder_chn_exit(ENC_JPEG_CHANNEL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Encoder Channel %d exit  error: %d\n",
					ENC_JPEG_CHANNEL, ret);
		return -1;
	}

	ret = IMP_Encoder_DestroyGroup(0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_DestroyGroup(0) error: %d\n", ret);
		return -1;
	}

	return 0;
}

IMPRgnHandle *sample_osd_init(int grpNum)
{
	int ret;
	IMPRgnHandle *prHander;
	IMPRgnHandle rHanderFont;
	IMPRgnHandle rHanderLogo;
	IMPRgnHandle rHanderCover;
	IMPRgnHandle rHanderRect;

	prHander = malloc(4 * sizeof(IMPRgnHandle));
	if (prHander <= 0) {
		IMP_LOG_ERR(TAG, "malloc() error !\n");
		return NULL;
	}

	rHanderFont = IMP_OSD_CreateRgn(NULL);
	if (rHanderFont == INVHANDLE) {
		IMP_LOG_ERR(TAG, "IMP_OSD_CreateRgn TimeStamp error !\n");
		return NULL;
	}

	rHanderLogo = IMP_OSD_CreateRgn(NULL);
	if (rHanderLogo == INVHANDLE) {
		IMP_LOG_ERR(TAG, "IMP_OSD_CreateRgn Logo error !\n");
		return NULL;
	}

	rHanderCover = IMP_OSD_CreateRgn(NULL);
	if (rHanderCover == INVHANDLE) {
		IMP_LOG_ERR(TAG, "IMP_OSD_CreateRgn Cover error !\n");
		return NULL;
	}

	rHanderRect = IMP_OSD_CreateRgn(NULL);
	if (rHanderRect == INVHANDLE) {
		IMP_LOG_ERR(TAG, "IMP_OSD_CreateRgn Rect error !\n");
		return NULL;
	}


	ret = IMP_OSD_RegisterRgn(rHanderFont, grpNum, NULL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IVS IMP_OSD_RegisterRgn failed\n");
		return NULL;
	}

	ret = IMP_OSD_RegisterRgn(rHanderLogo, grpNum, NULL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IVS IMP_OSD_RegisterRgn failed\n");
		return NULL;
	}

	ret = IMP_OSD_RegisterRgn(rHanderCover, grpNum, NULL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IVS IMP_OSD_RegisterRgn failed\n");
		return NULL;
	}

	ret = IMP_OSD_RegisterRgn(rHanderRect, grpNum, NULL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IVS IMP_OSD_RegisterRgn failed\n");
		return NULL;
	}


	IMPOSDRgnAttr rAttrFont;
	memset(&rAttrFont, 0, sizeof(IMPOSDRgnAttr));
	rAttrFont.type = OSD_REG_PIC;
	rAttrFont.rect.p0.x = 10;
	rAttrFont.rect.p0.y = 10;
	rAttrFont.rect.p1.x = rAttrFont.rect.p0.x + 20 * OSD_REGION_WIDTH- 1;   //p0 is start，and p1 well be epual p0+width(or heigth)-1
	rAttrFont.rect.p1.y = rAttrFont.rect.p0.y + OSD_REGION_HEIGHT - 1;
#ifdef SUPPORT_RGB555LE
	rAttrFont.fmt = PIX_FMT_RGB555LE;
#else
	rAttrFont.fmt = PIX_FMT_BGRA;
#endif
	rAttrFont.data.picData.pData = NULL;
	ret = IMP_OSD_SetRgnAttr(rHanderFont, &rAttrFont);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_SetRgnAttr TimeStamp error !\n");
		return NULL;
	}

	IMPOSDGrpRgnAttr grAttrFont;

	if (IMP_OSD_GetGrpRgnAttr(rHanderFont, grpNum, &grAttrFont) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_GetGrpRgnAttr Logo error !\n");
		return NULL;

	}
	memset(&grAttrFont, 0, sizeof(IMPOSDGrpRgnAttr));
	grAttrFont.show = 0;

	/* Disable Font global alpha, only use pixel alpha. */
	grAttrFont.gAlphaEn = 1;
	grAttrFont.fgAlhpa = 0xff;
	grAttrFont.layer = 3;
	if (IMP_OSD_SetGrpRgnAttr(rHanderFont, grpNum, &grAttrFont) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_SetGrpRgnAttr Logo error !\n");
		return NULL;
	}

	IMPOSDRgnAttr rAttrLogo;
	memset(&rAttrLogo, 0, sizeof(IMPOSDRgnAttr));
	int picw = 100;
	int pich = 100;
	rAttrLogo.type = OSD_REG_PIC;
	rAttrLogo.rect.p0.x = SENSOR_WIDTH - 100;
	rAttrLogo.rect.p0.y = SENSOR_HEIGHT - 100;
	rAttrLogo.rect.p1.x = rAttrLogo.rect.p0.x+picw-1;     //p0 is start，and p1 well be epual p0+width(or heigth)-1
	rAttrLogo.rect.p1.y = rAttrLogo.rect.p0.y+pich-1;
	rAttrLogo.fmt = PIX_FMT_BGRA;
	rAttrLogo.data.picData.pData = logodata_100x100_bgra;
	ret = IMP_OSD_SetRgnAttr(rHanderLogo, &rAttrLogo);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_SetRgnAttr Logo error !\n");
		return NULL;
	}
	IMPOSDGrpRgnAttr grAttrLogo;

	if (IMP_OSD_GetGrpRgnAttr(rHanderLogo, grpNum, &grAttrLogo) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_GetGrpRgnAttr Logo error !\n");
		return NULL;

	}
	memset(&grAttrLogo, 0, sizeof(IMPOSDGrpRgnAttr));
	grAttrLogo.show = 0;

	/* Set Logo global alpha to 0x7f, it is semi-transparent. */
	grAttrLogo.gAlphaEn = 1;
	grAttrLogo.fgAlhpa = 0x7f;
	grAttrLogo.layer = 2;

	if (IMP_OSD_SetGrpRgnAttr(rHanderLogo, grpNum, &grAttrLogo) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_SetGrpRgnAttr Logo error !\n");
		return NULL;
	}

	IMPOSDRgnAttr rAttrCover;
	memset(&rAttrCover, 0, sizeof(IMPOSDRgnAttr));
	rAttrCover.type = OSD_REG_COVER;
	rAttrCover.rect.p0.x = SENSOR_WIDTH/2-100;
	rAttrCover.rect.p0.y = SENSOR_HEIGHT/2-100;
	rAttrCover.rect.p1.x = rAttrCover.rect.p0.x+SENSOR_WIDTH/2-1+50;     //p0 is start，and p1 well be epual p0+width(or heigth)-1
	rAttrCover.rect.p1.y = rAttrCover.rect.p0.y+SENSOR_HEIGHT/2-1+50;
	rAttrCover.fmt = PIX_FMT_BGRA;
	rAttrCover.data.coverData.color = OSD_BLACK;
	ret = IMP_OSD_SetRgnAttr(rHanderCover, &rAttrCover);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_SetRgnAttr Cover error !\n");
		return NULL;
	}
	IMPOSDGrpRgnAttr grAttrCover;

	if (IMP_OSD_GetGrpRgnAttr(rHanderCover, grpNum, &grAttrCover) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_GetGrpRgnAttr Cover error !\n");
		return NULL;

	}
	memset(&grAttrCover, 0, sizeof(IMPOSDGrpRgnAttr));
	grAttrCover.show = 0;

	/* Disable Cover global alpha, it is absolutely no transparent. */
	grAttrCover.gAlphaEn = 1;
	grAttrCover.fgAlhpa = 0x7f;
	grAttrCover.layer = 2;
	if (IMP_OSD_SetGrpRgnAttr(rHanderCover, grpNum, &grAttrCover) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_SetGrpRgnAttr Cover error !\n");
		return NULL;
	}

	IMPOSDRgnAttr rAttrRect;
	memset(&rAttrRect, 0, sizeof(IMPOSDRgnAttr));

	rAttrRect.type = OSD_REG_RECT;
	rAttrRect.rect.p0.x = 300;
	rAttrRect.rect.p0.y = 300;
	rAttrRect.rect.p1.x = rAttrRect.rect.p0.x + 600 - 1;
	rAttrRect.rect.p1.y = rAttrRect.rect.p0.y + 300 - 1;
	rAttrRect.fmt = PIX_FMT_MONOWHITE;
	rAttrRect.data.lineRectData.color = OSD_GREEN;
	rAttrRect.data.lineRectData.linewidth = 5;
	ret = IMP_OSD_SetRgnAttr(rHanderRect, &rAttrRect);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_SetRgnAttr Rect error !\n");
		return NULL;
	}
	IMPOSDGrpRgnAttr grAttrRect;

	if (IMP_OSD_GetGrpRgnAttr(rHanderRect, grpNum, &grAttrRect) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_GetGrpRgnAttr Rect error !\n");
		return NULL;

	}
	memset(&grAttrRect, 0, sizeof(IMPOSDGrpRgnAttr));
	grAttrRect.show = 0;
	grAttrRect.layer = 1;
	grAttrRect.scalex = 1;
	grAttrRect.scaley = 1;
	if (IMP_OSD_SetGrpRgnAttr(rHanderRect, grpNum, &grAttrRect) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_SetGrpRgnAttr Rect error !\n");
		return NULL;
	}


	ret = IMP_OSD_Start(grpNum);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_Start TimeStamp, Logo, Cover and Rect error !\n");
		return NULL;
	}

	prHander[0] = rHanderFont;
	prHander[1] = rHanderLogo;
	prHander[2] = rHanderCover;
	prHander[3] = rHanderRect;
	return prHander;
}

int sample_osd_exit(IMPRgnHandle *prHander,int grpNum)
{
	int ret;

	ret = IMP_OSD_ShowRgn(prHander[0], grpNum, 0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn close timeStamp error\n");
	}

	ret = IMP_OSD_ShowRgn(prHander[1], grpNum, 0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn close Logo error\n");
	}

	ret = IMP_OSD_ShowRgn(prHander[2], grpNum, 0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn close cover error\n");
	}

	ret = IMP_OSD_ShowRgn(prHander[3], grpNum, 0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn close Rect error\n");
	}


	ret = IMP_OSD_UnRegisterRgn(prHander[0], grpNum);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_UnRegisterRgn timeStamp error\n");
	}

	ret = IMP_OSD_UnRegisterRgn(prHander[1], grpNum);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_UnRegisterRgn logo error\n");
	}

	ret = IMP_OSD_UnRegisterRgn(prHander[2], grpNum);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_UnRegisterRgn Cover error\n");
	}

	ret = IMP_OSD_UnRegisterRgn(prHander[3], grpNum);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_UnRegisterRgn Rect error\n");
	}


	IMP_OSD_DestroyRgn(prHander[0]);
	IMP_OSD_DestroyRgn(prHander[1]);
	IMP_OSD_DestroyRgn(prHander[2]);
	IMP_OSD_DestroyRgn(prHander[3]);

	ret = IMP_OSD_DestroyGroup(grpNum);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_DestroyGroup(0) error\n");
		return -1;
	}
	free(prHander);
	prHander = NULL;

	return 0;
}

#if 0
static int save_stream(int fd, IMPEncoderStream *stream)
{
	int ret, i, nr_pack = stream->packCount;

	for (i = 0; i < nr_pack; i++) {
		ret = write(fd, (void *)stream->pack[i].virAddr,
					stream->pack[i].length);
		if (ret != stream->pack[i].length) {
			IMP_LOG_ERR(TAG, "stream write error:%s\n", strerror(errno));
			return -1;
		}
		//IMP_LOG_DBG(TAG, "stream->pack[%d].dataType=%d\n", i, ((int)stream->pack[i].dataType.h264Type));
	}

	return 0;
}

int sample_do_get_h264_stream(int nr_frames)
{
	int ret;

	/* H264 Channel start receive picture */
	ret = IMP_Encoder_StartRecvPic(ENC_H264_CHANNEL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", ENC_H264_CHANNEL);
		return -1;
	}

	time_t now;
	time(&now);
	struct tm *now_tm;
	now_tm = localtime(&now);
	char now_str[32];
	strftime(now_str, 40, "%Y%m%d%I%M%S", now_tm);

	char stream_path[128];
	sprintf(stream_path, "%s/stream-%s.h264",
			STREAM_FILE_PATH_PREFIX, now_str);

	IMP_LOG_INFO(TAG, "Open Stream file %s ", stream_path);
	int stream_fd = open(stream_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (stream_fd < 0) {
		IMP_LOG_ERR(TAG, "failed: %s\n", strerror(errno));
		return -1;
	}
	IMP_LOG_INFO(TAG, "OK\n");

	int i;
	for (i = 0; i < nr_frames; i++) {
		/* Polling H264 Stream, set timeout as 1000msec */
		ret = IMP_Encoder_PollingStream(ENC_H264_CHANNEL, 1000);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "Polling stream timeout\n");
			continue;
		}

		IMPEncoderStream stream;
		/* Get H264 Stream */
		ret = IMP_Encoder_GetStream(ENC_H264_CHANNEL, &stream, 1);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
			return -1;
		}

		ret = save_stream(stream_fd, &stream);
		if (ret < 0) {
			close(stream_fd);
			return ret;
		}

		IMP_Encoder_ReleaseStream(ENC_H264_CHANNEL, &stream);
	}

	close(stream_fd);

	ret = IMP_Encoder_StopRecvPic(ENC_H264_CHANNEL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");
		return -1;
	}

	return 0;
}

int sample_do_get_jpeg_snap(void)
{
	int ret;

	/* JEPG Channel start receive picture */
	ret = IMP_Encoder_StartRecvPic(ENC_JPEG_CHANNEL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", ENC_JPEG_CHANNEL);
		return -1;
	}

	time_t now;
	time(&now);
	struct tm *now_tm;
	now_tm = localtime(&now);
	char now_str[32];
	strftime(now_str, 40, "%Y%m%d%I%M%S", now_tm);

	char snap_path[128];
	sprintf(snap_path, "%s/snap-%s.jpg",
			SNAP_FILE_PATH_PREFIX, now_str);

	IMP_LOG_ERR(TAG, "Open Snap file %s ", snap_path);
	int snap_fd = open(snap_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (snap_fd < 0) {
		IMP_LOG_ERR(TAG, "failed: %s\n", strerror(errno));
		return -1;
	}
	IMP_LOG_DBG(TAG, "OK\n");

	/* Polling JPEG Snap, set timeout as 1000msec */
	ret = IMP_Encoder_PollingStream(ENC_JPEG_CHANNEL, 1000);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Polling stream timeout\n");
		return -1;
	}

	IMPEncoderStream stream;
	/* Get JPEG Snap */
	ret = IMP_Encoder_GetStream(ENC_JPEG_CHANNEL, &stream, 1);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
		return -1;
	}

	ret = save_stream(snap_fd, &stream);
	if (ret < 0) {
		close(snap_fd);
		return ret;
	}

	IMP_Encoder_ReleaseStream(ENC_JPEG_CHANNEL, &stream);

	close(snap_fd);

	ret = IMP_Encoder_StopRecvPic(ENC_JPEG_CHANNEL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");
		return -1;
	}

	return 0;
}

void *get_h264_stream(void *args)
{
	int i, j, ret;
	char stream_path[64];

	i = (int ) (*((int*)args));

	ret = IMP_Encoder_StartRecvPic(i);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", i);
		return ((void *)-1);
	}

	sprintf(stream_path, "%s/stream-%d.h264",
			STREAM_FILE_PATH_PREFIX, i);

	IMP_LOG_DBG(TAG, "Open Stream file %s ", stream_path);
	int stream_fd = open(stream_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (stream_fd < 0) {
		IMP_LOG_ERR(TAG, "failed: %s\n", strerror(errno));
		return ((void *)-1);
	}
	IMP_LOG_DBG(TAG, "OK\n");

	for (j = 0; j < NR_FRAMES_TO_SAVE; j++) {
		ret = IMP_Encoder_PollingStream(i, 1000);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "Polling stream timeout\n");
			continue;
		}

		IMPEncoderStream stream;
		/* Get H264 Stream */
		ret = IMP_Encoder_GetStream(i, &stream, 1);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
			return ((void *)-1);
		}
		//IMP_LOG_DBG(TAG, "i=%d, stream.packCount=%d, stream.h264RefType=%d\n", i, stream.packCount, stream.h264RefType);

		ret = save_stream(stream_fd, &stream);
		if (ret < 0) {
			close(stream_fd);
			return ((void *)ret);
		}

		IMP_Encoder_ReleaseStream(i, &stream);
	}

	close(stream_fd);

	ret = IMP_Encoder_StopRecvPic(i);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");
		return ((void *)-1);
	}

	return ((void *)0);
}

int sample_get_h264_stream()
{
	unsigned int i;
	int ret;
	pthread_t tid[FS_CHN_NUM];

	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = pthread_create(&tid[i], NULL, get_h264_stream, &chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "Create Chn%d get_h264_stream \n",chn[i].index);
			}
		}
	}

	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			pthread_join(tid[i],NULL);
		}
	}

	return 0;
}

int sample_get_jpeg_snap()
{
	int i, ret;
	char snap_path[64];

	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = IMP_Encoder_StartRecvPic(2 + chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", 2 + chn[i].index);
				return -1;
			}

			sprintf(snap_path, "%s/snap-%d.jpg",
					SNAP_FILE_PATH_PREFIX, chn[i].index);

			IMP_LOG_ERR(TAG, "Open Snap file %s ", snap_path);
			int snap_fd = open(snap_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (snap_fd < 0) {
				IMP_LOG_ERR(TAG, "failed: %s\n", strerror(errno));
				return -1;
			}
			IMP_LOG_DBG(TAG, "OK\n");

			/* Polling JPEG Snap, set timeout as 1000msec */
			ret = IMP_Encoder_PollingStream(2 + chn[i].index, 1000);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "Polling stream timeout\n");
				continue;
			}

			IMPEncoderStream stream;
			/* Get JPEG Snap */
			ret = IMP_Encoder_GetStream(chn[i].index + 2, &stream, 1);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
				return -1;
			}

			ret = save_stream(snap_fd, &stream);
			if (ret < 0) {
				close(snap_fd);
				return ret;
			}

			IMP_Encoder_ReleaseStream(2 + chn[i].index, &stream);

			close(snap_fd);

			ret = IMP_Encoder_StopRecvPic(2 + chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");
				return -1;
			}
		}
	}
	return 0;
}
#endif

#define STRINGIFY__(a) #a
#define STRINGIFY(a) STRINGIFY__(a)

// Enables the IR cut filter, which blocks out IR light for day time.
// enable=1 for day time, enable=0 for night.
int sample_set_IRCUT(int enable)
{
	int fd1, fd2;
	fd1 = open("/sys/class/gpio/gpio" STRINGIFY(IRCUT_DIS_GPIO) "/value", O_RDWR);
	if (fd1 < 0) {
		IMP_LOG_DBG(TAG, "open gpio " STRINGIFY(IRCUT_DIS_GPIO) " error !");
		return -1;
	}
	fd2 = open("/sys/class/gpio/gpio" STRINGIFY(IRCUT_EN_GPIO) "/value", O_RDWR);
	if (fd2 < 0) {
		close(fd1);
		IMP_LOG_DBG(TAG, "open gpio " STRINGIFY(IRCUT_EN_GPIO) " error !");
		return -1;
	}
	if (enable) {
		write(fd2, "1", 1);
	} else {
		write(fd1, "1", 1);
	}

	usleep(500*1000);
	write(fd1, "0", 1);
	write(fd2, "0", 1);

	close(fd1);
	close(fd2);
	return 0;
}

char *get_curr_timestr(char *buf) {
	time_t t;
	struct tm *tminfo;

	time(&t);
	tminfo = localtime(&t);
	sprintf(buf, "%02d:%02d:%02d", tminfo->tm_hour, tminfo->tm_min, tminfo->tm_sec);
	return buf;
}

#define EXP_LENGTH (8 /*sec*/ * 24 /*cycles*/)
#define NUM_EXP_VALUES (EXP_LENGTH / 8)


static int ir_illumination = 1;		// use IR LEDs when dark
static int force_color = 0;			// use color mode, even at night

int set_cam_option(const char *option, int value) {
	if (strcmp(option, "ir_leds") == 0) {
		ir_illumination = value;
	} else if (strcmp(option, "force_color") == 0) {
		force_color = value;
	} else {
		return -1;	// unknown option
	}

	return 0;
}

static int  g_soft_ps_running = 1;
void *sample_soft_photosensitive_ctrl(void *p)
{
	int evDebugCount = 0;
	char tmstr[16];
	int avgExp;
	int expCount = 0;
	int expValues[NUM_EXP_VALUES];
	int expPos = 0;
	IMPISPRunningMode pmode = IMPISP_RUNNING_MODE_BUTT;
	int ir_leds_active = 0;
	int r;

	// initialize PWM
	struct pwm_ioctl_t pwm_cfg = {
		.channel = 0,
		.period  = 1000000,
		.duty    = 0,
		.polarity = 1,
	};

	if (pwm_init() < 0) {
		IMP_LOG_INFO(TAG, "cant pwm_init(): errno %d", errno);
	} else if ((r = pwm_config(&pwm_cfg)) < 0) {
		IMP_LOG_INFO(TAG, "cant config pwm: %d, errno %d", r, errno);
	} else if ((r = pwm_enable(pwm_cfg.channel)) < 0) {
		IMP_LOG_INFO(TAG, "cant enable pwm channel: %d, errno %d", r, errno);
	}

	// make sure it's in an "uninitialized" state
	expValues[NUM_EXP_VALUES - 1] = 0;

	while (g_soft_ps_running) {
		IMPISPEVAttr expAttr;
		int ret = IMP_ISP_Tuning_GetEVAttr(&expAttr);
		if (ret == 0) {
			if (evDebugCount > 0) {
				IMP_LOG_DBG(TAG, "EV attr: exp %d aGain %d dGain %d\n",
						expAttr.ev, expAttr.again, expAttr.dgain);
				evDebugCount--;
			}
		} else
			return NULL;

		if (expCount == 0) {
			avgExp = expAttr.ev;
		} else {
			// exponential moving average
			avgExp -= avgExp / expCount;
			avgExp += expAttr.ev / expCount;
		}

		if (expCount < 3) expCount++;

		// keep a longer running average exposure
		if (expPos % 8 == 0)
			expValues[expPos / 8] = avgExp;
		if (++expPos >= EXP_LENGTH)
			expPos = 0;

		// calculate long average exposure value
		int longExpValue = 0;
		if (expValues[NUM_EXP_VALUES - 1] == 0) {
			longExpValue = expValues[0];
		} else {
			int stopPos = (expPos / 8) - 8;	// ignore recent 8 cycles
			if (stopPos < 0) stopPos += NUM_EXP_VALUES;

			int i, j;
			for (i = 1, j = expPos / 8;
					j < stopPos;
					i++, j = (j + 1) % NUM_EXP_VALUES)
				longExpValue += (expValues[j] - longExpValue) / i;
		}


		if (avgExp > EXP_NIGHT_THRESHOLD) {
			if (pmode != IMPISP_RUNNING_MODE_NIGHT) {
				pmode = IMPISP_RUNNING_MODE_NIGHT;
				IMP_LOG_INFO(TAG, "[%s] avg exp is %d. switching to night mode\n",
						get_curr_timestr((char *) &tmstr), avgExp);
				evDebugCount = 10; // start logging 10s of EV data
				if (! force_color) {
					IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_NIGHT);
					IMP_ISP_Tuning_SetSensorFPS(10, SENSOR_FRAME_RATE_DEN);
				} else {
					IMP_LOG_INFO(TAG, "[%s] force_color is on\n",
						get_curr_timestr((char *) &tmstr));
				}
				sample_set_IRCUT(0);
			}
		} else if (avgExp < EXP_DAY_THRESHOLD) {
			if (pmode != IMPISP_RUNNING_MODE_DAY) {
				pmode = IMPISP_RUNNING_MODE_DAY;
				IMP_LOG_INFO(TAG, "[%s] avg exp is %d. switching to day mode\n",
						get_curr_timestr((char *) &tmstr), avgExp);
				evDebugCount = 10; // start logging 10s of EV data

				if (! force_color) {
					IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_DAY);
					IMP_ISP_Tuning_SetSensorFPS(SENSOR_FRAME_RATE_NUM, SENSOR_FRAME_RATE_DEN);
				}
				sample_set_IRCUT(1);
			}
		}

		// control IR LEDs
		if (avgExp > EXP_IR_THRESHOLD) {
			// only log for first time
			if (! ir_leds_active) {
				IMP_LOG_INFO(TAG, "[%s] avg exp is %d. turning on IR LEDs\n",
						get_curr_timestr((char *) &tmstr), avgExp);
				evDebugCount = 10; // start logging 10s of EV data
			}

			// it's ok to go higher, but not lower than long-running average
			// this prevents sudden bright bursts of light from dimming IR LEDs
			int ev = (avgExp > longExpValue) ? avgExp : longExpValue;

			// map 3 to 13mil => 1 to 1mil for PWM
#ifdef PLATFORM_T31
			int level = (ev - EXP_IR_THRESHOLD) * 3;
#else
			int level = (ev - EXP_IR_THRESHOLD) / 10;
#endif

			// cap to maximum
			if (level > pwm_cfg.period)
				level = pwm_cfg.period;

			if (ir_illumination) pwm_set_duty(pwm_cfg.channel, level);
			ir_leds_active = 1;
		} else if (ir_leds_active) {
			IMP_LOG_INFO(TAG, "[%s] avg exp is %d. turning off IR LEDs\n",
						get_curr_timestr((char *) &tmstr), avgExp);
			evDebugCount = 10; // start logging 10s of EV data

			if (ir_illumination) pwm_set_duty(pwm_cfg.channel, 0);
			ir_leds_active = 0;
		}

end:
		sleep(1);
	}

	return NULL;
}

