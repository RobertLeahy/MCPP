#include <packet_router.hpp>
#include <type_traits>
#include <new>
#include <utility>


namespace MCPP {


	static const String packet_dne="Packet 0x{0} has no recognized handler";
	
	
	template <typename T, Word n>
	void init_array (T (& arr) [n]) noexcept(std::is_nothrow_constructible<T>::value) {
	
		for (auto & i : arr) new (&i) T ();
	
	}
	
	
	template <typename T, Word n>
	void destroy_array (T (& arr) [n]) noexcept {
	
		for (auto & i : arr) i.~T();
	
	}
	
	
	inline void PacketRouter::destroy () noexcept {
	
		destroy_array(play_routes);
		destroy_array(status_routes);
		destroy_array(login_routes);
		destroy_array(handshake_routes);
	
	}
	
	
	inline void PacketRouter::init () noexcept {
	
		init_array(play_routes);
		init_array(status_routes);
		init_array(login_routes);
		init_array(handshake_routes);
	
	}


	PacketRouter::PacketRouter () noexcept {
	
		init();
	
	}
	
	
	PacketRouter::~PacketRouter () noexcept {
	
		destroy();
	
	}
	
	
	auto PacketRouter::operator () (UInt32 id, ProtocolState state) noexcept -> Type & {
	
		switch (state) {
		
			case ProtocolState::Handshaking:return handshake_routes[id];
			case ProtocolState::Status:return status_routes[id];
			case ProtocolState::Login:return login_routes[id];
			case ProtocolState::Play:
			default:return play_routes[id];
		
		}
	
	}
	
	
	auto PacketRouter::operator () (UInt32 id, ProtocolState state) const noexcept -> const Type & {
	
		switch (state) {
		
			case ProtocolState::Handshaking:return handshake_routes[id];
			case ProtocolState::Status:return status_routes[id];
			case ProtocolState::Login:return login_routes[id];
			case ProtocolState::Play:
			default:return play_routes[id];
		
		}
	
	}
	
	
	void PacketRouter::operator () (PacketEvent event, ProtocolState state) const {
	
		auto & callback=(*this)(event.Data.ID,state);
		
		if (callback) callback(std::move(event));
	
	}
	
	
	void PacketRouter::Clear () noexcept {
	
		destroy();
		
		init();
	
	}


}
