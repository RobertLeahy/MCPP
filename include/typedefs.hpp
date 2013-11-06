/**
 *	\file
 */


#pragma once


#include <functional>


namespace MCPP {


	/**
	 *	The type of the callback which shall
	 *	be invoked to write to the log.
	 */
	typedef std::function<void (const String &, Service::LogType)> LogType;


}
