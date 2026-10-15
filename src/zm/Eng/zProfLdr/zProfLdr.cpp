#include "zm/Utils/zLogConsole.h"
#include "../../../engine_globals.h"
#include "mylib.h"
#include "zProfLdr.h"
#include "zm/Eng/zOptSave.h"
#include "runtime/locale_state.h"
#include "myerror.h"
#include "map.h"
#include "basetype.h"
#include <new>
#include "zm/Game/RecordDB/RecordDBStreambufControl.h"
#include <typeinfo>
#include <cstdio>
#include <cstring>
#include <direct.h>

unsigned int RecordDBGetOrAssignGlobalSequenceId(Msvc6::LocaleId* self);
void* RecordDBCtypeCharCtor(void* self, unsigned int table32, unsigned int deleteTable, unsigned int refs);
struct MsvcLocaleFacetRef;
MsvcLocaleFacetRef* RecordDBSaveCtypeFacet(MsvcLocaleFacetRef* value);

namespace
{
    const char profileSourceFile[] = "..\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp";
}

void* zProfLdr::QueryInterface(int interfaceId) const
{
    if (interfaceId == 54)
        return const_cast<zProfLdr*>(this);
    DebugAssert(5, "0", profileSourceFile, 24);
    return 0;
}

zProfLdr::zProfLdr()
{
    m_bLoaded = 0;
    m_currentProfile = -1;
    ClearObjPrf();
    for (int level = 0; level < MAX_LEVEL; ++level)
        m_questLevNums[level] = 0;
}

int zProfLdr::iRelease()
{
    if (this != nullptr)
    {
        this->~zProfLdr();
        zProfLdr::operator delete(this);
    }
    return 0;
}

void zProfLdr::operator delete(void* memory)
{
    CrtFreePointer(memory);
}

zProfLdr::~zProfLdr() noexcept(false)
{
    (void)ExpRegToFile();
    delete g_pOptSaveUser;
    g_pOptSaveUser = 0;
}

bool zProfLdr::GetLoadState()
{
    return m_bLoaded != 0;
}

void zProfLdr::ClearObjPrf()
{
    if (m_bLoaded != 0)
        DebugAssert(5, "!m_bLoaded", profileSourceFile, 56);
    for (int profile = 0; profile < MAX_PROF_NUM; ++profile)
    {
        m_aProfs[profile] = StringGlobals::EmptyText();
        m_profileChanged[profile] = 0;
    }
}

STRING zProfLdr::GetProfName(int profile) const
{
    if (profile < 0 || profile >= MAX_PROF_NUM)
        DebugAssert(5, "_iNum >= 0 && _iNum < MAX_PROF_NUM", profileSourceFile, 126);
    return m_aProfs[profile];
}

void zProfLdr::SetProfName(int profile, STRING name)
{
    if (profile < 0 || profile >= MAX_PROF_NUM)
        DebugAssert(5, "_iNum >= 0 && _iNum < MAX_PROF_NUM", profileSourceFile, 134);
    if (std::strcmp(m_aProfs[profile].m_value, name.m_value) != 0)
    {
        m_aProfs[profile] = name;
        m_profileChanged[profile] = 1;
    }
}

void zProfLdr::SetQuestLevNum(int level, int questLevel)
{
    if (level < 0 || level >= MAX_LEVEL)
        DebugAssert(5, "_iNum >= 0 && _iNum < MAX_LEVEL", profileSourceFile, 483);
    m_questLevNums[level] = questLevel;
}

int zProfLdr::GetQuestLevNum(int level) const
{
    return level >= 0 && level < MAX_LEVEL ? m_questLevNums[level] : -1;
}

bool zProfLdr::ExpFileToReg()
{
    if (m_bLoaded)
        return true;
    const bool result = ExpFileToObj();
    m_bLoaded = 1;
    return result;
}

bool zProfLdr::ExpRegToFile()
{
    if (!m_bLoaded)
        DebugAssert(5, "m_bLoaded", profileSourceFile, 213);
    (void)ExpObjToFile();
    if (g_pOptSaveUser)
        g_pOptSaveUser->Save();
    return true;
}

void zProfLdr::SetCurProf(int profile)
{
    if ((profile < 0 || profile >= MAX_PROF_NUM) && profile != -1)
        DebugAssert(5, "_iCurProf >= 0 && _iCurProf < MAX_PROF_NUM || _iCurProf == -1", profileSourceFile, 417);
    if (m_currentProfile != -1)
        (void)ExpRegToFile();
    if (m_currentProfile == profile)
        return;
    delete g_pOptSaveUser;
    g_pOptSaveUser = 0;
    m_currentProfile = profile;
    if (profile != -1)
    {
        const STRING folderName = GetProfFolderName(profile);
        g_pOptSaveUser = new zOptSaveFile(std::string(folderName.m_value) + "/_user.dat");
    }
}

