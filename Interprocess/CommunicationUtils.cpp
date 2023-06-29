#include "CommunicationUtils.h"

SharedData::SharedData()
	: m_messageIn(false)
{}

Request::Request(const std::string_view command, const std::vector<std::string>& args,
	const std::function<void()> onSuccess, const std::function<void()> onFailure)
	: m_command(command.data())
	, m_args(args)
	, m_onSuccess(onSuccess)
	, m_onFailure(onFailure)
	, m_accepted(false)
{
}

void Request::processRequest() {
	if (m_accepted)
		m_onSuccess();
	else
		m_onFailure();
}

/**
 * @brief Define se a solicita��o foi aceita ou n�o.
 * @param accepted Flag indicando se a solicita��o foi aceita.
 */
void Request::setAccepted(bool accepted) { m_accepted = accepted; }

/**
 * @brief Retorna o comando completo, juntando o comando principal e os argumentos em uma string.
 * @return Comando completo da solicita��o.
 */
std::string Request::getFullCommand()
{
	std::string fullCommand = m_command;
	for (const std::string& arg : m_args) {
		fullCommand += " " + arg;
	}
	return fullCommand;
}
void CommunicationUtils::identifyInstances(const std::function<void()> mainInstance, const std::function<void()> secundaryInstance, const std::string_view shared_memory_name, const std::string_view mutex_name) {
	// Cria ou abre o objeto de mem�ria compartilhada
	boost::interprocess::shared_memory_object shared_memory(boost::interprocess::open_or_create, shared_memory_name.data(), boost::interprocess::read_write);

	// Define o tamanho da regi�o de mem�ria compartilhada
	shared_memory.truncate(sizeof(bool));

	// Mapeia a regi�o de mem�ria compartilhada
	boost::interprocess::mapped_region region(shared_memory, boost::interprocess::read_write);

	// Verifica se outra inst�ncia da aplica��o est� em execu��o e tenta bloquear o mutex
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

void CommunicationUtils::coordinateCommunication(Request& rq, const std::function<bool(Request& rq)> processRequest, const std::function<void()> taskToDo, const std::string_view shared_memory_name)
{
	bool isMainInstanceExecuting = true;

	// Gerencia a comunica��o entre as instancias
	CommunicationUtils::identifyInstances(
		[&]() {
			//while (isMainInstanceExecuting) {
				// Apaga a mem�ria compartilhada anterior e agenda a remo��o na sa�da
				boost::interprocess::shared_memory_object::remove(shared_memory_name.data());

				// Cria um objeto de mem�ria compartilhada
				boost::interprocess::shared_memory_object object(boost::interprocess::create_only, // somente cria��o
					shared_memory_name.data(),                                                     // nome
					boost::interprocess::read_write // modo leitura-escrita
				);

				try {
					// Define o tamanho
					object.truncate(sizeof(SharedData));

					// Mapeia toda a mem�ria compartilhada neste processo
					boost::interprocess::mapped_region region(object, // o que mapear
						boost::interprocess::read_write               // mapeia como leitura-escrita
					);

					// Obt�m o endere�o da regi�o mapeada
					void* addr = region.get_address();

					// Constr�i a estrutura compartilhada na mem�ria
					SharedData* data = new (addr) SharedData;

					std::cout << "\t[waiting connection]" << std::endl;

					std::string command = "";

					// L� a mensagem
					{
						boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->m_mutex);
						data->m_condition.wait(lock);

						// Imprime a mensagem
						std::cout << "\t\t[in]" << data->m_items << std::endl;

						command = data->m_items;

						// Notifica o outro processo que o buffer est� vazio
						data->m_messageIn = false;
					}

					// Processa a mensagem 
					bool isAvailable = processRequest(rq);

					// Responde a mensagem
					{
						boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->m_mutex);

						if (isAvailable)
							strcpy(data->m_items, "accepted");
						else
							strcpy(data->m_items, "denied");

						std::cout << "\t\t[out]" << data->m_items << std::endl;

						if (isAvailable)
							taskToDo();

						// Notifica o outro processo que h� uma mensagem
						data->m_condition.notify_one();

						// Marca o buffer de mensagem como cheio
						data->m_messageIn = true;
					}

					std::cout << "\t[connection closed]" << std::endl;
				}
				catch (boost::interprocess::interprocess_exception& ex) {
					std::cout << ex.what() << std::endl;
				}
			//}
		},
		[&]() {
			// Cria um objeto de mem�ria compartilhada
			boost::interprocess::shared_memory_object object(boost::interprocess::open_only, // somente abertura
				shared_memory_name.data(),                                                   // nome
				boost::interprocess::read_write                                              // modo leitura-escrita
			);

			try {
				// Mapeia toda a mem�ria compartilhada neste processo
				boost::interprocess::mapped_region region(object, // o que mapear
					boost::interprocess::read_write               // mapeia como leitura-escrita
				);

				// Obt�m o endere�o da regi�o mapeada
				void* addr = region.get_address();

				// Obt�m um ponteiro para a estrutura compartilhada
				SharedData* data = static_cast<SharedData*>(addr);

				// Escreve uma mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->m_mutex);

					strcpy(data->m_items, rq.getFullCommand().c_str());

					std::cout << "\t[out]" << data->m_items << std::endl;

					// Notifica o outro processo que h� uma mensagem
					data->m_condition.notify_one();

					// Marca o buffer de mensagem como cheio
					data->m_messageIn = true;
				}

				// L� a resposta.
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->m_mutex);
					data->m_condition.wait(lock);

					// Imprime a mensagem
					std::cout << "\t[in]" << data->m_items << std::endl;

					// Notifica o outro processo que o buffer est� vazio
					data->m_messageIn = false;

					// Define o status da solicita��o.
					rq.setAccepted(std::strcmp(data->m_items, "accepted") == 0);
				}
			}
			catch (boost::interprocess::interprocess_exception& ex) {
				std::cout << ex.what() << std::endl;

				rq.setAccepted(false);
			}

			// Processa a solicita��o.
			rq.processRequest();
		});
}