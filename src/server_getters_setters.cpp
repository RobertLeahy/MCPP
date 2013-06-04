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
