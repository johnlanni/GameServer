#include "DataBuffer.h"

boost::asio::streambuf::mutable_buffers_type DataBuffer::GetMsgBuf(Type t) {
	switch(t) {
	case team://TODO
	case winner:
	case bonus:
		return m_buf.prepare(8);//包含类型字节
	case operate:
		return m_buf.prepare(2);
	case close:
		return m_buf.prepare(1);
	default:
		throw std::invalid_argument("wrong type for readbuffer");
	};
}
