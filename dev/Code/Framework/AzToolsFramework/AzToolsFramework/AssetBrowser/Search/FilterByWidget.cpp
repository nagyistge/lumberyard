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
#include <UI/UICore/QBetterLabel.h>
#include <AssetBrowser/Search/ui_FilterByWidget.h>
#include <AzToolsFramework/AssetBrowser/Search/FilterByWidget.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        FilterByWidget::FilterByWidget(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::FilterByWidgetClass)
        {
            m_ui->setupUi(this);
            connect(m_ui->m_clearFiltersButton, &QBetterLabel::clicked, this, &FilterByWidget::ClearSignal);
            // hide clear button as filters are reset at the startup
            ToggleClearButton(false);
        }

        FilterByWidget::~FilterByWidget() = default;

        void FilterByWidget::ToggleClearButton(bool visible) const
        {
            m_ui->m_clearFiltersButton->setVisible(visible);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include <AssetBrowser/Search/FilterByWidget.moc>