// Prevents this header file from being included multiple times
// Avoids duplicate definition errors
#pragma once

// Include database header
// Gives access to Song structure and Database class
#include "db.h"

// Used for std::string
#include <string>

// Used for std::map
// map stores key-value pairs
#include <map>

// ─────────────────────────────────────────────────────────────
// REQUEST STRUCTURE
//
// Stores data received from browser/client
//
// Example request:
// GET /songs?q=rock HTTP/1.1
// ─────────────────────────────────────────────────────────────
struct Request {

    // HTTP method
    // Examples:
    // GET, POST, DELETE
    std::string method;

    // URL path
    // Example:
    // /songs
    // /songs/5
    std::string path;

    // Query string after '?'
    // Example:
    // q=rock
    std::string query;

    // HTTP body data
    // Mainly used for POST requests
    std::string body;

    // Stores HTTP headers
    //
    // Example:
    // Content-Type → application/json
    //
    // map format:
    // key → value
    std::map<std::string, std::string> headers;
};

// ─────────────────────────────────────────────────────────────
// RESPONSE STRUCTURE
//
// Stores data sent back to browser/client
// ─────────────────────────────────────────────────────────────
struct Response {

    // HTTP status code
    //
    // Examples:
    // 200 → OK
    // 404 → Not Found
    // 201 → Created
    int status = 200;

    // Type of response content
    //
    // Example:
    // application/json
    std::string content_type = "application/json";

    // Actual response body
    //
    // Usually JSON text
    std::string body;
};

// ─────────────────────────────────────────────────────────────
// ROUTER CLASS
//
// Handles API routing
//
// Decides which CRUD operation to execute
// based on:
// - URL path
// - HTTP method
// ─────────────────────────────────────────────────────────────
class Router {

public:

    // Constructor
    //
    // Receives database object reference
    explicit Router(Database& db);

    // Main routing function
    //
    // Takes HTTP request
    // Returns HTTP response
    Response handle(const Request& req);

private:

    // Reference to database object
    //
    // Used to perform CRUD operations
    Database& db_;

    // ─────────────────────────────────────────────────────────
    // CRUD OPERATIONS
    // ─────────────────────────────────────────────────────────

    // VIEW / SEARCH songs
    //
    // Handles:
    // GET /songs
    // GET /songs?q=rock
    Response do_view(const std::string& query);

    // GET one song by ID
    //
    // Handles:
    // GET /songs/{id}
    Response do_get(int id);

    // ADD new song
    //
    // Handles:
    // POST /songs
    Response do_add(const std::string& body);

    // DELETE song
    //
    // Handles:
    // DELETE /songs/{id}
    Response do_delete(int id);

    // ─────────────────────────────────────────────────────────
    // HELPER FUNCTIONS
    // ─────────────────────────────────────────────────────────

    // Create successful HTTP response
    //
    // Returns:
    // status = 200
    static Response ok(const std::string& json);

    // Create error response
    //
    // Example:
    // 404 not found
    static Response err(
        int code,
        const std::string& msg
    );

    // Convert vector of songs into JSON array
    //
    // Example:
    // [
    //   {...},
    //   {...}
    // ]
    static std::string songsToJson(
        const std::vector<Song>& v
    );

    // Extract parameter from query string
    //
    // Example:
    // queryParam("q=rock", "q")
    // returns "rock"
    static std::string queryParam(
        const std::string& qs,
        const std::string& key
    );

    // Decode URL-encoded text
    //
    // Example:
    // "Taylor+Swift"
    // becomes:
    // "Taylor Swift"
    static std::string urlDecode(
        const std::string& s
    );
};