#ifndef MOCK_WEB_SERVER_H
#define MOCK_WEB_SERVER_H

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <memory>
#include <chrono>

class MockWebServer {
public:
    struct HttpRequest {
        std::string method;
        std::string url;
        std::string path;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> queryParams;
        std::string body;
        std::string clientIP;
        uint16_t clientPort;
    };

    struct HttpResponse {
        int statusCode = 200;
        std::string statusMessage = "OK";
        std::map<std::string, std::string> headers;
        std::string body;
        bool keepAlive = true;
    };

    struct Route {
        std::string method;
        std::string path;
        std::function<HttpResponse(const HttpRequest&)> handler;
        std::string description;
    };

    enum class ServerState {
        STOPPED,
        STARTING,
        RUNNING,
        STOPPING,
        ERROR
    };

    MockWebServer(uint16_t port = 80);
    virtual ~MockWebServer() = default;

    // Server control
    bool begin();
    void end();
    bool isRunning() const { return state_ == ServerState::RUNNING; }
    ServerState getState() const { return state_; }
    
    // Route management
    void on(const std::string& method, const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
    void onGet(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
    void onPost(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
    void onPut(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
    void onDelete(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
    
    // Static file serving
    void serveStatic(const std::string& urlPath, const std::string& filePath);
    
    // Request simulation for testing
    HttpResponse simulateRequest(const HttpRequest& request);
    HttpResponse simulateGet(const std::string& path);
    HttpResponse simulatePost(const std::string& path, const std::string& body = "");
    HttpResponse simulatePut(const std::string& path, const std::string& body = "");
    HttpResponse simulateDelete(const std::string& path);
    
    // Client simulation
    void simulateClientConnection(const std::string& clientIP, uint16_t port);
    void simulateClientDisconnection(const std::string& clientIP);
    
    // Status and statistics
    uint16_t getPort() const { return port_; }
    std::string getURL() const { return "http://localhost:" + std::to_string(port_); }
    uint32_t getRequestCount() const { return requestCount_; }
    std::vector<HttpRequest> getRequestHistory() const { return requestHistory_; }
    
    // Response simulation helpers
    static HttpResponse createJsonResponse(const std::string& json, int statusCode = 200);
    static HttpResponse createTextResponse(const std::string& text, const std::string& contentType = "text/plain", int statusCode = 200);
    static HttpResponse createErrorResponse(int statusCode, const std::string& message = "");
    
    // Middleware simulation
    using Middleware = std::function<bool(const HttpRequest&)>;
    void addMiddleware(Middleware middleware);
    
    // CORS simulation
    void enableCORS(const std::string& allowedOrigin = "*");
    void setCORSHeaders(std::map<std::string, std::string> headers);

private:
    uint16_t port_;
    ServerState state_ = ServerState::STOPPED;
    std::vector<Route> routes_;
    std::map<std::string, std::string> staticRoutes_;
    std::vector<Middleware> middlewares_;
    std::map<std::string, std::string> corsHeaders_;
    bool corsEnabled_ = false;
    
    // Statistics
    uint32_t requestCount_ = 0;
    std::vector<HttpRequest> requestHistory_;
    std::vector<std::string> connectedClients_;
    
    // Helper methods
    Route* findRoute(const std::string& method, const std::string& path);
    std::string extractPath(const std::string& url);
    std::map<std::string, std::string> parseQueryParams(const std::string& query);
    bool applyMiddleware(const HttpRequest& request);
    void recordRequest(const HttpRequest& request);
    void updateState(ServerState newState);
};

#endif // MOCK_WEB_SERVER_H