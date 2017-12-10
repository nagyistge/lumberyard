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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : For listing available script commands with their descriptions


#ifndef CRYINCLUDE_EDITOR_SCRIPTHELPDIALOG_H
#define CRYINCLUDE_EDITOR_SCRIPTHELPDIALOG_H
#pragma once

#include <QDialog>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVector>
#include <QMouseEvent>
#include <QHeaderView>
#include <QScopedPointer>

class QResizeEvent;

namespace Ui {
    class ScriptDialog;
}

class HeaderView
    : public QHeaderView
{
    Q_OBJECT
public:
    explicit HeaderView(QWidget* parent = nullptr);
    QSize sizeHint() const override;
Q_SIGNALS:
    void commandFilterChanged(const QString& text);
    void moduleFilterChanged(const QString& text);
private Q_SLOTS:
    void repositionLineEdits();
protected:
    void resizeEvent(QResizeEvent* ev) override;
private:
    QLineEdit* const m_commandFilter;
    QLineEdit* const m_moduleFilter;
    int m_lineEditHeightOffset;
};

class ScriptHelpProxyModel
    : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ScriptHelpProxyModel(QObject* parent = nullptr);
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

    void setCommandFilter(const QString&);
    void setModuleFilter(const QString&);
private:
    QString m_commandFilter;
    QString m_moduleFilter;
};

class ScriptHelpModel
    : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column
    {
        ColumnCommand,
        ColumnModule,
        ColumnDescription,
        ColumnExample,
        ColumnCount // keep at end, for iteration purposes
    };

    struct Item
    {
        QString command;
        QString module;
        QString description;
        QString example;
    };
    typedef QVector<Item> Items;


    explicit ScriptHelpModel(QObject* parent = nullptr);
    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& = {}) const override;
    int columnCount(const QModelIndex& = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void Reload();

private:
    Items m_items;
};

class ScriptTableView
    : public QTableView
{
    Q_OBJECT
public:
    explicit ScriptTableView(QWidget* parent = nullptr);

private:
    ScriptHelpModel* const m_model;
    ScriptHelpProxyModel* const m_proxyModel;
};

class CScriptHelpDialog
    : public QDialog
{
    Q_OBJECT
public:
    static CScriptHelpDialog* GetInstance()
    {
        static CScriptHelpDialog* pInstance = nullptr;
        if (!pInstance)
        {
            pInstance = new CScriptHelpDialog();
        }
        return pInstance;
    }

private Q_SLOTS:
    void OnDoubleClick(const QModelIndex&);

private:
    explicit CScriptHelpDialog(QWidget* parent = nullptr);
    QScopedPointer<Ui::ScriptDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_SCRIPTHELPDIALOG_H
