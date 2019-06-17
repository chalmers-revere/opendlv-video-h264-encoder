## OpenDLV Microservice to encode images in I420 format into h264 for network broadcast

This repository provides source code to encode images in I420 format that are accessible
via a shared memory area into h264 frames for the OpenDLV software ecosystem.

[![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

[OpenH264 Video Codec provided by Cisco Systems, Inc.](https://www.openh264.org/faq.html)

During the Docker-ized build process for this microservice, Cisco's binary
library is downloaded from Cisco's webserver and installed on the user's
computer due to legal implications arising from the patents around the [AVC/h264 format](http://www.mpegla.com/main/programs/avc/pages/intro.aspx).

End user's notice according to [AVC/H.264 Patent Portfolio License Conditions](https://www.openh264.org/BINARY_LICENSE.txt):
**When you are using this software and build scripts from this repository, you are agreeing to and obeying the terms under which Cisco is making the binary library available.**


## Table of Contents
* [Dependencies](#dependencies)
* [Building and Usage](#building-and-usage)
* [License](#license)


## Dependencies
You need a C++14-compliant compiler to compile this project.

The following dependency is part of the source distribution:
* [libcluon](https://github.com/chrberger/libcluon) - [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

The following dependency is downloaded and installed during the Docker-ized build:
* [openh264](https://www.openh264.org/index.html) - [![License: BSD 2-Clause](https://img.shields.io/badge/License-BSD%202--Clause-blue.svg)](https://opensource.org/licenses/BSD-2-Clause) - [AVC/H.264 Patent Portfolio License Conditions](https://www.openh264.org/BINARY_LICENSE.txt)

## Building and Usage
Due to legal implications arising from the patents around the [AVC/h264 format](http://www.mpegla.com/main/programs/avc/pages/intro.aspx),
we cannot provide and distribute pre-built Docker images. Therefore, we provide
the build instructions in a `Dockerfile` that can be easily integrated in a
`docker-compose.yml` file.

To run this microservice using `docker-compose`, you can simply add the following
section to your `docker-compose.yml` file to let Docker build this software for you:

```yml
version: '2' # Must be present exactly once at the beginning of the docker-compose.yml file
services:    # Must be present exactly once at the beginning of the docker-compose.yml file
    video-h264-encoder-amd64:
        build:
            context: https://github.com/chalmers-revere/opendlv-video-h264-encoder.git#v0.0.4
            dockerfile: Dockerfile.amd64
        restart: on-failure
        network_mode: "host"
        ipc: "host"
        volumes:
        - /tmp:/tmp
        command: "--cid=111 --name=video0.i420 --width=640 --height=480"
```

As this microservice is connecting to another video frame-providing microservice
via a shared memory area using SysV IPC, the `docker-compose.yml` file specifies
the use of `ipc:host`. The parameter `network_mode: "host"` is necessary to
broadcast the resulting frames into an `OD4Session` for OpenDLV. The folder
`/tmp` is shared into the Docker container to attach to the shared memory area.
The parameters to the application are:

* `--cid=111`: Identifier of the OD4Session to broadcast the h264 frames to
* `--id=2`: Optional identifier to set the senderStamp in broadcasted h264 frames in case of multiple instances of this microservice
* `--name=XYZ`: Name of the shared memory area to attach to
* `--width=W`: Width of the image in the shared memory area
* `--height=H`: Height of the image in the shared memory area
* `--bitrate=B`: desired bitrate (default: 100,000)
* `--gop=G`: desired length of group of pictures (default: 10)
* `--bitrate-max`: optional: maximum bitrate (default: 5,000,000, min: 100,000 max: 5,000,000)
* `--gop`: optional: length of group of pictures (default = 10)
* `--rc-mode`: optional: rate control mode (default: RC_QUALITY_MODE (0), min: 0, max: 4)
* `--ecomplexity`: optional: complexity mode (default: LOW_COMPLEXITY (0), min: 0, max: 2)
* `--sps-pps`: optional: SPS/PPS strategy (default: CONSTANT_ID (0), min: 0, max: 3)
* `--num-ref-frame`: optional: number of reference frame used (default: 1, 0: auto, >0 reference frames)
* `--ssei`: optional: toggle ssei (default: 0)
* `--prefix-nal`: optional: toggle prefix NAL adding control (default: 0)
* `--entropy-coding`: optional: toggle entropy encoding (default: CAVLC (0))
* `--frame-skip`: optional: toggle fram-skipping to keep the bitrate within limits (default: 1)
* `--qp-max`: optional: Quantization Parameter max (default: 42, min: 0 max: 51)
* `--qp-min`: optional: Quantization Parameter min (default: 12, min: 0 max: 51)
* `--long-term-ref`: optional: toggle long term reference control (default: 0)
* `--loop-filter`: optional: deblocking loop filter (default: 0, 0: on, 1: off, 2: on except for slice boundaries)
* `--denoise`: optional: toggle denoise control (default: 0)
* `--background-detection`: optional: toggle background detection control (default: 1)
* `--adaptive-quant`: optional: toggle adaptive quantization control (default: 1)
* `--frame-cropping`: optional: toggle frame cropping (default: 1)
* `--scene-change-detect`: optional: toggle scene change detection control (default: 1)
* `--threads`: optional: number of threads (default: 1, O: auto, >1: number of theads, max 4)


## License

* This project is released under the terms of the GNU GPLv3 License

