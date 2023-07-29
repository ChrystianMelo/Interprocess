#include "InstanceCommunication.h"

#include <boost/chrono.hpp>
namespace {

	/**
	 * @brief Function to convert enum class Color to string
	 */
	const std::string toString(const RequestOperation& operation) {
		switch (operation) {
		case RequestOperation::READ_FILE:
			return "ReadFile";
		default:
			return "None";
		}
	}
}
Request::Request()
	: m_message{}, m_operation(RequestOperation::NONE) { }

Request::Request(const std::string_view message, const RequestOperation& operation) : m_operation(operation)
{
	if (message.size() > MESSAGE_SIZE)
		throw(std::exception("The message is too large to be processed as a request for instance communication."));

	m_message = {};
	std::copy(message.begin(), message.end(), m_message.begin());
}

const std::array<char, MESSAGE_SIZE>& Request::getMessage() const { return m_message; }

bool Request::operator==(const Request& other) const { return m_message == other.m_message; }

std::ostream& operator<<(std::ostream& os, const Request& obj) {
	os << "Type: " << toString(obj.m_operation) << ", Message: " << obj.m_message.data();
	return os;
}

ReadFileRequest::ReadFileRequest(fs::path filename)
	: Request(filename.string(), RequestOperation::READ_FILE),
	m_filename(filename)
{
}

const fs::path& ReadFileRequest::getFilename() const { return m_filename; }

InstanceCommunication::InstanceCommunication(
	const std::string_view mutexName, const std::string_view communicationMemoryName)
	: m_data(nullptr)
	, m_mutex(boost::interprocess::open_or_create, mutexName.data())
	, m_sharedMemoryName(communicationMemoryName.data())
	, m_request()
	, m_isServerAvailable(true)
	, m_isServer(false)
{
}

InstanceCommunication ::~InstanceCommunication() { }

void InstanceCommunication::setAbortConnection(bool status)
{
	if (m_data == nullptr)
		throw(std::exception("The memory has not been completely removed."));

	boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_data->m_mutex);
	m_data->m_abortConnection = status;
}
void InstanceCommunication::setServerAvailability(bool status)
{
	if (m_data != nullptr) {
		boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_data->m_mutex);
		m_data->m_isServerAvailable = status;
	}

	m_isServerAvailable = status;
}

bool InstanceCommunication::getServerAvailability() { return m_isServerAvailable; }

bool InstanceCommunication::isRunningAsServer()
{
	if (!m_isServer && m_mutex.try_lock()) {
		m_mutex.unlock();
		m_isServer = true;
	}

	return m_isServer;
}

const Request& InstanceCommunication::getRequest() const { return m_request; }

bool InstanceCommunication::runAsServer()
{
	if (m_data != nullptr)
		throw(std::exception("The memory has not been completely removed."));

	if (m_mutex.try_lock()) {
		try {
			boost::interprocess::shared_memory_object object(
				boost::interprocess::create_only, m_sharedMemoryName, boost::interprocess::read_write);
			object.truncate(sizeof(CommunicationData));

			boost::interprocess::mapped_region region(object, boost::interprocess::read_write);
			void* addr = region.get_address();

			m_data = new (addr) CommunicationData;
			setServerAvailability(m_isServerAvailable);

			// Lê a mensagem
			{
				boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_data->m_mutex);
				std::cout << "Connection initialized as primary instance." << std::endl;

				// Trava uma vez para estabelecer conexão.
				{
					m_data->m_condition.wait(lock);
					if (m_data->m_abortConnection) {
						std::cout << "Connection failed: connection was terminated remotely." << std::endl;

						destroyConnection();

						std::cout << "Connection closed." << std::endl;

						return false;
					}
				}

				// Trava uma vez para fazer a comunicação.
				{
					m_data->m_condition.wait(lock);
					if (m_data->m_abortConnection) {
						std::cout << "Connection lost: unable to establish a connection." << std::endl;

						destroyConnection();

						std::cout << "Connection closed." << std::endl;

						return false;
					}

					m_request = m_data->m_request;

					std::cout << "Message received from secundary instance:" << m_request << std::endl;
				}
			}

			// Responde a mensagem
			{
				boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_data->m_mutex);

				bool isAvailable = m_data->m_isServerAvailable;

				std::cout << "Sending result to secundary instance:" << std::boolalpha << isAvailable << std::endl;

				m_data->m_condition.notify_one();

				// Aguarda um tempo para o cliente processaar a resposta e então a execução é finalizada.
				// (m_isClientFinished)
				m_data->m_condition.wait_for(lock, boost::chrono::milliseconds(500));

				std::cout << "Connection closed." << std::endl;

				return isAvailable;
			}

		}
		catch (boost::interprocess::interprocess_exception& ex) {
			throw(std::exception(ex.what()));
		}
	}
	else
		throw(std::exception("Failed to acquire lock. Another process may already have exclusive access."));
}

