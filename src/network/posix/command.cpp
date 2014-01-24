#include <network.hpp>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	ConnectionHandler::Command::Command (CommandType type, FDType fd, SmartPointer<ChannelBase> channel) noexcept
		:	Type(type),
			FD(fd),
			Impl(std::move(channel))
	{	}


}
