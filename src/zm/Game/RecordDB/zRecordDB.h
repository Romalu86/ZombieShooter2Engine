#pragma once
#include <stddef.h>
#include <memory>
#include <string>

enum { REC_TYPES_CNT = 4 };

struct AS_RECORD_STRING_CONTROL
{
    static const unsigned int npos = static_cast<unsigned int>(-1);
    unsigned char allocatorState[4];
    char* data;
    unsigned int size;
    unsigned int capacity;
};

struct AS_RECORD_DWORD_VECTOR
{
    int* begin;
    int* end;
    int* capacityEnd;
    std::allocator<int> allocator;

    void CopyConstruct(const AS_RECORD_DWORD_VECTOR& other);
    AS_RECORD_DWORD_VECTOR& operator=(const AS_RECORD_DWORD_VECTOR& other);
    void insert(int* position, unsigned int count, const int& value);
    int* insert(int* position, const int& value);
};

struct AS_RECORD_TEXT
{
    union
    {
        char localText[16];
        char* allocatedText;
    };
    unsigned int size;
    unsigned int capacity;
    std::allocator<char> allocator;

    AS_RECORD_TEXT() noexcept;
    AS_RECORD_TEXT(const AS_RECORD_TEXT& other);
    ~AS_RECORD_TEXT() noexcept;
    AS_RECORD_TEXT& operator=(const AS_RECORD_TEXT& other);
    AS_RECORD_TEXT& operator=(const char* text);
    AS_RECORD_TEXT& assign(const char* text, unsigned int length);
    const char* c_str() const noexcept;
    char* Data() noexcept;
    operator std::string() const;
};

struct AS_RECORD
{
    AS_RECORD_TEXT m_playerName;
    int m_totalScore;
    int m_monsterKills;
    int m_elapsedSeconds;
    unsigned int m_checksum;
    AS_RECORD_TEXT m_recordTimestamp;
    AS_RECORD_DWORD_VECTOR m_weaponScores;
    AS_RECORD_DWORD_VECTOR m_weaponKills;
    unsigned char m_cheater;

    AS_RECORD() noexcept;
    AS_RECORD(const AS_RECORD& other);
    ~AS_RECORD() noexcept(false);
    AS_RECORD& operator=(const AS_RECORD& other);


    int GetHash(int mode);
};

struct AS_RECORD_VECTOR
{
    AS_RECORD* begin;
    AS_RECORD* end;
    AS_RECORD* capacityEnd;
    std::allocator<AS_RECORD> allocator;

    void reserve(unsigned int newCapacity);
    void push_back(const AS_RECORD& record);
};

struct AS_RECORD_PTR_VECTOR
{
    AS_RECORD** begin;
    AS_RECORD** end;
    AS_RECORD** capacityEnd;
    std::allocator<AS_RECORD*> allocator;

    void insert(AS_RECORD** position, unsigned int count, AS_RECORD* const& value);
};

class zRecordDB
{
public:


    unsigned char m_DataFlags[REC_TYPES_CNT];
    AS_RECORD_VECTOR m_Data[REC_TYPES_CNT];
    unsigned char m_OutFlags[REC_TYPES_CNT];
    AS_RECORD_PTR_VECTOR m_OutData[REC_TYPES_CNT];


    virtual void* QueryInterface(int interfaceId) const;
    virtual int iRelease();
    zRecordDB();
    virtual ~zRecordDB() noexcept(false);
    static void operator delete(void* memory);
    void AddRecord(int iType, const AS_RECORD& record);
    int GetRecCnt(int iType);
    const AS_RECORD* GetRecData(int iType, int index);
    void ClearTable(int iType);

private:
    bool LoadIfNeed(int iType);
    bool LoadFromFile(int iType);
    bool SaveToFile(int iType);
    void BuildDataIfNeed(int iType);
};

extern zRecordDB* RecordDatabase;


