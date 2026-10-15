#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"

#include "zShopDlgMenu.h"
#include "irpg.h"
#include "zHeroInvMenu.h"
#include "zShopItemInfo.h"
#include "zm/Game/ScriptExec/zScriptExec.h"
#include "graph.h"
#include "net.h"
#include <vector>
#include "sprite.h"
#include "vid.h"
#include "input.h"
#include "zm/Inventory/zInventory.h"
#include "zm/Inventory/zInventory3D.h"
#include "zm/Inventory/zInventory2D.h"
#include "zm/RPGHero/zRPGWeapon.h"
#include "zm/RPGHero/zRPGAmmo.h"
#include "zm/RPGHero/zRPGArmor.h"
#include "zm/RPGHero/zRPGImplant.h"
#include "mylib.h"
#include <new>
#include <stdexcept>

#include "basetype.h"

void GetNew_zShopDlgMenu(iShopDlgMenu** outObject);
zShopListNode**  ShopListErase(zShopListLayout* list, zShopListNode** outNext, zShopListNode* node);
zShopListNode**  ShopListInsert(zShopListLayout* list, zShopListNode** outNode, zShopListNode* position, iRPGObj* const* value);


#include "map.h"

void __cdecl GetNew_zShopDlgMenu(iShopDlgMenu** outObject)

{
    void* const storage = CrtNew(sizeof(zShopDlgMenu));
    zShopDlgMenu* object = 0;
    if (storage)
    {
        try
        {
            object = ::new (storage) zShopDlgMenu();
        }
        catch (...)
        {
            CrtFreePointer(storage);
            throw;
        }
    }
    *outObject = object;
}

void  zShopDlgMenu::SetRPGFrameWork(iRPGFrameWork* frameWork)
{
    m_framework = frameWork;
}

void  zShopDlgMenu::SetHeroInvMenu(iHeroInvMenu* heroInventoryMenu)
{
    m_heroInvMenu = heroInventoryMenu;
}

void  zShopDlgMenu::SetPlayer(iPlayer* player)
{
    m_player = player;
}

void  zShopDlgMenu::SetCurLevel(int level)
{
    m_curLevel = level;
}

iBaseDlgMenu*  zShopDlgMenu::AsBaseDlgMenu()
{
    return static_cast<iBaseDlgMenu*>(QueryInterface(11));
}

void  zShopDlgMenu::RefreshAll()
{
    InitLoad();
    RefreshBuyButtons();
}




void  zShopDlgMenu::PageUp(int inventoryId)
{
    iInventory* inventory = m_invServerRef.m_object->GetInv(inventoryId);
    zInventory3D* inventory3D = static_cast<zInventory3D*>(inventory->QueryInterface(48));
    inventory3D->Scroll(0.0f, -100.0f);
}

void  zShopDlgMenu::PageDown(int inventoryId)
{
    iInventory* inventory = m_invServerRef.m_object->GetInv(inventoryId);
    zInventory3D* inventory3D = static_cast<zInventory3D*>(inventory->QueryInterface(48));
    inventory3D->Scroll(0.0f, 100.0f);
}


void*  zShopDlgMenu::QueryInterface(int id) const
{
    if (id == 14) return const_cast<zShopDlgMenu*>(this);
    return zBaseDlgMenu::QueryInterface(id);
}

int  zShopDlgMenu::iRelease()
{
    if (this)
    {
        this->~zShopDlgMenu();
        zBaseDlgMenu::operator delete(this);
    }
    return 0;
}

bool  zShopDlgMenu::ScrollToItem(OBJ_TYPE objectType, OBJ_EXTTYPE extType)
{
    if (static_cast<int>(objectType) == 7)
    {
        RefreshBuyButtons();
        return false;
    }
    LoadListOfObjects(objectType, extType);
    return true;
}

void  zShopDlgMenu::Control(INPUT* input)
{
    (void) InitLoad();
    zBaseDlgMenu::Control(input);
    RefreshBuyButtons();
}

void  zShopDlgMenu::PageScroll(int scrollPercent, int inventoryId)
{
    if (scrollPercent<0) scrollPercent = 0;
    else if (scrollPercent>1000) scrollPercent = 1000;
    iInventory* inventory = m_invServerRef.m_object->GetInv(inventoryId);
    zInventory3D* inventory3D = static_cast<zInventory3D*>(inventory->QueryInterface(48));
    inventory3D->ScrollPer(scrollPercent);
}

void zShopDlgMenu::DeletePointerToSprite(SPRITE* sprite)
{
    bool found = false;
    for (int i = 0; i < 9; ++i)
        if (sprite && m_categoryButtons[i] == sprite) { found = true; break; }
    if (!found) return;
    for (int i = 0; i < 9; ++i) m_categoryButtons[i] = 0;
    m_buttonsFound = false;
    m_filterButtonCount = 0;
    m_objectType = static_cast<OBJ_TYPE>(15);
    m_extType = static_cast<OBJ_EXTTYPE>(16);
    if (zScriptExec::m_pShopItemInfo) zScriptExec::m_pShopItemInfo->Initialize();
}








void zShopDlgMenu::OnWeaponBought(iRPGObj* item)
{
    if (!item || item->GetGameObj() != TYPE_WEAPON)
        DebugAssert(5u, "item && item->GetType() == TYPE_WEAPON", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1115u);
    zRPGWeapon* weapon = static_cast<zRPGWeapon*>(item->QueryInterface(50));
    zRPGAmmo* ammo = weapon->GetAmmo();
    if (ammo && m_invServerRef.m_object->GetObjectCnt(ammo->GetBaseId()) == 0)
        (void)BuyAmmoForWeapon(item, false);
}

