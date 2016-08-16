/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#include "engine.h"
#include "card.h"
#include "settings.h"
#include "scenario.h"
#include "audio.h"
#include "structs.h"
#include "lua-wrapper.h"
#include "roomstate.h"
#include "banpair.h"

#include <lua.hpp>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QFile>

QSgsEngine *Sanguosha = nullptr;

int QSgsEngine::getMiniSceneCounts()
{
    return m_miniScenes.size();
}

void QSgsEngine::_loadMiniScenarios()
{
    static bool loaded = false;
    if (loaded) return;
    int i = 1;
    while (true) {
        if (!QFile::exists(QString("etc/customScenes/%1.txt").arg(QString::number(i))))
            break;

        QString sceneKey = QString(MiniScene::S_KEY_MINISCENE).arg(QString::number(i));
        m_miniScenes[sceneKey] = new LoadedScenario(QString::number(i));
        i++;
    }
    loaded = true;
}

void QSgsEngine::_loadModScenarios()
{
    //wait for a new scenario
    addScenario(new JiangeDefenseScenario());
}

void QSgsEngine::addPackage(const QString &name)
{
    Package *pack = PackageAdder::packages()[name];
    if (pack)
        addPackage(pack);
    else
        qWarning("Package %s cannot be loaded!", qPrintable(name));
}

QSgsEngine::QSgsEngine()
{
    Sanguosha = this;

    lua = CreateLuaState();
    DoLuaScript(lua, "lua/config.lua");

    QStringList stringlist_sp_convert = GetConfigFromLuaState(lua, "convert_pairs").toStringList();
    foreach (const QString &cv_pair, stringlist_sp_convert) {
        QStringList pairs = cv_pair.split("->");
        QStringList cv_to = pairs.at(1).split("|");
        foreach(const QString &to, cv_to)
            sp_convert_pairs.insertMulti(pairs.at(0), to);
    }

    QStringList package_names = GetConfigFromLuaState(lua, "package_names").toStringList();
    foreach(const QString &name, package_names)
        addPackage(name);

    metaobjects.insert("TransferCard", &TransferCard::staticMetaObject);

    transfer = new TransferSkill;

    _loadMiniScenarios();
    _loadModScenarios();
    m_customScene = new CustomScenario();

    DoLuaScript(lua, "lua/sanguosha.lua");

    // available game modes
    modes["02p"] = tr("2 players");
    modes["03p"] = tr("3 players");
    modes["04p"] = tr("4 players");
    modes["05p"] = tr("5 players");
    modes["06p"] = tr("6 players");
    modes["07p"] = tr("7 players");
    modes["08p"] = tr("8 players");
    modes["09p"] = tr("9 players");
    modes["10p"] = tr("10 players");

    BanPair::loadBanPairs();

    connect(qApp, &QApplication::aboutToQuit, this, &QSgsEngine::deleteLater);

    foreach (const Skill *skill, skills) {
        Skill *mutable_skill = const_cast<Skill *>(skill);
        mutable_skill->initMediaSource();
    }
}

lua_State *QSgsEngine::getLuaState() const
{
    return lua;
}

void QSgsEngine::addTranslationEntry(const char *key, const char *value)
{
    translations.insert(key, QString::fromUtf8(value));
}

QSgsEngine::~QSgsEngine()
{
    lua_close(lua);
    delete m_customScene;
    delete transfer;
#ifdef AUDIO_SUPPORT
    Audio::quit();
#endif

    foreach (ExpPattern * const &pattern, enginePatterns)
        delete pattern;
}

QStringList QSgsEngine::getModScenarioNames() const
{
    return m_scenarios.keys();
}

void QSgsEngine::addScenario(Scenario *scenario)
{
    QString key = scenario->objectName();
    if (m_scenarios.contains(key))
        return;

    m_scenarios[key] = scenario;
    addPackage(scenario);
}

