#include <iostream>
#include <cassert>
#include <fstream>
#include <chrono>

#include "CommunicationUtils.h"


void taskToDo(const std::string_view filename) {
	// Executa a instancia como se fosse a instancia principal
	std::ifstream inputFile(filename.data());

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

int main()
{
	std::string filename("filename.txt");

	IntanceCommunication communication = IntanceCommunication();

	communication.setCancelled(false);

	if (communication.lockMainIntance()) {
		//while (isMainInstanceExecuting) {
		try {
			communication.stopInstanceCommunication();

			boost::interprocess::shared_memory_object object(boost::interprocess::create_only,
				communication.getCommunicationMemoryName().data(),
				boost::interprocess::read_write
			);
			object.truncate(sizeof(SharedData));

			boost::interprocess::mapped_region region(object, boost::interprocess::read_write);
			void* addr = region.get_address();

			communication.setSharedData(new (addr) SharedData);

			std::string command = "";

			// Lê a mensagem
			{
				boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(communication.getSharedData()->m_mutex);
				std::cout << "[opening connection]" << std::endl;

				communication.getSharedData()->m_condition.wait(lock);
				if (communication.getCancelled()) return -1;

				// Imprime a mensagem
				std::cout << "\t[in]" << communication.getSharedData()->m_items << std::endl;

				command = communication.getSharedData()->m_items;

				// Notifica o outro processo que o buffer está vazio
				communication.getSharedData()->m_messageIn = false;
			}

			bool isAvailable = false;

			// Algorimo aleatorio pra processar o comando e deternminar a resposta.
			{
				std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
				std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
				std::tm* timeInfo = std::localtime(&currentTime);
				int second = timeInfo->tm_sec;

				isAvailable = second % 2 == 0;
			}

			// Responde a mensagem
			{
				boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(communication.getSharedData()->m_mutex);

				if (isAvailable)
					strcpy(communication.getSharedData()->m_items, "accepted");
				else
					strcpy(communication.getSharedData()->m_items, "denied");

				std::cout << "\t[out]" << communication.getSharedData()->m_items << std::endl;

				communication.getSharedData()->m_condition.notify_one();
				communication.getSharedData()->m_messageIn = true;

				std::cout << "[connection closed]" << std::endl;
			}

			if (isAvailable)
				taskToDo(filename);

		}
		catch (boost::interprocess::interprocess_exception& ex) {
			std::cout << ex.what() << std::endl;
		}

		// Apaga a memória compartilhada anterior e agenda a remoção na saída
		communication.stopInstanceCommunication();
		communication.releaseMainInstance();
		//}
	}
	else {
		try {
			boost::interprocess::shared_memory_object object(boost::interprocess::open_only,
				communication.getCommunicationMemoryName().data(), boost::interprocess::read_write);
			boost::interprocess::mapped_region region(object, boost::interprocess::read_write);
			void* addr = region.get_address();

			communication.setSharedData(static_cast<SharedData*>(addr));

			// Escreve uma mensagem
			{
				boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(communication.getSharedData()->m_mutex);

				std::string command = "ReadFile" + filename;

				strcpy(communication.getSharedData()->m_items, command.c_str());

				std::cout << "[out]" << communication.getSharedData()->m_items << std::endl;

				// Notifica o outro processo que há uma mensagem
				communication.getSharedData()->m_condition.notify_one();

				// Marca o buffer de mensagem como cheio
				communication.getSharedData()->m_messageIn = true;
			}

			// Lê a resposta.
			{
				boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(communication.getSharedData()->m_mutex);

				communication.getSharedData()->m_condition.wait(lock);
				if (communication.getCancelled()) return -1;

				// Imprime a mensagem
				std::cout << "[in]" << communication.getSharedData()->m_items << std::endl;

				// Notifica o outro processo que o buffer está vazio
				communication.getSharedData()->m_messageIn = false;
			}

			// Processa a tarefa na instancia atual se necessario.
			if (std::strcmp(communication.getSharedData()->m_items, "denied") == 0)
				taskToDo(filename);
		}
		catch (boost::interprocess::interprocess_exception& ex) {
			std::cout << ex.what() << std::endl;
		}
	}

	// Espera algum digito antes de fechar o terminal, para verificar os resultados.
	std::cin.ignore();

	return 0;
}