bool zShopDlgMenu::BuyAmmoForWeapon(iRPGObj* item, bool checkOnly)
{
    if (!item)
        DebugAssert(5u, "item", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1199u);
    if (item->GetGameObj() != TYPE_WEAPON && item->GetGameObj() != TYPE_AMMO)
        return false;
    zRPGAmmo* weaponAmmo = 0;
    if (item->GetGameObj() == TYPE_WEAPON)
    {
        zRPGWeapon* const weapon = static_cast<zRPGWeapon*>(item->QueryInterface(50));
        weaponAmmo = weapon->GetAmmo();
    }
    if (item->GetGameObj() == TYPE_AMMO)
        weaponAmmo = static_cast<zRPGAmmo*>(item->QueryInterface(45));
    if (!weaponAmmo)
        return false;

    zRPGAmmo* shopAmmo = 0;
    zShopListNode* shopAmmoNode = 0;
    for (zShopListNode* node = m_objects.m_sentinel->next;
        node != m_objects.m_sentinel; node = node->next)
    {
        iRPGObj* object = node->value;
        if (!object)
            DebugAssert(5u, "*j", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1221u);
        if (object->GetGameObj() != TYPE_AMMO)
            continue;
        zRPGAmmo* candidate = static_cast<zRPGAmmo*>(object->QueryInterface(45));
        const int weaponBaseId = weaponAmmo->GetBaseId();
        const int candidateBaseId = candidate->GetBaseId();
        if (candidateBaseId == weaponBaseId)
        {
            shopAmmo = candidate;
            shopAmmoNode = node;
            break;
        }
    }

    zRPGAmmo* ammoToBuy = shopAmmo;
    if (!ammoToBuy)
    {
        const int inventoryId = item->GetGameObj() == TYPE_AMMO ? 20 : 21;
        zInventory3D* const inventory = static_cast<zInventory3D*>(
            m_invServerRef.m_object->GetInv(inventoryId)->QueryInterface(48));
        int index = -1;
        zItem* available = inventory->GetItemByBaseId(weaponAmmo->GetBaseId(), index, false);
        if (available)
            ammoToBuy = static_cast<zRPGAmmo*>(available->QueryInterface(45));
    }
    if (!ammoToBuy)
        return false;
    if (!checkOnly)
    {
        (void)TryToBuyItem(ammoToBuy);
        if (ammoToBuy == shopAmmo && ammoToBuy->GetNumInCell() == 0)
        {
            shopAmmoNode->previous->next = shopAmmoNode->next;
            shopAmmoNode->next->previous = shopAmmoNode->previous;
            CrtFreePointer(shopAmmoNode);
            --m_objects.m_size;
        }
    }
    return true;
}

void  zShopDlgMenu::GoHome(int inventoryId)
{
    zInventory3D* const inventory =
        static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(inventoryId)->QueryInterface(48));
    float left = 0.0f, top = 0.0f, right = 0.0f, bottom = 0.0f;
    inventory->GetLimits(left, top, right, bottom);
    inventory->SetLimits(left, 0.0f, right, bottom);
}

void  zShopDlgMenu::GoEnd(int inventoryId)
{
    zInventory3D* const inventory =
        static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(inventoryId)->QueryInterface(48));
    float left = 0.0f, top = 0.0f, right = 0.0f, bottom = 0.0f;
    inventory->GetLimits(left, top, right, bottom);
    inventory->SetLimits(left, 100000.0f, right, bottom);
}

int  zShopDlgMenu::GetItemsNum(int inventoryId)
{
    iInventory* group = m_invServerRef.m_object->GetInv(inventoryId);
    zInventory3D* inventory = static_cast<zInventory3D*>(group->QueryInterface(48));
    return MultiplySigned32Wrap(inventory->m_gridHeight, inventory->GetCellSizeY());
}

int  zShopDlgMenu::GetListItemsNum(int inventoryId)
{
    iInventory* group = m_invServerRef.m_object->GetInv(inventoryId);
    zInventory3D* inventory = static_cast<zInventory3D*>(group->QueryInterface(48));
    float left, top, right, bottom;
    inventory->GetLimits(left, top, right, bottom);
    return TruncateFloatViaCrtToInt32(bottom);
}

int  zShopDlgMenu::GetFirstShownItemNum(int inventoryId)
{
    iInventory* group = m_invServerRef.m_object->GetInv(inventoryId);
    zInventory3D* inventory = static_cast<zInventory3D*>(group->QueryInterface(48));
    float left, top, right, bottom;
    inventory->GetLimits(left, top, right, bottom);
    return TruncateFloatViaCrtToInt32(top);
}

void  zShopDlgMenu::SetInvServer(iInvServer* inventoryServer)
{
    m_invServerRef.Set(inventoryServer);

    ReloadAll();
}

bool  zShopDlgMenu::ScrollToItem(int objectKey, bool unusedArgument)
{
    (void) unusedArgument;
    if (Net && (Net->m_flags & 0x10u)) return false;
    iRPGObj* const object = m_framework->GetItemById(objectKey);
    if (!object) return false;
    if (object->GetGameObj() == 7)
    {
        RefreshBuyButtons();
        return false;
    }
    const OBJ_EXTTYPE extType = static_cast<OBJ_EXTTYPE>(object->GetAddMoney());
    const OBJ_TYPE objectType = static_cast<OBJ_TYPE>(object->GetGameObj());
    LoadListOfObjects(objectType, extType);
    return false;
}




