inline void Server::server_startup () {

	//	Clear clients in case an existing
	//	instance is being recycled
	Clients.Clear();
	//	Clear routes for same reason

	//	Get a data provider
	data=DataProvider::GetDataProvider();

	//	Initialize a thread pool
	
	//	Attempt to grab number of threads
	//	from the data source
	Nullable<String> num_threads_str(data->GetSetting(num_threads_setting));
	
	Word num_threads;
	if (
		num_threads_str.IsNull() ||
		!num_threads_str->ToInteger(&num_threads)
	) num_threads=default_num_threads;
	
	//	Fire up the thread pool
	pool.Construct(num_threads);

	//	Callbacks
	MCPP::LogType log(
		[=] (const String & message, Service::LogType type) -> void {	WriteLog(message,type);	}
	);
	PanicType panic(
		[=] () -> void {	}
	);
	
	//	Try and fire up a connection
	//	manager
	connections.Construct(
		OnConnect,
		OnDisconnect,
		OnReceive,
		log,
		panic,
		*pool
	);
	
	//	Get binds
	Nullable<String> binds_str(data->GetSetting(binds_setting));
	
	Vector<Tuple<IPAddress,UInt16>> binds;
	
	//	Parse binds
	if (!binds_str.IsNull()) {
	
		//	Binds are separated by a semi-colon
		Vector<String> split(Regex(";").Split(*binds_str));
		
		for (String & str : split) {
		
			bool set_port=false;
			bool set_ip=false;
		
			//	Attempt to extract port number
			RegexMatch port_match=Regex(
				"(?<!\\:)\\:\\s*(\\d+)\\s*$",
				RegexOptions().SetRightToLeft()
			).Match(str);
			
			UInt16 port_no;
			
			if (
				port_match.Success() &&
				port_match[1].Value().ToInteger(&port_no)
			) set_port=true;
			else port_no=default_port;
			
			IPAddress ip(IPAddress::Any());
			
			//	Attempt to extract IP address
			RegexMatch ip_match=Regex(
				"^(?:[^\\:]|\\:(?=\\:))+"
			).Match(str);
			
			if (ip_match.Success()) {
			
				try {
				
					IPAddress extracted_ip(ip_match.Value().Trim());
					
					ip=extracted_ip;
					
					set_ip=true;
				
				} catch (...) {	}
			
			}
			
			//	Did we actually set either of
			//	the values?
			if (set_port || set_ip) {
			
				//	Add bind
				binds.EmplaceBack(ip,port_no);
			
			} else {
			
				//	Log that we couldn't parse this bind
				WriteLog(
					String::Format(
						couldnt_parse_bind,
						str
					),
					Service::LogType::Warning
				);
				
			}
		
		}
	
	//	No binds
	} else {
	
		no_binds:
		
		//	IPv4 all, default port
		binds.EmplaceBack(IPAddress::Any(),default_port);
		//	IPv6 all, default port
		binds.EmplaceBack(IPAddress::Any(true),default_port);
	
	}
	
	//	If no binds
	if (binds.Count()==0) goto no_binds;
	
	//	Log binds
	String binds_desc;
	for (Word i=0;i<binds.Count();++i) {
	
		if (i!=0) binds_desc << list_separator;
		
		binds_desc << String::Format(
			endpoint_template,
			binds[i].Item<0>(),
			binds[i].Item<1>()
		);
	
	}
	
	WriteLog(
		String::Format(
			binding_to,
			binds_desc
		),
		Service::LogType::Information
	);
	
	//	Attempt to fire up listen handler
	socket_server.Construct(
		main_thread_desc,
		binds,
		*pool,
		OnAccept,
		[=] (Socket && socket, const IPAddress & ip, UInt16 port) {	connections->Add(std::move(socket),ip,port);	},
		log,
		panic
	);
	
}
