#include <network.hpp>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	SendHandle::SendHandle (Vector<Byte> buffer) noexcept : Command(std::move(buffer)), state(SendState::Sending) {	}


}
