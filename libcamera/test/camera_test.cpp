/* Copyright (c) 2015, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>

#include "camera.h"
#include "camera_log.h"
#include "camera_parameters.h"

#define DEFAULT_EXPOSURE_VALUE_STR "2500"
#define MIN_EXPOSURE_VALUE 0
#define MAX_EXPOSURE_VALUE 65535
#define DEFAULT_GAIN_VALUE_STR "100"
#define MIN_GAIN_VALUE 0
#define MAX_GAIN_VALUE 255

using namespace std;
using namespace camera;

struct CameraCaps
{
    vector<ImageSize> pSizes, vSizes;
    vector<string> focusModes, wbModes, isoModes;
    Range brightness, sharpness, contrast;
    vector<Range> previewFpsRanges;
    vector<VideoFPS> videoFpsValues;
    vector<string> previewFormats;
};

enum OutputFormatType{
    YUV_FORMAT,
    RAW_FORMAT,
};

enum CamFunction {
    CAM_FUNC_HIRES = 0,
    CAM_FUNC_OPTIC_FLOW = 1,
};

struct TestConfig
{
    bool dumpFrames;
    bool infoMode;
    int runTime;
    string exposureValue;  /* 0 -3000 Supported. -1 Indicates default setting of the setting  */
    string gainValue;
    CamFunction func;
    OutputFormatType outputFormat;
};

class CameraTest : ICameraListener
{
public:

    CameraTest();
    CameraTest(TestConfig config);
    ~CameraTest();
    int run();

    int initialize(int camId);

    /* listener methods */
    virtual void onError();
    virtual void onPreviewFrame(ICameraFrame* frame);
    virtual void onVideoFrame(ICameraFrame* frame);

private:
    ICameraDevice* camera_;
    CameraParams params_;
    ImageSize pSize_, vSize_;
    CameraCaps caps_;
    TestConfig config_;

    uint32_t vFrameCount_, pFrameCount_;
    float vFpsAvg_, pFpsAvg_;

    uint64_t vTimeStampPrev_, pTimeStampPrev_;

    int printCapabilities();
    int setParameters();
};

CameraTest::CameraTest() :
    vFrameCount_(0),
    pFrameCount_(0),
    vFpsAvg_(0.0f),
    pFpsAvg_(0.0f),
    vTimeStampPrev_(0),
    pTimeStampPrev_(0),
    camera_(NULL)
{
}

CameraTest::CameraTest(TestConfig config) :
    vFrameCount_(0),
    pFrameCount_(0),
    vFpsAvg_(0.0f),
    pFpsAvg_(0.0f),
    vTimeStampPrev_(0),
    pTimeStampPrev_(0)
{
    config_ = config;
}

int CameraTest::initialize(int camId)
{
    int rc;
    rc = ICameraDevice::createInstance(camId, &camera_);
    if (rc != 0) {
        printf("could not open camera %d\n", camId);
        return rc;
    }
    camera_->addListener(this);

    rc = params_.init(camera_);
    if (rc != 0) {
        printf("failed to init parameters\n");
        ICameraDevice::deleteInstance(&camera_);
        return rc;
    }
    //printf("params = %s\n", params_.toString().c_str());
    /* query capabilities */
    caps_.pSizes = params_.getSupportedPreviewSizes();
    caps_.vSizes = params_.getSupportedVideoSizes();
    caps_.focusModes = params_.getSupportedFocusModes();
    caps_.wbModes = params_.getSupportedWhiteBalance();
    caps_.isoModes = params_.getSupportedISO();
    caps_.brightness = params_.getSupportedBrightness();
    caps_.sharpness = params_.getSupportedSharpness();
    caps_.contrast = params_.getSupportedContrast();
    caps_.previewFpsRanges = params_.getSupportedPreviewFpsRanges();
    caps_.videoFpsValues = params_.getSupportedVideoFps();
    caps_.previewFormats = params_.getSupportedPreviewFormats();
}

CameraTest::~CameraTest()
{
}

static int dumpToFile(uint8_t* data, uint32_t size, char* name, uint64_t timestamp)
{
    FILE* fp;
    fp = fopen(name, "wb");
    if (!fp) {
        printf("fopen failed for %s\n", name);
        return -1;
    }
    fwrite(data, size, 1, fp);
    printf("saved filename %s, timestamp %llu nSecs\n", name, timestamp);
    fclose(fp);
}

