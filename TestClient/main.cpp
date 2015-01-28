#include <boost/asio.hpp>
#include <iostream>
#include <iterator>
#include <functional>
#include "DataBuffer.h"
#include <boost/bind.hpp>
#include <boost/date_time.hpp>
#include <thread>
#include <mutex>
#include <boost/functional/factory.hpp>
#include "Session.h"
#include "IOServicePool.h"
#include <chrono>

using namespace boost;
using namespace boost::asio;
using namespace std;


DataBuffer read_buf, write_buf;
std::mutex mtx;

string timestamp() {
	std::string strTime = boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());  
	int pos = strTime.find('T');  
	strTime.replace(pos,1,std::string("-"));  
	strTime.replace(pos + 3,0,std::string(":"));  
	strTime.replace(pos + 6,0,std::string(":"));
	return strTime;
}
void handle_read(ip::tcp::socket& sock, const boost::system::error_code& err, size_t byte_transferred) {
	if(err) {
		std::cout<<err.message()<<std::endl;
		boost::system::error_code ignored_ec;
		sock.shutdown(ip::tcp::socket::shutdown_both, ignored_ec);
		sock.close(ignored_ec);
		if(ignored_ec) {
			std::cout<<ignored_ec.message()<<std::endl;
		}
		return;
	}
	void Read(ip::tcp::socket& sock);
	read_buf.Retrive(byte_transferred);
	const char* content = read_buf.Peek();

	{
		std::lock_guard<std::mutex> lck (mtx);
		cout << "收到消息：";
		for(size_t i = 0; i < byte_transferred; ++i) {
			cout << (int)content[i]<<"\t|";
		}
		cout << endl;
		// 这时候strTime里存放时间的格式是YYYYMMDDTHHMMSS，日期和时间用大写字母T隔开了  

		cout << "接收时间：" << timestamp() << endl;  
	}

	read_buf.Consume(byte_transferred);
	Read(sock);
}
void Read(ip::tcp::socket& sock) {
	//若用async_read，则一定要读满缓冲区才会执行句柄
	sock.async_read_some(read_buf.prepare(), boost::bind(handle_read, boost::ref(sock), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}
void Write(ip::tcp::socket& sock) {
	void handle_write(ip::tcp::socket& sock, const boost::system::error_code& err, size_t byte_transferred);
	async_write(sock, write_buf.Data(), boost::bind(handle_write, boost::ref(sock), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void handle_write(ip::tcp::socket& sock, const boost::system::error_code& err, size_t byte_transferred) {
	if(err) {
		std::cout<<err.message()<<std::endl;
		boost::system::error_code ignored_ec;
		sock.shutdown(ip::tcp::socket::shutdown_both, ignored_ec);
		sock.close(ignored_ec);
		if(ignored_ec) {
			std::cout<<ignored_ec.message()<<std::endl;
		}
		return;
	}
	write_buf.Consume(byte_transferred);
	if(!write_buf.IsEmptyBuf())
		Write(sock);//当写缓冲区非空时，继续调用异步写
}


void writebuf(ip::tcp::socket& sock, const void* data, std::size_t len) {
	bool emptybefore = true;
	if(!write_buf.IsEmptyBuf()) emptybefore = false;
	write_buf.Append(data, len);
	if(emptybefore) {//若写缓冲之前缓冲为空，则启动异步写
		Write(sock);
	}
}

void cinrun(ip::tcp::socket& sock) {
	cout<<"请输入数据包中字节对应的ASCII码，以1024结束"<<endl;
	while(true) {
		string line;
		istream_iterator<int> int_it(cin);
		if(*int_it == 1024) break;
		while(*int_it != 1024) {
			line += (char)(*int_it++);
		}
		writebuf(sock, line.c_str(), line.size());

		{
			//获得锁
			std::lock_guard<std::mutex> lck (mtx);
			cout << "发送消息：" ;
			ostream_iterator<int> out_it(cout,"\t|");
			for(auto c : line) {
				*out_it++ = c;
			}
			cout<<endl;
			cout << "发送时间：" << timestamp() << endl;
			//释放锁
		}

		//Read(sock);
		//sock.read_some(read_buf.prepare());
	}
}
#define psize 8
#define rsize 100

int main() {
	cout<<"测试用客户端程序"<<endl;
	cout<<"Input a: 验证服务器逻辑流程"<<endl;
	cout<<"Input b: 验证服务器并发性能"<<endl;
	char i;
	cin>>i;
	//cin.ignore();
	if(i == 'a') {
		io_service ios;
		boost::asio::io_service::work work(ios); 
		std::thread ios_thread([&](){ios.run();});
		ip::tcp::socket sock(ios);
		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8384);
		sock.connect(ep);
		Read(sock);
		cinrun(sock);
	} else {
		_setmaxstdio(1024);//默认为512，系统支持同时能打开的文件上限
		IOServicePool ios_pool(psize);
		std::thread threadpool([&](){ios_pool.run();});
		//typedef boost::shared_ptr<Session> Session_Ptr;
		char* pinfo = new char[8];
		pinfo[0] = 1;
		pinfo[3] = 0;
		pinfo[4] = 0;
		pinfo[5] = 102;
		std::allocator<Session> alloc;
		auto sp = alloc.allocate(rsize * psize);
		auto sq = sp;
		for(int r = 0; r < rsize; ++r) {
			for(int p = 0; p < psize; ++p) {
				//Session_Ptr session = factory<Session_Ptr>()(Session(ios_pool.get_io_service(), r * 10 + p));
				alloc.construct(sq++, ios_pool.get_io_service(), r * psize + p);
				auto session = sq - 1;
				session->init();
				unsigned short uid = r * psize + p;
				pinfo[1] = uid >> 8;
				pinfo[2] = uid & 0x00ff;
				if(p == 0) {
					pinfo[5] = 101;
					pinfo[6] = psize;
					pinfo[7] = 0;
					session->WriteBuf(pinfo, 8);
					session->Read();
				} else {
					pinfo[5] = r + 1;
					pinfo[6] = 0;
					pinfo[7] = 0;
					session->WriteBuf(pinfo, 8);
					session->Read();
				}
			}
		}
		delete[] pinfo;
		std::this_thread::sleep_for(std::chrono::minutes(1));
		while(sq != sp) {
			auto session = --sq;
			session->Close();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			alloc.destroy(session);
		}
		alloc.deallocate(sp, rsize * psize);
		threadpool.join();
	}

}
