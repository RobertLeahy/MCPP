/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <random.hpp>
#include <rsa_key.hpp>
#include <yggdrasil.hpp>


namespace MCPP {


	/**
	 *	
	 */
	class Authentication : public Module {
	
	
		private:
		
		
			//	Master server encryption key
			RSAKey key;
			//	Yggdrasil authenticator
			Yggdrasil::Client ygg;
			//	Generates verify tokens
			Random<Byte> token_generator;
			//	Randomly selects a length for
			//	a server ID string
			Random<Word> id_len_generator;
			//	Randomly selects ASCII characters
			//	for the server ID string
			Random<Byte> id_generator;
		
		
		public:
		
		
			/**
			 *	\cond
			 */
			 
			 
			Authentication ();
			
			
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	Retrieves a reference to an instance of this
			 *	class.
			 *
			 *	\return
			 *		A reference to an instance of this class.
			 */
			static Authentication & Get ();
			
			
			/**
			 *	Verifies two verify tokens by ensuring that
			 *	they match.
			 *
			 *	\param [in] a
			 *		A verify token.
			 *	\param [in] b
			 *		A verify token.
			 *
			 *	\return
			 *		\em true if \em a and \em b match, \em false
			 *		otherwise.
			 */
			static bool VerifyTokens (const Vector<Byte> & a, const Vector<Byte> & b) noexcept;
			
			
			/**
			 *	Retrieves this authenticator's RSA public/private
			 *	key pair.
			 *
			 *	\return
			 *		A reference to an RSA public/private key
			 *		pair.
			 */
			const RSAKey & GetKey () const noexcept;
			/**
			 *	Gets this authenticator's Yggdrasil client.
			 *
			 *	\return
			 *		A reference to a Yggdrasil client.
			 */
			Yggdrasil::Client & GetClient () noexcept;
			/**
			 *	Generates a random verify token.
			 *
			 *	\return
			 *		A buffer of bytes containing a
			 *		randmly-generated verify token.
			 */
			Vector<Byte> GetVerifyToken ();
			/**
			 *	Generates a random server ID string.
			 *
			 *	\return
			 *		A randomly-generated server ID string
			 *		suitable for use in vanilla authentication.
			 */
			String GetServerID ();
	
	
	};


}
 