void CameraTest::onError()
{
    printf("camera error!, aborting\n");
    exit(EXIT_FAILURE);
}

void CameraTest::onPreviewFrame(ICameraFrame* frame)
{
    if (pFrameCount_ > 0 && pFrameCount_ % 30 == 0) {
        char name[50];

        if ( config_.outputFormat == RAW_FORMAT )
        {
            snprintf(name, 50, "P_%dx%d_%04d_%llu.raw",
                 pSize_.width, pSize_.height, pFrameCount_,frame->timeStamp);
        }else{
             snprintf(name, 50, "P_%dx%d_%04d_%llu.yuv",
                 pSize_.width, pSize_.height, pFrameCount_,frame->timeStamp);
        }

        if (config_.dumpFrames == true) {
            dumpToFile(frame->data, frame->size, name, frame->timeStamp);
        }
        //printf("Preview FPS = %.2f\n", pFpsAvg_);
    }

    uint64_t diff = frame->timeStamp - pTimeStampPrev_;
    pFpsAvg_ = ((pFpsAvg_ * pFrameCount_) + (1e9 / diff)) / (pFrameCount_ + 1);
    pFrameCount_++;
    pTimeStampPrev_  = frame->timeStamp;
}

void CameraTest::onVideoFrame(ICameraFrame* frame)
{
    if (vFrameCount_ > 0 && vFrameCount_ % 30 == 0) {
        char name[50];
        snprintf(name, 50, "V_%dx%d_%04d_%llu.yuv",
                 vSize_.width, vSize_.height, vFrameCount_,frame->timeStamp);
        if (config_.dumpFrames == true) {
            dumpToFile(frame->data, frame->size, name, frame->timeStamp);
        }
        //printf("Video FPS = %.2f\n", vFpsAvg_);
    }

    uint64_t diff = frame->timeStamp - vTimeStampPrev_;
    vFpsAvg_ = ((vFpsAvg_ * vFrameCount_) + (1e9 / diff)) / (vFrameCount_ + 1);
    vFrameCount_++;
    vTimeStampPrev_  = frame->timeStamp;
}

int CameraTest::printCapabilities()
{
    printf("Camera capabilities\n");

    printf("available preview sizes:\n");
    for (int i = 0; i < caps_.pSizes.size(); i++) {
        printf("%d: %d x %d\n", i, caps_.pSizes[i].width, caps_.pSizes[i].height);
    }
    printf("available video sizes:\n");
    for (int i = 0; i < caps_.vSizes.size(); i++) {
        printf("%d: %d x %d\n", i, caps_.vSizes[i].width, caps_.vSizes[i].height);
    }
    printf("available preview formats:\n");
    for (int i = 0; i < caps_.previewFormats.size(); i++) {
        printf("%d: %s\n", i, caps_.previewFormats[i].c_str());
    }
    printf("available focus modes:\n");
    for (int i = 0; i < caps_.focusModes.size(); i++) {
        printf("%d: %s\n", i, caps_.focusModes[i].c_str());
    }
    printf("available whitebalance modes:\n");
    for (int i = 0; i < caps_.wbModes.size(); i++) {
        printf("%d: %s\n", i, caps_.wbModes[i].c_str());
    }
    printf("available ISO modes:\n");
    for (int i = 0; i < caps_.isoModes.size(); i++) {
        printf("%d: %s\n", i, caps_.isoModes[i].c_str());
    }
    printf("available brightness values:\n");
    printf("min=%d, max=%d, step=%d\n", caps_.brightness.min,
           caps_.brightness.max, caps_.brightness.step);
    printf("available sharpness values:\n");
    printf("min=%d, max=%d, step=%d\n", caps_.sharpness.min,
           caps_.sharpness.max, caps_.sharpness.step);
    printf("available contrast values:\n");
    printf("min=%d, max=%d, step=%d\n", caps_.contrast.min,
           caps_.contrast.max, caps_.contrast.step);

    printf("available preview fps ranges:\n");
    for (int i = 0; i < caps_.previewFpsRanges.size(); i++) {
        printf("%d: [%d, %d]\n", i, caps_.previewFpsRanges[i].min,
               caps_.previewFpsRanges[i].max);
    }
    printf("available video fps values:\n");
    for (int i = 0; i < caps_.videoFpsValues.size(); i++) {
        printf("%d: %d\n", i, caps_.videoFpsValues[i]);
    }
    return 0;
}

