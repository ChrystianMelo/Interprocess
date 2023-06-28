#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <thread>
#include <filesystem>
#include <mutex>
#include <cstdio>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

/**
 * @brief Nome da memória compartilhada utilizada para comunicação entre instâncias.
 */
const std::string_view shared_memory_name = "InstanceManagement11";

/**
 * @brief Nome do mutex utilizado para controlar o acesso à memória compartilhada.
 */
const std::string_view mutex_name = "InstanceMutex11";

/**
 * @brief Tamanho máximo da mensagem a ser armazenada na memória compartilhada.
 */
const int messageSize = 100;

/**
 * @brief Classe SharedData, representando os dados compartilhados entre os processos. Ela contém as estruturas e variáveis necessárias para
 * a comunicação e sincronização entre os processos por meio de memória compartilhada.
 */
class SharedData
{
public:
	/**
	 * @brief Construtor da classe SharedData.
	 */
	SharedData()
		: message_in(false)
	{}

	/**
	 * @brief Mutex para proteger o acesso aos recursos compartilhados.
	 */
	boost::interprocess::interprocess_mutex mutex;

	/**
	 * @brief Condição para aguardar quando a fila estiver vazia.
	 */
	boost::interprocess::interprocess_condition condition;

	/**
	 * @brief Array para armazenar os itens a serem preenchidos.
	 */
	char items[messageSize];

	/**
	 * @brief Flag indicando se há alguma mensagem presente.
	 */
	bool message_in;
};

/**
 * @brief Realiza a comunicação entre as instâncias/processos. Recebe duas funções como parâmetro: mainInstance e secundaryInstance, que serão
 * executadas dependendo se a instância atual estiver executando ou não. A função verifica se outra instância da aplicação
 * já está em execução e tenta bloquear o mutex para controlar o acesso aos recursos compartilhados.
 *
 * @param mainInstance Função a ser executada como instância principal.
 * @param secundaryInstance Função a ser executada como instancia secundária.
 */
void coordinateInstanceCommunication(std::function<void()> mainInstance, std::function<void()> secundaryInstance) {
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
		std::cout << "Outra instância da aplicação já está em execução." << std::endl;

		secundaryInstance();

		*is_mainInstance = true;
	}
	else {
		// O bloqueio do mutex foi bem-sucedido, execute a lógica da sua aplicação aqui
		std::cout << "Aplicação está sendo executada" << std::endl;

		mainInstance();

		// Após concluir o processamento, desbloqueie o mutex e marque o objeto de memória compartilhada
		mutex.unlock();
		*is_mainInstance = false;
	}

	boost::interprocess::shared_memory_object::remove(shared_memory_name.data());
}

int main()
{
	bool isMainInstanceExecuting = true;
	const char* shared_memory_name = "InstanceCommunication";

	// Gerencia a comunicação entre as instancias
	coordinateInstanceCommunication([&]() {
		while (isMainInstanceExecuting) {
			// Apaga a memória compartilhada anterior e agenda a remoção na saída
			boost::interprocess::shared_memory_object::remove(shared_memory_name);
			
			// Cria um objeto de memória compartilhada
			boost::interprocess::shared_memory_object object(
				boost::interprocess::create_only,  // somente criação
				shared_memory_name,                  // nome
				boost::interprocess::read_write     // modo leitura-escrita
			);

			try {
				// Define o tamanho
				object.truncate(sizeof(SharedData));

				// Mapeia toda a memória compartilhada neste processo
				boost::interprocess::mapped_region region(
					object,                            // o que mapear
					boost::interprocess::read_write    // mapeia como leitura-escrita
				);

				// Obtém o endereço da região mapeada
				void* addr = region.get_address();

				// Constrói a estrutura compartilhada na memória
				SharedData* data = new (addr) SharedData;

				std::cout << "[waiting connection]" << std::endl;

				// Lê a mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->mutex);
					data->condition.wait(lock);

					// Imprime a mensagem
					std::cout << "[in]" << data->items << std::endl;
					// Notifica o outro processo que o buffer está vazio
					data->message_in = false;
				}


				// Responde a mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->mutex);

					strcpy(data->items, "The model is being open.");

					std::cout << "[out]" << data->items << std::endl;

					// Notifica o outro processo que há uma mensagem
					data->condition.notify_one();

					// Marca o buffer de mensagem como cheio
					data->message_in = true;
				}

				std::cout << "[connection closed]" << std::endl;
			}
			catch (boost::interprocess::interprocess_exception& ex) {
				std::cout << ex.what() << std::endl;
			}
		}
		}, [&]() {
			// Cria um objeto de memória compartilhada
			boost::interprocess::shared_memory_object object(
				boost::interprocess::open_only,       // somente abertura
				shared_memory_name,                     // nome
				boost::interprocess::read_write        // modo leitura-escrita
			);

			try {
				// Mapeia toda a memória compartilhada neste processo
				boost::interprocess::mapped_region region(
					object,                                // o que mapear
					boost::interprocess::read_write     // mapeia como leitura-escrita
				);

				// Obtém o endereço da região mapeada
				void* addr = region.get_address();

				// Obtém um ponteiro para a estrutura compartilhada
				SharedData* data = static_cast<SharedData*>(addr);

				// Escreve uma mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->mutex);

					strcpy(data->items, "read_moedel: something.egomlx");

					std::cout << "[out]" << data->items << std::endl;

					// Notifica o outro processo que há uma mensagem
					data->condition.notify_one();

					// Marca o buffer de mensagem como cheio
					data->message_in = true;
				}

				// Lê a resposta.
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->mutex);
					data->condition.wait(lock);

					// Imprime a mensagem
					std::cout << "[in]" << data->items << std::endl;
					// Notifica o outro processo que o buffer está vazio
					data->message_in = false;
				}
			}
			catch (boost::interprocess::interprocess_exception& ex) {
				std::cout << ex.what() << std::endl;
			}
		});

	// Espera algum digito antes de fechar o terminal, para verificar os resultados.
	std::cin.ignore();

	return 0;
}
