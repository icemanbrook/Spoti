#include "db.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

// ── JSON helper ────────────────────────────────────────────────────────────
static std::string jsonStr(const std::string& s) {
    std::string o;
    for (char c : s) {
        if      (c == '"')  o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else                o += c;
    }
    return "\"" + o + "\"";
}

std::string Song::toJson() const {
    std::ostringstream o;
    o << "{"
      << "\"id\":"           << id           << ","
      << "\"title\":"        << jsonStr(title)        << ","
      << "\"artist\":"       << jsonStr(artist)       << ","
      << "\"genre\":"        << jsonStr(genre)        << ","
      << "\"release_date\":" << jsonStr(release_date)
      << "}";
    return o.str();
}

// ── Helpers ────────────────────────────────────────────────────────────────
std::string Database::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

std::string Database::toLower(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = (char)std::tolower((unsigned char)c);
    return r;
}

// ── CSV parser (handles quoted fields) ────────────────────────────────────
static std::vector<std::string> parseCSVRow(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            in_quote = !in_quote;
        } else if (c == ',' && !in_quote) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

// ── Load CSV ───────────────────────────────────────────────────────────────
void Database::load() {
    rows_.clear();
    next_id_ = 1;

    std::ifstream f(path_);
    if (!f.is_open()) return;   // no file yet — start empty

    std::string line;
    bool header = true;
    while (std::getline(f, line)) {
        if (line.empty() || line == "\r") continue;
        if (header) { header = false; continue; }   // skip header row

        auto cols = parseCSVRow(line);
        if (cols.size() < 5) continue;

        Song s;
        try { s.id = std::stoi(Database::trim(cols[0])); } catch (...) { continue; }
        s.title        = Database::trim(cols[1]);
        s.artist       = Database::trim(cols[2]);
        s.genre        = Database::trim(cols[3]);
        s.release_date = Database::trim(cols[4]);

        rows_.push_back(s);
        if (s.id >= next_id_) next_id_ = s.id + 1;
    }
}

// ── Save CSV ───────────────────────────────────────────────────────────────
// Wrap a field in quotes if it contains a comma
static std::string csvField(const std::string& s) {
    if (s.find(',') != std::string::npos)
        return "\"" + s + "\"";
    return s;
}

void Database::save() const {
    std::ofstream f(path_);
    if (!f.is_open()) return;
    f << "id,title,artist,genre,release_date\n";
    for (const auto& s : rows_) {
        f << s.id              << ","
          << csvField(s.title) << ","
          << csvField(s.artist)<< ","
          << csvField(s.genre) << ","
          << s.release_date    << "\n";
    }
}

// ── Constructor ────────────────────────────────────────────────────────────
Database::Database(const std::string& csv_path) : path_(csv_path) {
    load();
}

// ── CRUD ───────────────────────────────────────────────────────────────────
std::vector<Song> Database::all() const { return rows_; }

const Song* Database::findById(int id) const {
    for (const auto& s : rows_)
        if (s.id == id) return &s;
    return nullptr;
}

std::vector<Song> Database::search(const std::string& q) const {
    std::string ql = toLower(q);
    std::vector<Song> res;
    for (const auto& s : rows_) {
        if (toLower(s.title).find(ql)        != std::string::npos ||
            toLower(s.artist).find(ql)       != std::string::npos ||
            toLower(s.genre).find(ql)        != std::string::npos ||
            s.release_date.find(ql)          != std::string::npos)
            res.push_back(s);
    }
    return res;
}

Song Database::add(const std::string& title,
                   const std::string& artist,
                   const std::string& genre,
                   const std::string& release_date) {
    Song s{ next_id_++, title, artist, genre, release_date };
    rows_.push_back(s);
    save();
    return rows_.back();
}

bool Database::remove(int id) {
    auto it = std::find_if(rows_.begin(), rows_.end(),
                           [id](const Song& s){ return s.id == id; });
    if (it == rows_.end()) return false;
    rows_.erase(it);
    save();
    return true;
}
