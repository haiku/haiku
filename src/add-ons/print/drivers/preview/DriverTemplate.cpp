extern "C" _EXPORT char * add_printer(char * printer_name) {
	return printer_name;
}

extern "C" _EXPORT BMessage * config_page(BNode * spool_dir, BMessage * msg) {
	return NULL;
}

extern "C" _EXPORT BMessage * config_job(BNode * spool_dir, BMessage * msg) {
	return NULL;
}

extern "C" _EXPORT BMessage * default_settings(BNode * printer) {
	return NULL;
}

extern "C" _EXPORT BMessage * take_job(BFile * spool_file, BNode * spool_dir, BMessage * msg) {
	return NULL;
}
