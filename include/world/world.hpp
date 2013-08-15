/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <common.hpp>


namespace MCPP {


	class Block {
	
	
		private:
		
		
			UInt32 flags;
			UInt16 id;
			//	Skylight is the high 4 bits,
			//	metadata is the low 4 bits
			Byte skylightmetadata;
			Byte light;
	
	
		public:
		
		
			inline bool TestFlag (Word num) const noexcept {
			
				if (num>31) return false;
				
				return (flags&(static_cast<UInt32>(1)<<static_cast<UInt32>(num)))!=0;
			
			}
			
			
			inline Block & SetFlag (Word num) noexcept {
			
				if (num<=31) flags|=static_cast<UInt32>(1)<<static_cast<UInt32>(num);
				
				return *this;
			
			}
			
			
			inline Block & UnsetFlag (Word num) noexcept {
			
				if (num<=31) flags&=~(static_cast<UInt32>(1)<<static_cast<UInt32>(num));
				
				return *this;
			
			}
			
			
			inline Byte GetLight () const noexcept {
			
				return light;
			
			}
			
			
			inline Block & SetLight (Byte light) noexcept {
			
				if (light<=15) this->light=light;
				
				return *this;
			
			}
			
			
			inline Byte GetMetadata () const noexcept {
			
				return skylightmetadata&15;
			
			}
			
			
			inline Block & SetMetadata (Byte metadata) noexcept {
			
				if (metadata<=15) skylightmetadata=(skylightmetadata&240)|metadata;
				
				return *this;
			
			}
			
			
			inline Byte GetSkylight () const noexcept {
			
				return skylightmetadata>>4;
			
			}
			
			
			inline Block & SetSkylight (Byte skylight) noexcept {
			
				if (skylight<=15) skylightmetadata=(skylightmetadata&15)|(skylight<<4);
				
				return *this;
			
			}
			
			
			inline UInt16 GetID () const noexcept {
			
				return id;
			
			}
			
			
			inline UInt16 SetID (UInt16 id) noexcept {
			
				if (id<=4095) this->id=id;
			
			}
			
	
	};


}
