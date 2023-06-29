#include <iostream>
#include <cassert>
#include <fstream>
#include <chrono>

#include "CommunicationUtils.h"

int main()
{
	std::string filename("filename.txt");

	const std::function<void()> taskToDo = [&]() {
		// Executa a instancia como se fosse a instancia principal
		std::ifstream inputFile(filename);

		assert(inputFile.is_open());

		std::cout << "\t[file]" << std::endl;
		std::string line;
		while (std::getline(inputFile, line)) {
			std::cout << "\t\t" << line << std::endl;
		}
		std::cout << "\t[file]" << std::endl;

		// Close the file
		inputFile.close();
	};

	Request rq("ReadFile", { filename }, []() {
			// Fecha a instancia.
			std::cout << "\t[file read in main instance]" << std::endl;
			return;
		}, taskToDo);

	// Gerencia a comunicação entre as instancias	
	CommunicationUtils::coordinateCommunication(rq, [](Request& rq) {
			//Algorimo aleatorio pra alternar a permissão.
			std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
			std::tm* timeInfo = std::localtime(&currentTime);
			int second = timeInfo->tm_sec;

			return second % 2 == 0;
		}, taskToDo);

	// Espera algum digito antes de fechar o terminal, para verificar os resultados.
	std::cin.ignore();

	return 0;
}
