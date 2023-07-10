#include <iostream>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#ifndef COMMUNICATION_UTILS_H
#define COMMUNICATION_UTILS_H

/**
 * @brief Tamanho máximo da mensagem a ser armazenada na memória compartilhada.
 */
const int MESSAGE_SIZE = 100;

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
	SharedData();

	/**
	 * @brief Mutex para proteger o acesso aos recursos compartilhados.
	 */
	boost::interprocess::interprocess_mutex m_mutex;

	/**
	 * @brief Condição para aguardar quando a fila estiver vazia.
	 */
	boost::interprocess::interprocess_condition m_condition;

	/**
	 * @brief Array para armazenar os itens a serem preenchidos.
	 */
	char m_items[MESSAGE_SIZE];

	/**
	 * @brief Flag indicando se há alguma mensagem presente.
	 */
	bool m_messageIn;

	/**
	 * @brief Flag indicando se a mensagem foi lida.
	 */
	bool m_isConnected;
};

/**
 * @brief Classe que representa uma solicitação.
 */
class Request {
private:
	/**
	 * @brief Comando da solicitação.
	 */
	std::string m_command;

	/**
	 * @brief Argumentos da solicitação.
	 */
	std::vector<std::string> m_args;

	/**
	 * @brief Flag indicando se a solicitação foi aceita.
	 */
	bool m_accepted;

	/**
	 * @brief Função a ser executada em caso de sucesso.
	 */
	std::function<void()> m_onSuccess;

	/**
	 * @brief Função a ser executada em caso de falha.
	 */
	std::function<void()> m_onFailure;

public:
	/**
	 * @brief Construtor da classe Request.
	 *
	 * @param command Comando da solicitação.
	 * @param args Argumentos da solicitação.
	 * @param onSuccess Função a ser executada em caso de sucesso.
	 * @param onFailure Função a ser executada em caso de falha.
	 */
	Request(const std::string_view command, const std::vector<std::string>& args, const std::function<void()> onSuccess,
		const std::function<void()> onFailure);

	/**
	 * @brief Processa a solicitação.
	 */
	void processRequest();

	/**
	 * @brief Define se a solicitação foi aceita ou não.
	 *
	 * @param accepted Flag indicando se a solicitação foi aceita.
	 */
	void setAccepted(bool accepted);

	/**
	 * @brief Retorna o comando completo, juntando o comando principal e os argumentos em uma string.
	 *
	 * @return Comando completo da solicitação.
	 */
	std::string getFullCommand();
};

/**
 * @brief
 */
class IntanceCommunication {
private:
	/**
	 * @brief
	 */
	SharedData* m_communicationData;

	/**
	 * @brief Variável de controle para indicar o cancelamento.
	 */
	bool m_cancelled;

	/**
	 * @brief
	 */
	boost::interprocess::named_mutex m_mutex = boost::interprocess::named_mutex(boost::interprocess::open_or_create, m_mutexName.data());

	/**
	 * @brief
	 */
	const std::string_view m_communicationMemoryName;

	/**
	 * @brief
	 */
	const std::string_view m_mutexName;

public:

	/**
	 * @brief
	 */
	IntanceCommunication(const std::string_view mutexName = "InstanceMutex2", const std::string_view communicationMemoryName = "InstanceCommunication2");

	/**
	 * @brief
	 */
	void setCancelled(bool status) { m_cancelled = status; }

	/**
	 * @brief
	 */
	bool getCancelled() { return m_cancelled; }

	/**
	 * @brief
	 */
	void setSharedData(SharedData* communicationData) { m_communicationData = communicationData; }

	/**
	 * @brief
	 */
	SharedData* getSharedData() { return m_communicationData; }

	/**
	 * @brief
	 */
	const std::string_view getCommunicationMemoryName() {
		return m_communicationMemoryName;
	}

	/**
	 * @brief
	 */
	bool stopInstanceCommunication();

	/**
	 * @brief Coordena a comunicação entre instâncias de um programa usando memória compartilhada.
	 *
	 * @param rq O objeto de requisição a ser processado.
	 * @param processRequest A função que processa a requisição e retorna um valor booleano.
	 * @param taskToDo A função a ser executada se a requisição for aceita.
	 * @param shared_memory_name O nome da memória compartilhada usada para comunicação.
	 */
	void coordinateCommunication(Request& rq, const std::function<bool(Request& rq)> processRequest, const std::function<void()> taskToDo);

	/**
	 * @brief Realiza a comunicação entre as instâncias/processos. Recebe duas funções como parâmetro: mainInstance e secundaryInstance, que serão
	 * executadas dependendo se a instância atual estiver executando ou não. A função verifica se outra instância da aplicação
	 * já está em execução e tenta bloquear o mutex para controlar o acesso aos recursos compartilhados.
	 *
	 * @param mainInstance Função a ser executada como instância principal.
	 * @param secundaryInstance Função a ser executada como instancia secundária.
	 * @param secundaryInstance Nome da memória compartilhada utilizada para comunicação entre instâncias.
	 */
	void identifyInstances(const std::function<void()> mainInstance, const std::function<void()> secundaryInstance);

	/**
	 * @brief
	 */
	bool lockMainIntance();

	/**
	 * @brief
	 */
	void releaseMainInstance();
};

#endif