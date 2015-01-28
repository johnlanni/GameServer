#ifndef _SESSION_H_
#define _SESSION_H_
#include <boost/enable_shared_from_this.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include "DataBuffer.h"
#include "Server.h"
#include <memory>

class Server;
struct Room;

class Session : public boost::enable_shared_from_this<Session> {
public:
	typedef boost::asio::ip::tcp::socket socket_type;
	typedef boost::asio::io_service ios_type;
	//所属的服务器
	static Server* server;
	//构造函数
	Session(ios_type& ios);
	//获得socket
	inline socket_type& get_socket() {return socket;}
	//获得io_service
	inline ios_type& get_io_service() {return socket.get_io_service();}
	//获得读缓冲
	inline DataBuffer& get_read_buf() {return read_buf;}
	//获得写缓冲
	inline DataBuffer& get_write_buf() {return write_buf;}
	//关闭TCP连接
	void Close();
	//读取玩家信息包
	void ReadPlayerInfo();
	//玩家进入房间成功后调用
	void WriteRoomSuccess();
	//向玩家进入房间失败后调用
	void WriteRoomFail();
	//向玩家发送心跳包
	void WriteHeartPackage();
	//向玩家发送游戏开始信息包
	void WriteGameStart();
	//向玩家发送游戏结束信息包
	void WriteGameOver();
	//向缓冲区写数据
	void WriteBuf(const void* data, std::size_t len);
	//向缓冲区写数据,用智能指针保证该数据在多线程调用中正确使用
	void WriteBuf_Shared(std::shared_ptr<char> data_ptr, std::size_t len);
	//异步发送写缓冲里数据
	void Write();
	//异步读数据到读缓冲
	void Read();
	//获得房间的引用
	Room& GetRoom();
private:
	//玩家信息处理函数
	void handle_read_player(const boost::system::error_code& err,
		size_t byte_transferred);
	//游戏开始后读头处理函数
	void handle_read_head(const boost::system::error_code& err,
		size_t byte_transferred);
	//游戏开始后读处理函数
	void handle_read(const boost::system::error_code& err,
		size_t byte_transferred, Type t);
	//写处理函数
	void handle_write(const boost::system::error_code& err,
		size_t byte_transferred);
	//配合deadline_timer到时关闭socket
	void handle_close();
	//asio的sokcet封装
	socket_type socket;
	//读缓冲
	DataBuffer read_buf;
	//写缓冲
	DataBuffer write_buf;
	//用来判断socket连接是否已经断开
	bool isclose;
	//玩家选择的游戏类型
	Type game_type;
	//玩家所在房间id
	int room_id;
	//玩家在数据库中对应的id
	unsigned short u_id;
	//玩家选择的车辆类型
	Car car_type;
	//玩家选择的配饰类型
	CarImg car_img;
	//定时器，保证在session的生命周期内一直存在
	boost::asio::deadline_timer dtimer;
	boost::asio::deadline_timer gtimer;
	
};
#endif