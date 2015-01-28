#include "Server.h"
#include "GameLog.h"

void init() {
	namespace logging = boost::log;
	namespace src = boost::log::sources;
	namespace expr = boost::log::expressions;
	namespace sinks = boost::log::sinks;
	namespace attrs = boost::log::attributes;
	namespace keywords = boost::log::keywords;
	typedef sinks::asynchronous_sink<sinks::text_file_backend> TextSink;  
	boost::shared_ptr<sinks::text_file_backend> backend = boost::make_shared<sinks::text_file_backend>(  
		keywords::file_name = "server_%Y-%m-%d_%H-%M-%S.%N.log",  
		keywords::rotation_size = 10 * 1024 * 1024,  //超过10M重建
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),  //每天0时0分0秒重建
		keywords::min_free_space = 30 * 1024 * 1024);//最低磁盘空间限制
	boost::shared_ptr<TextSink> sink(new TextSink(backend));  
	sink->set_formatter (  
		expr::format("[%1%]<%2%>(thread：%3%):\n %4%")  
		% expr::format_date_time< boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")  
		/* % expr::attr< unsigned int >("LineID")  行号*/
        % logging::trivial::severity
		% expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
		% expr::smessage  
		);
	logging::core::get()->add_global_attribute("ThreadID", attrs::current_thread_id());
	logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
	logging::core::get()->add_sink(sink);
	logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );
}

int main() { 
	init();
	Server s(8384, 8);
	Session::server = &s;
	s.Start();
	BOOST_LOG_TRIVIAL(info) << "服务器开始运行";
} 