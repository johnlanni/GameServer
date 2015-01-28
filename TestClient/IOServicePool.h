#ifndef _IO_SERVICE_POOL_H_
#define _IO_SERVICE_POOL_H_
#include <boost/asio.hpp>
#include <vector>
#include <boost/noncopyable.hpp>
#include <memory>
class IOServicePool
	: private boost::noncopyable
{
public:
	/// Construct the io_service pool.
	explicit IOServicePool(std::size_t poolsize);

	/// Run all io_service objects in the pool.
	void run();

	/// Stop all io_service objects in the pool.
	void stop();

	/// Get an io_service to use.
	boost::asio::io_service& get_io_service();

private:
	typedef std::shared_ptr<boost::asio::io_service> io_service_ptr;
	typedef std::shared_ptr<boost::asio::io_service::work> work_ptr;

	/// The pool of io_services.
	std::vector<io_service_ptr> io_services_;

	/// The work that keeps the io_services running.
	std::vector<work_ptr> work_;

	/// The next io_service to use for a connection.
	std::size_t next_io_service_;
};
#endif 