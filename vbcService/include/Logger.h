//
// Created by jackie on 12/17/20.
//

#ifndef VBCSERVICE_LOGGER_H
#define VBCSERVICE_LOGGER_H

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/log/trivial.hpp"
#include "boost/log/expressions.hpp"
#include "boost/log/sinks/text_file_backend.hpp"
#include "boost/log/sinks/text_ostream_backend.hpp"
#include "boost/log/utility/setup/common_attributes.hpp"
#include "boost/log/utility/setup/file.hpp"
#include "boost/log/sources/logger.hpp"
#include "boost/log/sources/severity_logger.hpp"
#include "boost/log/sources/record_ostream.hpp"
#include "boost/core/null_deleter.hpp"
#include "boost/log/sources/global_logger_storage.hpp"

namespace logger {
    // write logs into different file every 24 hours.
    void initLogStrategy(int h=24, int m=0, int s=0);

    static boost::log::sources::severity_logger<boost::log::trivial::severity_level> sLogger;
}

#define LOG_TRACE BOOST_LOG_SEV(logger::sLogger, boost::log::trivial::trace)
#define LOG_DEBUG BOOST_LOG_SEV(logger::sLogger, boost::log::trivial::debug)
#define LOG_INFO BOOST_LOG_SEV(logger::sLogger, boost::log::trivial::info)
#define LOG_WARNING BOOST_LOG_SEV(logger::sLogger, boost::log::trivial::warning)
#define LOG_ERROR BOOST_LOG_SEV(logger::sLogger, boost::log::trivial::error)
#define LOG_FATAL BOOST_LOG_SEV(logger::sLogger, boost::log::trivial::fatal)

#endif //VBCSERVICE_LOGGER_H
