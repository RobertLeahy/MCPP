/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>
#include <traits.hpp>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
 
 
namespace MCPP {


	/**
	 *	\cond
	 */


	namespace VariantImpl {
	
	
		template <typename...>
		class MaxAlignOfImpl {	};
		
		
		template <>
		class MaxAlignOfImpl<> {
		
		
			public:
			
			
				constexpr static Word Value=0;
		
		
		};
		
		
		template <typename T, typename... Args>
		class MaxAlignOfImpl<T,Args...> {
		
		
			private:
			
			
				constexpr static Word next=MaxAlignOfImpl<Args...>::Value;
		
		
			public:
			
			
				constexpr static Word Value=(alignof(T)>next) ? alignof(T) : next;
		
		
		};
		
		
		template <typename...>
		class MaxSizeOfImpl {	};
		
		
		template <>
		class MaxSizeOfImpl<> {
		
		
			public:
			
			
				constexpr static Word Value=0;
		
		
		};
		
		
		template <typename T, typename... Args>
		class MaxSizeOfImpl<T,Args...> {
		
		
			private:
			
			
				constexpr static Word next=MaxSizeOfImpl<Args...>::Value;
		
		
			public:
			
			
				constexpr static Word Value=(sizeof(T)>next) ? sizeof(T) : next;
		
		
		};
		
		
		template <Word, typename, typename...>
		class FindTypeImpl {	};
		
		
		template <Word i, typename T>
		class FindTypeImpl<i,T> {
		
		
			public:
			
			
				constexpr static Word Value=i;
		
		
		};
		
		
		template <Word i, typename T, typename Curr, typename... Args>
		class FindTypeImpl<i,T,Curr,Args...> {
		
		
			public:
			
			
				constexpr static Word Value=std::is_same<
					typename std::decay<T>::type,
					typename std::decay<Curr>::type
				>::value ? i : FindTypeImpl<i+1,T,Args...>::Value;
		
		
		};
		
		
		template <typename...>
		class NoThrowCopyableImpl {	};
		
		
		template <typename T, typename... Args>
		class NoThrowCopyableImpl<T,Args...> {
		
		
			public:
			
			
				constexpr static bool Value=std::is_nothrow_copy_constructible<T>::value ? NoThrowCopyableImpl<Args...>::Value : false;
		
		
		};
		
		
		template <>
		class NoThrowCopyableImpl<> {
		
		
			public:
			
			
				constexpr static bool Value=true;
		
		
		};
		
		
		template <typename...>
		class NoThrowMovableImpl {	};
		
		
		template <typename T, typename... Args>
		class NoThrowMovableImpl<T,Args...> {
		
		
			public:
			
			
				constexpr static bool Value=std::is_nothrow_move_constructible<T>::value ? NoThrowMovableImpl<Args...>::Value : false;
		
		
		};
		
		
		template <>
		class NoThrowMovableImpl<> {
		
		
			public:
			
			
				constexpr static bool Value=true;
		
		
		};
	
	
		template <typename... Args>
		class VariantInfo {
		
		
			public:
			
			
				template <Word i>
				class Types {
				
				
					public:
					
					
						typedef typename std::decay<typename GetType<i,Args...>::Type>::type Type;
				
				
				};
				
				
				template <typename T>
				class FindType {
				
				
					public:
					
					
						constexpr static Word Value=FindTypeImpl<0,T,Args...>::Value;
				
				
				};
				
				
				constexpr static Word MaxAlignOf=MaxAlignOfImpl<Args...>::Value;
				
				
				constexpr static Word MaxSizeOf=MaxSizeOfImpl<Args...>::Value;
				
				
				constexpr static bool NoThrowCopyable=NoThrowCopyableImpl<Args...>::Value;
				
				
				constexpr static bool NoThrowMovable=NoThrowMovableImpl<Args...>::Value;
		
		
		};
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	 
	 
