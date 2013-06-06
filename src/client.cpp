#include <client.hpp>


namespace MCPP {


	Client::Client (SmartPointer<Connection> && conn) noexcept
		:	conn(std::move(conn)),
			encrypted(false),
			encryption_buffer(0)
	{
	
		state=static_cast<Word>(ClientState::Connected);
	
	}
	
	
	void Client::EnableEncryption (const Vector<Byte> & key, const Vector<Byte> & iv) {
	
		encryptor.Construct(key,iv);
		
		encrypted=true;
	
	}
	
	
	SmartPointer<SendHandle> Client::Send (Vector<Byte> && buffer) {
	
		if (encrypted) {
		
			SmartPointer<SendHandle> handle;
			
			//	Lock to keep stream cipher
			//	synchronized between client
			//	and server (and because
			//	encryption is not parallelizable.
			encryptor->BeginEncrypt();
			
			try {
		
				//	Encrypt
				Vector<Byte> encrypted(encryptor->Encrypt(buffer));
				
				//	Send
				handle=conn->Send(std::move(encrypted));
				
			} catch (...) {
			
				encryptor->EndEncrypt();
				
				throw;
			
			}
			
			encryptor->EndEncrypt();
			
			//	Return
			return handle;
		
		}
		
		//	No encryption, just send
		return conn->Send(std::move(buffer));
	
	}
	
	
	void Client::Disconnect () noexcept {
	
		conn->Disconnect();
	
	}
	
	
	void Client::Disconnect (const String & reason) noexcept {
	
		conn->Disconnect(reason);
	
	}
	
	
	void Client::Disconnect (String && reason) noexcept {
	
		conn->Disconnect(std::move(reason));
	
	}
	
	
	bool Client::Receive (Vector<Byte> * buffer) {
	
		if (buffer==nullptr) throw std::out_of_range(NullPointerError);
		
		if (encrypted) {
		
			//	No need to lock, receive operations
			//	aren't thread safe
			
			//	Decrypt directly into buffer
			encryptor->Decrypt(buffer,&encryption_buffer);
			
			//	Attempt to extract a packet
			return in_progress.FromBytes(encryption_buffer);
		
		}
		
		//	Draw bytes directly from buffer
		return in_progress.FromBytes(*buffer);
	
	}
	
	
	Packet Client::CompleteReceive () noexcept {
	
		return std::move(in_progress);
	
	}
	
	
	void Client::SetState (ClientState state) noexcept {
	
		this->state=static_cast<Word>(state);
	
	}
	
	
	ClientState Client::GetState () const noexcept {
	
		return static_cast<ClientState>(
			static_cast<Word>(state)
		);
	
	}


}
