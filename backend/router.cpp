// Include router header file
// Gives access to Router class, Request, Response, etc.
#include "router.h"

// Used for string stream operations
#include <sstream>

// Used for dynamic arrays (vector)
#include <vector>

// ─────────────────────────────────────────────────────────────
// HELPER FUNCTIONS
// ─────────────────────────────────────────────────────────────

// URL decoder function
// Converts encoded URL text into readable text
//
// Example:
// "Taylor+Swift" → "Taylor Swift"
// "%20" → space
std::string Router::urlDecode(const std::string& s) {

    // Final decoded output
    std::string out;

    // Loop through each character
    for (size_t i = 0; i < s.size(); ++i) {

        // '+' in URL means space
        if (s[i] == '+') {

            out += ' ';

        }

        // Handle hexadecimal encoded characters
        // Example: %20
        else if (s[i] == '%' && i + 2 < s.size()) {

            // Store decoded hex value
            int val = 0;

            // Read next 2 hexadecimal digits
            for (int j = 1; j <= 2; ++j) {

                char c = s[i + j];

                // Shift bits left
                val <<= 4;

                // Convert hex character to integer
                if (c >= '0' && c <= '9')
                    val += c - '0';

                else if (c >= 'a' && c <= 'f')
                    val += c - 'a' + 10;

                else if (c >= 'A' && c <= 'F')
                    val += c - 'A' + 10;
            }

            // Convert integer to character
            out += (char)val;

            // Skip next 2 hex characters
            i += 2;

        }

        // Normal character
        else {

            out += s[i];
        }
    }

    // Return decoded string
    return out;
}

// ─────────────────────────────────────────────────────────────
// Extract query parameter from query string
//
// Example:
// qs = "title=Shape+Of+You&artist=Ed"
// key = "artist"
//
// Returns:
// "Ed"
// ─────────────────────────────────────────────────────────────
std::string Router::queryParam(
    const std::string& qs,
    const std::string& key
) {

    // Create search prefix
    // Example: "artist="
    std::string prefix = key + "=";

    // Find position of key
    size_t pos = qs.find(prefix);

    // If key not found
    if (pos == std::string::npos)
        return "";

    // Start position of value
    size_t start = pos + prefix.size();

    // Find next '&'
    size_t end = qs.find('&', start);

    // Extract raw value
    std::string raw =
        (end == std::string::npos)
        ? qs.substr(start)
        : qs.substr(start, end - start);

    // Decode URL text
    return urlDecode(raw);
}

// ─────────────────────────────────────────────────────────────
// Create successful HTTP response
//
// Returns status 200 with JSON body
// ─────────────────────────────────────────────────────────────
Response Router::ok(const std::string& json) {

    return {
        200,
        "application/json",
        json
    };
}

// ─────────────────────────────────────────────────────────────
// Create error HTTP response
//
// Example:
// 404 → song not found
// ─────────────────────────────────────────────────────────────
Response Router::err(
    int code,
    const std::string& msg
) {

    return {
        code,
        "application/json",
        "{\"error\":\"" + msg + "\"}"
    };
}

// ─────────────────────────────────────────────────────────────
// Convert vector of songs into JSON array
//
// Example:
// [
//   {...},
//   {...}
// ]
// ─────────────────────────────────────────────────────────────
std::string Router::songsToJson(
    const std::vector<Song>& v
) {

    // Output stream
    std::ostringstream o;

    // Start JSON array
    o << "[";

    // Loop through songs
    for (size_t i = 0; i < v.size(); ++i) {

        // Add comma between objects
        if (i)
            o << ",";

        // Convert song object to JSON
        o << v[i].toJson();
    }

    // End JSON array
    o << "]";

    // Return JSON text
    return o.str();
}

// ─────────────────────────────────────────────────────────────
// Split URL path into segments
//
// Example:
// "/songs/5"
//
// becomes:
// ["songs", "5"]
// ─────────────────────────────────────────────────────────────
static std::vector<std::string> splitPath(
    const std::string& path
) {

    // Vector to store segments
    std::vector<std::string> segs;

    // Stream path text
    std::istringstream ss(path);

    std::string seg;

    // Split using '/'
    while (std::getline(ss, seg, '/'))

        // Ignore empty parts
        if (!seg.empty())
            segs.push_back(seg);

    return segs;
}