STRING zProfLdr::GetProfFolderName(int profile)
{
    if (profile < 0 || profile >= MAX_PROF_NUM)
        DebugAssert(5, "_iProfileNum >= 0 && _iProfileNum < MAX_PROF_NUM", profileSourceFile, 444);
    return Printf("Profile%d", profile);
}

void zProfLdr::CreateFolderIfNeed(int profile)
{
    if (profile < 0 || profile >= MAX_PROF_NUM)
        DebugAssert(5, "_iNum >= 0 && _iNum < MAX_PROF_NUM", profileSourceFile, 463);
    const STRING path = Map->WindowsUserPath() + "Saves\\" + GetProfFolderName(profile);
    if (!zutl::FileExists(path.m_value))
    {
        const int result = _mkdir(path.m_value);
        if (result != 0)
            DebugAssert(5, "ret == 0", profileSourceFile, 476);
    }
}

STRING zProfLdr::GetPlayerNameFName(int profile)
{
    return Map->WindowsUserPath() + "Saves\\" + GetProfFolderName(profile) + "\\_profilename.txt";
}

STRING zProfLdr::GetOptFName(int profile, bool createFolder)
{
    if (createFolder)
        CreateFolderIfNeed(profile);
    return Map->WindowsUserPath() + "Saves\\" + GetProfFolderName(profile) + "\\ProfileOptions";
}

bool zProfLdr::ExpFileToObj()
{
    ClearObjPrf();
    for (int profile = 0; profile < MAX_PROF_NUM; ++profile)
    {
        const STRING filename = GetPlayerNameFName(profile);
        std::FILE* const input = std::fopen(filename.m_value, "rt");
        if (!input)
            continue;
        char name[2048] = {};
        if (std::fgets(name, sizeof(name), input))
        {
            std::size_t length = std::strlen(name);
            while (length && (name[length - 1] == '\n' || name[length - 1] == '\r'))
                name[--length] = '\0';
        }
        else
        {
            name[0] = '\0';
        }
        m_aProfs[profile] = name;
        (void)std::fclose(input);
    }
    return true;
}

bool zProfLdr::ExpObjToFile()
{
    for (int profile = 0; profile < MAX_PROF_NUM; ++profile)
    {
        if (m_profileChanged[profile] == 0)
            continue;
        const STRING filename = GetPlayerNameFName(profile);
        if (m_aProfs[profile].Length() > 0)
        {
            CreateFolderIfNeed(profile);
            std::FILE* const output = std::fopen(filename.m_value, "w");
            if (output)
            {
                (void)std::fprintf(output, "%s\n", m_aProfs[profile].m_value);
                (void)std::fclose(output);
            }
        }
        else
        {
            zutl::MoveToTrash(filename.m_value);
        }
    }
    return true;
}

int zProfLdr::GetCurLevNum(bool survival)
{
    if (m_currentProfile < 0)
        return -1;
    if (!g_pOptSaveUser)
    {
        DebugAssert(5, "g_pOptSaveUser", profileSourceFile, 364);
        if (!g_pOptSaveUser)
            return -1;
    }
    return g_pOptSaveUser->GetInt(survival ? "giCurLevSurv" : "giCurLevCmpgn", -1);
}

int zProfLdr::GetMaxLevNum(int profile, bool survival)
{
    if (profile == -1)
        return 0;
    if (profile < 0 || profile >= MAX_PROF_NUM)
        DebugAssert(5, "_iProfNum >= 0 && _iProfNum < MAX_PROF_NUM", profileSourceFile, 404);
    if (!g_pOptSaveUser)
    {
        DebugAssert(5, "g_pOptSaveUser", profileSourceFile, 406);
        if (!g_pOptSaveUser)
            return -1;
    }
    return g_pOptSaveUser->GetInt(survival ? "giMaxLevSurv" : "giMaxLevCmpgn", 0);
}

void zProfLdr::SetMaxLevNum(bool survival)
{
    if (m_currentProfile == -1)
        return;
    if (!g_pOptSaveUser)
    {
        DebugAssert(5, "g_pOptSaveUser", profileSourceFile, 378);
        if (!g_pOptSaveUser)
            return;
    }
    const int currentLevel = g_pOptSaveUser->GetInt(survival ? "giCurLevSurv" : "giCurLevCmpgn", -1);
    const char* const maxLevelKey = survival ? "giMaxLevSurv" : "giMaxLevCmpgn";
    const int maximumLevel = g_pOptSaveUser->GetInt(maxLevelKey, 0);
    const bool updateMaximum = survival
        ? currentLevel >= 0 && currentLevel < 5 && (maximumLevel < currentLevel || currentLevel == 0)
        : maximumLevel < currentLevel || currentLevel == 1;
    if (updateMaximum)
        g_pOptSaveUser->SetInt(maxLevelKey, currentLevel);
}

