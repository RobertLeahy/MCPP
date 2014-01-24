#include <network.hpp>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	ConnectionHandler::Worker::Worker () {
	
		Control.Attach(N);
		
		Count=0;
	
	}
	
	
	void ConnectionHandler::Worker::Update (FDType fd) {
	
		Control.Send(Command(CommandType::Update,fd));
	
	}


}
