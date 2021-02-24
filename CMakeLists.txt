cmake_minimum_required(VERSION 3.11)
project(vbcService)


set(CMAKE_EXE_LINKER_FLAGS "")

set(GNU_GPROF ON)
set(UNIT_TEST ON)
set(FACE_RECG OFF)

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
if(NOT ${FACE_RECG})
    file(GLOB SOURCES_FOR_NOFACEDEC vbcService/src/*.cpp)
    #exclude / remove vbcService.cpp from the unittest source file list
    #get_filename_component(full_path_test_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/vbcService.cpp  ABSOLUTE)
    #MESSAGE( STATUS "full_path_test_cpp = ${full_path_test_cpp}.")
    get_filename_component(full_path_baseEngine_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/baseEngine.cpp  ABSOLUTE)
    #MESSAGE( STATUS "full_path_baseEngine_cpp = ${full_path_baseEngine_cpp}.")
    get_filename_component(full_path_mtcnn_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/mtcnn.cpp  ABSOLUTE)
    get_filename_component(full_path_network_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/network.cpp  ABSOLUTE)
    get_filename_component(full_path_onet_rt_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/onet_rt.cpp  ABSOLUTE)
    get_filename_component(full_path_pnet_rt_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/pnet_rt.cpp  ABSOLUTE)
    get_filename_component(full_path_rnet_rt_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/rnet_rt.cpp  ABSOLUTE)

    list(REMOVE_ITEM SOURCES_FOR_NOFACEDEC ${full_path_test_cpp} ${full_path_baseEngine_cpp} ${full_path_baseEngine_cpp} ${full_path_mtcnn_cpp} ${full_path_network_cpp} ${full_path_onet_rt_cpp} ${full_path_pnet_rt_cpp} ${full_path_rnet_rt_cpp})
    MESSAGE("SOURCES_FOR_NOFACEDEC = ${SOURCES_FOR_NOFACEDEC}")
    MESSAGE("SOURCE_FILES = ${SOURCE_FILES}")
    add_executable(vbcService ${SOURCES_FOR_NOFACEDEC}) 
else()
    add_executable(vbcService ${SOURCE_FILES})
endif(${FACE_RECG})


target_link_libraries(vbcService paho-mqtt3as paho-mqttpp3 jsoncpp
        avcodec avfilter avutil swscale avformat swresample avdevice
        opencv_core opencv_highgui opencv_videoio opencv_imgcodecs opencv_imgproc
        boost_log_setup boost_log boost_thread pthread openblas)


set(CMAKE_CXX_FLAGS  "-lpthread")
set(CMAKE_CXX_FLAGS  "-O3")

#set(CMAKE_CXX_FLAGS  "-lgcov")
#set(CMAKE_CXX_FLAGS  "--coverage")



#add_compile_options("-fprofile-generate")
#add_compile_options("--coverage")
#set(CMAKE_CXX_FLAGS  "-fprofile-instr-generate")
#set(CMAKE_CXX_FLAGS  "-fcoverage-mapping")

target_link_libraries( vbcService ${OpenCV_LIBS} )
target_link_libraries( vbcService openblas)

if(${FACE_RECG})
target_link_libraries( vbcService cudart)
target_link_libraries( vbcService nvinfer)
target_link_libraries( vbcService nvparsers)
endif(${FACE_RECG})



target_link_libraries( vbcService opencv_core opencv_highgui opencv_videoio opencv_imgcodecs opencv_imgproc)
#生成ffmpeg版本信息和OpenCV版本信息的测试程序
add_executable(ffmpegConfTest ffmpegConfTest.cpp)



file(GLOB SOURCES_FOR_UNITTEST vbcService/src/*.cpp)
#exclude / remove vbcService.cpp from the unittest source file list
get_filename_component(full_path_test_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/vbcService.cpp ABSOLUTE)
list(REMOVE_ITEM SOURCES_FOR_UNITTEST ${full_path_test_cpp})

if(${UNIT_TEST})
#生成单元测试程序 ./vbcService/test/TCPConnectionTest.cpp ./vbcService/src/DeviceManager.cpp ./vbcService/src/Device.cpp ./vbcService/src/OrderControl.cpp ./vbcService/src/StreamManager.cpp ./vbcService/src/MQTTClient.cpp
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")

    get_filename_component(full_main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/vbcService/src/vbcService.cpp  ABSOLUTE)
    list(REMOVE_ITEM SOURCES_FOR_NOFACEDEC ${full_main_cpp})

    #FILE(GLOB_RECURSE UNITTEST_SOURCE_FILES ./vbcService/test/*.cpp) #${UNITTEST_SOURCE_FILES}
  MESSAGE( STATUS "SOURCES_UNITTEST = ${SOURCES_FOR_NOFACEDEC}.")
   add_executable(UnitTest ./vbcService/test/DeviceManagerTest.cpp   ${SOURCES_FOR_NOFACEDEC})
    target_link_libraries(UnitTest paho-mqtt3as paho-mqttpp3 jsoncpp
            avcodec avfilter avutil swscale avformat avdevice
            opencv_core opencv_highgui opencv_videoio opencv_imgcodecs opencv_imgproc
            boost_log_setup boost_log boost_thread pthread)

target_link_libraries(UnitTest avcodec opencv_core)
target_link_libraries(UnitTest ACE)
target_link_libraries( UnitTest opencv_core opencv_highgui opencv_videoio opencv_imgcodecs opencv_imgproc)
endif(${UNIT_TEST})

target_link_libraries(ffmpegConfTest avcodec opencv_core)
target_link_libraries(ffmpegConfTest ACE)

