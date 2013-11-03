#include <network.hpp>
#include <system_error>


namespace MCPP {


	namespace NetworkImpl {
	
		
		void Raise () {
		
			throw std::system_error(
				std::error_code(
					GetLastError(),
					std::system_category()
				)
			);
		
		}
		
		
		void RaiseWSA () {
		
			throw std::system_error(
				std::error_code(
					WSAGetLastError(),
					std::system_category()
				)
			);
		
		}
		
		
		void Raise (DWORD code) {
		
			throw std::system_error(
				std::error_code(
					code,
					std::system_category()
				)
			);
		
		}
		
		
		std::exception_ptr GetError (DWORD code) noexcept {
		
			return std::make_exception_ptr(
				std::system_error(
					std::error_code(
						code,
						std::system_category()
					)
				)
			);
		
		}
		
		
		String GetErrorMessage (DWORD code) {
		
			LPWSTR ptr;
			auto num=FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr,
				code,
				0,
				reinterpret_cast<LPWSTR>(&ptr),
				0,
				nullptr
			);
			
			if (num==0) return String();
			
			String retr;
			try {
			
				retr=UTF16(false).Decode(
					reinterpret_cast<Byte *>(ptr),
					reinterpret_cast<Byte *>(ptr+num)
				);
			
			} catch (...) {
			
				LocalFree(ptr);
				
				throw;
			
			}
			
			LocalFree(ptr);
			
			return retr;
		
		}
		
	
	}


}