// ─────────────────────────────────────────────────────────────
// Router constructor
//
// Connects router with database object
// ─────────────────────────────────────────────────────────────
Router::Router(Database& db)
    : db_(db)
{
}

// ─────────────────────────────────────────────────────────────
// MAIN ROUTING FUNCTION
//
// This is the heart of router.cpp
//
// It decides:
// - which API endpoint was requested
// - which CRUD function should run
// ─────────────────────────────────────────────────────────────
Response Router::handle(const Request& req) {

    // Split request path
    auto segs = splitPath(req.path);

    // Check if first segment is "songs"
    if (segs.empty() || segs[0] != "songs")

        // Invalid route
        return err(404, "not found");

    // ─────────────────────────────────────────────────────────
    // HANDLE: /songs
    // ─────────────────────────────────────────────────────────
    if (segs.size() == 1) {

        // GET /songs
        if (req.method == "GET")
            return do_view(req.query);

        // POST /songs
        if (req.method == "POST")
            return do_add(req.body);

        // Unsupported method
        return err(405, "method not allowed");
    }

    // ─────────────────────────────────────────────────────────
    // HANDLE: /songs/{id}
    // ─────────────────────────────────────────────────────────
    if (segs.size() == 2) {

        // Song ID
        int id = -1;

        // Convert path segment to integer
        try {

            id = std::stoi(segs[1]);

        }

        // Invalid integer
        catch (...) {

            return err(400, "invalid id");
        }

        // GET /songs/5
        if (req.method == "GET")
            return do_get(id);

        // DELETE /songs/5
        if (req.method == "DELETE")
            return do_delete(id);

        // Unsupported method
        return err(405, "method not allowed");
    }

    // Invalid path
    return err(404, "not found");
}

// ─────────────────────────────────────────────────────────────
// VIEW / SEARCH SONGS
//
// Handles:
// GET /songs
// GET /songs?q=rock
// ─────────────────────────────────────────────────────────────
Response Router::do_view(
    const std::string& query
) {

    // Get search parameter 'q'
    std::string q = queryParam(query, "q");

    // If query empty → return all songs
    // Otherwise search songs
    std::vector<Song> songs =
        q.empty()
        ? db_.all()
        : db_.search(q);

    // Return JSON response
    return ok(songsToJson(songs));
}

// ─────────────────────────────────────────────────────────────
// GET ONE SONG
//
// Handles:
// GET /songs/{id}
// ─────────────────────────────────────────────────────────────
Response Router::do_get(int id) {

    // Find song in database
    const Song* s = db_.findById(id);

    // Song not found
    if (!s)
        return err(404, "song not found");

    // Return song JSON
    return ok(s->toJson());
}

// ─────────────────────────────────────────────────────────────
// ADD SONG
//
// Handles:
// POST /songs
// ─────────────────────────────────────────────────────────────
Response Router::do_add(
    const std::string& body
) {

    // Extract title from request body
    std::string title =
        queryParam(body, "title");

    // Extract artist
    std::string artist =
        queryParam(body, "artist");

    // Extract genre
    std::string genre =
        queryParam(body, "genre");

    // Extract release date
    std::string release_date =
        queryParam(body, "release_date");

    // Validate title
    if (title.empty())
        return err(400, "title is required");

    // Validate artist
    if (artist.empty())
        return err(400, "artist is required");

    // Default genre
    if (genre.empty())
        genre = "Unknown";

    // Default release date
    if (release_date.empty())
        release_date = "Unknown";

    // Add song into database
    Song s = db_.add(
        title,
        artist,
        genre,
        release_date
    );

    // Return success response
    return {
        201,
        "application/json",
        s.toJson()
    };
}

// ─────────────────────────────────────────────────────────────
// DELETE SONG
//
// Handles:
// DELETE /songs/{id}
// ─────────────────────────────────────────────────────────────
Response Router::do_delete(int id) {

    // Try deleting song
    if (!db_.remove(id))

        // Song not found
        return err(404, "song not found");

    // Return success with no content
    return {
        204,
        "application/json",
        ""
    };
}