ImageSize VGASize(640,480);
int CameraTest::setParameters()
{
    /* temp: using hard-coded values to test the api
       need to add a user interface or script to get the values to test*/
    int pSizeIdx = 3;
    int vSizeIdx = 2;
    int focusModeIdx = 3;
    int wbModeIdx = 2;
    int isoModeIdx = 3;
    int pFpsIdx = 3;
    int vFpsIdx = 3;
    int prevFmtIdx = 0;

    pSize_ = caps_.pSizes[pSizeIdx];
    vSize_ = caps_.pSizes[vSizeIdx];

    if ( config_.func == CAM_FUNC_OPTIC_FLOW ){
	pSize_ = VGASize;
	vSize_ = VGASize;
    }

    printf("setting preview size: %dx%d\n", pSize_.width, pSize_.height);
    params_.setPreviewSize(pSize_);
    printf("setting video size: %dx%d\n", vSize_.width, vSize_.height);
    params_.setVideoSize(vSize_);

    if (config_.func != CAM_FUNC_OPTIC_FLOW ) {
      printf("setting focus mode: %s\n",
             caps_.focusModes[focusModeIdx].c_str());
      params_.setFocusMode(caps_.focusModes[focusModeIdx]);
      printf("setting WB mode: %s\n", caps_.wbModes[wbModeIdx].c_str());
      params_.setWhiteBalance(caps_.wbModes[wbModeIdx]);
      printf("setting ISO mode: %s\n", caps_.isoModes[isoModeIdx].c_str());
      params_.setISO(caps_.isoModes[isoModeIdx]);

      printf("setting preview fps range: %d, %d\n",
           caps_.previewFpsRanges[pFpsIdx].min,
           caps_.previewFpsRanges[pFpsIdx].max);
      params_.setPreviewFpsRange(caps_.previewFpsRanges[pFpsIdx]);

      printf("setting video fps: %d\n", caps_.videoFpsValues[vFpsIdx]);
      params_.setVideoFPS(caps_.videoFpsValues[vFpsIdx]);

      printf("setting preview format: %s\n",
             caps_.previewFormats[prevFmtIdx].c_str());
      params_.setPreviewFormat(caps_.previewFormats[prevFmtIdx]);
    }
    if (config_.outputFormat == RAW_FORMAT)
    {
        params_.set("preview-format", "bayer-rggb");
        params_.set("picture-format", "bayer-mipi-10gbrg");
        params_.set("raw-size", "640x480");
    }

    return params_.commit();
}

int CameraTest::run()
{
    int rc = EXIT_SUCCESS;

    int n = getNumberOfCameras();

    if (n < 0) {
        printf("getNumberOfCameras() failed, rc=%d\n", n);
        return EXIT_FAILURE;
    }

    printf("num_cameras = %d\n", n);

    if (n < 1) {
        printf("No cameras found.\n");
        return EXIT_FAILURE;
    }

    int camId=0;

    /* find camera based on function */
    for (int i=0; i<n; i++) {
        CameraInfo info;
        getCameraInfo(i, info);
        if (info.func == config_.func) {
            camId = i;
        }
    }

    printf("testing camera id=%d\n", camId);

    initialize(camId);

    if (config_.infoMode) {
        printCapabilities();
        return rc;
    }

    setParameters();

    /* initialize perf counters */
    vFrameCount_ = 0;
    pFrameCount_ = 0;
    vFpsAvg_ = 0.0f;
    pFpsAvg_ = 0.0f;

    printf("start preview\n");
    camera_->startPreview();
    if ( config_.func == CAM_FUNC_OPTIC_FLOW )
    {
        params_.set("qc-exposure-manual", config_.exposureValue.c_str() );
        params_.set("qc-gain-manual", config_.gainValue.c_str() );
        printf("Setting exposure value =  %s , gain value = %s \n", config_.exposureValue.c_str(), config_.gainValue.c_str());
    }

    params_.commit();

    if( config_.outputFormat != RAW_FORMAT )
    {
    printf("start recording\n");
    camera_->startRecording();
    }
    printf("waiting for %d seconds ...\n", config_.runTime);

    sleep(config_.runTime);

    if( config_.outputFormat != RAW_FORMAT )
    {
    printf("stop recording\n");
    camera_->stopRecording();
    }
    printf("stop preview\n");
    camera_->stopPreview();

    printf("Average preview FPS = %.2f\n", pFpsAvg_);
    printf("Average video FPS = %.2f\n", vFpsAvg_);

    /* release camera device */
    ICameraDevice::deleteInstance(&camera_);
    return rc;
}