void  zShopDlgMenu::ReleaseListOfObjects()
{
    zShopListNode* node = m_objects.m_sentinel->next;
    while (node != m_objects.m_sentinel)
    {
        zShopListNode* const current = node;
        node = node->next;
        zShopListNode* next = 0;
        (void) ShopListErase(&m_objects, &next, current);
    }
    m_generated.m_end = m_generated.m_begin;
    if (!m_invServerRef.m_object) return;
    for (int inventoryId = 20; inventoryId <= 21; ++inventoryId)
    {
        iInventory* inventory = m_invServerRef.m_object->GetInv(inventoryId);
        if (inventory)
        {
            zInventory3D* complete = static_cast<zInventory3D*>(inventory->QueryInterface(48));
            complete->RemoveAllItems();
        }
    }
}

void zShopDlgMenu::TryToFindButtons()
{
    for (int i = 0; i < 9; ++i) m_categoryButtons[i] = 0;
    m_filterButtonCount = 0;
    MENU& menu = Map->MenuState();
    const int count = menu.m_count;
    for (int i = 0; i < count; ++i)
    {
        SPRITE* const sprite = menu.m_data[i];
        VID* const vid = sprite->Vid();
        if (vid->nVid == 997)
        {
            const int index = vid->RealDirection(sprite->Direction().value);
            if (index < 0 || index >= 9)
                DebugAssert(5u, "dir >= 0 && dir < BUTTONS_NUM", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 642u);
            if (static_cast<unsigned int>(index) < 9u) m_categoryButtons[index] = sprite;
            m_buttonsFound = true;
        }
        if (sprite->Vid()->nVid == 715)
        {
            m_filterButtons[m_filterButtonCount++] = sprite;
            m_buttonsFound = true;
        }
    }
    Sort(m_filterButtons, m_filterButtonCount);
}


void  zShopDlgMenu::SetRPGHero(iRPGHero* hero)
{
    m_rpgHeroRef.Set(hero);
    iInvKeeper* keeper = 0;
    if (m_rpgHeroRef.m_object)
        keeper = static_cast<iInvKeeper*>(m_rpgHeroRef.m_object->QueryInterface(1));
    m_keeperRef.Set(keeper);
    if (zScriptExec::m_pShopItemInfo && m_rpgHeroRef.m_object)
        zScriptExec::m_pShopItemInfo->SetInventoryKeeper(m_keeperRef.m_object);
}

static const OBJ_EXTTYPE categoryExtTypes[9] = {
    static_cast<OBJ_EXTTYPE>(1), static_cast<OBJ_EXTTYPE>(2), static_cast<OBJ_EXTTYPE>(3),
    static_cast<OBJ_EXTTYPE>(4), static_cast<OBJ_EXTTYPE>(5), static_cast<OBJ_EXTTYPE>(16),
    static_cast<OBJ_EXTTYPE>(9), static_cast<OBJ_EXTTYPE>(16), static_cast<OBJ_EXTTYPE>(16)
};
static const OBJ_TYPE categoryTypes[9] = {
    static_cast<OBJ_TYPE>(15), static_cast<OBJ_TYPE>(15), static_cast<OBJ_TYPE>(15),
    static_cast<OBJ_TYPE>(15), static_cast<OBJ_TYPE>(15), static_cast<OBJ_TYPE>(4),
    static_cast<OBJ_TYPE>(6), static_cast<OBJ_TYPE>(5), static_cast<OBJ_TYPE>(3)
};

void  zShopDlgMenu::ControlInput(INPUT* input)
{
    const VECTOR2 cursor(input->ClientMouseXY().x + Map->m_shiftXY.x,
        input->ClientMouseXY().y + Map->m_shiftXY.y);
    int hoveredCategory = -1;
    VID* const overlay = Map->Vid(3740);
    unsigned int overlaySprites = 0u;
    if (overlay)
        for (int index = 0; index < VID::PlayerSlotCount; ++index)
            overlaySprites += overlay->spriteCountsByPlayer[index];
    if (!overlay || overlaySprites == 0u)
    {
        for (int index = 0; index < 9; ++index)
        {
            if (m_categoryButtons[index] && MenuSpriteContainsPoint(m_categoryButtons[index], cursor))
            {
                hoveredCategory = index;
                break;
            }
        }
    }
    for (int index = 0; index < 9; ++index)
    {
        SPRITE* const tab = Map->MenuState().SpriteWithName(Printf("IS_TAB_%d", index + 1));
        if (tab) tab->Action(ACT_SET_INVISIBLE, index != hoveredCategory, 0, 0);
    }
    if (hoveredCategory != -1 && input->GetAction(IM_LBUTTONUP))
        LoadListOfObjects(categoryTypes[hoveredCategory], categoryExtTypes[hoveredCategory]);

    unsigned int action = input->GetAction(IM_KEYDOWN);
    if (!action) return;
    action >>= 8;
    const float halfWidth = static_cast<float>(static_cast<double>(Graph->screenWidth) * 0.5);
    const int inventoryId = input->ClientMouseXY().x < halfWidth &&
        input->ClientMouseXY().y < halfWidth ? 0 : 20;
    if (action == 33u) PageUp(inventoryId);
    else if (action == 34u) PageDown(inventoryId);
    else if (action == 36u) GoHome(inventoryId);
    else if (action == 35u) GoEnd(inventoryId);
}

