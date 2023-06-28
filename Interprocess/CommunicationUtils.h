#include <iostream>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#ifndef COMMUNICATION_UTILS_H
#define COMMUNICATION_UTILS_H

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
	SharedData();

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
	static void coordinateInstanceCommunication(const std::function<void()> mainInstance, const std::function<void()> secundaryInstance, const std::string_view shared_memory_name = "InstanceManagement", const std::string_view mutex_name = "InstanceMutex");

};

#endif