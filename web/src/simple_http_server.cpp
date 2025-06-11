#include "../include/simple_http_server.h"
#include <sstream>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <chrono>
#include <cerrno>
#include <string.h>  // for strerror

namespace ahmiyat {
namespace web {

SimpleHttpServer::SimpleHttpServer(int port) : m_port(port), m_socket(-1), m_running(false) {
}

SimpleHttpServer::~SimpleHttpServer() {
    stop();
}

void SimpleHttpServer::start() {
    if (m_running) {
        std::cerr << "Server is already running" << std::endl;
        return;
    }
    
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Allow socket reuse
    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(m_socket);
        return;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);
    
    // Try binding with retries
    int bind_result = -1;
    int max_retries = 3;
    int retry_count = 0;
    
    while (bind_result < 0 && retry_count < max_retries) {
        bind_result = bind(m_socket, (struct sockaddr *)&address, sizeof(address));
        
        if (bind_result < 0) {
            std::cerr << "Bind attempt " << (retry_count + 1) << " failed (errno: " << errno << ")" << std::endl;
            
            // Check specific error
            if (errno == EADDRINUSE) {
                std::cerr << "Port " << m_port << " is already in use, waiting briefly before retry" << std::endl;
            } else {
                std::cerr << "Failed to bind socket to port " << m_port << ": " << strerror(errno) << std::endl;
            }
            
            // Wait a bit before retry
            retry_count++;
            if (retry_count < max_retries) {
                std::cerr << "Waiting 2 seconds before retry..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    }
    
    if (bind_result < 0) {
        std::cerr << "All bind attempts failed for port " << m_port << std::endl;
        close(m_socket);
        m_socket = -1;
        return;
    }
    
    if (listen(m_socket, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(m_socket);
        return;
    }
    
    m_running = true;
    m_serverThread = std::thread(&SimpleHttpServer::serverLoop, this);
    
    std::cout << "Server started on port " << m_port << std::endl;
}

void SimpleHttpServer::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // Close the server socket to unblock accept()
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
    
    // Wait for the server thread to finish
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    
    std::cout << "Server stopped" << std::endl;
}

void SimpleHttpServer::addRoute(HttpMethod method, const std::string& path, HttpHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routes.push_back({method, path, handler});
}

void SimpleHttpServer::serverLoop() {
    while (m_running) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        
        int client_socket = accept(m_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0) {
            // accept() was interrupted or server is shutting down
            if (!m_running) {
                break;
            }
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        // Handle client in a separate thread
        std::thread(&SimpleHttpServer::handleClient, this, client_socket).detach();
    }
}

void SimpleHttpServer::handleClient(int client_socket) {
    // Set a timeout for receive
    struct timeval tv;
    tv.tv_sec = 5;  // 5 seconds timeout
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    // Read the request
    char buffer[4096] = {0};
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    // Parse the request
    HttpRequest request = parseRequest(std::string(buffer, bytes_read));
    
    // Find a matching route
    HttpResponse response(404, "text/plain", "Not Found");
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool routeFound = false;
        
        // First try exact match
        for (const auto& route : m_routes) {
            if (route.method == request.method && route.path == request.uri) {
                response = route.handler(request);
                routeFound = true;
                break;
            }
        }
        
        // If not found, try prefix match for static files
        if (!routeFound && request.method == HttpMethod::GET) {
            for (const auto& route : m_routes) {
                // Check if the route is a static file route and URI starts with it
                if (route.path == "/public" && request.uri.find("/public/") == 0) {
                    response = route.handler(request);
                    routeFound = true;
                    break;
                }
            }
        }
        
        // Add debug info
        if (!routeFound) {
            std::cerr << "No route found for: " << methodToString(request.method) << " " << request.uri << std::endl;
        }
    }
    
    // Build the response
    std::string response_str = buildResponse(response);
    
    // Send the response
    send(client_socket, response_str.c_str(), response_str.length(), 0);
    
    // Close the connection
    close(client_socket);
}

HttpRequest SimpleHttpServer::parseRequest(const std::string& request_str) {
    HttpRequest request;
    std::istringstream stream(request_str);
    std::string line;
    
    // Parse the request line
    if (std::getline(stream, line)) {
        std::istringstream request_line(line);
        std::string method_str, uri, version;
        request_line >> method_str >> uri >> version;
        
        request.method = stringToMethod(method_str);
        request.uri = uri;
        
        // URL-decode the URI
        urlDecode(request.uri);
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        // Remove trailing \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim leading/trailing whitespace from the value
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            request.headers[name] = value;
        }
    }
    
    // Parse body if present (e.g. for POST requests)
    std::string body;
    while (std::getline(stream, line)) {
        body += line + "\n";
    }
    request.body = body;
    
    return request;
}

std::string SimpleHttpServer::buildResponse(const HttpResponse& response) {
    std::ostringstream stream;
    
    // Status line
    stream << "HTTP/1.1 " << response.status_code << " " << statusCodeToString(response.status_code) << "\r\n";
    
    // Headers
    for (const auto& header : response.headers) {
        stream << header.first << ": " << header.second << "\r\n";
    }
    
    // Add Content-Length header
    stream << "Content-Length: " << response.body.length() << "\r\n";
    
    // End of headers
    stream << "\r\n";
    
    // Body
    stream << response.body;
    
    return stream.str();
}

HttpMethod SimpleHttpServer::stringToMethod(const std::string& method_str) {
    if (method_str == "GET") return HttpMethod::GET;
    if (method_str == "POST") return HttpMethod::POST;
    if (method_str == "PUT") return HttpMethod::PUT;
    if (method_str == "DELETE") return HttpMethod::DELETE;
    if (method_str == "OPTIONS") return HttpMethod::OPTIONS;
    return HttpMethod::UNKNOWN;
}

std::string SimpleHttpServer::methodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

std::string SimpleHttpServer::statusCodeToString(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

void SimpleHttpServer::urlDecode(std::string& text) {
    std::string result;
    result.reserve(text.length());
    
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '+') {
            result += ' ';
        } else if (text[i] == '%' && i + 2 < text.length()) {
            int value;
            std::istringstream is(text.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += '%';
            }
        } else {
            result += text[i];
        }
    }
    
    text = result;
}

} // namespace web
} // namespace ahmiyat