void zShopDlgMenu::RefreshCategoryButtons()
{
    int selectedCategory = -1;
    for (int index = 0; index < 9; ++index)
    {
        if (categoryTypes[index] == m_objectType && categoryExtTypes[index] == m_extType)
        {
            selectedCategory = index;
            break;
        }
    }
    for (int index = 0; index < 9; ++index)
        if (m_categoryButtons[index])
            m_categoryButtons[index]->Action(ACT_SET_INVISIBLE, index != selectedCategory, 0, 0);
}

bool zShopDlgMenu::TryToSellItem(iRPGObj* item, unsigned int mode)
{
    if (!item)
    {
        DebugAssert(5u, "_pItem", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1432u);
        return false;
    }
    if (item->GetBuyCostFull() == 0) return false;
    if (item->GetNumInCell() <= 0)
        DebugAssert(4u, "_pItem->GetNumInCell() > 0", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1437u);
    const int soldCount = m_rpgHeroRef.m_object->TryToSellObjj(item, static_cast<unsigned char>(mode) != 0u);
    if (soldCount)
    {
        m_player->PlaySFx(45, false);
        iRPGObj* baseObject = item->GetObjCopy();
        baseObject->SetNumInCell(soldCount);
        zShopListNode* node = 0;
        ShopListInsert(&m_objects, &node, m_objects.m_sentinel, &baseObject);
        m_objectType = static_cast<OBJ_TYPE>(15);
        m_extType = static_cast<OBJ_EXTTYPE>(16);
        const OBJ_EXTTYPE extType = static_cast<OBJ_EXTTYPE>(baseObject->GetAddMoney());
        const OBJ_TYPE objectType = static_cast<OBJ_TYPE>(baseObject->GetGameObj());
        LoadListOfObjects(objectType, extType);
        m_heroInvMenu->ResetSelItem();
    }
    else
        m_player->PlaySFx(46, false);
    if (static_cast<unsigned char>(mode) != 0u) return soldCount != 0;
    return item->GetNumInCell() == 0;
}

int  zShopDlgMenu::TryToBuyItem(iRPGObj* _pItem)
{
    if (!_pItem || !m_rpgHeroRef.m_object)
        DebugAssert(5u, "_pItem && m_pRPGHero", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1068u);

    iRPGObj* moved = 0;
    const int buyResult = m_rpgHeroRef.m_object->TryToBuyObj(_pItem, &moved);
    if (buyResult)
    {
        m_player->PlaySFx(45, false);
        if (moved)
        {
            if (_pItem->GetNumInCell() == 0 && m_heroInvMenu->GetSelItem() == _pItem)
            {
                m_heroInvMenu->ResetSelItem();
                m_heroInvMenu->HighLightItem(moved);
            }
            return buyResult;
        }
        m_heroInvMenu->ResetSelItem();
        if (_pItem && _pItem->GetNumInCell() == 0)
            zScriptExec::m_pShopItemInfo->ShowObjInfo(0, false, 0);
        return buyResult;
    }

    m_player->PlaySFx(46, false);
    if (!m_player) return buyResult;
    iRPGHero* const moneyHero = m_rpgHeroRef.m_object;
    const int money = moneyHero->GetMoney();
    const int requiredMoney = _pItem->GetBuyCost();
    if (requiredMoney>money)
    {
        m_player->SendInfoMessage(3, 0);
        return buyResult;
    }
    if (!m_rpgHeroRef.m_object->CanBuyItem(_pItem))
    {
        m_player->SendInfoMessage(2, 0);
        return buyResult;
    }
    m_player->SendInfoMessage(1, 0);
    return buyResult;
}

zShopDlgMenu::zShopDlgMenu()
    : zBaseDlgMenu(), m_filterButtonCount(0), m_buttonsFound(false),
      m_generatedMode(false), m_framework(0), m_heroInvMenu(0),
      m_objectType(static_cast<OBJ_TYPE>(15)), m_extType(static_cast<OBJ_EXTTYPE>(16)),
      m_player(0), m_curLevel(0)
{
    for (int i = 0; i < 9; ++i) m_categoryButtons[i] = 0;
    for (int i = 0; i < 3; ++i) m_buyAnim[i] = 0;
}


zShopListLayout::zShopListLayout()
    : m_sentinel(static_cast<zShopListNode*>(CrtNew(sizeof(zShopListNode)))),
      m_size(0)
{
    m_sentinel->next = m_sentinel;
    m_sentinel->previous = m_sentinel;
}

zShopObjectPointerVector::~zShopObjectPointerVector()
{
    if (m_begin)
        CrtFreePointer(m_begin);
    m_begin = m_end = m_capEnd = 0;
}

zShopListLayout::~zShopListLayout()
{
    zShopListNode* const root = m_sentinel;
    zShopListNode* iterator = root->next;
    while (iterator != root)
    {
        zShopListNode* const node = iterator;
        iterator = node->next;
        zShopListNode* const next = node->next;
        zShopListNode* const previous = node->previous;
        previous->next = next;
        next->previous = previous;
        CrtFreePointer(node);
        --m_size;
    }
    CrtFreePointer(root);
    m_sentinel = 0;
    m_size = 0;
}

zShopDlgMenu::~zShopDlgMenu()
{
    ReleaseListOfObjects();
}

bool zShopDlgMenu::InitLoad()
{
    if (m_buttonsFound) return false;
    TryToFindButtons();
    if (!m_buttonsFound) return false;
    LoadListOfObjects(categoryTypes[0], categoryExtTypes[0]);
    m_framework->ResetShopItemFlags();
    for (zShopListNode* node = m_objects.m_sentinel->next;
        node != m_objects.m_sentinel; node = node->next)
        static_cast<zItem*>(node->value->QueryInterface(49))->m_priceFlags |= 1u;
    RefreshBuyButtons();
    return true;
}


