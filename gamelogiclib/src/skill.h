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

#ifndef _SKILL_H
#define _SKILL_H

#include "libqsgsgamelogicglobal.h"
#include "enumeration.h"
#include "structs.h"
#include "player.h"

class Card;
class RoomObject;

class SkillPrivate;

class LIBQSGSGAMELOGIC_EXPORT Skill : public QObject
{
    Q_OBJECT

public:
    virtual ~Skill();

    bool isLordSkill() const;
    void setLordSkill(bool l);
    bool isAttachedSkill() const;
    void setAttachedSkill(bool a);
    bool isVisible() const;
    void setVisible(bool v);
    const QString &limitMark() const;
    void setLimitMark(const QString &lm);
    bool canPreshow() const;
    void setCanPreshow(bool c);

    QSgsEnum::SkillFrequency frequency() const;

    // Note: Both functions should return true for skills without a description of head skill or deputy skill
    bool isHeadSkill() const;
    bool isDeputySkill() const;

protected:
    explicit Skill(const QString &name, QSgsEnum::SkillFrequency frequency = QSgsEnum::SkillFrequency::NotFrequent, QSgsEnum::SkillPlace place = QSgsEnum::SkillPlace::Both);
    SkillPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(Skill)
};

class LIBQSGSGAMELOGIC_EXPORT ViewAsSkill : public Skill
{
    Q_OBJECT

public:
    ViewAsSkill(const QString &name);

    virtual bool viewFilter(const QList<Card *> &selected, Card *to_select) const = 0;
    virtual Card *viewAs(const QList<Card *> &cards) const = 0;

    bool isAvailable(const Player *invoker, QSgsEnum::CardUseReason reason, const QString &pattern) const;
    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const;
    virtual bool isEnabledAtNullification(const Player *player) const;
    static const ViewAsSkill *parseViewAsSkill(const Skill *skill);

    inline bool isResponseOrUse() const
    {
        return m_responseOrUse;
    }
    inline QString expandPile() const
    {
        return m_expandPile;
    }

protected:
    QString m_responsePattern;
    bool m_responseOrUse;
    QString m_expandPile;
};

class LIBQSGSGAMELOGIC_EXPORT ZeroCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    ZeroCardViewAsSkill(const QString &name);

    virtual bool viewFilter(const QList<Card *> &selected, Card *to_select) const;
    virtual Card *viewAs(const QList<Card *> &cards) const;
    virtual Card *viewAs() const = 0;
};

class LIBQSGSGAMELOGIC_EXPORT OneCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    OneCardViewAsSkill(const QString &name);

    virtual bool viewFilter(const QList<Card *> &selected, Card *to_select) const;
    virtual Card *viewAs(const QList<Card *> &cards) const;

    virtual bool viewFilter(Card *to_select) const;
    virtual Card *viewAs(Card *originalCard) const = 0;

protected:
    QString m_filterPattern;
};

class LIBQSGSGAMELOGIC_EXPORT CardTransformSkill : public Skill
{
    Q_OBJECT

public:
    CardTransformSkill(const QString &name);

    virtual bool viewFilter(Card *to_select) const;
    virtual Card *viewAs(Card *originalCard) const = 0;

protected:
    QString m_filterPattern;
};

typedef QMap<Player *, QStringList> TriggerList;

class LIBQSGSGAMELOGIC_EXPORT TriggerSkill : public Skill
{
    Q_OBJECT

public:
    TriggerSkill(const QString &name);
    const ViewAsSkill *viewAsSkill() const;
    QList<QSgsEnum::TriggerEvent> triggerEvents() const;

    virtual int priority() const;
    void insertPriority(QSgsEnum::TriggerEvent e, double value);

    virtual bool triggerable(const Player *target) const;

    virtual void record(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data) const;

