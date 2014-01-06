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
		
			//	Attempt to get string from
			//	system
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
			
			//	If no characters were returned,
			//	just return the empty string
			if (num==0) return String();
			
			//	Get rid of trailing carriage return
			//	line feed, if present
			for (
				;
				(num!=0) &&
				(
					(ptr[num-1]=='\r') ||
					(ptr[num-1]=='\n')
				);
				--num
			);
			
			//	Return empty string if there
			//	are no characters left after
			//	eliminating trailing carriage
			//	returns and line feeds, otherwise
			//	decode
			String retr;
			if (num!=0) try {
			
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