void zShopDlgMenu::ReloadAll()
{
}







void  zShopDlgMenu::RefreshItems()
{
    RefreshBuyButtons();
}

zShopListNode**  ShopListErase(
    zShopListLayout* list, zShopListNode** outNext, zShopListNode* erased)
{
    zShopListNode* const next = erased->next;
    zShopListNode* const previous = erased->previous;
    previous->next = next;
    next->previous = previous;
    CrtFreePointer(erased);
    --list->m_size;
    *outNext = next;
    return outNext;
}

zShopListNode**  ShopListInsert(
    zShopListLayout* list, zShopListNode** outNode, zShopListNode* next,
    iRPGObj* const* itemReference)
{
    zShopListNode* const previous = next->previous;
    zShopListNode* const node = static_cast<zShopListNode*>(CrtNew(sizeof(zShopListNode)));
    node->next = next?next:node;
    node->previous = previous?previous:node;
    next->previous = node;
    node->previous->next = node;
    node->value = *itemReference;
    ++list->m_size;
    *outNode = node;
    return outNode;
}
static void ShopGeneratedPushBack(zShopDlgMenu* shop, iRPGObj* object)
{
    zShopObjectPointerVector& vector = shop->m_generated;
    iRPGObj** begin = vector.m_begin;
    iRPGObj** end = vector.m_end;
    iRPGObj** capacity = vector.m_capEnd;
    if (end != capacity)
    {
        if (end) *end = object;
        vector.m_end = end+1;
        return;
    }

    const unsigned int oldCount = begin?static_cast<unsigned int>(end-begin):0u;
    const unsigned int oldCapacity = begin?static_cast<unsigned int>(capacity-begin):0u;
    if (oldCount > 0x3ffffffeu)
        throw std::length_error("vector<T> too long");
    const unsigned int half = oldCapacity / 2u;
    unsigned int newCapacity = oldCapacity > 0x3fffffffu - half ? 0u : oldCapacity + half;
    if (newCapacity < oldCount + 1u)
        newCapacity = oldCount + 1u;
    iRPGObj** newBegin = static_cast<iRPGObj**>(CrtNew(newCapacity*sizeof(iRPGObj*)));
    if (!newBegin)
        throw std::bad_alloc();
    iRPGObj** destination = newBegin;
    for (iRPGObj** source = begin; source != end; ++source, ++destination)
        if (destination) *destination = *source;
    if (destination) *destination = object;
    ++destination;
    if (begin)
        CrtFreePointer(begin);
    vector.m_begin = newBegin;
    vector.m_end = destination;
    vector.m_capEnd = newBegin+newCapacity;
}

static void ShopListPushBack(zShopListLayout* list, iRPGObj* const* object)
{
    zShopListNode* node = 0;
    (void) ShopListInsert(list, &node, list->m_sentinel, object);
}

namespace
{
class ShopAmmoKeys
{
    struct Node {
        Node* next;
        Node* previous;
        int key;
    };
    Node* m_head;

public:
    ShopAmmoKeys()
        : m_head(static_cast<Node*>(CrtNew(sizeof(Node))))
    {
        m_head->next = m_head;
        m_head->previous = m_head;
    }

    ~ShopAmmoKeys()
    {
        Node* node = m_head->next;
        while (node != m_head)
        {
            Node* const next = node->next;
            node->previous->next = next;
            next->previous = node->previous;
            CrtFreePointer(node);
            node = next;
        }
        CrtFreePointer(m_head);
    }

    void PushBack(int key)
    {
        Node* const node = static_cast<Node*>(CrtNew(sizeof(Node)));
        node->next = m_head;
        node->previous = m_head->previous;
        m_head->previous = node;
        node->previous->next = node;
        node->key = key;
    }


};
}

bool  zShopDlgMenu::GenerateItems()
{
    iRPGObj** generatedBegin = m_generated.m_begin;
    iRPGObj** generatedEnd = m_generated.m_end;
    if (generatedBegin && generatedEnd != generatedBegin)
        DebugAssert(5u, "m_aItemsGen.empty()", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1476u);
    if (!m_rpgHeroRef.m_object)
        DebugAssert(5u, "m_pRPGHero", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1477u);

    iRPGHeroBase* const seedSource = static_cast<iRPGHeroBase*>(m_rpgHeroRef.m_object->QueryInterface(3));
    unsigned int seed = seedSource->GetSRandVal(true);
    seed += static_cast<unsigned int>(m_curLevel);
    iRPGImplant::SeedImplantRand(BitsToSigned32(seed));

    iRPGObj* baseItem = m_framework->FirstObject(1, 0, 0);
    while (baseItem)
    {
        if (baseItem->GetGameObj() == TYPE_IMPLANT && baseItem->GetBuyCostFull()>0)
        {
            const int generatedItemCount = baseItem->GetMaxShop(m_curLevel);
            if (generatedItemCount>0)
            {
                for (int generatedItemIndex = 0; generatedItemIndex<generatedItemCount; ++generatedItemIndex)
                {
                    iRPGObj* const baseObject = baseItem->GetObjCopy();
                    ShopGeneratedPushBack(this, baseObject);
                }
            }
        }
        baseItem = m_framework->NextObject();
    }
    iRPGImplant::SeedImplantRand(0);
    return true;
}

