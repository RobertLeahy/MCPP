#include <client.hpp>
#include <server.hpp>


namespace MCPP {


	static const String pa_banner("====PROTOCOL ANALYSIS====");
	static const String send_pa_template("Server ==> {0}:{1} - Packet of type {2}");
	static const String encrypt_banner("====ENCRYPTION ENABLED====");
	static const String encrypt_desc("Connection with {0}:{1} encrypted");
	static const String key_pa("Key:");
	static const String iv_pa("Initialization Vector:");
	static const String recv_pa_template("{0}:{1} ==> Server - Packet of type {2}");
	static const String raw_send_template("Server ==> {0}:{1} - RAW SEND ({2} bytes)");
	static const String encrypt_pa_key("encryption");
	static const String raw_send_key("raw_send");
	
	
	//	Formats a byte for display/logging
	static inline String byte_format (Byte b) {
	
		String returnthis("0x");
		
		String byte(b,16);
		
		if (byte.Count()==1) returnthis << "0";
		
		returnthis << byte;
		
		return returnthis;
	
	}
	
	
	//	Formats a buffer of bytes for
	//	display/logging
	static inline String buffer_format (const Vector<Byte> & buffer) {
	
		String returnthis;
	
		//	Print each byte is hexadecimal
		//	format
		for (Word i=0;i<buffer.Count();++i) {
		
			//	We don't need a space
			//	or a newline before
			//	the very first byte
			if (i!=0) {
			
				//	End of line
				if ((i%8)==0) returnthis << Newline;
				//	Space before each byte
				//	that isn't the first or
				//	right after a newline
				else returnthis << " ";
			
			}
			
			returnthis << byte_format(buffer[i]);
		
		}
		
		return returnthis;
	
	}


