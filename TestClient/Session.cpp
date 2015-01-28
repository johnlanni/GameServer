#include "Session.h"
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <cassert>
#include <sstream>
#include <boost/date_time.hpp>

using namespace boost;
using namespace boost::asio;

//用来确定传送的字节
char Byte[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

//非常量静态成员变量必须在类外初始化
//Server* Session::server = nullptr;
std::string timestamp();

Session::Session(ios_type& ios, int logno) : pid(logno + 1), socket(ios), isclose(false), dtimer(get_io_service()), gtimer(get_io_service(), boost::posix_time::seconds(10)){
}
void Session::init() {
	std::stringstream ss;
	ss << "./log/" << pid - 1 << ".log";
	ofs.open(ss.str());
	ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8384);
	socket.connect(ep);
	gtimer.async_wait(boost::bind(&Session::WriteJob, this));
}
void Session::Close() {
	boost::system::error_code ignored_ec;
	socket.shutdown(ip::tcp::socket::shutdown_both, ignored_ec);
	socket.close(ignored_ec);
	isclose = true;
	if(ignored_ec) {
		std::cout<<ignored_ec.message()<<std::endl;
	}
}

//void Session::ReadPlayerInfo() {
//	async_read(socket, 
//		read_buf.GetMsgBuf(bonus),            //三类游戏的玩家信息字节数一致，故这里用bonus包代替其他两类
//		boost::bind(&Session::handle_read_player, shared_from_this(),
//		placeholders::error, placeholders::bytes_transferred)
//		);
//}

//void Session::handle_read_player (const boost::system::error_code& err, size_t byte_transferred) {
//	if(err) {
//		std::cout<<err.message()<<std::endl;
//		Close();
//		return;
//	}
//	//TODO Log
//	read_buf.Retrive(byte_transferred);
//	const char* data = read_buf.Peek();
//	game_type = static_cast<Type>(data[0]);
//	u_id = *reinterpret_cast<const unsigned short*>(data + 1);
//	car_type = static_cast<Car>(data[3]);
//	car_img = static_cast<CarImg>(data[4]);
//	room_id = static_cast<int>(data[5]);
//	read_buf.Consume(byte_transferred);
//	if(room_id < 101 && room_id > 0) {
//		--room_id;
//		if(server->EnterRoom(game_type, room_id)) {//进入房间成功
//			WriteRoomSuccess();//向客户端发送房间信息,以及更新信息
//			WriteHeartPackage();//开始发送心跳包
//		} else {
//			WriteRoomFail();//向客户端发送进入房间失败信息
//		}
//	} else if(room_id == 101) {
//		int room_capacity = static_cast<int>(data[6]);
//		Map map_type = static_cast<Map>(data[7]);
//		if(server->CreateRoom(game_type, room_id, room_capacity, map_type)) {//创建房间成功，设置房间参数，取得房间号
//			WriteRoomSuccess();//向客户端发送房间信息,以及更新信息
//			WriteHeartPackage();//开始发送心跳包
//		} else {
//			WriteRoomFail();//向客户端发送进入房间失败信息
//		}
//	} else if(room_id == 102){//快速加入
//		try {
//			while(!server->QuickEnter(game_type, room_id));//进入失败则重新执行快速加入，直至返回true
//			WriteRoomSuccess();//向客户端发送房间信息,以及更新信息
//			WriteHeartPackage();//开始发送心跳包
//		} catch (std::exception& e) {
//			std::cout<<e.what();
//			WriteRoomFail();//没有空闲房间了
//		}
//	}
//}

//void Session::WriteRoomSuccess() {
//	char* data = new char[19];
//	auto up_data = std::shared_ptr<char>(new char[4],[](char* c){delete[] c;});
//	*up_data = Byte[update];//up_data[0]
//	auto uid = reinterpret_cast<unsigned short*>(up_data.get() + 2);//&up_data[2]
//	*uid = u_id;//本玩家的id
//	Room& r = GetRoom();
//	data[0] = Byte[success];
//	data[1] = room_id + 1;
//	data[2] = r.capacity;
//	data[3] = r.map;
//	int i = 0;
//
//	while(r.listlock.test_and_set(std::memory_order_acquire));//上锁
//
//	for(auto session : r.playerlist) {
//		auto uid =reinterpret_cast<unsigned short*>(data+5+i*2);
//		*uid = session->u_id;//房间内每位玩家的id
//		++i;
//	}
//	data[4] = Byte[i];//人数
//	up_data.get()[1] = Byte[i+1];//本用户在房间中的编号//up_data[1]
//	for(auto session : r.playerlist) {
//		//向对应客户端的写缓冲写更新信息（调用对应客户端的线程写）
//		session->get_io_service().post(boost::bind(&Session::WriteBuf_Shared, session, up_data, 4));
//	}
//	r.playerlist.push_back(shared_from_this());//向玩家列表中添加自己的SessionPtr
//
//	r.listlock.clear(std::memory_order_release);//解锁，允许其他线程读取该房间的玩家列表
//
//	WriteBuf(data, 19);//发送房间信息
//	if(i == r.capacity - 1) {//若刚好是房间中的最后一人
//		for(auto session : r.playerlist) {
//			//向房间内所有客户端发送游戏开始信息
//			session->get_io_service().post(boost::bind(&Session::WriteGameStart, session));
//		}
//		WriteGameOver();//由本线程负责执行游戏定时结束任务，调度发送游戏结束信息
//	}
//	delete []data;
//}

//void Session::WriteRoomFail() {
//	char data = Byte[fail];
//	WriteBuf(&data, 1);
//}
void Session::WriteBuf(const void* data, std::size_t len) {
	bool emptybefore = true;
	if(!write_buf.IsEmptyBuf()) emptybefore = false;
	write_buf.Append(data, len);
	if(emptybefore) {//若写缓冲之前缓冲为空，则启动异步写
		Write();
	}
}