void zShopDlgMenu::RefreshBuyButtons()
{
    for (int i = 0; i < 3; ++i)
    {
        zScriptExec::m_pShopItemInfo->GetButton(i);
        zScriptExec::m_pShopItemInfo->GetButtonText(i);
    }
    iRPGObj* const item = zScriptExec::m_pShopItemInfo ?
        zScriptExec::m_pShopItemInfo->GetCurrentItem() : 0;
    if (!item || item->GetInvId() == -1)
    {
        for (int i = 0; i < 3; ++i)
        {
            SetItemButton(i, false, true, false, STRING(""), 0);
            m_buyAnim[i] = 0;
        }
        return;
    }
    std::vector<int> actions;
    bool ammoAvailable = true;
    bool affordable = true;
    const bool buying = m_heroInvMenu->GetBuyMode(item);
    if (m_heroInvMenu->IsShopMode())
    {
        if (item->GetGameObj() != 7)
        {
            actions.push_back(buying ? 8 : 1);
            if (item->GetBuyCost() > m_rpgHeroRef.m_object->GetMoney()) affordable = false;
        }
        if (item->GetGameObj() == TYPE_WEAPON)
        {
            zRPGAmmo* const ammo = static_cast<zRPGWeapon*>(item->QueryInterface(50))->GetAmmo();
            if (ammo && !static_cast<iRPGObj*>(ammo)->IsInfinite())
            {
                actions.push_back(5);
                ammoAvailable = BuyAmmoForWeapon(item, true);
            }
        }
        if (item->GetGameObj() == TYPE_ARMOR && !buying &&
            static_cast<iRPGArmor*>(item->QueryInterface(51))->GetRepairCost() != 0)
            actions.push_back(4);
    }
    if (item->GetGameObj() != 7 && item->GetGameObj() != TYPE_AMMO && !buying && item->GetInvId() != 19)
        actions.push_back(item->GetInvId() == 0 ? 2 : 3);
    if (!m_heroInvMenu->IsShopMode() && item->GetGameObj() != 7)
    {
        if (item->GetInvId() == 19) actions.push_back(7);
        else if (item->GetGameObj() != TYPE_AMMO) actions.push_back(6);
    }
    for (int end = static_cast<int>(actions.size()) - 1; end >= 0; --end)
    {
        int swapIndex = end;
        for (int i = 0; i < end; ++i)
            if (actions[i] < actions[end]) swapIndex = i;
        const int saved = actions[end];
        actions[end] = actions[swapIndex];
        actions[swapIndex] = saved;
    }
    if (actions.size() > 3u)
        DebugAssert(5u, "aActs.size() <= IF_BTNS_CNT", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 557u);
    int slot = 2;
    for (int i = static_cast<int>(actions.size()) - 1; i >= 0 && slot >= 0; --i, --slot)
    {
        const int action = actions[i];
        m_buyAnim[slot] = action;
        const bool disabled = action == 5 && !ammoAvailable;
        const bool unaffordable = action == 8 && !affordable;
        int cost = 0;
        if (action == 8) cost = SubSigned32Wrap(0, item->GetBuyCost());
        if (action == 1) cost = item->GetBuyCost();
        if (action == 4)
            cost = SubSigned32Wrap(0, static_cast<iRPGArmor*>(item->QueryInterface(51))->GetRepairCost());
        SetItemButton(slot, true, disabled, unaffordable, GetItemActionText(action), cost);
    }
    for (; slot >= 0; --slot)
    {
        m_buyAnim[slot] = 0;
        SetItemButton(slot, false, true, false, STRING(""), 0);
    }
}

STRING zShopDlgMenu::GetItemActionText(int action)
{
    static const char* const keys[9] = {"", "Sell", "IF_Wear", "IF_UnWear", "Repair",
        "BuyAmmo", "If_Drop", "If_Pickup", "Buy"};
    if (static_cast<unsigned int>(action) > 8u || action == 0) return STRING("");
    return ::Strings->GetString(STRING("menu"), STRING(keys[action]), STRING(""));
}

void zShopDlgMenu::SetItemButton(int index, bool visible, bool disabled, bool unaffordable,
    STRING text, int cost)
{
    if (index < 0 || index >= 3)
        DebugAssert(5u, "_iNum >= 0 && _iNum < IF_BTNS_CNT", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 340u);
    SPRITE* const button = zScriptExec::m_pShopItemInfo->GetButton(index);
    if (button)
    {
        button->Action(ACT_SET_INVISIBLE, visible ? 0 : 1, 0, 0);
        if (!visible || disabled) button->ChangeAnimation(12);
        else if (button->Animation() == 12) button->ChangeAnimation(0);
    }
    SPRITE* const label = Map->MenuState().SpriteWithName(Printf("IF_BtnText_%d", index + 1));
    if (label)
    {
        label->Action(ACT_SET_TEXT, BitsToSigned32(PtrToU32(&text)), 0, 0);
        GAMMA gamma;
        gamma.modulate = 0;
        gamma.add = disabled ? 0x646464u : (unaffordable ? 0xFF0000u : 0u);
        label->SetGamma(gamma);
    }
    SPRITE* const price = Map->MenuState().SpriteWithName(Printf("IF_BtnCost_%d", index + 1));
    if (price)
    {
        STRING value("");
        if (cost > 0) value = Printf("$%d", cost);
        else if (cost < 0) value = Printf("-$%d", SubSigned32Wrap(0, cost));
        price->Action(ACT_SET_TEXT, BitsToSigned32(PtrToU32(&value)), 0, 0);
        if (label && cost != 0)
        {
            VECTOR coor = price->Coor();
            coor.y = static_cast<float>(static_cast<double>(coor.y) - 11.0);
            label->ChangeCoor(coor);
            label->Action(ACT_SET_BEHAVE, 9, 0, 0);
        }
    }
}

