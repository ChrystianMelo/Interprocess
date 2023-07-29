#include <iostream>

#include <cassert>
#include <fstream>
#include <chrono>
#include <thread>

#include "InstanceCommunication.h"

/**
 * @brief Main
 */
int main(int argc, char* argv[])
{
	const ReadFileRequest teste("Teste.txt");

	std::thread serverThread([&]() {
		InstanceCommunication server = InstanceCommunication();
		server.destroyConnection(true);
		assert(server.isRunningAsServer());

		server.setServerAvailability(true);

		const bool result = server.runAsServer();
		assert(result);

		assert(server.getRequest() == teste);

		// readFile(server.getRequest().getFilename());
		});

	std::thread clientThread([&]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		InstanceCommunication client = InstanceCommunication();
		assert(!client.isRunningAsServer());

		const bool result = client.runAsClient(teste);
		assert(result);

		});

	serverThread.join();
	clientThread.join();

	return 0;
}
