#include <mysql_data_provider/mysql_data_provider.hpp>


namespace MCPP {


	namespace MySQL {


		Vector<Byte> GetBuffer (MYSQL_BIND & bind, PreparedStatement & stmt, Word column) {
		
			auto real_len=*bind.length;
		
			//	If there's nothing to fetch,
			//	just return an empty buffer
			if (real_len==0) return Vector<Byte>();
			
			//	Determine how long the buffer
			//	needs to be
			auto len=safe_cast<Word>(real_len);
			
			//	Create an appropriately-sized
			//	buffer
			Vector<Byte> retr(len);
			
			//	Wire into the bind
			bind.buffer=retr.begin();
			bind.buffer_length=real_len;
			
			//	Fetch
			stmt.Fetch(bind,column);
			retr.SetCount(len);
			
			//	Reset buffer
			bind.buffer=nullptr;
			bind.buffer_length=0;
			
			//	Return
			return retr;
		
		}
	
	
	}


}