	Client::Client (SmartPointer<Connection> conn)
		:	conn(std::move(conn)),
			packet_in_progress(false),
			packet_encrypted(false),
			state(ClientState::Connected),
			inactive(Timer::CreateAndStart()),
			connected(Timer::CreateAndStart())
	{
	
		Ping=0;
	
	}
	
	
	void Client::enable_encryption (const Vector<Byte> & key, const Vector<Byte> & iv) {
	
		//	Protocol analysis
		if (Server::Get().IsVerbose(encrypt_pa_key)) {
		
			String log(pa_banner);
			log	<<	Newline
				<<	String::Format(
						encrypt_desc,
						IP(),
						Port()
					)
				<<	Newline
				<<	key_pa
				<<	Newline
				<<	buffer_format(key)
				<<	Newline
				<<	iv_pa
				<<	Newline
				<<	buffer_format(iv);
				
			Server::Get().WriteLog(
				log,
				Service::LogType::Debug
			);
		
		}
		
		//	Enable encryption
		encryptor.Construct(key,iv);
	
	}
	
	
	SmartPointer<SendHandle> Client::send (const Packet & packet) {
	
		//	Protocol analysis
		if (Server::Get().LogPacket(
			packet.Type(),
			ProtocolDirection::ServerToClient
		)) {
		
			String log(pa_banner);
			log	<<	Newline
				<<	String::Format(
						send_pa_template,
						IP(),
						Port(),
						byte_format(packet.Type())
					)
				<<	Newline
				<<	packet.ToString();
				
			Server::Get().WriteLog(
				log,
				Service::LogType::Debug
			);
		
		}
		
		auto buffer=packet.ToBytes();
		SmartPointer<SendHandle> retr;
		
		if (encryptor.IsNull()) {
		
			retr=conn->Send(std::move(buffer));
		
		} else {
		
			encryptor->BeginEncrypt();
			
			try {
			
				retr=conn->Send(
					encryptor->Encrypt(
						std::move(buffer)
					)
				);
			
			} catch (...) {
			
				encryptor->EndEncrypt();
				
				throw;
			
			}
			
			encryptor->EndEncrypt();
		
		}
		
		return retr;
	
	}
	
	
	SmartPointer<SendHandle> Client::Send (Vector<Byte> buffer) {
	
		if (Server::Get().IsVerbose(raw_send_key)) {
		
			String log(pa_banner);
			log	<<	Newline
				<<	String::Format(
						raw_send_template,
						IP(),
						Port(),
						buffer.Count()
					);
					
			Server::Get().WriteLog(
				log,
				Service::LogType::Debug
			);
			
		}
	
		return read([&] () {
		
			SmartPointer<SendHandle> retr;
			
			if (encryptor.IsNull()) {
			
				retr=conn->Send(std::move(buffer));
			
			} else {
			
				encryptor->BeginEncrypt();
				
				try {
				
					retr=conn->Send(
						encryptor->Encrypt(
							std::move(buffer)
						)
					);
				
				} catch (...) {
				
					encryptor->EndEncrypt();
					
					throw;
				
				}
				
				encryptor->EndEncrypt();
			
			}
			
			return retr;
			
		});
	
	}
	
	
	void Client::EnableEncryption (const Vector<Byte> & key, const Vector<Byte> & iv) {
	
		write([&] () {	enable_encryption(key,iv);	});
	
	}
	
	
	SmartPointer<SendHandle> Client::Send (const Packet & packet) {
	
		return read([&] () {	return send(packet);	});
	
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
	
		//	Check for null
		if (buffer==nullptr) throw std::out_of_range(NullPointerError);
		
		//	This is activity
		Active();
		
		//	Acquire lock so encryption
		//	state doesn't change
		return read([&] () {
		
			//	There's no encryption if...
			if (
				(
					//	A packet is in progress and...
					packet_in_progress &&
					//	...that packet started out unencrypted
					!packet_encrypted
				//	or
				) ||
				//	If there's no encryption provider
				encryptor.IsNull()
			) {
			
				//	No encryption
				packet_encrypted=false;
				
				//	Preserve count so we can
				//	determine whether the packet
				//	started constructing itself
				Word before=buffer->Count();
				
				bool complete=in_progress.FromBytes(*buffer);
				
				//	Flag packet as in progress
				//	if necessary
				if (
					complete ||
					before!=buffer->Count()
				) packet_in_progress=true;
				
				return complete;
			
			}
			
			//	Encryption
			
			//	Decrypt directly into decryption buffer
			encryptor->Decrypt(buffer,&encryption_buffer);
			
			//	Attempt to extract a packet
			return in_progress.FromBytes(encryption_buffer);
		
		});
	
	}
	
	
	Packet Client::CompleteReceive () noexcept {
	
		//	Protocol Analysis
		if (Server::Get().LogPacket(
			in_progress.Type(),
			ProtocolDirection::ClientToServer
		)) {
		
			String log(pa_banner);
			log << Newline << String::Format(
				recv_pa_template,
				conn->IP(),
				conn->Port(),
				byte_format(in_progress.Type())
			) << Newline << in_progress.ToString();
			
			Server::Get().WriteLog(
				log,
				Service::LogType::Debug
			);
		
		}
	
		//	Reset in progress flag
		packet_in_progress=false;
	
		return std::move(in_progress);
	
	}
	
	
	void Client::SetState (ClientState state) noexcept {
	
		write([&] () {	this->state=state;	});
	
	}
	
	
	ClientState Client::GetState () const noexcept {
	
		return read([&] () {	return state;	});
	
	}
	
	
	void Client::SetUsername (String username) noexcept {
	
		username_lock.Execute([&] () {	this->username=std::move(username);	});
	
	}
	
	
	String Client::GetUsername () const {
	
		return username_lock.Execute([&] () {	return username;	});
	
	}
	
	
	const Connection * Client::GetConn () const noexcept {
	
		return static_cast<const Connection *>(conn);
	
	}
	
	
	IPAddress Client::IP () const noexcept {
	
		return conn->IP();
	
	}
	
	
	UInt16 Client::Port () const noexcept {
	
		return conn->Port();
	
	}
	
	
	void Client::Active () {
	
		inactive_lock.Execute([&] () {	inactive.Reset();	});
	
	}
	
	
	Word Client::Inactive () const {
	
		return inactive_lock.Execute([&] () {	return static_cast<Word>(inactive.ElapsedMilliseconds());	});
	
	}
	
	
	Word Client::Count () const noexcept {
	
		return encryption_buffer.Count();
	
	}
	
	
	Word Client::Connected () const {
	
		return connected_lock.Execute([&] () {	return static_cast<Word>(connected.ElapsedMilliseconds());	});
	
	}
	
	
	UInt64 Client::Sent () const noexcept {
	
		return conn->Sent();
	
	}
	
	
	UInt64 Client::Received () const noexcept {
	
		return conn->Received();
	
	}
	
	
	Word Client::Pending () const noexcept {
	
		return conn->Pending();
	
	}


}
