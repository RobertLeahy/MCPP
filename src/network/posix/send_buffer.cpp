#include <network.hpp>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	Connection::SendBuffer::SendBuffer (Vector<Byte> buffer) noexcept : Buffer(std::move(buffer)), Sent(0) {	}


}
