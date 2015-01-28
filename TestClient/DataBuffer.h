#ifndef _DATA_BUFFER_H_
#define _DATA_BUFFER_H_
#include <boost/asio.hpp>
#include <boost/config/suffix.hpp>
#include <boost/cast.hpp>

enum Type {beat, bonus, winner, team, success, fail, update, start, over, operate, close};
enum Car {car1, car2, car3};
enum CarImg {img1, img2, img3};
enum Map {map1,map2,map3};

class DataBuffer {
public:
	typedef std::size_t size_type;
	typedef boost::asio::streambuf streambuf_type;
	typedef streambuf_type::const_buffers_type const_buffers_type;
	typedef streambuf_type::mutable_buffers_type mutable_buffers_type;
	
private:
	//包头类型字节大小
	static const int TYPE_SIZE = 1;
	//asio流缓冲
	streambuf_type m_buf;
public:
	inline mutable_buffers_type prepare() { return m_buf.prepare(512);}
	//返回类型字节大小的缓冲区用于接收数据
	inline mutable_buffers_type GetTypeBuf() { return m_buf.prepare(1);}
	//返回对应包类型的消息需要字节大小的缓冲区用于接收数据
	mutable_buffers_type GetMsgBuf(Type t);
	//获取接收到的字节
	inline void Retrive(size_type n) {m_buf.commit(n);}
	//返回当前缓冲区是否为空
	inline bool IsEmptyBuf() {return (m_buf.size() == 0 ? true : false);}
	//查看可读的字节
	inline const char* Peek() const {return boost::asio::buffer_cast<const char*>(m_buf.data());}
	//获得用于发送数据的缓冲区
	inline const_buffers_type Data() const {return m_buf.data();}
	//写数据到缓冲区
	inline void Append(const void* data, size_type len) {m_buf.sputn(static_cast<const char*>(data),boost::numeric_cast<std::streamsize>(len));}
	//指示缓冲区消费了n个字节
	inline void Consume(size_type n){ m_buf.consume(n);}

};
#endif