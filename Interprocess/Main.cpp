#include <iostream>

#include "CommunicationUtils.h"

int main()
{
	bool isMainInstanceExecuting = true;
	const char* shared_memory_name = "InstanceCommunication";

	// Gerencia a comunicação entre as instancias
	CommunicationUtils::coordinateInstanceCommunication([&]() {
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

				std::cout << "\t[waiting connection]" << std::endl;

				// Lê a mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->mutex);
					data->condition.wait(lock);

					// Imprime a mensagem
					std::cout << "\t\t[in]" << data->items << std::endl;
					// Notifica o outro processo que o buffer está vazio
					data->message_in = false;
				}


				// Responde a mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->mutex);

					strcpy(data->items, "The file is being open.");

					std::cout << "\t\t[out]" << data->items << std::endl;

					// Notifica o outro processo que há uma mensagem
					data->condition.notify_one();

					// Marca o buffer de mensagem como cheio
					data->message_in = true;
				}

				std::cout << "\t[connection closed]" << std::endl;
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

					strcpy(data->items, "read: something.txt");

					std::cout << "\t[out]" << data->items << std::endl;

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
					std::cout << "\t[in]" << data->items << std::endl;
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
