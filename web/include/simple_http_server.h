#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace ahmiyat {
namespace web {

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    OPTIONS,
    UNKNOWN
};

struct HttpRequest {
    HttpMethod method;
    std::string uri;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    std::string getHeader(const std::string& name) const {
        auto it = headers.find(name);
        if (it != headers.end()) {
            return it->second;
        }
        return "";
    }
};

struct HttpResponse {
    int status_code;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() : status_code(200) {
        headers["Content-Type"] = "text/plain";
    }
    
    HttpResponse(int code) : status_code(code) {
        headers["Content-Type"] = "text/plain";
    }
    
    HttpResponse(int code, const std::string& content_type, const std::string& body_content) 
        : status_code(code), body(body_content) {
        headers["Content-Type"] = content_type;
    }
    
    void setHeader(const std::string& name, const std::string& value) {
        headers[name] = value;
    }
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

class SimpleHttpServer {
public:
    SimpleHttpServer(int port = 5000);
    ~SimpleHttpServer();
    
    void start();
    void stop();
    
    void addRoute(HttpMethod method, const std::string& path, HttpHandler handler);
    
private:
    int m_port;
    int m_socket;
    std::atomic<bool> m_running;
    std::thread m_serverThread;
    std::mutex m_mutex;
    
    struct Route {
        HttpMethod method;
        std::string path;
        HttpHandler handler;
    };
    
    std::vector<Route> m_routes;
    
    void serverLoop();
    void handleClient(int client_socket);
    
    HttpRequest parseRequest(const std::string& request_str);
    std::string buildResponse(const HttpResponse& response);
    
    HttpMethod stringToMethod(const std::string& method_str);
    std::string methodToString(HttpMethod method);
    std::string statusCodeToString(int status_code);
    
    static void urlDecode(std::string& text);
};

} // namespace web
} // namespace ahmiyat