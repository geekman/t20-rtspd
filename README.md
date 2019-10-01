T20L RTSP Server
==================

This project originally came from <https://github.com/beihuijie/carrier-rtsp-server>.

It's a very spartan RTSP server, using the live555 library for RTSP implementation,
and a FIFO file descriptor to feed the H.264 frames from the SDK example.
It's not robust, but it works.

This repo differs by making it work with lower memory devices using the T20L variant,
like the Xiaofang 1S. I have reverse-engineered the 1S binary in the firmware, 
and found undocumented SDK calls to increase the pool sizes in order to allow
the SDK to function under low memory.

The IR LEDs are now also software-controlled via PWM channel 0.
The settings were also reversed from the 1S binary, and should work with the 
WyzeCam v2 since they also got rid of the CdS photoresistor.

Also, you can find more technical information from the original 
[Dafang Hacks project](https://github.com/EliasKotlyar/Xiaomi-Dafang-Hacks).

Compilation
============

1. Download the T20 toolchain, either from [the Dafang-Hacks GitHub repo](https://github.com/Dafang-Hacks/Ingenic-T10_20/tree/master/resource/toolchain) or the [Tuya GitHub repo](https://github.com/TuyaInc/TUYA_IPC_SDK/tree/master/mips-linux-4.7.2_64Bit).

2. Set up the `PATH` environment variable to add the toolchain directory.

2. Clone this repo

3. Run the `make` command.
   You should end up with the `t20-rtspd` binary in the directory.

License
========

The original source code doesn't really have a license.

However, the newer changes made by me are licensed under **the 3-clause
("modified") BSD License**, where applicable.

Copyright (C) 2019 Darell Tan

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

