#ifndef CHOWDREN_PATH_H
#define CHOWDREN_PATH_H

#include <string>
#include <algorithm>

#define PATH_SEP "\\/"

inline std::string get_app_path()
{
    return "./";
}

inline std::string get_app_drive()
{
    return "";
}

inline std::string get_app_dir()
{
    return get_app_path();
}

inline std::string get_path_filename(const std::string & path)
{
    size_t pos = path.find_last_of(PATH_SEP);
    if (pos == std::string::npos)
        return path;
    return path.substr(pos + 1);
}

inline std::string get_path_dirname(const std::string & path)
{
    size_t pos = path.find_last_of(PATH_SEP);
    if (pos == std::string::npos)
        return "";
    return path.substr(0, pos + 1);
}

inline std::string get_path_basename(const std::string & path)
{
    std::string path2 = get_path_filename(path);
    return path2.substr(0, path2.find_last_of("."));
}

#endif // CHOWDREN_PATH_H
