/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <limits>
#include <type_traits>
#ifdef ENVIRONMENT_WINDOWS
#include <system_error>
#include <Wincrypt.h>
#include <windows.h>
#else
#include <fstream>
#include <stdexcept>
#endif


namespace MCPP {


	/**
	 *	Generates random numbers of arbitrary
	 *	type suitable for use in cryptography.
	 *
	 *	Unlike std::random_device, this template
	 *	is guaranteed to generate non-deterministic
	 *	random numbers.
	 *
	 *	Also unlike std::random_device, this template
	 *	may generate random numbers of arbitrary
	 *	type.
	 *
	 *	\tparam T
	 *		The type of number that shall be
	 *		generated.
	 */
	template <typename T>
	class RandomDevice {
	
	
		private:
		
		
			#ifdef ENVIRONMENT_WINDOWS
		
			HCRYPTPROV handle;
			
			
			[[noreturn]]
			static void raise () {
			
				throw std::system_error(
					std::error_code(
						GetLastError(),
						std::system_category()
					)
				);
			
			}
			
			#else
			
			std::fstream stream;
			
			#endif
			
			
		public:
		
		
			/**
			 *	The type of random number this generator
			 *	will generate.
			 */
			typedef T result_type;
		
		
			/**
			 *	Creates a new random device.
			 */
			RandomDevice ()
			#ifndef ENVIRONMENT_WINDOWS
			: stream(
				"/dev/urandom",
				std::ios::in|std::ios::binary
			)
			#endif
			{
			
				#ifdef ENVIRONMENT_WINDOWS
			
				if (!CryptAcquireContext(
					&handle,
					nullptr,
					nullptr,
					PROV_RSA_FULL,
					CRYPT_VERIFYCONTEXT
				)) raise();
				
				#endif
			
			}
			
			
			RandomDevice (const RandomDevice &) = delete;
			RandomDevice (RandomDevice &&) = delete;
			RandomDevice & operator = (const RandomDevice &) = delete;
			RandomDevice & operator = (RandomDevice &&) = delete;
			
			
			/**
			 *	Cleans up a random device.
			 */
			~RandomDevice () noexcept {
			
				#ifdef ENVIRONMENT_WINDOWS
			
				CryptReleaseContext(handle,0);
				
				#endif
			
			}
			
			
			/**
			 *	Generates a random number.
			 *
			 *	\return
			 *		A random number of type \em T.
			 */
			T operator () () {
			
				static_assert(
					std::is_trivial<T>::value,
					"T is not trivial"
				);
			
				#ifdef ENVIRONMENT_WINDOWS
			
				union {
					T retr;
					BYTE buffer [sizeof(retr)];
				};
				if (!CryptGenRandom(
					handle,
					sizeof(retr),
					buffer
				)) raise();
				
				#else
				
				static_assert(
					sizeof(std::fstream::char_type)==1,
					"std::fstream::char_type is not one byte"
				);
				
				union {
					T retr;
					std::fstream::char_type buffer [sizeof(retr)];
				};
				stream.read(
					buffer,
					sizeof(retr)
				);
				if (stream.gcount()!=sizeof(retr)) throw std::runtime_error(
					"Failed to read /dev/urandom"
				);
				
				#endif
				
				return retr;
			
			}
			
			
			/**
			 *	Returns the smallest number this random
			 *	number generator will generate.
			 *
			 *	\return
			 *		The smallest number this generator
			 *		can generate.
			 */
			T min () const noexcept {
			
				return std::numeric_limits<T>::min();
			
			}
			
			
			/**
			 *	Returns the largest number this random
			 *	number generator will generate.
			 *
			 *	\return
			 *		The largest number this generator
			 *		can generate.
			 */
			T max () const noexcept {
			
				return std::numeric_limits<T>::max();
			
			}
	
	
	};


}