const Scenario *QSgsEngine::getScenario(const QString &name) const
{
    if (m_scenarios.contains(name))
        return m_scenarios[name];
    else if (m_miniScenes.contains(name))
        return m_miniScenes[name];
    else if (name == "custom_scenario")
        return m_customScene;
    else
        return nullptr;
}

void QSgsEngine::addSkills(const QList<const Skill *> &all_skills)
{
    foreach (const Skill *skill, all_skills) {
        if (!skill) {
            qWarning(tr("The engine tries to add an invalid skill"));
            continue;
        }
        if (skills.contains(skill->objectName()))
            qWarning(tr("Duplicated skill : %1").arg(skill->objectName()));

        skills.insert(skill->objectName(), skill);

        if (skill->inherits("ProhibitSkill"))
            prohibit_skills << qobject_cast<const ProhibitSkill *>(skill);
        else if (skill->inherits("DistanceSkill"))
            distance_skills << qobject_cast<const DistanceSkill *>(skill);
        else if (skill->inherits("MaxCardsSkill"))
            maxcards_skills << qobject_cast<const MaxCardsSkill *>(skill);
        else if (skill->inherits("TargetModSkill"))
            targetmod_skills << qobject_cast<const TargetModSkill *>(skill);
        else if (skill->inherits("AttackRangeSkill"))
            attackrange_skills << qobject_cast<const AttackRangeSkill *>(skill);
        else if (skill->inherits("TriggerSkill")) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            if (trigger_skill && trigger_skill->isGlobal())
                global_trigger_skills << trigger_skill;
        }
    }
}

QList<const DistanceSkill *> QSgsEngine::getDistanceSkills() const
{
    return distance_skills;
}

QList<const MaxCardsSkill *> QSgsEngine::getMaxCardsSkills() const
{
    return maxcards_skills;
}

QList<const TargetModSkill *> QSgsEngine::getTargetModSkills() const
{
    return targetmod_skills;
}

QList<const AttackRangeSkill *> QSgsEngine::getAttackRangeSkills() const
{
    return attackrange_skills;
}

QList<const TriggerSkill *> QSgsEngine::getGlobalTriggerSkills() const
{
    return global_trigger_skills;
}

