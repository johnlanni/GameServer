#include "Server.h"
#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>
#include <algorithm>
#include <iostream>
using namespace boost;
using namespace boost::asio;

Server::Server(unsigned short port, int n):
	ios_pool(n), 
	acceptor(ios_pool.get_io_service(),
	ip::tcp::endpoint(ip::tcp::v4(), port)) {
		start_accept();
}

void Server::start_accept() {
	Session_Ptr session = boost::make_shared<Session>(ios_pool.get_io_service());
	acceptor.async_accept(session->get_socket(),
		boost::bind(&Server::handle_accept, this, placeholders::error, session));
}

void Server::handle_accept(const system::error_code& err, Session_Ptr session) {
	start_accept();

	if(err) {
		std::cout<<err.message()<<std::endl;
		session->Close();
		return;
	}
	using namespace std;
	auto socket = &session->get_socket();
	BOOST_LOG_TRIVIAL(info) << "一个新的用户连接:"<<socket->remote_endpoint().address()<<": "<<socket->remote_endpoint().port();
	session->ReadPlayerInfo();
}

bool Server::CreateRoom(Type t, int& room_id, int room_capacity, Map map_type) {
	std::array<Room,100>* room_arr = nullptr;
	switch(t) {
	case bonus:
		room_arr = &bonus_room;
		break;
	case winner:
		room_arr = &winner_room;
		break;
	case team://TODO
		break;
	default:
		throw std::invalid_argument("wrong type for game");
	};
	int i;
	for(i = 0; i < 100; i++) {
		if((*room_arr)[i].isempty)
			break;
	}
	if(i == 100) throw std::runtime_error("no empty room");
	Room& r = (*room_arr)[i];
	room_id = i;
	//自旋锁版本
	//while(r.roomlock.test_and_set(std::memory_order_acquire));//上锁
	//if(r.size != 0) {
	//	r.roomlock.clear(std::memory_order_release);//解锁
	//	return false;
	//} else { 
	//	r.isempty = false;
	//	++r.size;
	//	r.map = map_type;
	//	r.capacity = room_capacity;
	//	r.roomlock.clear(std::memory_order_release);//解锁
	//	return true;
	//}
	//CAS版本
	int oldsize = 0;
	if(!r.size.compare_exchange_strong(oldsize, 1))
		return false;
	r.isempty = false;
	r.capacity = room_capacity;
	r.map = map_type;
	return true;

}

bool Server::EnterRoom(Type t, int room_id) {
	if(room_id > 99 || room_id < 0)
		throw std::invalid_argument("wrong room id");
	Room* r = nullptr;
	switch(t) {
	case bonus:
		r = &bonus_room[room_id];
		break;
	case winner:
		r = &winner_room[room_id];
		break;
	case team://TODO
		break;
	default:
		throw std::invalid_argument("wrong type for game");
	};
	//自旋锁版本
	//while(r->roomlock.test_and_set(std::memory_order_acquire));//上锁
	//if(r->size == r->capacity) {
	//	r->roomlock.clear(std::memory_order_release);//解锁
	//	return false;
	//} else { 
	//	if(r->isempty)
	//		r->isempty = false;
	//	if(++r->size == r->capacity)
	//		r->isfull = true;
	//	r->roomlock.clear(std::memory_order_release);//解锁
	//	return true;
	//}
	//CAS版本
	int oldsize = r->size;
	if(oldsize == 0) r->isempty = false;
	do {
		if(oldsize == r->capacity) {
			r->isfull = true;
			return false;
		}
	} while(!r->size.compare_exchange_weak(oldsize, oldsize + 1));
	return true;
}

bool Server::QuickEnter(Type t, int& room_id) {
	std::array<Room,100>* room_arr = nullptr;
	switch(t) {
	case bonus:
		room_arr = &bonus_room;
		break;
	case winner:
		room_arr = &winner_room;
		break;
	case team://TODO
		break;
	default:
		throw std::invalid_argument("wrong type for game");
	};

	int min = MAX_CAPACITY + 1;
	int i_min = 100;
	for(int i = 0; i < 100; ++i) {
		Room& room = (*room_arr)[i];
		if(room.capacity - room.size < min && !room.isfull) {
			min = room.capacity - room.size;
			i_min = i;
		}
	}
	room_id = i_min;
	if(room_id == 100)
		throw std::runtime_error("all room are full");
	Room& r = (*room_arr)[room_id];
	//自旋锁版本
	//while(r.roomlock.test_and_set(std::memory_order_acquire));//上锁
	//if(r.size == r.capacity) {
	//	r.roomlock.clear(std::memory_order_release);//解锁
	//	return false;
	//} else { 
	//	if(r.isempty)
	//		r.isempty = false;
	//	if(++r.size == r.capacity)
	//		r.isfull = true;
	//	r.roomlock.clear(std::memory_order_release);//解锁
	//	return true;
	//}
	//CAS版本
	int oldsize = r.size;
	if(oldsize == 0) r.isempty = false;
	do {
		if(oldsize == r.capacity) {
			r.isfull = true;
			return false;
		}
	} while(!r.size.compare_exchange_weak(oldsize, oldsize + 1));
	return true;
}