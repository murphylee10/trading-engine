#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_ctx;
        tcp::acceptor acceptor(io_ctx, tcp::endpoint(tcp::v4(), 9000));
        std::cout << "Trading engine listening on port 9000\n";

        for (;;) {
            tcp::socket socket(io_ctx);
            acceptor.accept(socket);
            std::cout << "New client connected: "
                      << socket.remote_endpoint() << "\n";
            // TODO: spawn session to read orders
        }
    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }
    return 0;
}