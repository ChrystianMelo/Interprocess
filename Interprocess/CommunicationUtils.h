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

class CommunicationUtils {
public:

	/**
	 * @brief Realiza a comunicação entre as instâncias/processos. Recebe duas funções como parâmetro: mainInstance e secundaryInstance, que serão
	 * executadas dependendo se a instância atual estiver executando ou não. A função verifica se outra instância da aplicação
	 * já está em execução e tenta bloquear o mutex para controlar o acesso aos recursos compartilhados.
	 *
	 * @param mainInstance Função a ser executada como instância principal.
	 * @param secundaryInstance Função a ser executada como instancia secundária.
	 * @param secundaryInstance Nome da memória compartilhada utilizada para comunicação entre instâncias.
	 * @param shared_memory_name Nome do mutex utilizado para controlar o acesso à memória compartilhada.
	 */
	static void identifyInstances(const std::function<void()> mainInstance, const std::function<void()> secundaryInstance, const std::string_view shared_memory_name = "InstanceManagement", const std::string_view mutex_name = "InstanceMutex");
	
	/**
	 * @brief Coordena a comunicação entre instâncias de um programa usando memória compartilhada.
	 *
	 * @param rq O objeto de requisição a ser processado.
	 * @param processRequest A função que processa a requisição e retorna um valor booleano.
	 * @param taskToDo A função a ser executada se a requisição for aceita.
	 * @param shared_memory_name O nome da memória compartilhada usada para comunicação.
	 */
	static void coordinateCommunication(Request& rq, const std::function<bool(Request& rq)> processRequest, const std::function<void()> taskToDo, const std::string_view shared_memory_name = "InstanceCommunication");
};

#endif