void QSgsEngine::addPackage(Package *package)
{
    foreach (const Package *p, packages) {
        if (p->objectName() == package->objectName())
            return;
    }

    packages << package;
    package->setParent(this);
    sp_convert_pairs.unite(package->getConvertPairs());
    patterns.unite(package->getPatterns());
    related_skills.unite(package->getRelatedSkills());

    QList<Card *> all_cards = package->findChildren<Card *>();
    foreach (Card *card, all_cards) {
        card->setId(cards.length());
        cards << card;

//        if (card->isKindOf("LuaBasicCard")) {
//            const LuaBasicCard *lcard = qobject_cast<const LuaBasicCard *>(card);
//            Q_ASSERT(lcard != nullptr);
//            //luaBasicCard_className2objectName.insert(lcard->getClassName(), lcard->objectName());
//            if (!luaBasicCards.contains(lcard->getClassName()))
//                luaBasicCards.insert(lcard->getClassName(), lcard->clone());
//        } else if (card->isKindOf("LuaTrickCard")) {
//            const LuaTrickCard *lcard = qobject_cast<const LuaTrickCard *>(card);
//            Q_ASSERT(lcard != nullptr);
//            //luaTrickCard_className2objectName.insert(lcard->getClassName(), lcard->objectName());
//            if (!luaTrickCards.contains(lcard->getClassName()))
//                luaTrickCards.insert(lcard->getClassName(), lcard->clone());
//        } else if (card->isKindOf("LuaWeapon")) {
//            const LuaWeapon *lcard = qobject_cast<const LuaWeapon *>(card);
//            Q_ASSERT(lcard != nullptr);
//            //luaWeapon_className2objectName.insert(lcard->getClassName(), lcard->objectName());
//            if (!luaWeapons.contains(lcard->getClassName()))
//                luaWeapons.insert(lcard->getClassName(), lcard->clone());
//        } else if (card->isKindOf("LuaArmor")) {
//            const LuaArmor *lcard = qobject_cast<const LuaArmor *>(card);
//            Q_ASSERT(lcard != nullptr);
//            //luaArmor_className2objectName.insert(lcard->getClassName(), lcard->objectName());
//            if (!luaArmors.contains(lcard->getClassName()))
//                luaArmors.insert(lcard->getClassName(), lcard->clone());
//        } else if (card->isKindOf("LuaTreasure")) {
//            const LuaTreasure *lcard = qobject_cast<const LuaTreasure *>(card);
//            Q_ASSERT(lcard != nullptr);
//            //luaTreasure_className2objectName.insert(lcard->getClassName(), lcard->objectName());
//            if (!luaTreasures.contains(lcard->getClassName()))
//                luaTreasures.insert(lcard->getClassName(), lcard->clone());
//        } else {
            QString class_name = card->metaObject()->className();
            metaobjects.insert(class_name, card->metaObject());
            className2objectName.insert(class_name, card->objectName());
        //}
    }

    addSkills(package->getSkills());

    QList<General *> all_generals = package->findChildren<General *>();
    foreach (General *general, all_generals) {
        addSkills(general->findChildren<const Skill *>());
        foreach (const QString &skill_name, general->getExtraSkillSet()) {
            if (skill_name.startsWith("#")) continue;
            foreach(const Skill *related, getRelatedSkills(skill_name))
                general->addSkill(related->objectName());
        }
        generalList << general;
        generalHash.insert(general->objectName(), general);
        if (isGeneralHidden(general->objectName())) continue;
        if (general->isLord()) lord_list << general->objectName();
    }

    QList<const QMetaObject *> metas = package->getMetaObjects();
    foreach(const QMetaObject *meta, metas)
        metaobjects.insert(meta->className(), meta);
}

void QSgsEngine::addBanPackage(const QString &package_name)
{
    ban_package.insert(package_name);
}

QList<const Package *> QSgsEngine::getPackages() const
{
    return packages;
}

QStringList QSgsEngine::getBanPackages() const
{
    if (qApp->arguments().contains("-server"))
        return Config.BanPackages;
    else
        return ban_package.toList();
}

QString QSgsEngine::translate(const QString &toTranslate) const
{
    QStringList list = toTranslate.split("\\");
    QString res;
    foreach(const QString &str, list)
        res.append(translations.value(str, str));
    return res;
}

QString QSgsEngine::translate(const QString &toTranslate, const QString &defaultValue) const
{
    return translations.value(toTranslate, defaultValue);
}

const CardPattern *QSgsEngine::getPattern(const QString &name) const
{
    const CardPattern *ptn = patterns.value(name, nullptr);
    if (ptn) return ptn;

    ExpPattern *expptn = new ExpPattern(name);
    enginePatterns << expptn;
    patterns.insert(name, expptn);

    return expptn;
}

bool QSgsEngine::matchExpPattern(const QString &pattern, const Player *player, const Card *card) const
{
    ExpPattern p(pattern);
    return p.match(player, card);
}

Card::HandlingMethod QSgsEngine::getCardHandlingMethod(const QString &method_name) const
{
    if (method_name == "use")
        return Card::MethodUse;
    else if (method_name == "response")
        return Card::MethodResponse;
    else if (method_name == "discard")
        return Card::MethodDiscard;
    else if (method_name == "recast")
        return Card::MethodRecast;
    else if (method_name == "pindian")
        return Card::MethodPindian;
    else if (method_name == "none")
        return Card::MethodNone;
    else {
        Q_ASSERT(false);
        return Card::MethodNone;
    }
}

QList<const Skill *> QSgsEngine::getRelatedSkills(const QString &skill_name) const
{
    QList<const Skill *> skills;
    foreach(const QString &name, related_skills.values(skill_name))
        skills << getSkill(name);

    return skills;
}

