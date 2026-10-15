#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

class zArg
{
public:
    enum ARG_TYPE { AT_NONE = 0, AT_INT = 1, AT_STR = 2 };

    explicit zArg(const std::string& name);
    const std::string& GetName() const;
    ARG_TYPE GetType() const;
    int GetInt() const;
    std::string GetString() const;

private:
    friend class zOptSave;
    std::string m_name;
    int m_intValue;
    std::string m_stringValue;
    ARG_TYPE m_eArgType;
};

class zOptSave
{
public:
    zOptSave();
    virtual ~zOptSave() noexcept(false);

    void SetInt(const std::string& name, int value);
    int GetInt(const std::string& name, int fallback = 0) const;
    void SetString(const std::string& name, const std::string& value);
    std::string GetString(const std::string& name,
                          const std::string& fallback = std::string()) const;
    bool TryGetString(const std::string& name, std::string& value) const;

    bool IsDirty() const;
    std::size_t GetArgumentCount() const;

    static void SetBasePath(const std::string& path);
    static const std::string& GetBasePath();
    static std::string ResolvePath(const std::string& filename);

protected:
    bool LoadFromFile(const std::string& filename);
    void SaveToFile(const std::string& filename);

private:
    zOptSave(const zOptSave&) = delete;
    zOptSave& operator=(const zOptSave&) = delete;

    zArg* Find(const std::string& name) const;
    zArg* CreateArg(const std::string& name);

    std::vector<std::unique_ptr<zArg> > m_args;
    std::map<unsigned int, zArg*> m_ArgsMap;
    bool m_dirty;
};

class zOptSaveFile : public zOptSave
{
public:
    explicit zOptSaveFile(const std::string& filename);
    ~zOptSaveFile() noexcept(false) override;

    bool Load();
    void Save();

private:
    std::string m_filename;
};

extern zOptSaveFile* g_pOptSaveGlobal;
extern zOptSaveFile* g_pOptSaveUser;
