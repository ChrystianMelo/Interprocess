#include <iostream>

#include <string>
#include <string_view>
#include <chrono>
#include <thread>
#include <filesystem>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#include <mutex>

class Request {
private:
	std::string m_command;
public:
	Request(const std::string& command) : m_command(command) {};
	virtual void processCommand() {};
};

class OpenModelRequest : Request {
private:
	const std::string commandPrefix = "openModel_";
	std::filesystem::path m_path;
public:
	OpenModelRequest(const std::filesystem::path& path) : Request(commandPrefix + path.string()), m_path(path) {};

	void processCommand() override {
		//..
		std::cout << "The model \"" << "\" was open successfully." << std::endl;
	};
};

const std::string_view shared_memory_name = "InstanceManagmment";
const std::string_view mutex_name = "InstanceMutex";

void doInstanceCommunication(std::function<void()> running, std::function<void()> notRunning) {
	// Crie ou abra o objeto de memória compartilhada
	boost::interprocess::shared_memory_object shared_memory(boost::interprocess::open_or_create, shared_memory_name.data(), boost::interprocess::read_write);

	// Defina o tamanho da região de memória compartilhada
	shared_memory.truncate(sizeof(bool));

	// Mapeie a região de memória compartilhada
	boost::interprocess::mapped_region region(shared_memory, boost::interprocess::read_write);

	// Verifique se outra instância da aplicação está em execução e tente bloquear o mutex
	bool* is_running = static_cast<bool*>(region.get_address());
	boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, mutex_name.data());

	if (*is_running || !mutex.try_lock())
	{
		std::cout << "Outra instância da aplicação já está em execução." << std::endl;

		notRunning();

		*is_running = true;

	}
	else {
		// O bloqueio do mutex foi bem-sucedido, execute a lógica da sua aplicação aqui
		std::cout << "Aplicação está sendo executada" << std::endl;

		running();

		// Após concluir o processamento, desbloqueie o mutex e marque o objeto de memória compartilhada
		mutex.unlock();
		*is_running = false;
	}
}

std::string read() {
	std::string msg;
	try
	{
		boost::interprocess::managed_shared_memory shared_memory(boost::interprocess::open_only, shared_memory_name.data());

		boost::interprocess::interprocess_semaphore* semaphore = shared_memory.find<boost::interprocess::interprocess_semaphore>("InstanceSemaphore").first;

		std::string* shared_message = shared_memory.find<std::string>("InstanceMessage").first;

		semaphore->wait();

		std::cout << "Mensagem recebida: " << *shared_message << std::endl;

		return *shared_message;
	}
	catch (const std::exception& ex)
	{
		boost::interprocess::shared_memory_object::remove(shared_memory_name.data());
	}

	return msg;

}

bool write(const std::string_view msg) {

	try
	{
		boost::interprocess::managed_shared_memory shared_memory(boost::interprocess::open_or_create, shared_memory_name.data(), 65536);

		boost::interprocess::interprocess_semaphore* semaphore = shared_memory.find_or_construct<boost::interprocess::interprocess_semaphore>("InstanceSemaphore")(0);

		std::string* shared_message = shared_memory.find_or_construct<std::string>("InstanceMessage")("");

		*shared_message = msg.data();

		semaphore->post();

		return true;
	}
	catch (const std::exception& ex)
	{
		boost::interprocess::shared_memory_object::remove(shared_memory_name.data());
		return false;
	}
}

int main()
{
	std::function<void()> reader = [&]() {
		std::cout << "Aberto para conexões..." << std::endl;
		bool systemAvailable = true;

		std::string msg;
		for (; msg.empty(); msg = read()) std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(1));

		std::string result;
		if (msg.compare("readModel") == 0 && systemAvailable)
			result = "Lendo modelo";
		else
			result = "Comando negado";

		std::cout << result << std::endl;

		while (!write(result));

		std::cout << "Fim" << std::endl;
		std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(10));
	};

	std::function<void()> writer = [&]() {
		std::cout << "Estabelecendo conexão..." << std::endl;

		std::string msg("readModel");
		while (!write(msg));

		std::cout << "Mensagem enviada: \"" << msg << "\"" << std::endl;

		std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(1));

		for (msg = ""; msg.empty(); msg = read()) std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(1));

		std::cout << "Fim" << std::endl;
		std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(10));
	};

	doInstanceCommunication(reader, writer);
	return 0;
}
