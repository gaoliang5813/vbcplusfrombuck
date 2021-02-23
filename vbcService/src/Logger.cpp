//
// Created by jackie on 12/17/20.
//

#include "Logger.h"

using namespace boost::log::trivial;

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

namespace logger {
    void initLogStrategy(int h, int m, int s) {
        static bool isFirst = true;
        if (!isFirst) {
            return;
        }

        isFirst = false;
        boost::posix_time::hours hour(h);
        boost::posix_time::minutes minutes(m);
        boost::posix_time::seconds seconds(s);
        boost::posix_time::time_duration timeDuration = hour + minutes + seconds;

        logging::add_common_attributes();

        logging::add_file_log(
                keywords::file_name = "vbcService_%N.log",
                keywords::rotation_size = 10*1024*1024,
                //keywords::time_based_rotation = sinks::file::rotation_at_time_interval(timeDuration),
                keywords::format = "[%TimeStamp%][%Severity%]: %Message%"
        );

        typedef sinks::synchronous_sink<sinks::text_ostream_backend> TextSink;
        // 创建输出槽对象
        boost::shared_ptr<TextSink> sink = boost::make_shared<TextSink>();

        // 向输出槽加入标准日志流对象
        boost::shared_ptr<std::ostream> stream(&std::clog, boost::null_deleter());
        sink->locked_backend()->add_stream(stream);

        // 把输出槽注册到日志库核心
        logging::core::get()->add_sink(sink);

        logging::core::get()->set_filter(
                logging::trivial::severity >= logging::trivial::debug
        );
    }
}
