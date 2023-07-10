#include <iostream>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#ifndef COMMUNICATION_UTILS_H
#define COMMUNICATION_UTILS_H

/**
 * @brief Tamanho m�ximo da mensagem a ser armazenada na mem�ria compartilhada.
 */
const int MESSAGE_SIZE = 100;

/**
 * @brief Classe SharedData, representando os dados compartilhados entre os processos. Ela cont�m as estruturas e vari�veis necess�rias para
 * a comunica��o e sincroniza��o entre os processos por meio de mem�ria compartilhada.
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
	 * @brief Condi��o para aguardar quando a fila estiver vazia.
	 */
	boost::interprocess::interprocess_condition m_condition;

	/**
	 * @brief Array para armazenar os itens a serem preenchidos.
	 */
	char m_items[MESSAGE_SIZE];

	/**
	 * @brief Flag indicando se h� alguma mensagem presente.
	 */
	bool m_messageIn;

	/**
	 * @brief Flag indicando se a mensagem foi lida.
	 */
	bool m_isConnected;
};

/**
 * @brief Classe que representa uma solicita��o.
 */
class Request {
private:
	/**
	 * @brief Comando da solicita��o.
	 */
	std::string m_command;

	/**
	 * @brief Argumentos da solicita��o.
	 */
	std::vector<std::string> m_args;

	/**
	 * @brief Flag indicando se a solicita��o foi aceita.
	 */
	bool m_accepted;

	/**
	 * @brief Fun��o a ser executada em caso de sucesso.
	 */
	std::function<void()> m_onSuccess;

	/**
	 * @brief Fun��o a ser executada em caso de falha.
	 */
	std::function<void()> m_onFailure;

public:
	/**
	 * @brief Construtor da classe Request.
	 *
	 * @param command Comando da solicita��o.
	 * @param args Argumentos da solicita��o.
	 * @param onSuccess Fun��o a ser executada em caso de sucesso.
	 * @param onFailure Fun��o a ser executada em caso de falha.
	 */
	Request(const std::string_view command, const std::vector<std::string>& args, const std::function<void()> onSuccess,
		const std::function<void()> onFailure);

	/**
	 * @brief Processa a solicita��o.
	 */
	void processRequest();

	/**
	 * @brief Define se a solicita��o foi aceita ou n�o.
	 *
	 * @param accepted Flag indicando se a solicita��o foi aceita.
	 */
	void setAccepted(bool accepted);

	/**
	 * @brief Retorna o comando completo, juntando o comando principal e os argumentos em uma string.
	 *
	 * @return Comando completo da solicita��o.
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
	 * @brief Vari�vel de controle para indicar o cancelamento.
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
	 * @brief Coordena a comunica��o entre inst�ncias de um programa usando mem�ria compartilhada.
	 *
	 * @param rq O objeto de requisi��o a ser processado.
	 * @param processRequest A fun��o que processa a requisi��o e retorna um valor booleano.
	 * @param taskToDo A fun��o a ser executada se a requisi��o for aceita.
	 * @param shared_memory_name O nome da mem�ria compartilhada usada para comunica��o.
	 */
	void coordinateCommunication(Request& rq, const std::function<bool(Request& rq)> processRequest, const std::function<void()> taskToDo);

	/**
	 * @brief Realiza a comunica��o entre as inst�ncias/processos. Recebe duas fun��es como par�metro: mainInstance e secundaryInstance, que ser�o
	 * executadas dependendo se a inst�ncia atual estiver executando ou n�o. A fun��o verifica se outra inst�ncia da aplica��o
	 * j� est� em execu��o e tenta bloquear o mutex para controlar o acesso aos recursos compartilhados.
	 *
	 * @param mainInstance Fun��o a ser executada como inst�ncia principal.
	 * @param secundaryInstance Fun��o a ser executada como instancia secund�ria.
	 * @param secundaryInstance Nome da mem�ria compartilhada utilizada para comunica��o entre inst�ncias.
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