	class BadVariantOperation : public std::exception {
	
	
		public:
		
		
			__attribute__((noreturn))
			static void Raise () {
			
				throw BadVariantOperation();
			
			}
	
	
	};
	
	
	template <typename... Args>
	class Variant {
	
	
		private:
		
		
			typedef VariantImpl::VariantInfo<Args...> info;
		
		
			bool engaged;
			Word active;
			alignas(
				max_align_t
				//info::MaxAlignOf
			) Byte buffer [info::MaxSizeOf];
			
			
			template <typename T>
			T * get_ptr () noexcept {
			
				return reinterpret_cast<T *>(
					reinterpret_cast<void *>(
						buffer
					)
				);
			
			}
			
			
			template <typename T>
			const T * get_ptr () const noexcept {
			
				return reinterpret_cast<const T *>(
					reinterpret_cast<const void *>(
						buffer
					)
				);
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i>=sizeof...(Args)
			>::type destroy_impl () const noexcept {	}
			
			
			template <Word i>
			typename std::enable_if<
				i<sizeof...(Args)
			>::type destroy_impl () noexcept {
			
				typedef typename info::template Types<i>::Type type;
			
				if (active==i) get_ptr<type>()->~type();
				else destroy_impl<i+1>();
			
			}
			
			
			void destroy () noexcept {
			
				if (engaged) {
				
					destroy_impl<0>();
					
					engaged=false;
					
				}
			
			}
			
			
			template <typename T>
			typename std::enable_if<
				!std::is_same<
					typename std::decay<T>::type,
					Variant
				>::value
			>::type construct (T && item) noexcept(
				std::is_nothrow_constructible<
					typename std::decay<T>::type,
					T
				>::value
			) {
			
				constexpr Word index=info::template FindType<T>::Value;
				
				static_assert(
					index!=sizeof...(Args),
					"Variant does not contain that type"
				);
				
				new (buffer) typename std::decay<T>::type (std::forward<T>(item));
				
				engaged=true;
				active=index;
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i>=sizeof...(Args)
			>::type copy_impl (const Variant &) const noexcept {	}
			
			
			template <Word i>
			typename std::enable_if<
				std::is_copy_constructible<typename info::template Types<i>::Type>::value
			>::type copy_impl_impl (const Variant & other) noexcept(
				std::is_nothrow_copy_constructible<typename info::template Types<i>::Type>::value
			) {
			
				typedef typename info::template Types<i>::Type type;
			
				new (buffer) type (*other.get_ptr<type>());
				
				engaged=true;
				active=i;
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				!std::is_copy_constructible<typename info::template Types<i>::Type>::value
			>::type copy_impl_impl (const Variant &) {
			
				BadVariantOperation::Raise();
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i<sizeof...(Args)
			>::type copy_impl (const Variant & other) noexcept (
				info::NoThrowCopyable
			) {
			
				if (i==other.active) copy_impl_impl<i>(other);
				else copy_impl<i+1>(other);
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i>=sizeof...(Args)
			>::type move_impl (const Variant &) const noexcept {	}
			
			
			template <Word i>
			typename std::enable_if<
				std::is_move_constructible<typename info::template Types<i>::Type>::value
			>::type move_impl_impl (Variant && other) noexcept(
				std::is_nothrow_move_constructible<typename info::template Types<i>::Type>::value
			) {
			
				typedef typename info::template Types<i>::Type type;
				
				new (buffer) type (std::move(*other.get_ptr<type>()));
				
				engaged=true;
				active=i;
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				!std::is_move_constructible<typename info::template Types<i>::Type>::value
			>::type move_impl_impl (const Variant &) {
			
				BadVariantOperation::Raise();
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i<sizeof...(Args)
			>::type move_impl (Variant && other) noexcept(
				info::NoThrowMovable
			) {
			
				if (i==other.active) move_impl_impl<i>(std::move(other));
				else move_impl<i+1>(std::move(other));
			
			}
			
			
			void construct (const Variant & other) noexcept(
				info::NoThrowCopyable
			) {
			
				if (other.engaged) copy_impl<0>(other);
			
			}
			
			
			void construct (Variant && other) noexcept(
				info::NoThrowMovable
			) {
			
				if (other.engaged) {
				
					move_impl<0>(std::move(other));
					
					other.destroy();
				
				}
			
			}
			
			
			template <typename T>
			void assign_impl (T && item) noexcept(
				info::NoThrowMovable && info::NoThrowCopyable
			) {
			
				destroy();
				
				construct(std::forward<T>(item));
			
			}
			
			
			template <typename T>
			typename std::enable_if<
				std::is_same<
					typename std::decay<T>::type,
					Variant
				>::value
			>::type assign (T && item) noexcept(
				info::NoThrowMovable && info::NoThrowCopyable
			) {
			
				if (reinterpret_cast<const void *>(&item)!=this) assign_impl(std::forward<T>(item));
			
			}
			
			
			template <typename T>
			typename std::enable_if<
				!std::is_same<
					typename std::decay<T>::type,
					Variant
				>::value
			>::type assign (T && item) noexcept(
				info::NoThrowMovable && info::NoThrowCopyable
			) {
			
				assign_impl(std::forward<T>(item));
			
			}
			
			
		public:
		
		
			Variant () noexcept : engaged(false) {	}
			
			
			Variant (Variant && other) noexcept(
				info::NoThrowMovable
			) : engaged(false) {
			
				construct(std::move(other));
			
			}
			
			
			Variant (const Variant & other) noexcept(
				info::NoThrowCopyable
			) : engaged(false) {
			
				construct(other);
			
			}
			
			
			template <typename T>
			Variant (T && item) noexcept(
				info::NoThrowMovable && info::NoThrowCopyable
			) : engaged(false) {
			
				construct(std::forward<T>(item));
			
			}
			
			
			template <typename T>
			Variant & operator = (T && item) noexcept(
				info::NoThrowMovable && info::NoThrowCopyable
			) {
			
				assign(std::forward<T>(item));
				
				return *this;
			
			}
			
			
			template <typename T, typename... EmplaceArgs>
			typename std::enable_if<
				info::template FindType<T>::Value!=sizeof...(Args)
			>::type Construct (EmplaceArgs &&... args) noexcept(
				std::is_nothrow_constructible<
					T,
					EmplaceArgs...
				>::value
			) {
			
				destroy();
				
				new (buffer) T (std::forward<EmplaceArgs>(args)...);
				
				engaged=true;
				active=info::template FindType<T>::Value;
			
			}
			
			
			~Variant () noexcept {
			
				destroy();
			
			}
			
			
			void Destroy () noexcept {
			
				destroy();
			
			}
			
			
			bool IsNull () const noexcept {
			
				return !engaged;
			
			}
			
			
			template <typename T>
			bool Is () const noexcept {
			
				return engaged && (active==info::template FindType<T>::Value);
			
			}
			
			
			template <typename T>
			T & Get () noexcept {
			
				return *get_ptr<T>();
			
			}
			
			
			template <typename T>
			const T & Get () const noexcept {
			
				return *get_ptr<T>();
			
			}
	
	
	};


}
