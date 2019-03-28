/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

#include <wels/codec_api.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

/*
 * @TODO remove after testing is done
 * docker run --rm -ti --init --ipc=host -v /tmp:/tmp -v $PWD:/data -w /data h264test --cid=122 --name=video0.i420
 * --width=640 --height=480 --verbose --rc-mode=0 --ecomplexity=0 --num-ref-frame=0 --sps-pps=0 --prefix-nal=0 --ssei=0
 * --padding=0 --entropy-coding=0 --frame-skip=0 --bitrate-max=0 --qp-max=0 --qp-min=0 --long-term-ref=0 --ltr-mark-period=0
 * --loop-filter=0 --denoise=0 --background-detection=0 --adaptive-quant=0 --frame-cropping=0 --scene-change-detect=0
 */

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ||
         (0 == commandlineArguments.count("rc-mode"))||
         (0 == commandlineArguments.count("ecomplexity"))||
         (0 == commandlineArguments.count("num-ref-frame")) ||
         (0 == commandlineArguments.count("sps-pps")) ||
         (0 == commandlineArguments.count("prefix-nal")) ||
         (0 == commandlineArguments.count("ssei")) ||
         (0 == commandlineArguments.count("padding")) ||
         (0 == commandlineArguments.count("entropy-coding")) ||
         (0 == commandlineArguments.count("frame-skip"))||
         (0 == commandlineArguments.count("bitrate-max")) ||
         (0 == commandlineArguments.count("qp-max")) ||
         (0 == commandlineArguments.count("qp-min")) ||
         (0 == commandlineArguments.count("long-term-ref"))||
         (0 == commandlineArguments.count("ltr-mark-period")) ||
         (0 == commandlineArguments.count("loop-filter")) ||
         (0 == commandlineArguments.count("denoise"))||
         (0 == commandlineArguments.count("background-detection")) ||
         (0 == commandlineArguments.count("adaptive-quant")) ||
         (0 == commandlineArguments.count("frame-cropping")) ||
         (0 == commandlineArguments.count("scene-change-detect"))) {
        std::cerr << argv[0] << " attaches to an I420-formatted image residing in a shared memory area to convert it into a corresponding h264 frame for publishing to a running OD4 session." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --name=<name of shared memory area> --width=<width> --height=<height> [--gop=<GOP>] [--bitrate=<bitrate>] [--verbose] [--id=<identifier in case of multiple instances]" << std::endl;
        std::cerr << "         --cid:     CID of the OD4Session to send h264 frames" << std::endl;
        std::cerr << "         --id:      when using several instances, this identifier is used as senderStamp" << std::endl;
        std::cerr << "         --name:    name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:   width of the frame" << std::endl;
        std::cerr << "         --height:  height of the frame" << std::endl;
        std::cerr << "         --gop:     optional: length of group of pictures (default = 10)" << std::endl;
        std::cerr << "         --bitrate: optional: desired bitrate (default: 1,500,000, min: 100,000 max: 5,000,000)" << std::endl;
        std::cerr << "         --verbose: print encoding information" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --name=data --width=640 --height=480 --verbose" << std::endl;
    }
    else {
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const uint32_t GOP_DEFAULT{10};
        const uint32_t GOP{(commandlineArguments["gop"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["gop"])) : GOP_DEFAULT};
        const uint32_t BITRATE_MIN{100000};
        const uint32_t BITRATE_DEFAULT{1500000};
        const uint32_t BITRATE_MAX{5000000};
        const uint32_t BITRATE{(commandlineArguments["bitrate"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["bitrate"])), BITRATE_MIN), BITRATE_MAX) : BITRATE_DEFAULT};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};


        /*
         * Thesis params
         * https://github.com/cisco/openh264/wiki/TypesAndStructures
         */
        const uint32_t RC_MODE{static_cast<uint32_t>(std::stoi(commandlineArguments["rc-mode"]))};
        const uint32_t ECOMPLEXITY{static_cast<uint32_t>(std::stoi(commandlineArguments["ecomplexity"]))};
        const uint32_t I_NUM_REF_FRAME{static_cast<uint32_t>(std::stoi(commandlineArguments["num-ref-frame"]))};
        const uint32_t SPS_PPS_STRATEGY{static_cast<uint32_t>(std::stoi(commandlineArguments["sps-pps"]))};
        const uint32_t B_PREFIX_NAL{static_cast<uint32_t>(std::stoi(commandlineArguments["prefix-nal"]))};
        const uint32_t B_SSEI{static_cast<uint32_t>(std::stoi(commandlineArguments["ssei"]))};
        const uint32_t I_PADDING{static_cast<uint32_t>(std::stoi(commandlineArguments["padding"]))};
        const uint32_t I_ENTROPY_CODING{static_cast<uint32_t>(std::stoi(commandlineArguments["entropy-coding"]))};
        const uint32_t B_FRAME_SKIP{static_cast<uint32_t>(std::stoi(commandlineArguments["frame-skip"]))};
        const uint32_t I_BITRATE_MAX{static_cast<uint32_t>(std::stoi(commandlineArguments["bitrate-max"]))};
        const uint32_t I_MAX_QP{static_cast<uint32_t>(std::stoi(commandlineArguments["qp-max"]))};
        const uint32_t I_MIN_QP{static_cast<uint32_t>(std::stoi(commandlineArguments["qp-min"]))};
        const uint32_t I_LONG_TERM_REFERENCE{static_cast<uint32_t>(std::stoi(commandlineArguments["long-term-ref"]))};
        const uint32_t I_LTR_MARK_PERIOD{static_cast<uint32_t>(std::stoi(commandlineArguments["ltr-mark-period"]))};
        const uint32_t I_LOOP_FILTER{static_cast<uint32_t>(std::stoi(commandlineArguments["loop-filter"]))};
        const uint32_t B_DENOISE{static_cast<uint32_t>(std::stoi(commandlineArguments["denoise"]))};
        const uint32_t B_BACKGROUND_DETECTION{static_cast<uint32_t>(std::stoi(commandlineArguments["background-detection"]))};
        const uint32_t B_ADAPTIVE_QUANT{static_cast<uint32_t>(std::stoi(commandlineArguments["adaptive-quant"]))};
        const uint32_t B_FRAME_CROPPING{static_cast<uint32_t>(std::stoi(commandlineArguments["frame-cropping"]))};
        const uint32_t B_SCENE_CHANGE_DETECT{static_cast<uint32_t>(std::stoi(commandlineArguments["scene-change-detect"]))};


        std::unique_ptr<cluon::SharedMemory> sharedMemory(new cluon::SharedMemory{NAME});
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << argv[0] << ": Attached to '" << sharedMemory->name() << "' (" << sharedMemory->size() << " bytes)." << std::endl;

            ISVCEncoder *encoder{nullptr};
            if (0 != WelsCreateSVCEncoder(&encoder) || (nullptr == encoder)) {
                std::cerr << argv[0] << ": Failed to create openh264 encoder." << std::endl;
                return retCode;
            }

            int logLevel{VERBOSE ? WELS_LOG_INFO : WELS_LOG_QUIET};
            encoder->SetOption(ENCODER_OPTION_TRACE_LEVEL, &logLevel);

            // Configure parameters for openh264 encoder.
            SEncParamExt parameters;
            {
                memset(&parameters, 0, sizeof(SEncParamBase));
                encoder->GetDefaultParams(&parameters);

                parameters.fMaxFrameRate = 20 /*FPS*/; // This parameter is implicitly given by the notifications from the shared memory.
                parameters.iUsageType = EUsageType::CAMERA_VIDEO_REAL_TIME;
                parameters.iPicWidth = WIDTH;
                parameters.iPicHeight = HEIGHT;
                parameters.uiIntraPeriod = GOP;
                parameters.iTargetBitrate = BITRATE;
                //parameters.iMaxBitrate = BITRATE_MAX;
                //parameters.iRCMode = RC_MODES::RC_QUALITY_MODE;
                parameters.iSpatialLayerNum = 1;
                parameters.iTemporalLayerNum = 1;
                ///parameters.iLoopFilterDisableIdc = 0;
                //parameters.iLtrMarkPeriod = 30;
                parameters.iMultipleThreadIdc = 1; // 1 = disable multi threads.
                //parameters.iEntropyCodingModeFlag = 0; // 0 = CAVLC, 1 = CABAC (not supported in BaseLine profile).
                //parameters.iComplexityMode = ECOMPLEXITY_MODE::LOW_COMPLEXITY;
                //parameters.bEnableAdaptiveQuant = 1;
                //parameters.bEnableBackgroundDetection = 1;
                //parameters.bEnableDenoise = 1;
                //parameters.bEnableFrameSkip = 0;
                //parameters.bEnableLongTermReference = 0;
                //parameters.bEnableSceneChangeDetect = 1;
                //parameters.bPrefixNalAddingCtrl = 0; // do nod add NAL prefixes.
                //parameters.eSpsPpsIdStrategy = EParameterSetStrategy::CONSTANT_ID;

                parameters.sSpatialLayers[0].iVideoWidth = parameters.iPicWidth;
                parameters.sSpatialLayers[0].iVideoHeight = parameters.iPicHeight;
                parameters.sSpatialLayers[0].fFrameRate = parameters.fMaxFrameRate;
                parameters.sSpatialLayers[0].iSpatialBitrate = parameters.iTargetBitrate;
                parameters.sSpatialLayers[0].iMaxSpatialBitrate = parameters.iMaxBitrate;
                parameters.sSpatialLayers[0].sSliceArgument.uiSliceMode = SliceModeEnum::SM_SIZELIMITED_SLICE;
                parameters.sSpatialLayers[0].sSliceArgument.uiSliceNum = 1;

                /*
                 * Thesis parameters
                 */
                if (RC_MODE == 0) parameters.iRCMode = RC_MODES::RC_QUALITY_MODE;
                else if(RC_MODE == 1) parameters.iRCMode = RC_MODES::RC_BITRATE_MODE;
                else if(RC_MODE == 2) parameters.iRCMode = RC_MODES::RC_BUFFERBASED_MODE;
                else if(RC_MODE == 3) parameters.iRCMode = RC_MODES::RC_TIMESTAMP_MODE;
                else parameters.iRCMode = RC_MODES::RC_OFF_MODE;

                if (SPS_PPS_STRATEGY == 0) parameters.eSpsPpsIdStrategy = EParameterSetStrategy::CONSTANT_ID;
                else if(SPS_PPS_STRATEGY == 1) parameters.eSpsPpsIdStrategy = EParameterSetStrategy::INCREASING_ID;
                else if(SPS_PPS_STRATEGY == 2) parameters.eSpsPpsIdStrategy = EParameterSetStrategy::SPS_LISTING;
                else if(SPS_PPS_STRATEGY == 3)parameters.eSpsPpsIdStrategy = EParameterSetStrategy::SPS_LISTING_AND_PPS_INCREASING;
                else parameters.eSpsPpsIdStrategy = EParameterSetStrategy::SPS_PPS_LISTING;

                if (ECOMPLEXITY == 0) parameters.iComplexityMode = ECOMPLEXITY_MODE::LOW_COMPLEXITY;
                else if(ECOMPLEXITY == 1) parameters.iComplexityMode = ECOMPLEXITY_MODE::MEDIUM_COMPLEXITY;
                else parameters.iComplexityMode = ECOMPLEXITY_MODE::HIGH_COMPLEXITY;

                parameters.iNumRefFrame = I_NUM_REF_FRAME;
                parameters.bPrefixNalAddingCtrl = B_PREFIX_NAL;
                parameters.bEnableSSEI = B_SSEI;
                parameters.iPaddingFlag = I_PADDING;
                parameters.iEntropyCodingModeFlag = I_ENTROPY_CODING;
                parameters.bEnableFrameSkip = B_FRAME_SKIP;
                parameters.iMaxBitrate = I_BITRATE_MAX;
                parameters.iMaxQp = I_MAX_QP;
                parameters.iMinQp = I_MIN_QP;
                parameters.bEnableLongTermReference = I_LONG_TERM_REFERENCE;
                parameters.iLtrMarkPeriod = I_LTR_MARK_PERIOD;
                // should we use multi threading? //parameters.iMultipleThreadIdc =
                parameters.iLoopFilterDisableIdc = I_LOOP_FILTER;
                parameters.bEnableDenoise = B_DENOISE;
                parameters.bEnableBackgroundDetection = B_BACKGROUND_DETECTION;
                parameters.bEnableAdaptiveQuant = B_ADAPTIVE_QUANT;
                parameters.bEnableFrameCroppingFlag = B_FRAME_CROPPING;
                parameters.bEnableSceneChangeDetect = B_SCENE_CHANGE_DETECT;

            }
            if (cmResultSuccess != encoder->InitializeExt(&parameters)) {
                std::cerr << argv[0] << ": Failed to set parameters for openh264." << std::endl;
                return retCode;
            }
            else {
                std::clog << argv[0] << ": Encoding bitrate = " << BITRATE << std::endl;
            }

            // Allocate image buffer to hold h264 frame as output.
            std::vector<char> h264Buffer;
            h264Buffer.resize(WIDTH * HEIGHT, '0'); // In practice, this is small than WIDTH * HEIGHT

            cluon::data::TimeStamp before, after, sampleTimeStamp;

            // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            while ( (sharedMemory && sharedMemory->valid()) && od4.isRunning() ) {
                // Wait for incoming frame.
                sharedMemory->wait();

                sampleTimeStamp = cluon::time::now();

                int totalSize{0};
                sharedMemory->lock();
                {
                    // Read notification timestamp.
                    auto r = sharedMemory->getTimeStamp();
                    sampleTimeStamp = (r.first ? r.second : sampleTimeStamp);
                }
                {
                    SFrameBSInfo frameInfo;
                    memset(&frameInfo, 0, sizeof(SFrameBSInfo));

                    SSourcePicture sourceFrame;
                    memset(&sourceFrame, 0, sizeof(SSourcePicture));

                    sourceFrame.iColorFormat = EVideoFormatType::videoFormatI420;
                    sourceFrame.iPicWidth = WIDTH;
                    sourceFrame.iPicHeight = HEIGHT;
                    sourceFrame.iStride[0] = WIDTH;
                    sourceFrame.iStride[1] = WIDTH/2;
                    sourceFrame.iStride[2] = WIDTH/2;
                    sourceFrame.pData[0] = reinterpret_cast<uint8_t*>(sharedMemory->data());
                    sourceFrame.pData[1] = reinterpret_cast<uint8_t*>(sharedMemory->data() + (WIDTH * HEIGHT));
                    sourceFrame.pData[2] = reinterpret_cast<uint8_t*>(sharedMemory->data() + (WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2)));

                    if (VERBOSE) {
                        before = cluon::time::now();
                    }
                    auto result = encoder->EncodeFrame(&sourceFrame, &frameInfo);
                    if (VERBOSE) {
                        after = cluon::time::now();
                    }
                    if (cmResultSuccess == result) {
                        if (videoFrameTypeSkip == frameInfo.eFrameType) {
                            std::cerr << argv[0] << ": Warning, skipping frame." << std::endl;
                        }
                        else {
                            for(int layer{0}; layer < frameInfo.iLayerNum; layer++) {
                                int sizeOfLayer{0};
                                for(int nal{0}; nal < frameInfo.sLayerInfo[layer].iNalCount; nal++) {
                                    sizeOfLayer += frameInfo.sLayerInfo[layer].pNalLengthInByte[nal];
                                }
                                memcpy(&h264Buffer[totalSize], frameInfo.sLayerInfo[layer].pBsBuf, sizeOfLayer);
                                totalSize += sizeOfLayer;
                            }
                        }
                    }
                    else {
                        std::cerr << argv[0] << ": Failed to encode frame: " << result << std::endl;
                    }
                }
                sharedMemory->unlock();

                if (0 < totalSize) {
                    opendlv::proxy::ImageReading ir;
                    ir.fourcc("h264").width(WIDTH).height(HEIGHT).data(std::string(&h264Buffer[0], totalSize));
                    od4.send(ir, sampleTimeStamp, ID);

                    if (VERBOSE) {
                        std::clog << argv[0] << ": Frame size = " << totalSize << " bytes; sample time = " << cluon::time::toMicroseconds(sampleTimeStamp) << " microseconds; encoding took " << cluon::time::deltaInMicroseconds(after, before) << " microseconds." << std::endl;
                    }
                }
            }
            if (nullptr != encoder) {
                encoder->Uninitialize();
                WelsDestroySVCEncoder(encoder);
            }
            retCode = 0;
        }
        else {
            std::cerr << argv[0] << ": Failed to attach to shared memory '" << NAME << "'." << std::endl;
        }
    }
    return retCode;
}
