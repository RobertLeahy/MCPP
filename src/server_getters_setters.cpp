static const String server_id_setting("server_id");
static const String motd_setting("motd");
static const String default_server_id("Minecraft++");


bool Server::IsInteractive () const noexcept {

	return is_interactive;

}


DataProvider & Server::Data () {

	if (data==nullptr) throw std::out_of_range(NullPointerError);

	return *data;

}


ThreadPool & Server::Pool () {

	if (pool.IsNull()) throw std::out_of_range(NullPointerError);
	
	return *pool;

}

	
String Server::GetMessageOfTheDay () {

	if (data==nullptr) return String();
	
	Nullable<String> motd=data->GetSetting(motd_setting);
	
	if (motd.IsNull()) return String();
	
	return *motd;

}


static const Regex date_trim(" {2,}");
static const RegexReplacement date_trim_r(" ");
String Server::BuildDate () const {

	return date_trim.Replace(
		String::Format(
			"{0} {1}",
			__DATE__,
			__TIME__
		),
		date_trim_r
	);

}


String Server::CompiledWith () const {

	#ifdef __GNUC__
	return String::Format(
		"GNU C++ Compiler (g++) {0}.{1}.{2}",
		__GNUC__,
		__GNUC_MINOR__,
		__GNUC_PATCHLEVEL__
	);
	#else
	return "UNKNOWN";
	#endif

}


Server & Server::Get () noexcept {

	return *RunningServer;

}
