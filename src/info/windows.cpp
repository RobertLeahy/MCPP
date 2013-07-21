#include <windows.h>


static inline String get_os_string () {

	typedef void (WINAPI *PGNSI) (LPSYSTEM_INFO);
	typedef BOOL (WINAPI *PGPI) (DWORD, DWORD, DWORD, DWORD, PDWORD);
	
	SYSTEM_INFO si;
	OSVERSIONINFOEXW osvi;
	
	memset(&si,0,sizeof(SYSTEM_INFO));
	memset(&osvi,0,sizeof(OSVERSIONINFOEXW));
	
	//	Call GetVersionEx
	osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEXW);
	if (!GetVersionExW(
		reinterpret_cast<OSVERSIONINFOW *>(
			&osvi
		)
	)) throw std::system_error(
		std::error_code(
			GetLastError(),
			std::system_category()
		)
	);
	
	//	Call GetNativeSystemInfo if supported or
	//	GetSystemInfo otherwise
	Vector<Byte> kernel32(
		String("kernel32.dll").ToOSString()
	);
	Vector<ASCIIChar> native_sys_info_name(
		String("GetNativeSystemInfo").ToCString()
	);
	PGNSI native_sys_info=reinterpret_cast<PGNSI>(
		GetProcAddress(
			GetModuleHandleW(
				reinterpret_cast<LPWSTR>(
					static_cast<Byte *>(
						kernel32
					)
				)
			),
			reinterpret_cast<LPCSTR>(
				static_cast<ASCIIChar *>(
					native_sys_info_name
				)
			)
		)
	);
	
	if (native_sys_info==nullptr) GetSystemInfo(&si);
	else native_sys_info(&si);
	
	String output("Microsoft ");
	
	//	Check for Windows NT
	if (osvi.dwPlatformId==VER_PLATFORM_WIN32_NT) {
	
		//	Modern Windows
		if (osvi.dwMajorVersion>4) {

			bool is_workstation=osvi.wProductType==VER_NT_WORKSTATION;
	
			//	Test for the specific product
			if (osvi.dwMajorVersion==6) {
			
				//	Windows 7, Windows 8, Windows Vista,
				//	Windows Server 2008, Windows Server 2008 R2,
				//	or Windows Server 2012
				
				switch (osvi.dwMinorVersion) {
				
					case 0:
						if (is_workstation) output << "Windows Vista";
						else output << "Windows Server 2008";
						break;
					case 1:
						if (is_workstation) output << "Windows 7";
						else output << "Windows Server 2008 R2";
						break;
					case 2:
						if (is_workstation) output << "Windows 8";
						else output << "Windows Server 2012";
						break;
					case 3:
					default:
						if (is_workstation) output << "Windows 8.1";
						else output << "Windows Server 2012";
						break;
						
				}
				
				//	Call GetProductInfo function
				Vector<ASCIIChar> get_product_info(
					String("GetProductInfo").ToCString()
				);
				DWORD type;
				reinterpret_cast<PGPI>(
					GetProcAddress(
						GetModuleHandleW(
							reinterpret_cast<LPWSTR>(
								static_cast<Byte *>(
									kernel32
								)
							)
						),
						reinterpret_cast<LPCSTR>(
							static_cast<ASCIIChar *>(
								get_product_info
							)
						)
					)
				)(
					osvi.dwMajorVersion,
					osvi.dwMinorVersion,
					0,
					0,
					&type
				);
				
				output << " ";
				
				switch (type) {
				
					case PRODUCT_ULTIMATE:
						output << "Ultimate Edition";
						break;
					case PRODUCT_PROFESSIONAL:
						output << "Professional";
						break;
					case PRODUCT_HOME_PREMIUM:
						output << "Home Premium Edition";
						break;
					case PRODUCT_HOME_BASIC:
						output << "Home Basic Edition";
						break;
					case PRODUCT_ENTERPRISE:
						output << "Enterprite Edition";
						break;
					case PRODUCT_BUSINESS:
						output << "Business Edition";
						break;
					case PRODUCT_STARTER:
						output << "Starter Edition";
						break;
					case PRODUCT_CLUSTER_SERVER:
						output << "Cluster Server Edition";
						break;
					case PRODUCT_DATACENTER_SERVER:
						output << "Datacenter Edition";
						break;
					case PRODUCT_DATACENTER_SERVER_CORE:
						output << "Datacenter Edition (server core installation)";
						break;
					case PRODUCT_ENTERPRISE_SERVER:
						output << "Enterprise Edition";
						break;
					case PRODUCT_ENTERPRISE_SERVER_CORE:
						output << "Enterprise Edition (server core installation)";
						break;
					case PRODUCT_ENTERPRISE_SERVER_IA64:
						output << "Enterprise Edition for Itanium-based Systems";
						break;
					case PRODUCT_SMALLBUSINESS_SERVER:
						output << "Small Business Server";
						break;
					case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
						output << "Small Business Server Premium Edition";
						break;
					case PRODUCT_STANDARD_SERVER:
						output << "Standard Edition";
						break;
					case PRODUCT_STANDARD_SERVER_CORE:
						output << "Standard Edition (server core installation)";
						break;
					case PRODUCT_WEB_SERVER:
					default:
						output << "Web Server Edition";
						break;
						
				}
			
			} else {
			
				//	Windows XP, Windows Server 2003, Windows Server 2003
				//	R2, or Windows 2000
				
				switch (osvi.dwMinorVersion) {
				
					case 0:
					
						output << "Windows 2000 ";
						
						if (is_workstation) output << "Professional";
						else if ((osvi.wSuiteMask&VER_SUITE_DATACENTER)!=0) output << "Datacenter Server";
						else if ((osvi.wSuiteMask&VER_SUITE_ENTERPRISE)!=0) output << "Advanced Server";
						else output << "Server";
						
						break;
						
					case 1:
					
						output << "Windows XP ";
						
						if ((osvi.wSuiteMask&VER_SUITE_PERSONAL)!=0) output << "Home Edition";
						else output << "Professional";
						
						break;
						
					case 2:
					default:
					
						if (GetSystemMetrics(SM_SERVERR2)) output << "Windows Server 2003 R2 ";
						else if ((osvi.wSuiteMask&VER_SUITE_STORAGE_SERVER)!=0) output << "Windows Storage Server 2003";
						else if ((osvi.wSuiteMask&VER_SUITE_WH_SERVER)!=0) output << "Windows Home Server";
						else if (
							is_workstation &&
							(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
						) output << "Windows XP Professional x64 Edition";
						else output << "Windows Server 2003";
						
						//	Test for the server type
						if (!is_workstation) {
						
							switch (si.wProcessorArchitecture) {
							
								case PROCESSOR_ARCHITECTURE_IA64:
								
									if ((osvi.wSuiteMask&VER_SUITE_DATACENTER)!=0) output << "Datacenter Edition for Itanium-based Systems";
									else if ((osvi.wSuiteMask&VER_SUITE_ENTERPRISE)!=0) output << "Enterprise Edition for Itanium-based Systems";
									
									break;
								
								case PROCESSOR_ARCHITECTURE_AMD64:
								
									if ((osvi.wSuiteMask&VER_SUITE_DATACENTER)!=0) output << "Datacenter x64 Edition";
									else if ((osvi.wSuiteMask&VER_SUITE_ENTERPRISE)!=0) output << "Enterprise x64 Edition";
									else output << "Standard x64 Edition";
									
									break;
								
								default:
								
									if ((osvi.wSuiteMask&VER_SUITE_COMPUTE_SERVER)!=0) output << "Compute Cluster Edition";
									else if ((osvi.wSuiteMask&VER_SUITE_DATACENTER)!=0) output << "Datacenter Edition";
									else if ((osvi.wSuiteMask&VER_SUITE_ENTERPRISE)!=0) output << "Enterprise Edition";
									else if ((osvi.wSuiteMask&VER_SUITE_BLADE)!=0) output << "Web Edition";
									else output << "Standard Edition";
								
									break;
							
							}
						
						}
				
				}
			
			}
			
			//	Get service pack information
			Word count;
			for (count=0;osvi.szCSDVersion[count]!=0;++count);
			
			if (count>0) {
			
				String sp(
					UTF16(false).Decode(
						reinterpret_cast<const Byte *>(
							&(osvi.szCSDVersion[0])
						),
						reinterpret_cast<const Byte *>(
							&(osvi.szCSDVersion[count])
						)
					)
				);
				
				output << " " << sp;
			
			}
			
			//	Get build number
			output << String::Format(
				" (build {0})",
				osvi.dwBuildNumber
			);
			
			if (osvi.dwMajorVersion>=6) {
			
				if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64) output << ", 64-bit";
				else output << ", 32-bit";
			
			}
	
		//	NT
		} else {
		
			output << String::Format(
				"Windows NT {0}",
				osvi.dwMajorVersion
			);
		
		}
	
	//	DOS or 9.x
	} else {
	
		output << "Windows 9.x-based or MS-DOS";
	
	}
	
	return output;

}