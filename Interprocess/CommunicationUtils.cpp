#include "CommunicationUtils.h"

SharedData::SharedData()
	: message_in(false)
{}

void CommunicationUtils::coordinateInstanceCommunication(const std::function<void()> mainInstance, const std::function<void()> secundaryInstance, const std::string_view shared_memory_name, const std::string_view mutex_name) {
	// Cria ou abre o objeto de memória compartilhada
	boost::interprocess::shared_memory_object shared_memory(boost::interprocess::open_or_create, shared_memory_name.data(), boost::interprocess::read_write);

	// Define o tamanho da região de memória compartilhada
	shared_memory.truncate(sizeof(bool));

	// Mapeia a região de memória compartilhada
	boost::interprocess::mapped_region region(shared_memory, boost::interprocess::read_write);

	// Verifica se outra instância da aplicação está em execução e tenta bloquear o mutex
	bool* is_mainInstance = static_cast<bool*>(region.get_address());
	boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, mutex_name.data());

	if (*is_mainInstance || !mutex.try_lock())
	{
		std::cout << "Secundary instance" << std::endl;

		secundaryInstance();

		*is_mainInstance = true;
	}
	else {
		std::cout << "Main instance." << std::endl;

		mainInstance();

		mutex.unlock();
		*is_mainInstance = false;
	}

	std::cout << "Finish." << std::endl;

	boost::interprocess::shared_memory_object::remove(shared_memory_name.data());
}