const Skill *QSgsEngine::getMainSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if (!skill || skill->isVisible() || related_skills.contains(skill_name)) return skill;
    foreach (const QString &key, related_skills.keys()) {
        foreach(const QString &name, related_skills.values(key))
            if (name == skill_name) return getSkill(key);
    }
    return skill;
}

const General *QSgsEngine::getGeneral(const QString &name) const
{
    if (generalHash.contains(name))
        return generalHash.value(name);
    else
        return nullptr;
}

int QSgsEngine::getGeneralCount(bool include_banned) const
{
    if (include_banned)
        return generalList.size();

    int total = generalList.size();
    foreach (const General *general, generalList) {
        if (getBanPackages().contains(general->getPackage()))
            total--;
    }

    return total;
}

//void QSgsEngine::registerRoom(QObject *room)
//{
//    m_mutex.lock();
//    m_rooms[QThread::currentThread()] = room;
//    m_mutex.unlock();
//}

//void QSgsEngine::unregisterRoom()
//{
//    m_mutex.lock();
//    m_rooms.remove(QThread::currentThread());
//    m_mutex.unlock();
//}

//QObject *QSgsEngine::currentRoomObject()
//{
//    QObject *room = nullptr;
//    m_mutex.lock();
//    room = m_rooms[QThread::currentThread()];
//    Q_ASSERT(room);
//    m_mutex.unlock();
//    return room;
//}

//Room *QSgsEngine::currentRoom()
//{
//    QObject *roomObject = currentRoomObject();
//    Room *room = qobject_cast<Room *>(roomObject);
//    Q_ASSERT(room != nullptr);
//    return room;
//}

//RoomState *QSgsEngine::currentRoomState()
//{
//    QObject *roomObject = currentRoomObject();
//    Room *room = qobject_cast<Room *>(roomObject);
//    if (room != nullptr) {
//        return room->getRoomState();
//    } else {
//        Client *client = qobject_cast<Client *>(roomObject);
//        Q_ASSERT(client != nullptr);
//        return client->getRoomState();
//    }
//}

//QString QSgsEngine::getCurrentCardUsePattern()
//{
//    return currentRoomState()->getCurrentCardUsePattern();
//}

//CardUseStruct::CardUseReason QSgsEngine::getCurrentCardUseReason()
//{
//    return currentRoomState()->getCurrentCardUseReason();
//}

bool QSgsEngine::isGeneralHidden(const QString &general_name) const
{
    const General *general = getGeneral(general_name);
    if (!general) return false;
    if (!general->isHidden())
        return Config.ExtraHiddenGenerals.contains(general_name);
    else
        return !Config.RemovedHiddenGenerals.contains(general_name);
}

TransferSkill *QSgsEngine::getTransfer() const
{
   // return transfer;
    return nullptr;
}

WrappedCard *QSgsEngine::getWrappedCard(int cardId)
{
    Card *card = getCard(cardId);
    WrappedCard *wrappedCard = qobject_cast<WrappedCard *>(card);
    Q_ASSERT(wrappedCard != nullptr && wrappedCard->id() == cardId);
    return wrappedCard;
}

Card *QSgsEngine::getCard(int cardId)
{
//    Card *card = nullptr;
//    if (cardId < 0 || cardId >= m_cards.length())
//        return nullptr;
//    //QObject *room = currentRoomObject();
//   // Q_ASSERT(room);
//    //Room *serverRoom = qobject_cast<Room *>(room);
//   // if (serverRoom != nullptr) {
//       // card = serverRoom->getCard(cardId);
//   // } else {
//        Client *clientRoom = qobject_cast<Client *>(room);
//        Q_ASSERT(clientRoom != nullptr);
//        card = clientRoom->getCard(cardId);
//   // }
//    Q_ASSERT(card);
//    return card;
    if (cardId == Card::S_UNKNOWN_CARD_ID)
        return nullptr;
    else if (cardId < 0 || cardId >= m_cards.length()) {
        Q_ASSERT(false);
        return nullptr;
    } else {
        Q_ASSERT(m_cards[cardId] != nullptr);
        return m_cards[cardId];
    }
}

