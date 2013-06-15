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