void zShopDlgMenu::ProcessItemButton(SPRITE* button)
{
    for (int i = 0; i < 3; ++i)
    {
        if (button == zScriptExec::m_pShopItemInfo->GetButton(i))
        {
            ExecuteItemAction(m_buyAnim[i]);
            return;
        }
    }
    DebugAssert(4u, "0", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 285u);
}

void zShopDlgMenu::ExecuteItemAction(int action)
{
    if (!m_heroInvMenu || !zScriptExec::m_pShopItemInfo)
        DebugAssert(5u, "m_pHeroInvMenu && g_pShopItemInfo", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1126u);
    iRPGObj* const item = zScriptExec::m_pShopItemInfo->GetCurrentItem();
    bool close = false;
    if (action == 8)
        close = (TryToBuyItem(item) & 1) != 0;
    else if (action == 1)
        close = TryToSellItem(item, 0u);
    else if (action == 4)
    {
        if (item->GetGameObj() != TYPE_ARMOR)
            DebugAssert(5u, "item->GetType() == TYPE_ARMOR", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1141u);
        if (item->GetGameObj() == TYPE_ARMOR)
        {
            iRPGArmor* const armor = static_cast<iRPGArmor*>(item->QueryInterface(51));
            iRPGHero* const hero = m_rpgHeroRef.m_object;
            hero->AddMoney(SubSigned32Wrap(0, armor->Repair(hero->GetMoney())), false);
            zScriptExec::m_pShopItemInfo->Refresh();
        }
    }
    else if (action == 5)
        BuyAmmoForWeapon(item, false);
    else if (action == 2 || action == 3)
    {
        m_invServerRef.m_object->PutToActiveInv(item);
        close = true;
    }
    else if (action == 6)
    {
        if (m_invServerRef.m_object->TakeItem(item))
        {
            zInventory* const ground = static_cast<zInventory*>(m_invServerRef.m_object->GetInv(19)->QueryInterface(46));
            close = ground->PutItem(static_cast<zItem*>(item->QueryInterface(49)), 0) != 0;
        }
    }
    else if (action == 7)
    {
        if (m_invServerRef.m_object->TakeItem(item))
        {
            zInventory* const main = static_cast<zInventory*>(m_invServerRef.m_object->GetInv(0)->QueryInterface(46));
            if (!(main->PutItem(static_cast<zItem*>(item->QueryInterface(49)), 0) & 1) && item->GetNumInCell() > 0)
            {
                m_rpgHeroRef.m_object->CreateGroundSprite(item);
                m_invServerRef.m_object->ControlGrndInventory(0xFFFFFFFFu);
            }
        }
        close = true;
    }
    else
        DebugAssert(5u, "0", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1186u);
    if (close) zScriptExec::m_pShopItemInfo->ShowObjInfo(0, false, 0);
    RefreshAll();
}


