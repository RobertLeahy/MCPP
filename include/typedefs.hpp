/**
 *	\file
 */


#pragma once


namespace MCPP {


	/**
	 *	The type of the callback which shall
	 *	be invoked to write to the log.
	 */
	typedef std::function<void (const String &, Service::LogType)> LogType;
	/**
	 *	The type of the callback which shall
	 *	be invoked in case of an unexpected
	 *	error.
	 */
	typedef std::function<void ()> PanicType;


}
