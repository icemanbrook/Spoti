// Spoti — HTTP server
// Windows: g++ -std=c++17 -O2 -o spoti-server.exe main.cpp db.cpp router.cpp -lws2_32
// Linux  : g++ -std=c++17 -O2 -o spoti-server      main.cpp db.cpp router.cpp

#include "db.h"
#include "router.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cctype>

// ── Platform sockets ───────────────────────────────────────────────────────
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  // Build with -lws2_32

  typedef SOCKET sock_t;
  static const sock_t SOCK_NONE = INVALID_SOCKET;
  static bool  sbad(sock_t s)   { return s == INVALID_SOCKET; }
  static void  sclose(sock_t s) { closesocket(s); }
  static bool  net_up()         { WSADATA w; return WSAStartup(MAKEWORD(2,2),&w)==0; }
  static void  net_down()       { WSACleanup(); }

#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>

  typedef int  sock_t;
  static const sock_t SOCK_NONE = -1;
  static bool  sbad(sock_t s)   { return s < 0; }
  static void  sclose(sock_t s) { ::close(s); }
  static bool  net_up()         { return true; }
  static void  net_down()       { }
#endif

static const int PORT = 8080;

// ── HTTP parser ────────────────────────────────────────────────────────────
static Request parseHTTP(const std::string& raw) {
    Request req;
    std::istringstream ss(raw);
    std::string line;

    if (!std::getline(ss, line)) return req;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream rl(line);
    std::string target;
    rl >> req.method >> target;

    size_t q = target.find('?');
    if (q != std::string::npos) {
        req.path  = target.substr(0, q);
        req.query = target.substr(q + 1);
    } else {
        req.path = target;
    }

    int clen = 0;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string val = (colon + 2 < line.size()) ? line.substr(colon + 2) : "";
        for (char& c : key) c = (char)std::tolower((unsigned char)c);
        req.headers[key] = val;
        if (key == "content-length")
            try { clen = std::stoi(val); } catch (...) {}
    }

    if (clen > 0) {
        req.body.resize((size_t)clen, '\0');
        ss.read(&req.body[0], clen);
    }
    return req;
}

// ── HTTP response ──────────────────────────────────────────────────────────
static std::string buildHTTP(const Response& res) {
    const char* reason = "Unknown";
    switch (res.status) {
        case 200: reason = "OK";          break;
        case 201: reason = "Created";     break;
        case 204: reason = "No Content";  break;
        case 400: reason = "Bad Request"; break;
        case 404: reason = "Not Found";   break;
        case 405: reason = "Not Allowed"; break;
        default:  break;
    }
    std::ostringstream o;
    o << "HTTP/1.1 " << res.status << " " << reason << "\r\n"
      << "Content-Type: "   << res.content_type << "; charset=utf-8\r\n"
      << "Content-Length: " << res.body.size()  << "\r\n"
      << "Access-Control-Allow-Origin: *\r\n"
      << "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
      << "Access-Control-Allow-Headers: Content-Type\r\n"
      << "Connection: close\r\n\r\n"
      << res.body;
    return o.str();
}

// ── Main ───────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // CSV path: first argument, or default ../data/songs.csv
    std::string csv = (argc > 1) ? argv[1] : "../data/songs.csv";

    if (!net_up()) { std::cerr << "Network init failed\n"; return 1; }

    Database db(csv);
    Router   router(db);

    sock_t srv = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sbad(srv)) { std::cerr << "socket() failed\n"; net_down(); return 1; }

    int yes = 1;
    ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<const char*>(&yes), (int)sizeof(yes));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)PORT);

    if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), (int)sizeof(addr)) != 0) {
        std::cerr << "bind() failed — port " << PORT << " already in use?\n";
        sclose(srv); net_down(); return 1;
    }
    if (::listen(srv, 16) != 0) {
        std::cerr << "listen() failed\n";
        sclose(srv); net_down(); return 1;
    }

    std::cout << "Spoti server running on http://localhost:" << PORT << "\n"
              << "CSV database: " << csv << " (" << db.all().size() << " songs)\n"
              << "Press Ctrl+C to stop.\n\n";

    while (true) {
        sockaddr_in cli_addr;
        std::memset(&cli_addr, 0, sizeof(cli_addr));
#ifdef _WIN32
        int cli_len = (int)sizeof(cli_addr);
#else
        socklen_t cli_len = (socklen_t)sizeof(cli_addr);
#endif
        sock_t cli = ::accept(srv, reinterpret_cast<sockaddr*>(&cli_addr), &cli_len);
        if (sbad(cli)) continue;

        std::string raw;
        raw.reserve(2048);
        char buf[4096];
        int  n;
        while ((n = ::recv(cli, buf, (int)sizeof(buf), 0)) > 0) {
            raw.append(buf, (size_t)n);
            if (raw.find("\r\n\r\n") != std::string::npos) break;
        }

        if (raw.empty()) { sclose(cli); continue; }

        Request req = parseHTTP(raw);

        if (req.method == "OPTIONS") {
            Response pre{ 200, "text/plain", "" };
            std::string r = buildHTTP(pre);
            ::send(cli, r.c_str(), (int)r.size(), 0);
            sclose(cli);
            continue;
        }

        std::cout << req.method << " " << req.path;
        if (!req.query.empty()) std::cout << "?" << req.query;
        std::cout << "\n";

        Response    res  = router.handle(req);
        std::string resp = buildHTTP(res);
        ::send(cli, resp.c_str(), (int)resp.size(), 0);
        sclose(cli);
    }

    sclose(srv);
    net_down();
    return 0;
}
