all:
	jam
	./co_server > server.log & 2>&1
	./gs_client > client.log & 2>&1
