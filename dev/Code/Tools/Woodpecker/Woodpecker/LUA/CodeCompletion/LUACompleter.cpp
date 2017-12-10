/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "stdafx.h"
#include "LUACompleter.hxx"

#include <Woodpecker/LUA/CodeCompletion/LUACompleter.moc>

namespace LUAEditor
{
    Completer::Completer(QAbstractItemModel* model, QObject* pParent)
        : QCompleter(model, pParent)
    {
        setCaseSensitivity(Qt::CaseInsensitive);
        setCompletionMode(QCompleter::CompletionMode::PopupCompletion);
        setModelSorting(QCompleter::ModelSorting::CaseSensitivelySortedModel);
    }

    Completer::~Completer()
    {
    }
    QStringList Completer::splitPath(const QString& path) const
    {
        auto result = path.split(QRegularExpression(c_luaSplit));
        return result;
    }

    int Completer::GetCompletionPrefixTailLength()
    {
        auto result = completionPrefix().split(QRegularExpression(c_luaSplit));
        return result.back().length();
    }
}