//const Card *QSgsEngine::getEngineCard(int cardId) const
//{
//    if (cardId == Card::S_UNKNOWN_CARD_ID)
//        return nullptr;
//    else if (cardId < 0 || cardId >= m_cards.length()) {
//        Q_ASSERT(false);
//        return nullptr;
//    } else {
//        Q_ASSERT(m_cards[cardId] != nullptr);
//        return m_cards[cardId];
//    }
//}

Card *QSgsEngine::cloneCard(const Card *card) const
{
    Q_ASSERT(card->metaObject() != nullptr);
    QString name = card->metaObject()->className();
    Card *result = cloneCard(name, card->suit(), card->number(), card->flags());
    if (result == nullptr)
        return nullptr;
    result->setId(card->effectiveId());
    result->setSkillName(card->skillName(false));
    result->setObjectName(card->objectName());
    return result;
}

Card *QSgsEngine::cloneCard(const QString &name, Card::Suit suit, int number, const QStringList &flags) const
{
    Card *card = nullptr;
//    if (luaBasicCard_className2objectName.contains(name)) {
//        const LuaBasicCard *lcard = luaBasicCards.value(name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaBasicCard_className2objectName.values().contains(name)) {
//        QString class_name = luaBasicCard_className2objectName.key(name, name);
//        const LuaBasicCard *lcard = luaBasicCards.value(class_name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaTrickCard_className2objectName.contains(name)) {
//        const LuaTrickCard *lcard = luaTrickCards.value(name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaTrickCard_className2objectName.values().contains(name)) {
//        QString class_name = luaTrickCard_className2objectName.key(name, name);
//        const LuaTrickCard *lcard = luaTrickCards.value(class_name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaWeapon_className2objectName.contains(name)) {
//        const LuaWeapon *lcard = luaWeapons.value(name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaWeapon_className2objectName.values().contains(name)) {
//        QString class_name = luaWeapon_className2objectName.key(name, name);
//        const LuaWeapon *lcard = luaWeapons.value(class_name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaArmor_className2objectName.contains(name)) {
//        const LuaArmor *lcard = luaArmors.value(name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaArmor_className2objectName.values().contains(name)) {
//        QString class_name = luaArmor_className2objectName.key(name, name);
//        const LuaArmor *lcard = luaArmors.value(class_name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaTreasure_className2objectName.contains(name)) {
//        const LuaTreasure *lcard = luaTreasures.value(name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else if (luaTreasure_className2objectName.values().contains(name)) {
//        QString class_name = luaTreasure_className2objectName.key(name, name);
//        const LuaTreasure *lcard = luaTreasures.value(class_name, nullptr);
//        if (!lcard) return nullptr;
//        card = lcard->clone(suit, number);
//    } else {
        const QMetaObject *meta = metaobjects.value(name, nullptr);
        if (meta == nullptr)
            meta = metaobjects.value(className2objectName.key(name, QString()), nullptr);
        if (meta) {
            QObject *card_obj = meta->newInstance(Q_ARG(Card::Suit, suit), Q_ARG(int, number));
            card_obj->setObjectName(className2objectName.value(name, name));
            card = qobject_cast<Card *>(card_obj);
        }
    //}
    if (!card) return nullptr;
    card->clearFlags();
    if (!flags.isEmpty()) {
        foreach(const QString &flag, flags)
            card->setFlags(flag);
    }
    return card;
}

SkillCard *QSgsEngine::cloneSkillCard(const QString &name) const
{
    const QMetaObject *meta = m_metaobjects.value(name, nullptr);
    if (meta) {
        QObject *card_obj = meta->newInstance();
        SkillCard *card = qobject_cast<SkillCard *>(card_obj);
        return card;
    } else {
        return nullptr;
    }
}

