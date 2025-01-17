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

#ifndef _EXPPATTERN_H
#define _EXPPATTERN_H

#include "libqsgsgamelogicglobal.h"
class Player;
class Card;

class LIBQSGSGAMELOGIC_EXPORT ExpPattern
{
public:
    ExpPattern(const QString &exp);
    bool match(const Player *player, const Card *card) const;
    const QString &getPatternString() const;

private:
    QString exp;
    bool matchOne(const Player *player, const Card *card, QString exp) const;
};

#endif

