cmake_minimum_required(VERSION 3.11)
project(vbcService)

set(GNU_GPROF OFF)
set(UNIT_TEST OFF)

# boost 日志库需该定义，否则链接库的时候会报错
add_compile_definitions(BOOST_LOG_DYN_LINK=1)

include_directories(./vendors/include)
include_directories(./vbcService/include)
link_directories(./vendors/lib)


include_directories(/opt/OpenBLAS/include)
include_directories(/usr/local/cuda/include)
include_directories(/usr/local/TensorRT-7.2.2.3/include)
include_directories(./vendors/include)
include_directories(/usr/local/cudnn/include)

link_directories(/usr/local/TensorRT-7.2.2.3/lib)
link_directories(/usr/local/cuda/lib64)
link_directories(./vendors/lib)
link_directories(/usr/local/cudnn/lib64)

set(CMAKE_CXX_STANDARD 11)

if(${GNU_GPROF})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
endif(${GNU_GPROF})

FILE(GLOB_RECURSE SOURCE_FILES ./vbcService/src/*.cpp)
add_executable(vbcService ${SOURCE_FILES} )

target_link_libraries(vbcService paho-mqtt3as paho-mqttpp3 jsoncpp
        avcodec avfilter avutil swscale avformat swresample avdevice
        opencv_core opencv_highgui opencv_videoio opencv_imgcodecs opencv_imgproc
        boost_log_setup boost_log boost_thread pthread openblas)


set(CMAKE_CXX_FLAGS  "-lpthread")
set(CMAKE_CXX_FLAGS  "-O3")
target_link_libraries( vbcService ${OpenCV_LIBS} )
target_link_libraries( vbcService openblas)
target_link_libraries( vbcService cudart)
target_link_libraries( vbcService nvinfer)
target_link_libraries( vbcService nvparsers)


target_link_libraries( vbcService opencv_core opencv_highgui opencv_videoio opencv_imgcodecs opencv_imgproc)
#生成ffmpeg版本信息和OpenCV版本信息的测试程序
add_executable(ffmpegConfTest ffmpegConfTest.cpp)

if(${UNIT_TEST})
#生成单元测试程序 ./vbcService/test/TCPConnectionTest.cpp ./vbcService/src/DeviceManager.cpp ./vbcService/src/Device.cpp ./vbcService/src/OrderControl.cpp ./vbcService/src/StreamManager.cpp ./vbcService/src/MQTTClient.cpp
    FILE(GLOB_RECURSE UNITTEST_SOURCE_FILES ./vbcService/test/*.cpp) #${UNITTEST_SOURCE_FILES}
    add_executable(UnitTest ./vbcService/test/UnitTestMain.cpp ./vbcService/test/DeviceManagerTest.cpp ./vbcService/src/TCPConnection.cpp ./vbcService/src/DeviceManager.cpp  ./vbcService/src/Device.cpp )
    target_link_libraries(UnitTest paho-mqtt3as paho-mqttpp3 jsoncpp
            avcodec avfilter avutil swscale avformat
            opencv_core opencv_highgui opencv_videoio opencv_imgcodecs
            boost_log_setup boost_log boost_thread pthread)
endif(${UNIT_TEST})

target_link_libraries(ffmpegConfTest avcodec opencv_core)
target_link_libraries(ffmpegConfTest ACE)
