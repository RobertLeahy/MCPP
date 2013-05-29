/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <new>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <typeinfo>


namespace MCPP {


	enum class MetadatumType : Byte {
	
		Byte=0,
		Short=1,
		Int=2,
		Float=3,
		String=4,
		Slot=5,
		Coordinates=6
	
	};
	
	
	class Coordinates {
	
	
		public:
	
	
			Int32 X;
			Int32 Y;
			Int32 Z;
		
	
	};


	class Metadatum {
	
	
		private:
		
		
			Byte key;
			MetadatumType type;
			union {
				SByte sbyte;
				Int16 int16;
				Int32 int32;
				Single single;
				String string;
				//	TODO: Add slot
				Coordinates coords;
			};
			
			
			template <typename T>
			inline const T & get () const {
			
				static_assert(
					sizeof(const T *)==sizeof(const SByte *),
					"Types do not have requisite sizes on this platform"
				);
			
				typedef typename std::remove_cv<T>::type curr_type;
				
				if (!(
					(
						std::is_same<curr_type,SByte>::value &&
						(type==MetadatumType::Byte)
					) ||
					(
						std::is_same<curr_type,Int16>::value &&
						(type==MetadatumType::Short)
					) ||
					(
						std::is_same<curr_type,Int32>::value &&
						(type==MetadatumType::Int)
					) ||
					(
						std::is_same<curr_type,Single>::value &&
						(type==MetadatumType::Float)
					) ||
					(
						std::is_same<curr_type,String>::value &&
						(type==MetadatumType::String)
					) ||
					//	TODO: Add slot
					(
						std::is_same<curr_type,Coordinates>::value &&
						(type==MetadatumType::Coordinates)
					)
				)) throw std::bad_cast();
				
				union {
					const SByte * union_ptr;
					const T * ptr;
				};
				
				union_ptr=&sbyte;
				
				return *ptr;
			
			}
			
			
			inline void destroy () noexcept;
			inline void copy_move_shared (const Metadatum &) noexcept;
			inline void move (Metadatum &&) noexcept;
			inline void copy (const Metadatum &);
	
	
		public:
		
		
			Metadatum () = delete;
			Metadatum (const Metadatum & other);
			Metadatum (Metadatum && other) noexcept;
			Metadatum & operator = (const Metadatum & other);
			Metadatum & operator = (Metadatum && other) noexcept;
		
		
			template <typename T>
			Metadatum (Byte key, T && obj) noexcept(std::is_nothrow_constructible<
				typename std::remove_reference<T>::type,
				T
			>::value) {
			
				//	Check values for key
				//	Can't be bigger than 5 bits
				//	i.e. 31
				if (key>31) throw std::overflow_error(OverflowError);
				
				this->key=key;
			
				typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type curr_type;
			
				static_assert(
					std::is_same<curr_type,SByte>::value ||
					std::is_same<curr_type,Int16>::value ||
					std::is_same<curr_type,Int32>::value ||
					std::is_same<curr_type,Single>::value ||
					std::is_same<curr_type,String>::value ||
					//	TODO: Add slot
					std::is_same<curr_type,Coordinates>::value,
					"Unsupported type"
				);
			
				if (std::is_same<curr_type,SByte>::value) type=MetadatumType::Byte;
				else if (std::is_same<curr_type,Int16>::value) type=MetadatumType::Short;
				else if (std::is_same<curr_type,Int32>::value) type=MetadatumType::Int;
				else if (std::is_same<curr_type,Single>::value) type=MetadatumType::Float;
				else if (std::is_same<curr_type,String>::value) type=MetadatumType::String;
				//	TODO: Add slot
				else type=MetadatumType::Coordinates;
				
				//	Construct
				new (&sbyte) curr_type (std::forward<T>(obj));
			
			}
			
			
			~Metadatum () noexcept;
			
			
			template <typename T>
			const T & Value () const {
			
				return get<T>();
			
			}
			
			
			template <typename T>
			T & Value () {
			
				return const_cast<T &>(get<T>());
			
			}
			
			
			Byte Key () const noexcept;
			
			
			MetadatumType Type () const noexcept;
	
	
	};


}