const char usageStr[] =
    "Camera API test application \n"
    "\n"
    "usage: camera-test [options]\n"
    "\n"
    "  -t <duration>   capture duration in seconds [10]\n"
    "  -d              dump frames\n"
    "  -i              info mode\n"
    "                    - print camera capabilities\n"
    "                    - streaming will not be started\n"
    "  -f <type>       camera type\n"
    "                    - hires\n"
    "                    - optic\n"
    "  -e <value>      set exposure control (only for ov7251)\n"
    "                     min - 0\n"
    "                     max - 65535\n"
    "  -g <value>      set gain value (only for ov7251)\n"
    "                     min - 0\n"
    "                     max - 255\n"
    "  -o <value>      Output format\n"
    "                     0 :YUV format (default)\n"
    "                     1 : RAW format \n"
    "  -h              print this message\n"
;

static inline void printUsageExit(int code)
{
    printf("%s", usageStr);
    exit(code);
}

/* parses commandline options and populates the config
   data structure */
static TestConfig parseCommandline(int argc, char* argv[])
{
    TestConfig cfg;
    cfg.outputFormat = YUV_FORMAT;
    int outputFormat;
    /* default config */
    cfg.dumpFrames = false;
    cfg.runTime = 10;
    cfg.func = CAM_FUNC_HIRES;
    cfg.infoMode = false;
    cfg.exposureValue = DEFAULT_EXPOSURE_VALUE_STR;  /* Default exposure value */
    int exposureValueInt = 0;
    cfg.gainValue = DEFAULT_GAIN_VALUE_STR;  /* Default gain value */
    int gainValueInt = 0;
    int c;
    while ((c = getopt(argc, argv, "hdt:if:o:e:g:")) != -1) {
        switch (c) {
          case 't':
              cfg.runTime = atoi(optarg);
              break;
          case 'f':
          {
                  string str(optarg);
                  if (str == "hires") {
                      cfg.func = CAM_FUNC_HIRES;
                  } else if (str == "optic") {
                      cfg.func = CAM_FUNC_OPTIC_FLOW;
                  }
                  break;
          }
          case 'd':
              cfg.dumpFrames = true;
              break;
          case 'i':
              cfg.infoMode = true;
              break;
        case  'e':
              exposureValueInt =  atoi(optarg);              
              if ( exposureValueInt < MIN_EXPOSURE_VALUE || exposureValueInt > MAX_EXPOSURE_VALUE )
              {
                  printf("Invalid exposure value. Using default\n");
                  cfg.exposureValue = DEFAULT_EXPOSURE_VALUE_STR;
              }else{
                  cfg.exposureValue = optarg;
              }
			  break;	
		 case  'g':
              gainValueInt =  atoi(optarg);              
              if ( gainValueInt < MIN_GAIN_VALUE || gainValueInt > MAX_GAIN_VALUE)
              {
                  printf("Invalid exposure value. Using default\n");
                  cfg.gainValue = DEFAULT_GAIN_VALUE_STR;
              }else{
                  cfg.gainValue = optarg;
              }
              break;
         case 'o':
            outputFormat = atoi(optarg);
            switch ( outputFormat )
            {
                case 0: /* IMX135 , IMX214 */
                   cfg.outputFormat = YUV_FORMAT;
                   break;
                case 1: /* IMX214 */
                    cfg.outputFormat = RAW_FORMAT;
                    break;
                default:
                    printf("Invalid format. Setting to default YUV_FORMAT");
                    cfg.outputFormat = YUV_FORMAT;
                    break;
            }
	        break;
          case 'h':
          case '?':
              printUsageExit(0);
          default:
              abort();
        }
    }
    return cfg;
}

int main(int argc, char* argv[])
{

    TestConfig config = parseCommandline(argc, argv);

    CameraTest test(config);
    test.run();

    return EXIT_SUCCESS;
}
