#pragma once

class FileBuffer {
public:
    FileBuffer() {}

    auto begin() -> decltype(lines_.begin()) {
        return lines_.begin();
    }
    auto end() -> decltype(lines_.end()) {
        return lines_.end();
    }
    auto cbegin() -> decltype(lines_.cbegin()) {
        return lines_.cbegin();
    }
    auto cend() -> decltype(lines_.cend()) {
        return lines_.cend();
    }
private:
    std::vector<std::string> lines_;

};
