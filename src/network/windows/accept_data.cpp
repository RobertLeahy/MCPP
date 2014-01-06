#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		AcceptData::AcceptData (SOCKET socket) noexcept : socket(socket) {	}
		
		
		void AcceptData::cleanup () noexcept {
		
			if (socket!=INVALID_SOCKET) {
			
				closesocket(socket);
				socket=INVALID_SOCKET;
				
			}
		
		}
		
		
		AcceptData::~AcceptData () noexcept {
		
			cleanup();
		
		}
		
		
		AcceptData::AcceptData (AcceptData && other) noexcept
			:	socket(other.socket),
				RemoteIP(other.RemoteIP),
				RemotePort(other.RemotePort),
				LocalIP(other.LocalIP),
				LocalPort(other.LocalPort)
		{
		
			other.socket=INVALID_SOCKET;
		
		}
		
		
		AcceptData & AcceptData::operator = (AcceptData && other) noexcept {
		
			//	Guard against self-assignment
			if (this!=&other) {
			
				cleanup();
				
				socket=other.socket;
				other.socket=INVALID_SOCKET;
				RemoteIP=std::move(other.RemoteIP);
				RemotePort=other.RemotePort;
				LocalIP=std::move(other.LocalIP);
				LocalPort=other.LocalPort;
			
			}
			
			return *this;
		
		}
		
		
		AcceptData::AcceptData (const AcceptData & other) noexcept {
		
			//	We don't copy the socket
			socket=INVALID_SOCKET;
			
			RemoteIP=other.RemoteIP;
			RemotePort=other.RemotePort;
			LocalIP=other.LocalIP;
			LocalPort=other.LocalPort;
		
		}
		
		
		AcceptData & AcceptData::operator = (const AcceptData & other) noexcept {
		
			//	Guard against self-assignment
			if (this!=&other) {
			
				cleanup();
				
				RemoteIP=other.RemoteIP;
				RemotePort=other.RemotePort;
				LocalIP=other.LocalIP;
				LocalPort=other.LocalPort;
			
			}
		
			return *this;
		
		}
		
		
		SOCKET AcceptData::Get () noexcept {
		
			auto retr=socket;
			socket=INVALID_SOCKET;
			
			return retr;
		
		}
	
	
	}


}
