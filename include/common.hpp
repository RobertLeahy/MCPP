/**
 *	\file
 */


#pragma once


#include <rleahylib/rleahylib.hpp>
#include <functional>
#include <utility>
#include <new>
#include <limits>
#include <type_traits>
#include <stdexcept>
#include <typeinfo>


/**
 *	The namespace which contains the MCPP server
 *	and all related utilities, classes, and
 *	functions.
 */
namespace MCPP {	}


#include <hash.hpp>
#include <random.hpp>
#include <mod.hpp>
#include <url.hpp>
#include <sha1.hpp>
#include <http_handler.hpp>
#include <aes_128_cfb_8.hpp>
#include <rsa_key.hpp>
#include <chunk.hpp>
#include <compression.hpp>
#include <client.hpp>
#include <nbt.hpp>
#include <metadata.hpp>
#include <packet.hpp>
#include <packet_router.hpp>
#include <typedefs.hpp>
#include <event.hpp>
#include <data_provider.hpp>
#include <thread_pool.hpp>
#include <listen_handler.hpp>
#include <connection_manager.hpp>
#include <mod_loader.hpp>
#include <server.hpp>


using namespace MCPP;