void  zShopDlgMenu::LoadListOfObjects(OBJ_TYPE objectTypeEnum, OBJ_EXTTYPE extTypeEnum)
{
    unsigned int objectType = static_cast<unsigned int>(objectTypeEnum);
    unsigned int extType = static_cast<unsigned int>(extTypeEnum);

    if (objectType == 2u && extType != 16u) objectType = 15u;
    else if (objectType == 5u) extType = 16u;
    if (objectType == 15u && extType == 16u)
        DebugAssert(5u, "_eObjType != TYPE_UNKNOWN || _eObjExtType != EXTTYPE_UNKNOWN", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 667u);

    if (static_cast<unsigned int>(m_objectType) == objectType)
    {
        const unsigned int oldExt = static_cast<unsigned int>(m_extType);
        if (oldExt == extType) return;
        if (oldExt >= 9u && oldExt <= 13u && extType >= 9u && extType <= 13u) return;
    }

    (void) ::gpLogCons->LogStr(LT_INFO, "LoadListOfObjects-begin");

    iRPGObj** const generatedBegin = m_generated.m_begin;
    if (!generatedBegin || m_generated.m_end == generatedBegin) GenerateItems();

    m_objectType = static_cast<OBJ_TYPE>(objectType);
    m_extType = static_cast<OBJ_EXTTYPE>(extType);
    ReloadAll();
    m_generatedMode = objectType == TYPE_IMPLANT;

    zShopListLayout* const shopList = &m_objects;
    if (m_objects.m_size == 0u)
    {
        for (unsigned int i = 0u; m_generated.m_begin &&
            i<static_cast<unsigned int>(m_generated.m_end-m_generated.m_begin); ++i)
        {
            iRPGObj* const object = m_generated.m_begin[i];
            if (object->GetAddLevel() <= m_curLevel)
                ShopListPushBack(shopList, &m_generated.m_begin[i]);
        }

        iRPGObj* object = m_framework->FirstObject(1, 0, 0);
        while (object)
        {
            if (object->GetBaseCostFull()>0 &&
                object->GetAddLevel() <= m_curLevel &&
                !object->ItemIsNotReal() &&
                object->GetGameObj() != TYPE_IMPLANT)
            {
                if (object->GetGameObj() != TYPE_AMMO)
                {
                    const int saved = object->GetNumInCell();
                    object->SetNumInCell(object->GetMaxNumInCell());
                    int count = object->GetMaxShop(m_curLevel);
                    if (count>0)
                    {
                        do
                        {
                            iRPGObj* const copy = object->GetObjCopy();
                            ShopListPushBack(shopList, &copy);
                            --count;
                        }
                        while (count != 0);
                    }
                    object->SetNumInCell(saved);
                }
                else
                {
                    const int randomRange = object->GetShopNum(1);
                    const int randomCount = Random(randomRange);
                    int count = AddSigned32Wrap(randomCount, object->GetShopNum(0));
                    if (count>0)
                    {
                        do
                        {
                            iRPGObj* const baseObject = object->GetObjCopy();
                            baseObject->SetNumInCell(baseObject->GetMaxNumInCell());
                            ShopListPushBack(shopList, &baseObject);
                            --count;
                        }
                        while (count != 0);
                    }
                }
            }
            object = m_framework->NextObject();
        }
    }

    iHeroInvMenu* const heroMenu = m_heroInvMenu;
    iRPGObj* const selected = heroMenu->GetSelItem();
    if (selected &&
        (selected->GetInvId() == 20 || selected->GetInvId() == 21))
        m_heroInvMenu->ResetSelItem();

    zInventory3D* inventory14 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(20)->QueryInterface(48));
    int inventoryCount = inventory14->GetItemCount();
    for (int i = 0; i<inventoryCount; ++i)
    {
        iRPGObj* const item = inventory14->GetItem(i);
        ShopListPushBack(shopList, &item);
    }
    inventory14->RemoveAllItems();

    zInventory3D* inventory15 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(21)->QueryInterface(48));
    inventoryCount = inventory15->GetItemCount();
    for (int i = 0; i<inventoryCount; ++i)
    {
        iRPGObj* const item = inventory15->GetItem(i);
        ShopListPushBack(shopList, &item);
    }
    inventory15->RemoveAllItems();

    inventory14 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(20)->QueryInterface(48));
    std::vector<iRPGObj*> displayItems;
    ShopAmmoKeys weaponAmmoKeys;
    zShopListNode* sentinel = m_objects.m_sentinel;
    zShopListNode* node = sentinel->next;
    while (node != sentinel)
    {
        if (!node->value) DebugAssert(5u, "*j != NULL", "..\\src\\zm\\Menu\\zShopDlgMenu.cpp", 773u);

        bool matches = false;
        if (static_cast<unsigned int>(m_extType) != 16u)
        {
            const int detail = node->value->GetAddMoney();
            if (static_cast<unsigned int>(detail) == static_cast<unsigned int>(m_extType)) matches = true;
        }
        if (!matches && static_cast<unsigned int>(m_objectType) != 15u)
        {
            const int type = node->value->GetGameObj();
            if (static_cast<unsigned int>(type) == static_cast<unsigned int>(m_objectType)) matches = true;
        }
        if (matches)
        {
            iRPGObj* transferObject = node->value;
            if (transferObject->GetGameObj() != TYPE_IMPLANT) transferObject = node->value->GetObjCopy();
            displayItems.push_back(transferObject);

            zShopListNode* const next = node->next;
            node->previous->next = next;
            next->previous = node->previous;
            CrtFreePointer(node);
            --m_objects.m_size;
            node = next;
            continue;
        }
        node = node->next;
    }

    for (int last = static_cast<int>(displayItems.size()) - 1; last >= 0; --last)
    {
        int selected = last;
        for (int candidate = 0; candidate < last; ++candidate)
        {
            const int selectedCost = displayItems[selected]->GetBaseCostFull();
            if (displayItems[candidate]->GetBaseCostFull() > selectedCost)
                selected = candidate;
        }
        if (selected != last)
        {
            iRPGObj* const saved = displayItems[last];
            displayItems[last] = displayItems[selected];
            displayItems[selected] = saved;
        }
    }

    for (std::vector<iRPGObj*>::const_iterator item = displayItems.begin();
        item != displayItems.end(); ++item)
    {
        zItem* const inventoryItem = static_cast<zItem*>((*item)->QueryInterface(49));
        (void) inventory14->PutItem(inventoryItem, static_cast<zItem**>(0));
        if ((*item)->GetGameObj() == TYPE_WEAPON)
        {
            zRPGWeapon* const weapon = static_cast<zRPGWeapon*>((*item)->QueryInterface(50));
            zRPGAmmo* const ammo = weapon->GetAmmo();
            iRPGObj* const ammoObject = static_cast<zRPGObj*>(ammo);
            if (!ammoObject->IsInfinite())
                weaponAmmoKeys.PushBack(ammoObject->GetBaseId());
        }
    }

    int cellX = 37;
    int cellY = 37;
    int rows = 7;
    if (extType == 1u || objectType == 5u || objectType == 6u || objectType == 3u)
    {
        cellX = 50;
        cellY = 50;
        rows = 5;
    }
    else if (extType == 2u || extType == 3u || extType == 4u || extType == 5u)
    {
        cellX = 41;
        cellY = 50;
        rows = 6;
    }
    else if (objectType == 4u)
    {
        cellX = 42;
        cellY = 50;
        rows = 6;
    }

    inventory14 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(20)->QueryInterface(48));
    inventory14->SetCellSize(cellX, cellY);
    zInventory2D* const grid2D = inventory14;
    (void) static_cast<iInventory2D*>(grid2D)->SetSizeInCells(rows, grid2D->m_gridHeight);
    GoHome(20);
    RefreshBuyButtons();
    RefreshCategoryButtons();
    (void) ::gpLogCons->LogStr(LT_INFO, "LoadListOfObjects-end");
}


