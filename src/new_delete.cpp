#include <rleahylib/rleahylib.hpp>
#include <new>


void * operator new (std::size_t size) {

	return Memory::RawAllocate(size);

}


void * operator new (std::size_t size, const std::nothrow_t &) noexcept {

	try {
	
		return Memory::RawAllocate(size);
	
	} catch (...) {
	
		return nullptr;
	
	}

}


void * operator new [] (std::size_t size) {

	return Memory::RawAllocate(size);

}


void * operator new [] (std::size_t size, const std::nothrow_t &) noexcept {

	try {
	
		return Memory::RawAllocate(size);
	
	} catch (...) {
	
		return nullptr;
	
	}

}


void operator delete (void * ptr) noexcept {

	Memory::Free(ptr);

}


void operator delete (void * ptr, const std::nothrow_t &) noexcept {

	Memory::Free(ptr);

}


void operator delete [] (void * ptr) noexcept {

	Memory::Free(ptr);

}


void operator delete [] (void * ptr, const std::nothrow_t &) noexcept {

	Memory::Free(ptr);

}
