#include "MockWebServer.h"
#include <algorithm>
#include <sstream>
#include <cctype>

MockWebServer::MockWebServer(uint16_t port) : port_(port) {
    state_ = ServerState::STOPPED;
}

bool MockWebServer::begin() {
    if (state_ == ServerState::RUNNING) {
        return true; // Already running
    }
    
    updateState(ServerState::STARTING);
    
    // Simulate server startup delay (embedded-friendly)
    // In real implementation, this would be handled by the async web server
    
    updateState(ServerState::RUNNING);
    return true;
}

void MockWebServer::end() {
    if (state_ == ServerState::RUNNING) {
        updateState(ServerState::STOPPING);
        
        // Simulate server shutdown
        // In real implementation, this would be handled by the async web server
        
        updateState(ServerState::STOPPED);
    }
}

void MockWebServer::on(const std::string& method, const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
    Route route;
    route.method = method;
    route.path = path;
    route.handler = handler;
    routes_.push_back(route);
}

void MockWebServer::onGet(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
    on("GET", path, handler);
}

void MockWebServer::onPost(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
    on("POST", path, handler);
}

void MockWebServer::onPut(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
    on("PUT", path, handler);
}

void MockWebServer::onDelete(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
    on("DELETE", path, handler);
}

void MockWebServer::serveStatic(const std::string& urlPath, const std::string& filePath) {
    staticRoutes_[urlPath] = filePath;
}

MockWebServer::HttpResponse MockWebServer::simulateRequest(const HttpRequest& request) {
    if (state_ != ServerState::RUNNING) {
        return createErrorResponse(503, "Service Unavailable");
    }
    
    recordRequest(request);
    
    // Apply middleware
    if (!applyMiddleware(request)) {
        return createErrorResponse(403, "Forbidden");
    }
    
    // CORS headers
    if (corsEnabled_) {
        // Add CORS headers would be done here
    }
    
    // Find matching route
    Route* route = findRoute(request.method, request.path);
    if (!route) {
        // Check for static files
        auto staticIt = staticRoutes_.find(request.path);
        if (staticIt != staticRoutes_.end()) {
            HttpResponse response;
            response.statusCode = 200;
            response.statusMessage = "OK";
            response.body = "Static file content for: " + staticIt->second;
            return response;
        }
        
        return createErrorResponse(404, "Not Found");
    }
    
    // Execute route handler
    try {
        HttpResponse response = route->handler(request);
        return response;
    } catch (const std::exception& e) {
        return createErrorResponse(500, std::string("Internal Server Error: ") + e.what());
    }
}

MockWebServer::HttpResponse MockWebServer::simulateGet(const std::string& path) {
    HttpRequest request;
    request.method = "GET";
    request.path = extractPath(path);
    request.queryParams = parseQueryParams(path);
    
    // Parse URL to get path and query params
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        request.path = path.substr(0, queryPos);
        request.queryParams = parseQueryParams(path.substr(queryPos + 1));
    }
    
    return simulateRequest(request);
}

MockWebServer::HttpResponse MockWebServer::simulatePost(const std::string& path, const std::string& body) {
    HttpRequest request;
    request.method = "POST";
    request.path = extractPath(path);
    request.body = body;
    
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        request.path = path.substr(0, queryPos);
        request.queryParams = parseQueryParams(path.substr(queryPos + 1));
    }
    
    return simulateRequest(request);
}

MockWebServer::HttpResponse MockWebServer::simulatePut(const std::string& path, const std::string& body) {
    HttpRequest request;
    request.method = "PUT";
    request.path = extractPath(path);
    request.body = body;
    
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        request.path = path.substr(0, queryPos);
        request.queryParams = parseQueryParams(path.substr(queryPos + 1));
    }
    
    return simulateRequest(request);
}

MockWebServer::HttpResponse MockWebServer::simulateDelete(const std::string& path) {
    HttpRequest request;
    request.method = "DELETE";
    request.path = extractPath(path);
    
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        request.path = path.substr(0, queryPos);
        request.queryParams = parseQueryParams(path.substr(queryPos + 1));
    }
    
    return simulateRequest(request);
}

void MockWebServer::simulateClientConnection(const std::string& clientIP, uint16_t port) {
    connectedClients_.push_back(clientIP + ":" + std::to_string(port));
}

void MockWebServer::simulateClientDisconnection(const std::string& clientIP) {
    connectedClients_.erase(
        std::remove(connectedClients_.begin(), connectedClients_.end(), clientIP),
        connectedClients_.end()
    );
}

MockWebServer::HttpResponse MockWebServer::createJsonResponse(const std::string& json, int statusCode) {
    HttpResponse response;
    response.statusCode = statusCode;
    response.statusMessage = (statusCode == 200) ? "OK" : "Error";
    response.body = json;
    response.headers["Content-Type"] = "application/json";
    return response;
}

MockWebServer::HttpResponse MockWebServer::createTextResponse(const std::string& text, const std::string& contentType, int statusCode) {
    HttpResponse response;
    response.statusCode = statusCode;
    response.statusMessage = (statusCode == 200) ? "OK" : "Error";
    response.body = text;
    response.headers["Content-Type"] = contentType;
    return response;
}

MockWebServer::HttpResponse MockWebServer::createErrorResponse(int statusCode, const std::string& message) {
    HttpResponse response;
    response.statusCode = statusCode;
    response.statusMessage = "Error";
    response.body = message.empty() ? "Error " + std::to_string(statusCode) : message;
    response.headers["Content-Type"] = "text/plain";
    return response;
}

void MockWebServer::addMiddleware(Middleware middleware) {
    middlewares_.push_back(middleware);
}

void MockWebServer::enableCORS(const std::string& allowedOrigin) {
    corsEnabled_ = true;
    corsHeaders_["Access-Control-Allow-Origin"] = allowedOrigin;
    corsHeaders_["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
    corsHeaders_["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
}

void MockWebServer::setCORSHeaders(std::map<std::string, std::string> headers) {
    corsHeaders_ = headers;
}

MockWebServer::Route* MockWebServer::findRoute(const std::string& method, const std::string& path) {
    for (auto& route : routes_) {
        if (route.method == method && route.path == path) {
            return &route;
        }
    }
    return nullptr;
}

std::string MockWebServer::extractPath(const std::string& url) {
    size_t queryPos = url.find('?');
    if (queryPos != std::string::npos) {
        return url.substr(0, queryPos);
    }
    return url;
}

std::map<std::string, std::string> MockWebServer::parseQueryParams(const std::string& query) {
    std::map<std::string, std::string> params;
    
    if (query.empty()) {
        return params;
    }
    
    std::istringstream iss(query);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);
            params[key] = value;
        }
    }
    
    return params;
}

bool MockWebServer::applyMiddleware(const HttpRequest& request) {
    for (auto& middleware : middlewares_) {
        if (!middleware(request)) {
            return false;
        }
    }
    return true;
}

void MockWebServer::recordRequest(const HttpRequest& request) {
    requestCount_++;
    requestHistory_.push_back(request);
    
    // Keep history limited for testing
    if (requestHistory_.size() > 100) {
        requestHistory_.erase(requestHistory_.begin());
    }
}

void MockWebServer::updateState(ServerState newState) {
    state_ = newState;
}