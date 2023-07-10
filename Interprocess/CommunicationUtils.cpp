#include "CommunicationUtils.h"
#include <chrono>

SharedData::SharedData()
	: m_messageIn(false)
	, m_isConnected(false)
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
 * @brief Define se a solicitação foi aceita ou não.
 * @param accepted Flag indicando se a solicitação foi aceita.
 */
void Request::setAccepted(bool accepted) { m_accepted = accepted; }

/**
 * @brief Retorna o comando completo, juntando o comando principal e os argumentos em uma string.
 * @return Comando completo da solicitação.
 */
std::string Request::getFullCommand()
{
	std::string fullCommand = m_command;
	for (const std::string& arg : m_args) {
		fullCommand += " " + arg;
	}
	return fullCommand;
}

IntanceCommunication::IntanceCommunication(const std::string_view mutexName, const std::string_view communicationMemoryName)
	: m_communicationData(nullptr)
	, m_cancelled(false)
	, m_mutex(boost::interprocess::open_or_create, mutexName.data())
	, m_mutexName(mutexName)
	, m_communicationMemoryName(communicationMemoryName)
{
}

bool IntanceCommunication::stopInstanceCommunication()
{
	boost::interprocess::shared_memory_object::remove(m_communicationMemoryName.data());
	return true;
}

void IntanceCommunication::coordinateCommunication(Request& rq, const std::function<bool(Request& rq)> processRequest, const std::function<void()> taskToDo)
{
	m_cancelled = false;

	// Gerencia a comunicação entre as instancias
	IntanceCommunication::identifyInstances(
		[&]() {
			//while (isMainInstanceExecuting) {
				// Apaga a memória compartilhada anterior e agenda a remoção na saída
			boost::interprocess::shared_memory_object::remove(m_communicationMemoryName.data());

			// Cria um objeto de memória compartilhada
			boost::interprocess::shared_memory_object object(boost::interprocess::create_only, // somente criação
				m_communicationMemoryName.data(),                                                     // nome
				boost::interprocess::read_write // modo leitura-escrita
			);

			try {
				// Define o tamanho
				object.truncate(sizeof(SharedData));

				// Mapeia toda a memória compartilhada neste processo
				boost::interprocess::mapped_region region(object, // o que mapear
					boost::interprocess::read_write               // mapeia como leitura-escrita
				);

				// Obtém o endereço da região mapeada
				void* addr = region.get_address();

				// Constrói a estrutura compartilhada na memória
				m_communicationData = new (addr) SharedData;

				std::cout << "[waiting connection]" << std::endl;

				std::string command = "";

				// Lê a mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_communicationData->m_mutex);

					m_communicationData->m_condition.wait(lock);
					if (m_cancelled) return;

					// Imprime a mensagem
					std::cout << "\t[in]" << m_communicationData->m_items << std::endl;

					command = m_communicationData->m_items;

					// Notifica o outro processo que o buffer está vazio
					m_communicationData->m_messageIn = false;
				}

				// Processa a mensagem 
				bool isAvailable = processRequest(rq);

				// Responde a mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_communicationData->m_mutex);

					if (isAvailable)
						strcpy(m_communicationData->m_items, "accepted");
					else
						strcpy(m_communicationData->m_items, "denied");

					std::cout << "\t[out]" << m_communicationData->m_items << std::endl;

					if (isAvailable)
						taskToDo();

					// Notifica o outro processo que há uma mensagem
					m_communicationData->m_condition.notify_one();

					// Marca o buffer de mensagem como cheio
					m_communicationData->m_messageIn = true;
				}

				std::cout << "[connection closed]" << std::endl;
			}
			catch (boost::interprocess::interprocess_exception& ex) {
				std::cout << ex.what() << std::endl;
			}
			//}
		},
		[&]() {
			// Cria um objeto de memória compartilhada
			boost::interprocess::shared_memory_object object(boost::interprocess::open_only, // somente abertura
				m_communicationMemoryName.data(),                                                   // nome
				boost::interprocess::read_write                                              // modo leitura-escrita
			);

			try {
				// Mapeia toda a memória compartilhada neste processo
				boost::interprocess::mapped_region region(object, // o que mapear
					boost::interprocess::read_write               // mapeia como leitura-escrita
				);

				// Obtém o endereço da região mapeada
				void* addr = region.get_address();

				// Obtém um ponteiro para a estrutura compartilhada
				m_communicationData = static_cast<SharedData*>(addr);

				// Escreve uma mensagem
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_communicationData->m_mutex);

					strcpy(m_communicationData->m_items, rq.getFullCommand().c_str());

					std::cout << "[out]" << m_communicationData->m_items << std::endl;

					// Notifica o outro processo que há uma mensagem
					m_communicationData->m_condition.notify_one();

					// Marca o buffer de mensagem como cheio
					m_communicationData->m_messageIn = true;
				}

				// Lê a resposta.
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_communicationData->m_mutex);

					m_communicationData->m_condition.wait(lock);
					if (m_cancelled) return;

					// Imprime a mensagem
					std::cout << "[in]" << m_communicationData->m_items << std::endl;

					// Notifica o outro processo que o buffer está vazio
					m_communicationData->m_messageIn = false;

					// Define o status da solicitação.
					rq.setAccepted(std::strcmp(m_communicationData->m_items, "accepted") == 0);
				}
			}
			catch (boost::interprocess::interprocess_exception& ex) {
				std::cout << ex.what() << std::endl;

				rq.setAccepted(false);
			}

			// Processa a solicitação.
			rq.processRequest();
		});
}

void IntanceCommunication::identifyInstances(const std::function<void()> mainInstance, const std::function<void()> secundaryInstance) {
	if (m_mutex.try_lock()) {
		std::cout << "Main instance." << std::endl;

		mainInstance();

		m_mutex.unlock();
	}
	else {
		std::cout << "Secondary instance" << std::endl;

		secundaryInstance();
	}

	std::cout << "Finish." << std::endl;
}

bool IntanceCommunication::lockMainIntance(){
	return m_mutex.try_lock();
}

void IntanceCommunication::releaseMainInstance() {
	m_mutex.unlock();
}
