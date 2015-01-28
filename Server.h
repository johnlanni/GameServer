#ifndef _SERVER_H_
#define _SERVER_H_
#include <boost/smart_ptr.hpp>
#include <atomic>
#include <vector>
#include <array>
#include <iostream>
#include "IOServicePool.h"
#include "DataBuffer.h"
#include "Session.h"
#include "GameLog.h"
#define ROOM_CAPACITY 4
#define MAX_CAPACITY 8

class Session;

struct Room {
	typedef boost::shared_ptr<Session> Session_Ptr;
	//房间容量
	int capacity;
	//当前房间人数
	int size;
	//房间地图
	Map map;
	//玩家成员列表
	std::vector<Session_Ptr> playerlist;
	//用来判断房间人数的自旋锁
	std::atomic_flag roomlock;
	//用来实现访问成员列表的自旋锁
	std::atomic_flag listlock;
	//用来判断房间是否为空
	bool isempty;
	//用来判断房间是否满员
	bool isfull;
	//默认构造函数
	Room():capacity(ROOM_CAPACITY),size(0),isempty(true),isfull(false),map(map1){
		roomlock.clear(std::memory_order_relaxed);
		listlock.clear(std::memory_order_relaxed);
	}
};

class Server {
public:
	typedef boost::asio::ip::tcp::acceptor acceptor_type;
	typedef boost::shared_ptr<Session> Session_Ptr;
	std::array<Room, 100> bonus_room;
	std::array<Room, 100> winner_room;
	IOServicePool ios_pool;
	acceptor_type acceptor;
	//启动端口监听，异步接受连接
	void start_accept();
	//accept的异步处理函数
	void handle_accept(const boost::system::error_code& err, Session_Ptr session);
public:
	//构造函数，由n指定线程数
	Server(unsigned short port, int n = 1);
	//启动服务器
	inline void Start() { ios_pool.run();}
	//创建指定类型的房间，引用类型的参数赋值为房间号
	bool CreateRoom(Type t, int& room_id, int room_capacity, Map map_type);
	//进入指定类型和房间号的房间
	bool EnterRoom(Type t, int room_id);
	//快速加入房间
	bool QuickEnter(Type t, int& room_id);

};
#endif