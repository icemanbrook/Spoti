# Spoti — Music Library System

A full-stack music library application built entirely in **C++17** (backend) and **HTML/CSS/JavaScript** (frontend), applying the REST API design conventions of the Spotify Web API — without using Spotify itself.

Submitted as a Unix Systems Programming project at **JSS Science and Technology University**, Department of Information Science and Engineering.

---

## What It Does

Spoti is a local HTTP server that manages a song library stored in a CSV file. It exposes a REST API that a browser-based frontend calls to perform four operations:

| Operation | How |
|-----------|-----|
| **View** | See all 50 songs — title, artist, genre, release date |
| **Search** | Live search across any field |
| **Add** | Add a new song via a form |
| **Delete** | Remove a song by ID or via the list |

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Backend | C++17 — raw HTTP server, no framework |
| Networking | Windows Winsock2 (`ws2_32`) / POSIX sockets |
| Database | CSV flat file (`data/songs.csv`) — 50 songs pre-loaded |
| Frontend | Single HTML file — HTML, CSS, vanilla JavaScript |
| Build | MinGW-w64 `g++`, Windows batch script |

---

## Project Structure

```
spoti/
├── backend/
│   ├── main.cpp        — HTTP server (TCP socket, request parser)
│   ├── db.h / db.cpp   — CSV database (load, save, CRUD, search)
│   ├── router.h / router.cpp — REST router (4 endpoints)
│   ├── Makefile        — Linux/macOS build
│   └── Makefile.win    — Windows/MinGW build
├── frontend/
│   └── index.html      — Complete UI (HTML + CSS + JS)
├── data/
│   └── songs.csv       — Persistent song database (50 songs)
├── start.bat           — Windows one-click build and launch
└── README.md
```

---

## How to Run (Windows)

### Requirements
Install **MinGW-w64** via MSYS2:
1. Download from https://www.msys2.org
2. Open the MSYS2 UCRT64 terminal and run:
   ```
   pacman -S mingw-w64-ucrt-x86_64-gcc
   ```
3. Add `C:\msys64\ucrt64\bin` to your system **PATH**
4. Verify: open a new Command Prompt and run `g++ --version`

### Launch
Double-click **`start.bat`**

It will:
1. Compile the C++ backend with `g++`
2. Start the server on `http://localhost:8080`
3. Open `frontend/index.html` in your browser automatically

### Manual build (if needed)
```bat
cd backend
g++ -std=c++17 -O2 -o spoti-server.exe main.cpp db.cpp router.cpp -lws2_32
spoti-server.exe ..\data\songs.csv
```
Then open `frontend\index.html` in your browser.

---

## How to Run (Linux / macOS)

```bash
cd backend
make
./spoti-server ../data/songs.csv
```
Then open `frontend/index.html` in your browser.

---

## API Endpoints

The backend follows Spotify Web API design conventions — resource-based URLs, HTTP verbs for CRUD, JSON responses.

```
GET    /songs              → return all songs as JSON array
GET    /songs?q=<query>    → search songs by title/artist/genre/date
POST   /songs              → add a song (form-encoded body)
DELETE /songs/{id}         → delete song by ID
```

### Example with curl
```bash
# View all songs
curl http://localhost:8080/songs

# Search
curl "http://localhost:8080/songs?q=weeknd"

# Add a song
curl -X POST http://localhost:8080/songs \
  -d "title=My+Song&artist=Someone&genre=Pop&release_date=2024-01-01"

# Delete song with ID 3
curl -X DELETE http://localhost:8080/songs/3
```

---

## How It Works

```
Browser (index.html)
      |
      |  HTTP request (fetch API)
      ▼
C++ TCP Server (main.cpp)
      |
      |  parseHTTP() → fills Request struct
      ▼
Router (router.cpp)
      |
      |  matches method + path
      ▼
Database (db.cpp)
      |
      |  reads/writes std::vector<Song>
      ▼
songs.csv (persistent storage)
      |
      |  JSON response
      ▼
Browser renders HTML table
```
