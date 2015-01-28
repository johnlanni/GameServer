#include "IOServicePool.h"
#include <stdexcept>
#include <thread>
#include <functional>
#include <boost/bind.hpp>

IOServicePool::IOServicePool(std::size_t poolsize)
  : next_io_service_(0) {
  if (poolsize == 0)
    throw std::runtime_error("io_service_pool size is 0");

  // Give all the io_services work to do so that their run() functions will not
  // exit until they are explicitly stopped.
  for (std::size_t i = 0; i < poolsize; ++i) {
    io_service_ptr io_service(new boost::asio::io_service);
    work_ptr work(new boost::asio::io_service::work(*io_service));
    io_services_.push_back(io_service);
    work_.push_back(work);
  }
}

void IOServicePool::run() {
  // Create a pool of threads to run all of the io_services.
  std::vector<boost::shared_ptr<std::thread> > threads;
  for (std::size_t i = 0; i < io_services_.size(); ++i) {
    boost::shared_ptr<std::thread> thread(new std::thread(
          boost::bind(&boost::asio::io_service::run, io_services_[i])));
    threads.push_back(thread);
  }
  
  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i]->join();
}

void IOServicePool::stop() {
  // Explicitly stop all io_services.
  for (std::size_t i = 0; i < io_services_.size(); ++i)
    io_services_[i]->stop();
}

boost::asio::io_service& IOServicePool::get_io_service() {
  // Use a round-robin scheme to choose the next io_service to use.
  boost::asio::io_service& io_service = *io_services_[next_io_service_];
  ++next_io_service_;
  if (next_io_service_ == io_services_.size())
    next_io_service_ = 0;
  return io_service;
}