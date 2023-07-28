﻿#ifndef INSTANCE_COMMUNICATION_H
#define INSTANCE_COMMUNICATION_H

#include <filesystem>
#include <iostream>
#include <array>

#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

namespace fs = std::filesystem;

/**
 * @brief Tamanho máximo da mensagem a ser armazenada na memória compartilhada.
 */
const int MESSAGE_SIZE = 2048;

/**
 * @brief Classe que representa uma solicitação de leitura de algumn arquivo.
 */
class ReadFileRequest {
private:
    /**
     * @brief Mensagem sendo compartilhada entre as instancias.
     */
    std::array<char, MESSAGE_SIZE> m_message = {};

public:
    /**
     * @brief Construtor da classe.
     */
    ReadFileRequest();

    /**
     * @brief Construtor da classe.
     */
    ReadFileRequest(const std::string& message);

    /**
     * @brief Retorna a mensagem sendo compartilhada entre as instancias.
     */
    const std::array<char, MESSAGE_SIZE>& getMessage() const;

    /**
     * @brief Sobrecarga do operador igual.
     */
    bool operator==(const ReadFileRequest& other) const;
};

/**
 * @brief Representa os dados compartilhados entre os processos. Ela contém as estruturas e
 * variáveis necessárias para a comunicação e sincronização entre os processos por meio de memória compartilhada.
 */
struct CommunicationData {
    /**
     * @brief Mutex para proteger o acesso aos recursos compartilhados.
     */
    boost::interprocess::interprocess_mutex m_mutex;

    /**
     * @brief Condição para aguardar quando a fila estiver vazia.
     */
    boost::interprocess::interprocess_condition m_condition;

    /**
     * @brief Mensagem sendo compartilhada entre as instancias.
     */
    std::array<char, MESSAGE_SIZE> m_message = {};

    /**
     * @brief Indica se a instância principal (servidor) está disponível.
     */
    bool m_isServerAvailable = true;

    /**
     * @brief Flag para indicar abortar a conexão.
     */
    bool m_abortConnection = false;
};

/**
 * @brief Classe responsável pela comunicação entre instâncias, usando memória compartilhada e mutexes para garantir a
 * consistência dos dados.
 *
 * @details Comportamento:
 *  -> A classe permite criar uma única instância principal (servidor) que atua como o servidor do sistema.
 *  -> A instância secundária (cliente) é criada e envia uma tarefa para a instância principal (servidor).
 *  -> A classe oferece métodos para verificar a disponibilidade do servidor.
 *
 * @note Essa classe representa um sistema com uma única instância principal (servidor) e permite a comunicação com
 * instâncias secundárias (clientes), de forma sequencial.
 */
class InstanceCommunication {
private:
    /**
     * @brief Dados de comunicação compartilhados entre as instâncias.
     */
    CommunicationData* m_data;

    /**
     * @brief Mutex para controle de acesso à comunicação.
     */
    boost::interprocess::named_mutex m_mutex;

    /**
     * @brief Nome da memória compartilhada de comunicação.
     */
    const char* m_sharedMemoryName;

    /**
     * @brief Representação da tarefa que está sendo passada para o servidor.
     */
    ReadFileRequest m_request;

    /**
     * @brief Indica se a instância principal (servidor) está disponível.
     */
    bool m_isServerAvailable;

    /**
     * @brief Indica se esta instância é a instância principal (servidor).
     */
    bool m_isServer;

public:
    /**
     * @brief Construtor da classe.
     *
     * @param mutexName Nome do mutex para controle de acesso à comunicação.
     * @param communicationMemoryName Nome da memória compartilhada de comunicação.
     */
    InstanceCommunication(const std::string_view mutexName = "InstanceMutex",
        const std::string_view communicationMemoryName = "InstanceCommunication");

    /**
     * @brief Destrutor da classe.
     */
    ~InstanceCommunication();

    /**
     * @brief Verifica se a instância atual está sendo executada como a instância principal (servidor).
     *
     * @note Apenas recalcula o valor da flag quando o valor anterior é falso.
     */
    bool isRunningAsServer();

    /**
     * @brief
     */
    ReadFileRequest getRequest();

    /**
     * @brief Define a se a execução deve ser cancelada.
     *
     * @param status Valor 
     */
    void setAbortConnection(bool status);

    /**
     * @brief Define a disponibilidade da instância principal (servidor).
     *
     * @param status Valor da disponibilidade
     */
    void setServerAvailability(bool status);

    /**
     * @brief Retorna a disponibilidade da instância principal (servidor) de executar alguma tarefa para a instância
     * secundária (cliente).
     */
    bool getServerAvailability();

    /**
     * @brief Executa a instância principal (servidor).
     *
     * @return true se a execução foi bem-sucedida, false caso contrário.
     */
    bool runAsServer();

    /**
     * @brief Executa a instância secundária (cliente).
     *
     * @param filename Nome do arquivo de comunicação.
     *
     * @return true se a execução foi bem-sucedida, false caso contrário.
     */
    bool runAsClient(ReadFileRequest request);

    /**
     * @brief Destroi a conexão entre as instancias.
     *
     * @param forceRemoteDisconnections Força o encerramento das conexões de qualquer instância que use a mesma memoria
     * compartilhada.
     */
    void destroyConnection(bool forceRemoteDisconnections = false);
};

#endif