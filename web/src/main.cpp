#include "../include/ahmiyat_web.h"
#include <iostream>
#include <csignal>

static ahmiyat::web::AhmiyatWebApp* g_app = nullptr;

void signalHandler(int signal) {
    std::cout << "Received signal " << signal << std::endl;
    if (g_app) {
        g_app->stop();
    }
}

int main(int argc, char* argv[]) {
    // Set up signal handling for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Default port is 5000
    int port = 5000;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[i + 1]);
            i++;
        }
    }
    
    // Create the web application
    ahmiyat::web::AhmiyatWebApp app(port);
    g_app = &app;
    
    // Print welcome message
    std::cout << "====================================================" << std::endl;
    std::cout << "  Ahmiyat Blockchain - Proof of Memories Web App" << std::endl;
    std::cout << "====================================================" << std::endl;
    std::cout << "Access the web interface at http://localhost:" << port << std::endl;
    
    // Start the web server
    app.start();
    
    // Wait for user to press enter to stop the server
    std::cout << "Server is running... Press Enter to stop." << std::endl;
    std::string line;
    std::getline(std::cin, line);
    
    // Stop the server when user presses Enter
    app.stop();
    
    return 0;
}