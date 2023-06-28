#include <iostream>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#ifndef COMMUNICATION_UTILS_H
#define COMMUNICATION_UTILS_H

/**
 * @brief Tamanho m�ximo da mensagem a ser armazenada na mem�ria compartilhada.
 */
const int messageSize = 100;

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
	boost::interprocess::interprocess_mutex mutex;

	/**
	 * @brief Condi��o para aguardar quando a fila estiver vazia.
	 */
	boost::interprocess::interprocess_condition condition;

	/**
	 * @brief Array para armazenar os itens a serem preenchidos.
	 */
	char items[messageSize];

	/**
	 * @brief Flag indicando se h� alguma mensagem presente.
	 */
	bool message_in;
};

class CommunicationUtils {
public:

	/**
	 * @brief Realiza a comunica��o entre as inst�ncias/processos. Recebe duas fun��es como par�metro: mainInstance e secundaryInstance, que ser�o
	 * executadas dependendo se a inst�ncia atual estiver executando ou n�o. A fun��o verifica se outra inst�ncia da aplica��o
	 * j� est� em execu��o e tenta bloquear o mutex para controlar o acesso aos recursos compartilhados.
	 *
	 * @param mainInstance Fun��o a ser executada como inst�ncia principal.
	 * @param secundaryInstance Fun��o a ser executada como instancia secund�ria.
	 * @param secundaryInstance Nome da mem�ria compartilhada utilizada para comunica��o entre inst�ncias.
	 * @param shared_memory_name Nome do mutex utilizado para controlar o acesso � mem�ria compartilhada.
	 */
	static void coordinateInstanceCommunication(const std::function<void()> mainInstance, const std::function<void()> secundaryInstance, const std::string_view shared_memory_name = "InstanceManagement", const std::string_view mutex_name = "InstanceMutex");

};

#endif