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

#ifndef _GENERAL_H
#define _GENERAL_H

#include "libqsgsgamelogicglobal.h"
#include "enumeration.h"

class Skill;
class TriggerSkill;
class QSgsPackage;
class QSize;

class General : public QObject
{
    Q_OBJECT
    Q_ENUMS(Gender)
    Q_PROPERTY(QString m_kingdom READ kingdom CONSTANT)
    Q_PROPERTY(int maxhp READ doubleMaxHp CONSTANT)
    Q_PROPERTY(bool male READ isMale STORED false CONSTANT)
    Q_PROPERTY(bool female READ isFemale STORED false CONSTANT)
    Q_PROPERTY(QSgsEnum::GeneralGender m_gender READ gender CONSTANT)
    Q_PROPERTY(bool m_lord READ isLord CONSTANT)
    Q_PROPERTY(bool m_hidden READ isHidden CONSTANT)

public:
    explicit General(QSgsPackage *package, const QString &name, const QString &m_kingdom,
        int m_doubleMaxHp = 4, bool male = true, bool m_hidden = false, bool m_neverShown = false);
    // property getters/setters
    int doubleMaxHp() const;
    QString kingdom() const;
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;
    bool isLord() const;
    bool isHidden() const;
    bool isTotallyHidden() const;

    int maxHpHead() const;
    int maxHpDeputy() const;

    QSgsEnum::GeneralGender gender() const;
    void setGender(QSgsEnum::GeneralGender m_gender);

    void addSkill(Skill *skill);
    void addSkill(const QString &skill_name);
    bool hasSkill(const QString &skill_name) const;
    QList<const Skill *> getSkillList(bool relate_to_place = false, bool head_only = true) const;
    QList<const Skill *> getVisibleSkillList(bool relate_to_place = false, bool head_only = true) const;
    QSet<const Skill *> getVisibleSkills(bool relate_to_place = false, bool head_only = true) const;
    QSet<const TriggerSkill *> getTriggerSkills() const;

    void addRelateSkill(const QString &skill_name);
    QStringList relatedSkillNames() const;

    QString package() const;
    QString companions() const;
    QString skillDescription(bool include_name = false, bool inToolTip = true) const;

    inline QSet<QString> extraSkillSet() const
    {
        return m_extraSet;
    }

    void addCompanion(const QString &name);
    bool isCompanionWith(const QString &name) const;

    void setHeadMaxHpAdjustedValue(int adjusted_value = -1);
    void setDeputyMaxHpAdjustedValue(int adjusted_value = -1);

private:
    QString m_kingdom;
    int m_doubleMaxHp;
    QSgsEnum::GeneralGender m_gender;
    bool m_lord;
    QSet<QString> m_skillSet;
    QSet<QString> m_extraSet;
    QStringList m_skillnameList;
    QStringList m_relatedSkills;
    bool m_hidden;
    bool m_neverShown;
    QStringList m_companions;
    int m_headMaxHpAdjustedValue;
    int m_deputyMaxHpAdjustedValue;
};

Q_DECLARE_METATYPE(const General *)

#endif
