
#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <boost/asio.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

namespace sereadmo {
namespace olsrclient {

class TcpConnection
{
public:
	/// Constructor.
	TcpConnection(boost::asio::io_service& io_service)
		: m_socket(io_service)
	{
	}

	boost::asio::ip::tcp::socket& socket()
	{
		return m_socket;
	}

	/// Asynchronously write a data structure to the socket.
	template <typename T, typename Handler>
	void async_write(const T& t, Handler handler)
	{
		// Serialize the data first so we know how large it is.
		std::ostringstream archive_stream;
		boost::archive::text_oarchive archive(archive_stream);
		archive << t;
		m_outbound_data = archive_stream.str();

		// Format the header.
		std::ostringstream header_stream;
		header_stream << std::setw(header_length)
			<< std::hex << m_outbound_data.size();
		if (!header_stream || header_stream.str().size() != header_length)
		{
			// Something went wrong, inform the caller.
			boost::system::error_code error(boost::asio::error::invalid_argument);
			m_socket.io_service().post(boost::bind(handler, error));
			return;
		}
		m_outbound_header = header_stream.str();

		// Write the serialized data to the socket. We use "gather-write" to send
		// both the header and the data in a single write operation.
		std::vector<boost::asio::const_buffer> buffers;
		buffers.push_back(boost::asio::buffer(m_outbound_header));
		buffers.push_back(boost::asio::buffer(m_outbound_data));
		boost::asio::async_write(m_socket, buffers, handler);
	}

	/// Asynchronously read a data structure from the socket.
	template <typename T, typename Handler>
	void async_read(T& t, Handler handler)
	{
		// Issue a read operation to read exactly the number of bytes in a header.
		void (TcpConnection::*f)(
			const boost::system::error_code&,
			T&, boost::tuple<Handler>)
			= &TcpConnection::handle_read_header<T, Handler>;
		boost::asio::async_read(m_socket, boost::asio::buffer(m_inbound_header),
			boost::bind(f,
			this, boost::asio::placeholders::error, boost::ref(t),
			boost::make_tuple(handler)));
	}	

	/// Handle a completed read of a message header. The handler is passed using
	/// a tuple since boost::bind seems to have trouble binding a function object
	/// created using boost::bind as a parameter.
	template <typename T, typename Handler>
	void handle_read_header(const boost::system::error_code& e,
		T& t, boost::tuple<Handler> handler)
	{
        //std::cout << ">>> handle_read_header" << std::endl;
		if (e)
		{
			boost::get<0>(handler)(e);
		}
		else
		{
			// Determine the length of the serialized data.
			std::istringstream is(std::string(m_inbound_header, header_length));
			std::size_t m_inbound_datasize = 0;
			if (!(is >> std::hex >> m_inbound_datasize))
			{
				// Header doesn't seem to be valid. Inform the caller.
				boost::system::error_code error(boost::asio::error::invalid_argument);
				boost::get<0>(handler)(error);
				return;
			}

			// Start an asynchronous call to receive the data.
			m_inbound_data.resize(m_inbound_datasize);
			void (TcpConnection::*f)(
				const boost::system::error_code&,
				T&, boost::tuple<Handler>)
				= &TcpConnection::handle_read_data<T, Handler>;
			boost::asio::async_read(m_socket, boost::asio::buffer(m_inbound_data),
				boost::bind(f, this,
				boost::asio::placeholders::error, boost::ref(t), handler));
		}
        //std::cout << "<<< handle_read_header" << std::endl;
	}

	/// Handle a completed read of message data.
	template <typename T, typename Handler>
	void handle_read_data(const boost::system::error_code& e,
		T& t, boost::tuple<Handler> handler)
	{
        //std::cout << ">>> handle_read_data" << std::endl;
		if (e)
		{
			boost::get<0>(handler)(e);
		}
		else
		{
			// Extract the data structure from the data just received.
			try
			{
				std::string archive_data(&m_inbound_data[0], m_inbound_data.size());
				std::istringstream archive_stream(archive_data);
				boost::archive::text_iarchive archive(archive_stream);
				archive >> t;
			}
			catch (std::exception& e)
			{
				// Unable to decode data.
				boost::system::error_code error(boost::asio::error::invalid_argument);
				boost::get<0>(handler)(error);
				return;
			}

			// Inform caller that data has been received ok.
			boost::get<0>(handler)(e);
		}
        //std::cout << "<<< handle_read_data" << std::endl;
	}

private:
	/// The underlying socket.
	boost::asio::ip::tcp::socket m_socket;

	/// The size of a fixed length header.
	enum { header_length = 8 };

	/// Holds an outbound header.
	std::string m_outbound_header;

	/// Holds the outbound data.
	std::string m_outbound_data;

	/// Holds an inbound header.
	char m_inbound_header[header_length];

	/// Holds the inbound data.
	std::vector<char> m_inbound_data;
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

}
}
#endif 