QVersionNumber QSgsEngine::getVersionNumber() const
{
    return QVersionNumber(0, 1, 0);
}

QString QSgsEngine::getVersion() const
{
    QString version_number = getVersionNumber().toString();
    QString mod_name = getMODName();
    if (mod_name == "official")
        return version_number;
    else
        return QString("%1:%2").arg(version_number).arg(mod_name);
}

QString QSgsEngine::getVersionName() const
{
    return "HegV2";
}

QString QSgsEngine::getMODName() const
{
    return "official";
}

QStringList QSgsEngine::getExtensions() const
{
    QStringList extensions;
    foreach (const Package *package, m_packages) {
        if (package->inherits("Scenario"))
            continue;

        extensions << package->objectName();
    }
    return extensions;
}

QStringList QSgsEngine::getKingdoms() const
{
    static QStringList kingdoms;
    if (kingdoms.isEmpty())
        kingdoms = GetConfigFromLuaState(lua, "kingdoms").toStringList();

    return kingdoms;
}

QColor QSgsEngine::getKingdomColor(const QString &kingdom) const
{
    static QMap<QString, QColor> color_map;
    if (color_map.isEmpty()) {
        QVariantMap map = GetValueFromLuaState(lua, "config", "kingdom_colors").toMap();
        QMapIterator<QString, QVariant> itor(map);
        while (itor.hasNext()) {
            itor.next();
            QColor color(itor.value().toString());
            if (!color.isValid()) {
                qWarning("Invalid color for kingdom %s", qPrintable(itor.key()));
                color = QColor(128, 128, 128);
            }
            color_map[itor.key()] = color;
        }

        Q_ASSERT(!color_map.isEmpty());
    }

    return color_map.value(kingdom);
}

QMap<QString, QColor> QSgsEngine::getSkillColorMap() const
{
    static QMap<QString, QColor> color_map;
    if (color_map.isEmpty()) {
        QVariantMap map = GetValueFromLuaState(lua, "config", "skill_colors").toMap();
        QMapIterator<QString, QVariant> itor(map);
        while (itor.hasNext()) {
            itor.next();
            QColor color(itor.value().toString());
            if (!color.isValid()) {
                qWarning("Invalid color for skill %s", qPrintable(itor.key()));
                color = QColor(128, 128, 128);
            }
            color_map[itor.key()] = color;
        }

        Q_ASSERT(!color_map.isEmpty());
    }

    return color_map;
}

QColor QSgsEngine::getSkillColor(const QString &skill_type) const
{
    return QSgsEngine::getSkillColorMap().value(skill_type);
}

QStringList QSgsEngine::getChattingEasyTexts() const
{
    static QStringList easy_texts;
    if (easy_texts.isEmpty())
        easy_texts = GetConfigFromLuaState(lua, "easy_text").toStringList();

    return easy_texts;
}

QString QSgsEngine::getSetupString() const
{
    int timeout = Config.OperationNoLimit ? 0 : Config.OperationTimeout;
    QString flags;
    if (Config.RandomSeat)
        flags.append("R");
    if (Config.EnableCheat)
        flags.append("C");
    if (Config.EnableCheat && Config.FreeChoose)
        flags.append("F");
    if (Config.ForbidAddingRobot)
        flags.append("A");
    if (Config.DisableChat)
        flags.append("M");
    if (Config.RewardTheFirstShowingPlayer)
        flags.append("S");

    QString server_name = Config.ServerName;
    QStringList setup_items;
    setup_items << server_name
        << Config.GameMode
        << QString::number(timeout)
        << QString::number(Config.NullificationCountDown)
        << Sanguosha->getBanPackages().join("+")
        << flags;

    return setup_items.join(":");
}

QMap<QString, QString> QSgsEngine::getAvailableModes() const
{
    return m_modes;
}

QString QSgsEngine::getModeName(const QString &mode) const
{
    if (m_modes.contains(mode))
        return m_modes.value(mode);
    else
        return tr("%1 [Scenario mode]").arg(translate(mode));
}

