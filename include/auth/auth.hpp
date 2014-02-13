/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <synchronized_random.hpp>
#include <rsa_key.hpp>
#include <uniform_int_distribution.hpp>
#include <yggdrasil.hpp>
#include <random>


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
			//	Random number generator
			SynchronizedRandom<std::mt19937> gen;
			//	Distributes random numbers to produce
			//	a random length for the server ID
			//	string
			SynchronizedRandom<UniformIntDistribution<Word>> id_len;
			//	Distributes random numbers to produce
			//	ASCII characters for the server ID
			//	string
			SynchronizedRandom<UniformIntDistribution<Byte>> id_dist;
		
		
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
			 *		randomly-generated verify token.
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
 