STRING zProfLdr::GetSaveName2(int profile, SAVE_TYPE saveType, int level)
{
    const int rawType = static_cast<int>(saveType);
    if (level < 0 && rawType != 3)
        DebugAssert(4, "_iLevNum >= 0 || _eSType == ST_INPUT", profileSourceFile, 323);
    CreateFolderIfNeed(profile);
    STRING path = Map->WindowsUserPath() + "Saves\\" + GetProfFolderName(profile) + "\\";
    if (rawType != 3)
        path += Printf("%.2d", level);
    const char* suffix = 0;
    switch (rawType)
    {
    case 1: suffix = "_surv"; break;
    case 2: suffix = "_quest"; break;
    case 3: suffix = "input"; break;
    case 4: suffix = "_log"; break;
    case 5: suffix = "_hashcmpgn"; break;
    case 6: suffix = "_hashsurv"; break;
    default: break;
    }
    if (suffix)
        path += suffix;
    return path + ".cfg";
}

STRING zProfLdr::GetCurSaveName2(SAVE_TYPE saveType, bool survival)
{
    if (m_currentProfile < 0)
        return STRING(StringGlobals::EmptyText());
    const int level = GetCurLevNum(survival);
    if (level >= 0 || static_cast<int>(saveType) == 3)
        return GetSaveName2(m_currentProfile, saveType, level);
    return STRING(StringGlobals::EmptyText());
}

STRING zProfLdr::GetMaxLevSaveName(SAVE_TYPE saveType)
{
    if (m_currentProfile < 0)
        return STRING(StringGlobals::EmptyText());
    const int rawType = static_cast<int>(saveType);
    if (rawType != 0 && rawType != 1)
        DebugAssert(5, "_eSType == ST_NORMAL || _eSType == ST_SURVIVE", profileSourceFile, 306);
    const int level = GetMaxLevNum(m_currentProfile, rawType == 1);
    if (level >= 0)
        return GetSaveName2(m_currentProfile, saveType, level);
    return STRING(StringGlobals::EmptyText());
}

bool zProfLdr::DeleteProfile2(int profile)
{
    if (profile < 0 || profile >= MAX_PROF_NUM)
        DebugAssert(5, "_iNum >= 0 && _iNum < MAX_PROF_NUM", profileSourceFile, 233);
    if (m_aProfs[profile].m_value[0] == 0)
        return false;
    return DeleteProfileEx(profile);
}

bool zProfLdr::DeleteProfileEx(int profile)
{
    if (m_aProfs[profile].Length() <= 0)
        DebugAssert(5, "m_aProfNames[_iNum].Length() > 0", profileSourceFile, 244);
    const STRING folderName = GetProfFolderName(profile);
    (void)::gpLogCons->LogStr(LT_NOTICE, "Deleting profile %s", folderName.m_value);
    zutl::MoveToTrash(GetOptFName(profile, false).m_value);
    static const int saveTypesToDelete[4] = {0, 2, 4, 1};
    for (int level = 0; level < MAX_LEVEL; ++level)
    {
        for (int type = 0; type < 4; ++type)
            zutl::MoveToTrash(GetSaveName2(profile, static_cast<SAVE_TYPE>(saveTypesToDelete[type]), level).m_value);
    }
    zutl::MoveToTrash(GetSaveName2(profile, static_cast<SAVE_TYPE>(3), 0).m_value);
    if (m_currentProfile == profile)
        m_currentProfile = -1;
    SetProfName(profile, STRING(StringGlobals::EmptyText()));
    zutl::MoveToTrash(GetPlayerNameFName(profile).m_value);
    return true;
}

void* __cdecl ProfileUseCtypeFacet(void* localeOwner, void*          , unsigned char createIfMissing)
{
    unsigned int lockStorage;
    Msvc6Runtime::StaticLockConstruct(&lockStorage);
    void* facet;
    try
    {
        const unsigned int facetId =
            RecordDBGetOrAssignGlobalSequenceId(&Msvc6::FacetState<Msvc6::CharCtype>::id);
        facet = U32ToPtr(Msvc6Runtime::LocaleGetFacet(localeOwner, facetId, 1u));
        if (!facet)
        {
            if (!createIfMissing || !Msvc6Runtime::LocaleCanSupplyFacet(localeOwner))
                throw std::bad_cast();

            facet = const_cast<Msvc6::FacetData*>(Msvc6::FacetState<Msvc6::CharCtype>::_Psave);
            if (!facet)
            {
                void* const storage = CrtNew(24u);
                if (storage)
                {
                    try
                    {
                        (void)RecordDBCtypeCharCtor(storage, 0u, 0u, 0u);
                    }
                    catch (...)
                    {
                        CrtFreePointer(storage);
                        throw;
                    }
                }
                facet = RecordDBSaveCtypeFacet(static_cast<MsvcLocaleFacetRef*>(storage));
                Msvc6::FacetState<Msvc6::CharCtype>::_Psave = static_cast<const Msvc6::FacetData*>(facet);
            }
        }
    }
    catch (...)
    {
        Msvc6Runtime::StaticLockDestruct(&lockStorage);
        throw;
    }
    Msvc6Runtime::StaticLockDestruct(&lockStorage);
    return facet;
}