int QSgsEngine::getPlayerCount(const QString &mode) const
{
    if (m_modes.contains(mode)) {
        QRegExp rx("(\\d+)");
        int index = rx.indexIn(mode);
        if (index != -1)
            return rx.capturedTexts().first().toInt();
    } else {
        // scenario mode
        const Scenario *scenario = getScenario(mode);
        Q_ASSERT(scenario);
        return scenario->getPlayerCount();
    }

    return -1;
}

QString QSgsEngine::getRoles(const QString &mode) const
{
    int n = getPlayerCount(mode);

    if (modes.contains(mode)) {
        static const char *table[] = {
            "",
            "",

            "ZN", // 2
            "ZNN", // 3
            "ZNNN", // 4
            "ZNNNN", // 5
            "ZNNNNN", // 6
            "ZNNNNNN", // 7
            "ZNNNNNNN", // 8
            "ZNNNNNNNN", // 9
            "ZNNNNNNNNN" // 10
        };

        QString rolechar = table[n];

        return rolechar;
    } else {
        const Scenario *scenario = getScenario(mode);
        if (scenario)
            return scenario->getRoles();
    }
    return QString();
}

QStringList QSgsEngine::getRoleList(const QString &mode) const
{
    QString roles = getRoles(mode);

    QStringList role_list;
    for (int i = 0; roles[i] != '\0'; i++) {
        QString role;
        switch (roles[i].toLatin1()) {
            case 'Z': role = "lord"; break;
            case 'C': role = "loyalist"; break;
            case 'N': role = "renegade"; break;
            case 'F': role = "rebel"; break;
        }
        role_list << role;
    }

    return role_list;
}

int QSgsEngine::getCardCount() const
{
    return cards.length();
}

QStringList QSgsEngine::getGeneralNames() const
{
    QStringList generalNames;
    foreach (const General *general, m_generalList)
        generalNames << general->objectName();
    return generalNames;
}

QList<const General *> QSgsEngine::getGeneralList() const
{
    return m_generalList;
}

QStringList QSgsEngine::getLimitedGeneralNames() const
{
    //for later use
    QStringList general_names = getGeneralNames();
    QStringList general_names_copy = general_names;

    foreach (const QString &name, general_names_copy) {
        if (isGeneralHidden(name) || getBanPackages().contains(getGeneral(name)->getPackage()))
            general_names.removeOne(name);
    }

    QStringList banned_generals = Config.value("Banlist/Generals", "").toStringList();
    foreach (const QString &banned, banned_generals)
        general_names.removeOne(banned);

    return general_names;
}

QStringList QSgsEngine::getRandomGenerals(int count, const QSet<QString> &ban_set) const
{
    QStringList all_generals = getLimitedGeneralNames();
    QSet<QString> general_set = all_generals.toSet();

    count = qMin(count, all_generals.count());
    Q_ASSERT(all_generals.count() >= count);

    all_generals = general_set.subtract(ban_set).toList();

    // shuffle them
    qShuffle(all_generals);

    QStringList general_list = all_generals.mid(0, count);
    Q_ASSERT(general_list.count() == count);

    return general_list;
}

QList<int> QSgsEngine::getRandomCards() const
{
    QList<int> list;
    foreach (Card *card, cards) {
        card->clearFlags();

        if (!getBanPackages().contains(card->package()))
            list << card->id();
    }

    QStringList card_conversions = Config.value("CardConversions").toStringList();

    if (card_conversions.contains("DragonPhoenix"))
        list.removeOne(55);
    else
        list.removeOne(108);

    if (card_conversions.contains("PeaceSpell"))
        list.removeOne(157);
    else
        list.removeOne(109);

    qShuffle(list);

    return list;
}

QString QSgsEngine::getRandomGeneralName() const
{
    const General *general = generalList.at(qrand() % generalList.size());
    while (general->getKingdom() == "programmer")
        general = generalList.at(qrand() % generalList.size());
    return general->objectName();
}

