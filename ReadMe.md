## 视频监控系统 -- 服务器端（Server）

### Overview

#### Capture
文件capture.cpp和capture.hpp是类Capture的源文件，采集是视频监控系统的第一步，所以在主函数中
调用创建的第一个类就是Capture，具体的函数及实现都已在代码中注明，可以查看。

#### Encoder
文件encoder.cpp和encoder.hpp是类Encoder的源文件，编码压缩是视频监控系统的第二部，所以在主
函数中调用创建的第二类就是Encoder，该类中调用了x264开源编码的库的代码，所以在自动编译时要
加上-lx264

#### Sender
文件sender.cpp和sender.hpp是类Sender的源文件，码流发送时视频监控系统的第三部分，所以在主函数中调用第三个类是Sender，该类中使用了开源的Jrtp库，用以实现通过RTP和RTCP协议的码流传输，从而在自动编译时需要加入-ljrtp和-lpthread两个动态链接库。

#### Some notices：
由于现在的所有试验都是基于PC端的，所以还没有将代码向嵌入式开发板上移植，因此在自动化编译时，使用的编译器都是clang++。如果在后期需要向开发板上移植时，可以修改编译器为arm-linux-g++。
