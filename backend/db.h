#pragma once
#include <string>
#include <vector>

struct Song {
    int         id;
    std::string title;
    std::string artist;
    std::string genre;
    std::string release_date;   // YYYY-MM-DD

    std::string toJson() const;
};

// Simple flat-file database
class Database {
public:
    explicit Database(const std::string& csv_path);

    std::vector<Song>  all()          const;
    const Song*        findById(int id) const;
    std::vector<Song>  search(const std::string& q) const;  // title/artist/genre
    Song               add(const std::string& title,
                           const std::string& artist,
                           const std::string& genre,
                           const std::string& release_date);
    bool               remove(int id);
    void               save() const;   // flush back to CSV

private:
    std::string        path_;
    std::vector<Song>  rows_;
    int                next_id_ = 1;

    void               load();
    static std::string trim(const std::string& s);
    static std::string toLower(const std::string& s);
};
