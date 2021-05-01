#include <string.h>    
#include <errno.h>      

#include <imp/imp_isp.h>
#include <imp/imp_system.h>
#include <imp/imp_log.h>
#include <imp/imp_framesource.h>
#ifdef PLATFORM_T31
  #include <imp/imp_encoder_t31.h>
#else
  #include <imp/imp_encoder.h>
#endif

#include "imp-common.h" 
#include "capture_and_encoding.h"

#define TAG "capture_and_encoding"

extern struct chn_conf chn[];

extern "C" {
extern int IMP_OSD_SetPoolSize(int newPoolSize);
extern int IMP_Encoder_SetPoolSize(int newPoolSize0);
}

int destory()
{

	int ret, i;

	/* Exit sequence as follow */
	/* Step.a Stream Off */
	ret = sample_framesource_streamoff();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "FrameSource StreamOff failed\n");
		return -1;
	}

	/* Step.b UnBind */
	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = IMP_System_UnBind(&chn[i].framesource_chn, &chn[i].imp_encoder);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "UnBind FrameSource channel%d and Encoder failed\n",i);
				return -1;
			}
		}
	}

	/* Step.c Encoder exit */
	ret = sample_encoder_exit();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Encoder exit failed\n");
		return -1;
	}

	/* Step.d FrameSource exit */
	ret = sample_framesource_exit();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "FrameSource exit failed\n");
		return -1;
	}

	/* Step.e System exit */
	ret = sample_system_exit();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "sample_system_exit() failed\n");
		return -1;
	}

	ret = IMP_Encoder_StopRecvPic(0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");
		return -1;
	}

	return 0;
}

static int save_stream(int fd, IMPEncoderStream *stream)
{
	int ret, i, nr_pack = stream->packCount;

	for (i = 0; i < nr_pack; i++) {
#ifdef PLATFORM_T31
		IMPEncoderPack *pack = &stream->pack[i];
		//IMP_LOG_DBG(TAG, "stream vir %08x cnt %d ss %d, p%d off %d len %d",
		//	stream->virAddr, stream->packCount, stream->streamSize, 
		//	i, pack->offset, pack->length);

		if (pack->length) {
			uint32_t sz = stream->streamSize - pack->offset;
			if (sz < pack->length) {
				// write offset to end part first?
				ret = write(fd, (void *) (stream->virAddr + pack->offset), sz);
				if (ret != (ssize_t) sz) {
					IMP_LOG_ERR(TAG, "stream write error:%s\n", strerror(errno));
					return -1;
				}

				ret = write(fd, (void *) stream->virAddr, pack->length - sz);
				if (ret != (ssize_t) (pack->length - sz)) {
					IMP_LOG_ERR(TAG, "stream write error:%s\n", strerror(errno));
					return -1;
				}
			} else {
				ret = write(fd, (void *) (stream->virAddr + pack->offset), pack->length);
				if (ret != (ssize_t) pack->length) {
					IMP_LOG_ERR(TAG, "stream write error:%s\n", strerror(errno));
					return -1;
				}
			}
		}
#else
		ret = write(fd, (void *)stream->pack[i].virAddr,
					stream->pack[i].length);
		if (ret != stream->pack[i].length) {
			IMP_LOG_ERR(TAG, "stream write error:%s\n", strerror(errno));
			return -1;
		}
		//IMP_LOG_DBG(TAG, "stream->pack[%d].dataType=%d\n", i, ((int)stream->pack[i].dataType.h264Type));
#endif
	}

	return 0;
}


static int get_h264_stream(int fd, int chn)
{
	int ret;
	
	/* Polling H264 Stream, set timeout as 1000msec */
	ret = IMP_Encoder_PollingStream(chn, 100);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Polling stream timeout\n");
	}

	IMPEncoderStream stream;
	/* Get H264 Stream */
	ret = IMP_Encoder_GetStream(chn, &stream, 1);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
		return -1;
	}
	
	ret = save_stream(fd, &stream);
	if (ret < 0) {
		close(fd);
		return ret;
	}
	
	IMP_Encoder_ReleaseStream(chn, &stream);

	return 0;
}

int get_stream(int fd, int chn)
{
	int  ret;
	
	ret = IMP_Encoder_StartRecvPic(chn);
	if (ret < 0){
		IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", 1);
		return ret;
	}
	ret = get_h264_stream(fd, chn);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Get H264 stream failed\n");
		return ret;
	}
/*	ret = IMP_Encoder_StopRecvPic(chn);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");
		return ret;
	}
*/
	return 0;
}

int capture_and_encoding()
{
	int ret = 0;
	int i = 0;

	
	printf(">>>>>caputre_and_encoding start\n");

	// undocumented functions to increase pool size
	IMP_OSD_SetPoolSize(0x64000);
	IMP_Encoder_SetPoolSize(0x100000);

	ret = sample_system_init();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_System_Init() failed\n");
		return -1;
	}

	/* Step.2 FrameSource init */
	IMP_LOG_INFO(TAG, "FrameSource init...\n");
	ret = sample_framesource_init();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "FrameSource init failed\n");
		return -1;
	}

	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			IMP_LOG_INFO(TAG, "Enc CreateGrp for ch %d...\n", i);
			ret = IMP_Encoder_CreateGroup(chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_CreateGroup(%d) error !\n", i);
				return -1;
			}
		}
	}

	/* Step.3 Encoder init */
	IMP_LOG_INFO(TAG, "EncoderInit...\n");
	ret = sample_encoder_init();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Encoder init failed\n");
		return -1;
	}

	/* Step.4 Bind */
	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			IMP_LOG_INFO(TAG, "Binding for ch %d...\n", i);
			ret = IMP_System_Bind(&chn[i].framesource_chn, &chn[i].imp_encoder);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "Bind FrameSource channel%d and Encoder failed\n",i);
				return -1;
			}
		}
	}

	/* Step.5 Stream On */
	IMP_LOG_INFO(TAG, "StreamOn...\n");
	ret = sample_framesource_streamon();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "ImpStreamOn failed\n");
		return -1;
	}

	// start thread for activating night mode & IR cut filter
	pthread_t thread_info;
	pthread_create(&thread_info, NULL, sample_soft_photosensitive_ctrl, NULL);

	return 0;
}