void QSgsEngine::playSystemAudioEffect(const QString &name) const
{
    playAudioEffect(QString("audio/system/%1.ogg").arg(name));
}

void QSgsEngine::playAudioEffect(const QString &filename) const
{
#ifdef AUDIO_SUPPORT
    if (!Config.EnableEffects)
        return;
    if (filename.isNull())
        return;

    Audio::play(filename);
#endif
}

//void QSgsEngine::playSkillAudioEffect(const QString &skill_name, int index) const
//{
//    const Skill *skill = m_skills.value(skill_name, nullptr);
//    if (skill)
//        skill->playAudioEffect(index);
//}

//const Skill *QSgsEngine::getSkill(const QString &skill_name) const
//{
//    return m_skills.value(skill_name, nullptr);
//}

const Skill *QSgsEngine::getSkill(const EquipCard *equip) const
{
    const Skill *skill;
    if (equip == nullptr)
        skill = nullptr;
    else
        skill = Sanguosha->getSkill(equip->objectName());

    return skill;
}

QStringList QSgsEngine::getSkillNames() const
{
    return m_skills.keys();
}

const TriggerSkill *QSgsEngine::getTriggerSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if (skill)
        return qobject_cast<const TriggerSkill *>(skill);
    else
        return nullptr;
}

const ViewAsSkill *QSgsEngine::getViewAsSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if (skill == nullptr)
        return nullptr;

    if (skill->inherits("ViewAsSkill"))
        return qobject_cast<const ViewAsSkill *>(skill);
    else if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        return trigger_skill->getViewAsSkill();
    } else
        return nullptr;
}

const ProhibitSkill *QSgsEngine::isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others) const
{
    foreach (const ProhibitSkill *skill, m_prohibitSkills) {
        if (skill->isProhibited(from, to, card, others))
            return skill;
    }

    return nullptr;
}

int QSgsEngine::correctDistance(const Player *from, const Player *to) const
{
    int correct = 0;

    foreach (const DistanceSkill *skill, m_distanceSkills)
        correct += skill->getCorrect(from, to);

    return correct;
}

int QSgsEngine::correctMaxCards(const ServerPlayer *target, bool fixed, MaxCardsType::MaxCardsCount type) const
{
    int extra = 0;

    foreach (const MaxCardsSkill *skill, m_maxcardsSkills) {
        if (fixed) {
            int f = skill->getFixed(target, type);
            if (f > extra)
                extra = f;
        } else {
            extra += skill->getExtra(target, type);
        }
    }

    return extra;
}

int QSgsEngine::correctCardTarget(const TargetModSkill::ModType type, const Player *from, const Card *card) const
{
    int x = 0;

    if (type == TargetModSkill::Residue) {
        foreach (const TargetModSkill *skill, m_targetmodSkills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                int residue = skill->getResidueNum(from, card);
                if (residue >= 998) return residue;
                x += residue;
            }
        }
    } else if (type == TargetModSkill::DistanceLimit) {
        foreach (const TargetModSkill *skill, m_targetmodSkills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                int distance_limit = skill->getDistanceLimit(from, card);
                if (distance_limit >= 998) return distance_limit;
                x += distance_limit;
            }
        }
    } else if (type == TargetModSkill::ExtraTarget) {
        foreach (const TargetModSkill *skill, m_targetmodSkills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                x += skill->getExtraTargetNum(from, card);
            }
        }
    }

    return x;
}


int QSgsEngine::correctAttackRange(const Player *target, bool include_weapon, bool fixed) const
{
    int extra = 0;

    foreach (const AttackRangeSkill *skill, m_attackrangeSkills) {
        if (fixed) {
            int f = skill->getFixed(target, include_weapon);
            if (f > extra)
                extra = f;
        } else {
            extra += skill->getExtra(target, include_weapon);
        }
    }

    return extra;
}