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


int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ) {
        std::cerr << argv[0] << " attaches to an I420-formatted image residing in a shared memory area to convert it into a corresponding h264 frame for publishing to a running OD4 session." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --name=<name of shared memory area> --width=<width> --height=<height> [--gop=<GOP>] [--bitrate=<bitrate>] [--id=<identifier in case of multiple instances]"
                "[--bitrate-max=<bitrate-max>] [--rc-mode=<rc-mode>] [--ecomplexity=<ecomplexity>] [--sps-pps=<sps-pps>] [--num-ref-frame=<num-ref-frame>] [--ssei=<ssei>] [--prefix-nal=<prefix-nal>] [--entropy-coding=<entropy-coding>] "
                "[--frame-skip=<frame-skip>] [--qp-max=<qp-max>] [--qp-min=<qp-min>] [--long-term-ref=<long-term-ref>] [--loop-filter=<loop-filter>] [--denoise=<denoise>] [--background-detection=<background-detection>] "
                "[--adaptive-quant=<adaptive-quant>] [--frame-cropping=<frame-cropping>] [--scene-change-detect=<scene-change-detect>] [--threads=<threads>] [--verbose]" << std::endl;
        std::cerr << "         --cid:           CID of the OD4Session to send h264 frames" << std::endl;
        std::cerr << "         --id:            when using several instances, this identifier is used as senderStamp" << std::endl;
        std::cerr << "         --name:          name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:         width of the frame" << std::endl;
        std::cerr << "         --height:        height of the frame" << std::endl;
        std::cerr << "         --bitrate:       optional: desired bitrate (default: 1,500,000, min: 100,000 max: 5,000,000)" << std::endl;
        std::cerr << "         --bitrate-max:   optional: maximum bitrate (default: 5,000,000, min: 100,000 max: 5,000,000)" << std::endl;
        std::cerr << "         --gop:           optional: length of group of pictures (default = 10)" << std::endl;
        std::cerr << "         --rc-mode:       optional: rate control mode (default: RC_QUALITY_MODE (0), min: 0, max: 4)" << std::endl;
        std::cerr << "         --ecomplexity:   optional: complexity mode (default: LOW_COMPLEXITY (0), min: 0, max: 2)" << std::endl;
        std::cerr << "         --sps-pps:       optional: SPS/PPS strategy (default: CONSTANT_ID (0), min: 0, max: 3)" << std::endl;
        std::cerr << "         --num-ref-frame: optional: number of reference frame used (default: 1, 0: auto, >0 reference frames)" << std::endl;
        std::cerr << "         --ssei:          optional: toggle ssei (default: 0)" << std::endl;
        std::cerr << "         --prefix-nal:    optional: toggle prefix NAL adding control (default: 0)" << std::endl;
        std::cerr << "         --entropy-coding:optional: toggle entropy encoding (default: CAVLC (0))" << std::endl;
        std::cerr << "         --frame-skip:    optional: toggle fram-skipping to keep the bitrate within limits (default: 1)" << std::endl;
        std::cerr << "         --qp-max:        optional: Quantization Parameter max (default: 42, min: 0 max: 51)" << std::endl;
        std::cerr << "         --qp-min:        optional: Quantization Parameter min (default: 12, min: 0 max: 51)" << std::endl;
        std::cerr << "         --long-term-ref: optional: toggle long term reference control (default: 0)" << std::endl;
        std::cerr << "         --loop-filter:   optional: deblocking loop filter (default: 0, 0: on, 1: off, 2: on except for slice boundaries)" << std::endl;
        std::cerr << "         --denoise:       optional: toggle denoise control (default: 0)" << std::endl;
        std::cerr << "         --background-detection: optional: toggle background detection control (default: 1)" << std::endl;
        std::cerr << "         --adaptive-quant: optional: toggle adaptive quantization control (default: 1)" << std::endl;
        std::cerr << "         --frame-cropping: optional: toggle frame cropping (default: 1)" << std::endl;
        std::cerr << "         --scene-change-detect: optional: toggle scene change detection control (default: 1)" << std::endl;
        std::cerr << "         --threads        :optional: number of threads (default: 1, O: auto, >1: number of theads, max 4)" << std::endl;
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

        //Thesis constants
        const uint32_t ZERO{0};
        const uint32_t ONE{1};
        const uint32_t TWO{2};
        const uint32_t THREE{3};
        const uint32_t FOUR{4};
        const uint32_t RC_MODE{(commandlineArguments["rc-mode"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["rc-mode"])), ZERO), FOUR): 0}; // RC_MODES::RC_QUALITY_MODE
        const uint32_t ECOMPLEXITY{(commandlineArguments["ecomplexity"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["ecomplexity"])), ZERO), TWO): 0}; // ECOMPLEXITY_MODE::LOW_COMPLEXITY
        const uint32_t I_NUM_REF_FRAME{(commandlineArguments["num-ref-frame"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["num-ref-frame"])) : 1};
        const uint32_t SPS_PPS_STRATEGY{(commandlineArguments["sps-pps"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["sps-pps"])), ZERO), THREE): 0}; //EParameterSetStrategy::CONSTANT_ID
        const uint32_t B_PREFIX_NAL{(commandlineArguments["prefix-nal"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["prefix-nal"])), ZERO), ONE): 0};
        const uint32_t B_SSEI{(commandlineArguments["ssei"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["ssei"])), ZERO), ONE): 0};
        const uint32_t I_PADDING{(commandlineArguments["padding"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["padding"])) : 0};
        const uint32_t I_ENTROPY_CODING{(commandlineArguments["entropy-coding"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["entropy-coding"])), ZERO), ONE): 0};
        const uint32_t B_FRAME_SKIP{(commandlineArguments["frame-skip"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["frame-skip"])), ZERO), ONE): 1};
        const uint32_t I_BITRATE_MAX{(commandlineArguments["bitrate-max"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["bitrate-max"])), BITRATE_MIN), BITRATE_MAX) : BITRATE_MAX};
        const uint32_t QP_MIN{0};
        const uint32_t QP_MAX{51};
        const uint32_t I_MAX_QP{(commandlineArguments["qp-max"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["qp-max"])), QP_MIN), QP_MAX): 42};
        const uint32_t I_MIN_QP{(commandlineArguments["qp-max"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["qp-max"])), QP_MIN), QP_MAX): 12};
        const uint32_t B_LONG_TERM_REFERENCE{(commandlineArguments["long-term-ref"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["long-term-ref"])), ZERO), ONE): 0};
        const uint32_t I_LOOP_FILTER{(commandlineArguments["loop-filter"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["loop-filter"])), ZERO), TWO): 0};
        const uint32_t B_DENOISE{(commandlineArguments["denoise"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["denoise"])), ZERO), ONE): 0};
        const uint32_t B_BACKGROUND_DETECTION{(commandlineArguments["background-detection"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["background-detection"])), ZERO), ONE): 1};
        const uint32_t B_ADAPTIVE_QUANT{(commandlineArguments["adaptive-quant"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["adaptive-quant"])), ZERO), ONE): 1};
        const uint32_t B_FRAME_CROPPING{(commandlineArguments["frame-cropping"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["frame-cropping"])), ZERO), ONE): 1};
        const uint32_t B_SCENE_CHANGE_DETECT{(commandlineArguments["scene-change-detect"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["scene-change-detect"])), ZERO), ONE): 1};
        const uint32_t I_MULTIPLE_THREADS{(commandlineArguments["threads"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["threads"])), ZERO), FOUR): 1};

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
                parameters.iSpatialLayerNum = 1;
                parameters.iTemporalLayerNum = 1;
                parameters.iLtrMarkPeriod = 30;
                parameters.iMultipleThreadIdc = I_MULTIPLE_THREADS; // 1 = disable multi threads.

                parameters.sSpatialLayers[0].iVideoWidth = parameters.iPicWidth;
                parameters.sSpatialLayers[0].iVideoHeight = parameters.iPicHeight;
                parameters.sSpatialLayers[0].fFrameRate = parameters.fMaxFrameRate;
                parameters.sSpatialLayers[0].iSpatialBitrate = parameters.iTargetBitrate;
                parameters.sSpatialLayers[0].iMaxSpatialBitrate = I_BITRATE_MAX;
                parameters.sSpatialLayers[0].sSliceArgument.uiSliceMode = SliceModeEnum::SM_SIZELIMITED_SLICE;
                parameters.sSpatialLayers[0].sSliceArgument.uiSliceNum = 1;

                /*
                 * Thesis parameters
                 * https://github.com/cisco/openh264/wiki/TypesAndStructures
                 * https://github.com/cisco/openh264/blob/master/codec/encoder/core/inc/param_svc.h#L132
                 */
                if (I_NUM_REF_FRAME == 0) {
                    parameters.iNumRefFrame = AUTO_REF_PIC_COUNT;
                }
                else {
                    parameters.iNumRefFrame = I_NUM_REF_FRAME;
                }
                parameters.bPrefixNalAddingCtrl = B_PREFIX_NAL;
                parameters.bEnableSSEI = B_SSEI;
                parameters.iPaddingFlag = I_PADDING;
                parameters.iEntropyCodingModeFlag = I_ENTROPY_CODING;
                parameters.bEnableFrameSkip = B_FRAME_SKIP;
                parameters.iMaxBitrate = I_BITRATE_MAX;
                parameters.iMaxQp = I_MAX_QP;
                parameters.iMinQp = I_MIN_QP;
                parameters.bEnableLongTermReference = B_LONG_TERM_REFERENCE;
                parameters.iLoopFilterDisableIdc = I_LOOP_FILTER;
                parameters.bEnableDenoise = B_DENOISE;
                parameters.bEnableBackgroundDetection = B_BACKGROUND_DETECTION;
                parameters.bEnableAdaptiveQuant = B_ADAPTIVE_QUANT;
                parameters.bEnableFrameCroppingFlag = B_FRAME_CROPPING;
                parameters.bEnableSceneChangeDetect = B_SCENE_CHANGE_DETECT;

                switch (RC_MODE) {
                    case 0: { parameters.iRCMode = RC_MODES::RC_QUALITY_MODE; break; }
                    case 1: { parameters.iRCMode = RC_MODES::RC_BITRATE_MODE; break; }
                    case 2: { parameters.iRCMode = RC_MODES::RC_BUFFERBASED_MODE; break; }
                    case 3: { parameters.iRCMode = RC_MODES::RC_TIMESTAMP_MODE; break; }
                    case 4: { parameters.iRCMode = RC_MODES::RC_OFF_MODE; break; }
                }

                switch (SPS_PPS_STRATEGY) {
                    case 0: { parameters.eSpsPpsIdStrategy = EParameterSetStrategy::CONSTANT_ID; break; }
                    case 1: { parameters.eSpsPpsIdStrategy = EParameterSetStrategy::INCREASING_ID; break; }
                    case 2: { parameters.eSpsPpsIdStrategy = EParameterSetStrategy::SPS_LISTING; break; }
                    case 3: { parameters.eSpsPpsIdStrategy = EParameterSetStrategy::SPS_LISTING_AND_PPS_INCREASING; break; }
                }

                switch (ECOMPLEXITY) {
                    case 0: { parameters.iComplexityMode = ECOMPLEXITY_MODE::LOW_COMPLEXITY;; break; }
                    case 1: { parameters.iComplexityMode = ECOMPLEXITY_MODE::MEDIUM_COMPLEXITY; break; }
                    case 2: { parameters.iComplexityMode = ECOMPLEXITY_MODE::HIGH_COMPLEXITY; break; }
                }
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
