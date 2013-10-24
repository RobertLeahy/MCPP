#include <client.hpp>
#include <server.hpp>


namespace MCPP {


	static const String encrypt_desc("Connection with {0}:{1} encrypted");
	static const String key_label("Key:");
	static const String iv_label("Initialization Vector:");
	static const String raw_send_template("Server => {0}:{1} - {2} bytes");
	static const String encrypt_key("encryption");
	static const String raw_send_key("raw_send");
	static const String parse_key("raw_recv");
	static const String packet_sent("Server => {0}:{1} {2}");
	static const String packet_recvd("{0}:{1} => Server {2}");
	static const String buffer_recvd("{0}:{1} - Parsing buffer in state {2} - {3} bytes");
	static const String buffer_decrypted("{0}:{1} - Decrypted {1} bytes");
	static const String bytes_consumed("{0}:{1} - Parsing consumed {2} bytes");
	static const String ciphertext_banner("Ciphertext:");
	
	
	//	Formats a byte for display/logging
	static inline String byte_format (Byte b) {
	
		String returnthis("0x");
		
		String byte(b,16);
		
		if (byte.Count()==1) returnthis << "0";
		
		returnthis << byte;
		
		return returnthis;
	
	}
	
	
	static inline String buffer_format (const Byte * begin, const Byte * end) {
	
		String returnthis;
	
		//	Print each byte is hexadecimal
		//	format
		for (Word i=0;begin!=end;++i,++begin) {
		
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
			
			returnthis << byte_format(*begin);
		
		}
		
		return returnthis;
	
	}
	
	
	//	Formats a buffer of bytes for
	//	display/logging
	static inline String buffer_format (const Vector<Byte> & buffer) {
	
		return buffer_format(buffer.begin(),buffer.end());
	
	}


	Client::Client (SmartPointer<Connection> conn)
		:	conn(std::move(conn)),
			state(ProtocolState::Handshaking),
			inactive(Timer::CreateAndStart()),
			connected(Timer::CreateAndStart())
	{
	
		Ping=0;
	
	}
	
	
	void Client::enable_encryption (const Vector<Byte> & key, const Vector<Byte> & iv) {
	
		auto & server=Server::Get();
		
		//	Debug logging
		if (server.IsVerbose(encrypt_key)) {
		
			String log(
				String::Format(
					encrypt_desc,
					IP(),
					Port()
				)
			);
			log	<< Newline << key_label << buffer_format(key)
				<< Newline << iv_label << buffer_format(iv);
				
			server.WriteLog(
				log,
				Service::LogType::Debug
			);
		
		}
		
		//	Enable encryption
		encryptor.Construct(key,iv);
	
	}
	
	
	SmartPointer<SendHandle> Client::Send (Vector<Byte> buffer) {
	
		auto & server=Server::Get();
		
		if (server.IsVerbose(raw_send_key)) {
		
			String log(
				String::Format(
					raw_send_template,
					IP(),
					Port(),
					buffer.Count()
				)
			);
			log << Newline << buffer_format(buffer);
			
			server.WriteLog(
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
	
	
	void Client::Disconnect () noexcept {
	
		conn->Disconnect();
	
	}
	
	
	void Client::Disconnect (const String & reason) noexcept {
	
		conn->Disconnect(reason);
	
	}
	
	
	void Client::Disconnect (String && reason) noexcept {
	
		conn->Disconnect(std::move(reason));
	
	}
	
	
	bool Client::Receive (Vector<Byte> & buffer) {
		
		//	If there are no bytes in the buffer,
		//	nothing is going to happen, and it's
		//	not client activity
		if (buffer.Count()==0) return false;
		
		//	This is activity
		Active();
		
		auto state=GetState();
		
		auto & server=Server::Get();
		
		bool debug=server.IsVerbose(parse_key);
		
		if (debug) {
		
			String log(
				String::Format(
					buffer_recvd,
					IP(),
					Port(),
					ToString(state),
					buffer.Count()
				)
			);
			log << Newline << buffer_format(buffer);
			
			server.WriteLog(
				log,
				Service::LogType::Debug
			);
			
		}
		
		Word before=buffer.Count();
		
		//	Acquire lock so encryption
		//	state doesn't change
		auto retr=read([&] () {
		
			//	If no encryption, attempt to parse
			//	and return at once
			if (encryptor.IsNull()) return parser.FromBytes(buffer,state,ProtocolDirection::Serverbound);
			
			//	Encryption enabled
			
			auto begin=encryption_buffer.end();
			
			//	Decrypt directly into the decryption
			//	buffer
			encryptor->Decrypt(&buffer,&encryption_buffer);
			
			if (debug) {
			
				auto end=encryption_buffer.end();
				
				if (begin!=end) {
				
					String log(
						String::Format(
							buffer_decrypted,
							IP(),
							Port(),
							end-begin
						)
					);
					log << Newline << buffer_format(begin,end);
					
					server.WriteLog(
						log,
						Service::LogType::Debug
					);
				
				}
				
			}
			
			//	Attempt to extract a packet from
			//	the decryption buffer
			return parser.FromBytes(encryption_buffer,state,ProtocolDirection::Serverbound);
			
		});
		
		if (debug) server.WriteLog(
			String::Format(
				bytes_consumed,
				IP(),
				Port(),
				before-buffer.Count()
			),
			Service::LogType::Debug
		);
		
		return retr;
	
	}
	
	
	Packet & Client::GetPacket () noexcept {
	
		auto & retr=parser.Get();
		
		auto & server=Server::Get();
		
		auto state=GetState();
		
		if (server.LogPacket(retr.ID,state,ProtocolDirection::Serverbound)) server.WriteLog(
			String::Format(
				packet_recvd,
				IP(),
				Port(),
				ToString(retr,state,ProtocolDirection::Serverbound)
			),
			Service::LogType::Debug
		);
		
		return retr;
	
	}
	
	
	void Client::SetState (ProtocolState state) noexcept {
	
		write([&] () {	this->state=state;	});
	
	}
	
	
	ProtocolState Client::GetState () const noexcept {
	
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
	
	
	void Client::log (const Packet & packet, ProtocolState state, ProtocolDirection direction, const Vector<Byte> & buffer, const Vector<Byte> & ciphertext) const {
	
		auto & server=Server::Get();
	
		if (server.LogPacket(packet.ID,state,direction)) server.WriteLog(
			String::Format(
				packet_sent,
				IP(),
				Port(),
				ToString(packet,state,direction)
			),
			Service::LogType::Debug
		);
		
		if (server.IsVerbose(raw_send_key)) {
		
			String log(
				String::Format(
					raw_send_template,
					IP(),
					Port(),
					buffer.Count()
				)
			);
			log << Newline << buffer_format(buffer);
			if (ciphertext.Count()!=0) log << Newline << ciphertext_banner << Newline << buffer_format(ciphertext);
			
			server.WriteLog(
				log,
				Service::LogType::Debug
			);
		
		}
	
	}


}