void Session::WriteBuf_Shared(std::shared_ptr<char> data_ptr, std::size_t len) {
	const void* data = data_ptr.get();
	WriteBuf(data, len);
}//data_ptr指向内存在此释放

void Session::WriteHeartPackage() {
	char data = Byte[beat];
	WriteBuf(&data, 1);
	if(!isclose) { //当socket没有断开时继续执行此定时任务，否则share_ptr将使得本对象不能被释放
		dtimer.expires_from_now(boost::posix_time::seconds(30));//绑定本线程，每隔半分钟发一次
		dtimer.async_wait(boost::bind(&Session::WriteHeartPackage, this));
	}
}

void Session::WriteJob() {
	char* data = new char[3];
	data[0] = 9;
	data[1] = pid%8? pid%8 : 8;
	//srand(time(nullptr));
	data[2] = rand() % 127;
	WriteBuf(data, 3);
	if(!isclose) { //当socket没有断开时继续执行此定时任务，否则share_ptr将使得本对象不能被释放
		gtimer.expires_from_now(boost::posix_time::milliseconds(500));//绑定本线程，每隔半秒发一次
		gtimer.async_wait(boost::bind(&Session::WriteJob, this));
	}
	delete[] data;
}

//void Session::WriteGameOver() {
//	//只有积分模式需要计时
//	if(game_type == bonus) {
//		gtimer.expires_from_now(posix_time::minutes(5));//5分钟后结束游戏
//		gtimer.async_wait(boost::bind(&Session::handle_close, shared_from_this()));
//	}
//}

////不直接关闭socket，而是当服务端发现无法读写时进行关闭，同时用智能指针控制了Session的生命周期，保证其能执行完最后一个异步调用句柄
//void Session::handle_close() {
//	Room& r = GetRoom();
//	auto over_data = std::shared_ptr<char>(new char(Byte[over]));
//	for(auto session : r.playerlist) {
//		//向对应客户端的写缓冲写更新信息（调用对应客户端的线程写）
//		session->get_io_service().post(boost::bind(&Session::WriteBuf_Shared, session, over_data, 1));
//	}
//	std::vector<boost::shared_ptr<Session>>().swap(r.playerlist);//清除房间列表内的玩家
//	//重置房间参数
//	r.capacity = 4;
//	r.size = 0;
//	r.map = map1;
//	r.isempty = true;
//	r.isfull = false;
//}

//void Session::WriteGameStart() {
//	char* data = new char[17];
//	Room& r = GetRoom();
//	data[0] = Byte[start];
//	int i = 0;
//	for(auto session : r.playerlist) {
//		//获得每个玩家的车辆类型和配饰类型
//		data[++i] = session->car_type;
//		data[++i] = session->car_img;
//	}
//	WriteBuf(data, 17);
//	//开始异步读数据
//	Read();
//	delete []data;
//}

void Session::Write() {
	if(isclose)
		return;
	async_write(socket, 
		write_buf.Data(), 
		boost::bind(&Session::handle_write, this,
		placeholders::error, placeholders::bytes_transferred)
		);
}

void Session::handle_write(const boost::system::error_code& err, size_t byte_transferred) {
	if(err) {
		Close();
		return;
	}
	ofs << timestamp() << " 发送消息: ";
	const char* wd = write_buf.Peek();
	for(size_t i = 0; i < byte_transferred; ++i) {
		ofs << (int)wd[i] << "\t|";
	}
	ofs << std::endl;
	write_buf.Consume(byte_transferred);
	if(!write_buf.IsEmptyBuf())
		Write();//当写缓冲区非空时，继续调用异步写
}
void Session::Read() {
	socket.async_read_some(
		read_buf.prepare(), 
		boost::bind(&Session::handle_read, this,
		placeholders::error, placeholders::bytes_transferred)
		);
}

//void Session::handle_read_head(const boost::system::error_code& err, size_t byte_transferred) {
//	if(err) {
//		Close();
//		return;
//	}
//	read_buf.Retrive(byte_transferred);
//	const char* data = read_buf.Peek();
//	Type head_type = static_cast<Type>(*data);
//	if(*data == Byte[operate]) { //操作包
//		async_read(socket,
//			read_buf.GetMsgBuf(operate), 
//			boost::bind(&Session::handle_read, shared_from_this(),
//			placeholders::error, placeholders::bytes_transferred, head_type)
//			);
//	} else if(*data == Byte[close]) {//退出包
//		async_read(socket,
//			read_buf.GetMsgBuf(close), 
//			boost::bind(&Session::handle_read, shared_from_this(),
//			placeholders::error, placeholders::bytes_transferred, head_type)
//			);
//	} else {//心跳包及错误包不做处理
//		read_buf.Consume(byte_transferred);
//		Read();
//		return;
//	}
//}


void Session::handle_read(const boost::system::error_code& err, size_t byte_transferred) {
	if(err) {
		Close();
		return;
	}
	ofs << timestamp() << " 收到消息: ";
	const char* rd = read_buf.Peek();
	for(size_t i = 0; i < byte_transferred; ++i) {
		ofs << (int)rd[i] << "\t|";
	}
	ofs << std::endl;
	Read();
	read_buf.Consume(byte_transferred + 1);
}

//Room& Session::GetRoom(){
//	switch(game_type) {
//	case bonus:
//		return server->bonus_room[room_id];
//	case winner:
//		return server->winner_room[room_id];
//	case team://TODO
//	default:
//		throw std::invalid_argument("wrong type of game");
//	}
//}