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
#include <libyuv.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    const size_t PIXEL_LAYOUTS{commandlineArguments.count("bgr24") +
                               commandlineArguments.count("rgb24") +
                               commandlineArguments.count("yuv420") +
                               commandlineArguments.count("yuyv422")};
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ||
         (1 != PIXEL_LAYOUTS) ) {
        std::cerr << argv[0] << " attaches to an uncompressed video frame residing in a shared memory area to convert it into corresponding h264 frames for publishing to a running OD4 session." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --name=<name of shared memory area> --width=<width> --height=<height> --bgr24|--rgb24|--yuyv422|--yuv420 [--gop=<length of group of pictures>] [--verbose]" << std::endl;
        std::cerr << "         --cid:     CID of the OD4Session to send h264 frames" << std::endl;
        std::cerr << "         --name:    name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:   width of the frame" << std::endl;
        std::cerr << "         --height:  height of the frame" << std::endl;
        std::cerr << "         --gop:     length of group of pictures (default = 10)" << std::endl;
        std::cerr << "         --verbose: print encoding information" << std::endl;
        std::cerr << "         One of the following pixel layouts for the image data in the shared memory must be selected:" << std::endl;
        std::cerr << "         --bgr24:   pixel layout is RGB 24 bit" << std::endl;
        std::cerr << "         --rgb24:   pixel layout is RGB 24 bit" << std::endl;
        std::cerr << "         --yuv420:  pixel layout is YUV420" << std::endl;
        std::cerr << "         --yuyv422: pixel layout is YUYV422" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --name=data --width=640 --height=480 --yuyv422 --verbose" << std::endl;
    }
    else {
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const uint32_t GOP_DEFAULT{10};
        const uint32_t GOP{(commandlineArguments["gop"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["gop"])) : GOP_DEFAULT};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        const bool BGR24{commandlineArguments.count("bgr24") != 0};
        const bool RGB24{commandlineArguments.count("rgb24") != 0};
        const bool YUV420{commandlineArguments.count("yuv420") != 0};
        const bool YUYV422{commandlineArguments.count("yuyv422") != 0};

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
                parameters.iPicWidth = WIDTH;
                parameters.iPicHeight = HEIGHT;
                parameters.iTargetBitrate = 2500000;
                parameters.iMaxBitrate = 5000000;
                parameters.iRCMode = RC_QUALITY_MODE;
                parameters.iTemporalLayerNum = 1;
                parameters.iSpatialLayerNum = 1;
                parameters.bEnableAdaptiveQuant = 1;
                parameters.bEnableBackgroundDetection = 1;
                parameters.bEnableDenoise = 0;
                parameters.bEnableFrameSkip = 0;
                parameters.bEnableLongTermReference = 0;
                parameters.iLtrMarkPeriod = 30;
                parameters.uiIntraPeriod = GOP;
                parameters.eSpsPpsIdStrategy = CONSTANT_ID;
                parameters.bPrefixNalAddingCtrl = 0;
                parameters.iLoopFilterDisableIdc = 0;
                parameters.iEntropyCodingModeFlag = 0;
                parameters.iMultipleThreadIdc = 1;
                parameters.iEntropyCodingModeFlag = 0;

                parameters.sSpatialLayers[0].iVideoWidth = parameters.iPicWidth;
                parameters.sSpatialLayers[0].iVideoHeight = parameters.iPicHeight;
                parameters.sSpatialLayers[0].fFrameRate = parameters.fMaxFrameRate;
                parameters.sSpatialLayers[0].iSpatialBitrate = parameters.iTargetBitrate;
                parameters.sSpatialLayers[0].iMaxSpatialBitrate = parameters.iMaxBitrate;
                parameters.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
                parameters.sSpatialLayers[0].sSliceArgument.uiSliceNum = 1;
            }
            if (cmResultSuccess != encoder->InitializeExt(&parameters)) {
                std::cerr << argv[0] << ": Failed to set parameters for openh264." << std::endl;
                return retCode;
            }

            // Allocate image buffer to hold YUV420 frame as input.
            const uint32_t SIZE_OF_YUV420{WIDTH * HEIGHT * 3 / 2};
            std::vector<unsigned char> yuv420Frame;
            yuv420Frame.resize(SIZE_OF_YUV420, '0');

            // Allocate image buffer to hold h264 frame as output.
            std::vector<char> h264Buffer;
            h264Buffer.resize(WIDTH * HEIGHT, '0');
            std::string h264DataForImageReading;

            // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            while ( (sharedMemory && sharedMemory->valid()) && od4.isRunning() ) {
                // Wait for incoming frame.
                sharedMemory->wait();

                h264DataForImageReading.clear();
                sharedMemory->lock();
                {
                    // Convert incoming frame data to YUV420.
                    int result{0};

                    if (BGR24) {
                        result = libyuv::RAWToI420(reinterpret_cast<unsigned char*>(sharedMemory->data()), sharedMemory->size(),
                                                   &yuv420Frame[0], WIDTH,
                                                   &yuv420Frame[WIDTH * HEIGHT], WIDTH/2,
                                                   &yuv420Frame[WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2)], WIDTH/2,
                                                   WIDTH, HEIGHT);
                    }
                    if (RGB24) {
                        result = libyuv::RGB24ToI420(reinterpret_cast<unsigned char*>(sharedMemory->data()), sharedMemory->size(),
                                                     &yuv420Frame[0], WIDTH,
                                                     &yuv420Frame[WIDTH * HEIGHT], WIDTH/2,
                                                     &yuv420Frame[WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2)], WIDTH/2,
                                                     WIDTH, HEIGHT);
                    }
                    if (YUV420) {
                        result = (SIZE_OF_YUV420 == sharedMemory->size()) ? 0 : 1;
                        if (result) {
                            // Simply data from shared memory into processing buffer.
                            memcpy(&yuv420Frame[0], sharedMemory->data(), SIZE_OF_YUV420);
                            // TODO: Speed up by omitting this copy and directly pass the pointers into SSourcePicture.
                        }
                    }
                    if (YUYV422) {
                        result = libyuv::YUY2ToI420(reinterpret_cast<unsigned char*>(sharedMemory->data()), WIDTH * 2 /* 2*WIDTH for YUYV 422*/,
                                                    &yuv420Frame[0], WIDTH,
                                                    &yuv420Frame[WIDTH * HEIGHT], WIDTH/2,
                                                    &yuv420Frame[WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2)], WIDTH/2, 
                                                    WIDTH, HEIGHT);
                    }

                    // Test if frame conversion succeeded.
                    if (0 == result) {
                        SFrameBSInfo frameInfo;
                        memset(&frameInfo, 0, sizeof(SFrameBSInfo));

                        SSourcePicture sourceFrame;
                        memset(&sourceFrame, 0, sizeof(SSourcePicture));

                        sourceFrame.iColorFormat = videoFormatI420;
                        sourceFrame.iPicWidth = WIDTH;
                        sourceFrame.iPicHeight = HEIGHT;

                        sourceFrame.iStride[0] = WIDTH;
                        sourceFrame.iStride[1] = WIDTH/2;
                        sourceFrame.iStride[2] = WIDTH/2;
                        sourceFrame.pData[0] = &yuv420Frame[0];
                        sourceFrame.pData[1] = &yuv420Frame[WIDTH * HEIGHT];
                        sourceFrame.pData[2] = &yuv420Frame[WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2)];

                        result = encoder->EncodeFrame(&sourceFrame, &frameInfo);
                        if (cmResultSuccess == result) {
                            if (videoFrameTypeSkip == frameInfo.eFrameType) {
                                std::cerr << argv[0] << ": Warning, skipping frame." << std::endl;
                            }
                            else {
                                int totalSize{0};
                                for(int layer{0}; layer < frameInfo.iLayerNum; layer++) {
                                    int sizeOfLayer{0};
                                    for(int nal{0}; nal < frameInfo.sLayerInfo[layer].iNalCount; nal++) {
                                        sizeOfLayer += frameInfo.sLayerInfo[layer].pNalLengthInByte[nal];
                                    }
                                    memcpy(&h264Buffer[totalSize], frameInfo.sLayerInfo[layer].pBsBuf, sizeOfLayer);
                                    totalSize += sizeOfLayer;
                                }
                                if (0 < totalSize) {
                                    h264DataForImageReading = std::string(&h264Buffer[0], totalSize);
                                }
                            }
                        }
                        else {
                            std::cerr << argv[0] << ": Failed to encode frame: " << result << std::endl;
                        }
                    }
                    else {
                        std::cerr << argv[0] << ": Failed to convert input frame with libyuv." << std::endl;
                    }
                }
                sharedMemory->unlock();

                if (!h264DataForImageReading.empty()) {
                    opendlv::proxy::ImageReading ir;
                    ir.format("h264").width(WIDTH).height(HEIGHT).data(h264DataForImageReading);
                    od4.send(ir);
                }
            }

            if (encoder) {
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