    virtual TriggerList triggerable(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data) const;
    virtual QStringList triggerable(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player* &ask_who) const;
    virtual bool cost(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;
    virtual bool effect(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;

    inline bool isGlobal() const
    {
        return m_global;
    }

    virtual ~TriggerSkill();

protected:
    const ViewAsSkill *m_viewAsSkill;
    QList<QSgsEnum::TriggerEvent> m_events;
    bool m_global;
    QHash<QSgsEnum::TriggerEvent, double> m_priority;

};

class Scenario;

//class ScenarioRule : public TriggerSkill
//{
//    Q_OBJECT

//public:
//    ScenarioRule(Scenario *scenario);

//    virtual int m_priority() const;
//    virtual bool triggerable(const Player *target) const;
//};

class LIBQSGSGAMELOGIC_EXPORT MasochismSkill : public TriggerSkill
{
    Q_OBJECT

public:
    MasochismSkill(const QString &name);

    virtual bool cost(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;
    virtual bool effect(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;
    virtual void onDamaged(Player *target, const DamageStruct &damage) const = 0;
};

class LIBQSGSGAMELOGIC_EXPORT PhaseChangeSkill : public TriggerSkill
{
    Q_OBJECT

public:
    PhaseChangeSkill(const QString &name);

    virtual bool effect(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;
    virtual bool onPhaseChange(Player *target) const = 0;
};

class LIBQSGSGAMELOGIC_EXPORT DrawCardsSkill : public TriggerSkill
{
    Q_OBJECT

public:
    DrawCardsSkill(const QString &name);

    virtual bool effect(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;
    virtual int getDrawNum(Player *player, int n) const = 0;
};

class LIBQSGSGAMELOGIC_EXPORT GameStartSkill : public TriggerSkill
{
    Q_OBJECT

public:
    GameStartSkill(const QString &name);

    virtual bool effect(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;
    virtual void onGameStart(Player *player) const = 0;
};

class LIBQSGSGAMELOGIC_EXPORT BattleArraySkill : public TriggerSkill
{
    Q_OBJECT

public:

    BattleArraySkill(const QString &name, const QSgsEnum::ArrayType type);
    virtual bool triggerable(const Player *player) const;

    virtual void summonFriends(Player *player) const;

    inline QSgsEnum::ArrayType getArrayType() const
    {
        return m_arrayType;
    }
private:
    QSgsEnum::ArrayType m_arrayType;
};

class LIBQSGSGAMELOGIC_EXPORT ArraySummonSkill : public ZeroCardViewAsSkill
{
    Q_OBJECT

public:

    ArraySummonSkill(const QString &name);

    Card *viewAs() const;
    virtual bool isEnabledAtPlay(const Player *player) const;
};

class LIBQSGSGAMELOGIC_EXPORT ProhibitSkill : public Skill
{
    Q_OBJECT

public:
    ProhibitSkill(const QString &name);

    virtual bool isProhibited(const Player *from, const Player *to, Card *card, const QList<const Player *> &others = QList<const Player *>()) const = 0;
};

class LIBQSGSGAMELOGIC_EXPORT DistanceSkill : public Skill
{
    Q_OBJECT

public:
    DistanceSkill(const QString &name);

    virtual int getCorrect(const Player *from, const Player *to) const = 0;
};

class LIBQSGSGAMELOGIC_EXPORT MaxCardsSkill : public Skill
{
    Q_OBJECT

public:
    MaxCardsSkill(const QString &name);

    virtual int getExtra(const Player *target/*, MaxCardsType::MaxCardsCount type = MaxCardsType::Max*/) const;
    virtual int getFixed(const Player *target/*, MaxCardsType::MaxCardsCount type = MaxCardsType::Max*/) const;
};

class LIBQSGSGAMELOGIC_EXPORT TargetModSkill : public Skill
{
    Q_OBJECT

public:
    TargetModSkill(const QString &name);
    virtual QString pattern() const;

    virtual int residueNum(const Player *from, Card *card) const;
    virtual int distanceLimit(const Player *from, Card *card) const;
    virtual int extraTargetNum(const Player *from, Card *card) const;

protected:
    QString m_pattern;
};

class LIBQSGSGAMELOGIC_EXPORT SlashNoDistanceLimitSkill : public TargetModSkill
{
    Q_OBJECT

public:
    SlashNoDistanceLimitSkill(const QString &skill_name);

    virtual int distanceLimit(const Player *from, Card *card) const;

protected:
    QString m_name;
};

class LIBQSGSGAMELOGIC_EXPORT AttackRangeSkill : public Skill
{
    Q_OBJECT

public:
    AttackRangeSkill(const QString &name);

    virtual int extra(const Player *target, bool include_weapon) const;
    virtual int fixed(const Player *target, bool include_weapon) const;
};


// a nasty way for 'fake moves', usually used in the process of multi-card chosen
class LIBQSGSGAMELOGIC_EXPORT FakeMoveSkill : public TriggerSkill
{
    Q_OBJECT

public:
    FakeMoveSkill(const QString &skillname);

    virtual int priority() const;
    virtual QStringList triggerable(QSgsEnum::TriggerEvent, Room *, Player *target, QVariant &, Player * &ask_who) const;
    virtual bool effect(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;

private:
    QString m_name;
};

class LIBQSGSGAMELOGIC_EXPORT DetachEffectSkill : public TriggerSkill
{
    Q_OBJECT

public:
    DetachEffectSkill(const QString &skillname, const QString &pilename = QString());

    virtual QStringList triggerable(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player * &ask_who) const;
    virtual bool effect(QSgsEnum::TriggerEvent triggerEvent, Room *room, Player *player, QVariant &data, Player *ask_who = nullptr) const;
    virtual void onSkillDetached(Room *room, Player *player) const;

private:
    QString m_name, m_pileName;
};

class LIBQSGSGAMELOGIC_EXPORT WeaponSkill : public TriggerSkill
{
    Q_OBJECT

public:
    WeaponSkill(const QString &name);

    virtual int priority() const;
    virtual bool triggerable(const Player *target) const;
};

class LIBQSGSGAMELOGIC_EXPORT ArmorSkill : public TriggerSkill
{
    Q_OBJECT

public:
    ArmorSkill(const QString &name);

    virtual int priority() const;
    virtual bool triggerable(const Player *target) const;
};

class LIBQSGSGAMELOGIC_EXPORT TreasureSkill : public TriggerSkill
{
    Q_OBJECT

public:
    TreasureSkill(const QString &name);

    virtual int priority() const;
    virtual bool triggerable(const Player *target) const;
};

#endif