bool InstanceCommunication::runAsClient(const Request& request)
{
	try {
		boost::interprocess::shared_memory_object object(
			boost::interprocess::open_only, m_sharedMemoryName, boost::interprocess::read_write);
		boost::interprocess::mapped_region region(object, boost::interprocess::read_write);

		m_data = static_cast<CommunicationData*>(region.get_address());

		// Escreve uma mensagem
		{
			std::cout << "Connection initialized as secundary instance." << std::endl;

			boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_data->m_mutex);

			// Verifica se a conexão foi estabelecida com sucesso.
			{
				m_data->m_condition.notify_one();

				m_data->m_condition.wait_for(lock, boost::chrono::milliseconds(500));
				if (m_data->m_abortConnection) {
					std::cout << "Connection failed: unable to establish a connection." << std::endl;

					destroyConnection();

					std::cout << "Connection closed." << std::endl;

					return false;
				}

				std::cout << "Connection established successfully." << std::endl;
			}

			// Realiza a comunicação.
			{
				m_data->m_request = request;

				std::cout << "Sending request to primary instance:" << request << std::endl;

				m_data->m_condition.notify_one();
			}
		}

		// Lê a resposta.
		{
			boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_data->m_mutex);

			m_data->m_condition.wait(lock);
			if (m_data->m_abortConnection) {
				std::cout << "Connection failed: connection was terminated remotely." << std::endl;

				destroyConnection();

				std::cout << "Connection closed." << std::endl;
				return false;
			}

			bool response = m_data->m_isServerAvailable;

			std::cout << "Request response received from main instance:" << std::boolalpha << response << std::endl;

			std::cout << "Connection closed." << std::endl;

			return response;
		}

	}
	catch (std::exception& ex) {
		throw(std::exception(ex.what()));
	}
}

void InstanceCommunication::destroyConnection(bool forceRemoteDisconnections)
{
	if (forceRemoteDisconnections) {
		try {
			boost::interprocess::shared_memory_object object;
			boost::interprocess::mapped_region region;
			CommunicationData* data = nullptr;
			try {
				object = boost::interprocess::shared_memory_object(
					boost::interprocess::open_only, m_sharedMemoryName, boost::interprocess::read_write);
				region = boost::interprocess::mapped_region(object, boost::interprocess::read_write);

				data = static_cast<CommunicationData*>(region.get_address());
			}
			catch (...) {
				// A memoria compartilhada não foi criada.
			}

			if (data != nullptr) {
				data->m_mutex.unlock();

				boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(data->m_mutex);

				data->m_abortConnection = true;

				data->m_condition.notify_all();
			}
		}
		catch (std::exception& ex) {
			throw(std::exception(ex.what()));
		}
	}

	m_mutex.unlock();

	boost::interprocess::shared_memory_object::remove(m_sharedMemoryName);

	m_data = nullptr;
}