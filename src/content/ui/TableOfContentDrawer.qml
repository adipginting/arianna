// SPDX-FileCopyrightText: 2023 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-only or LGPL-3.0-only or LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.20 as Kirigami
import org.kde.kirigamiaddons.treeview 1.0 as Tree
import org.kde.arianna 1.0

Kirigami.OverlayDrawer {
    id: root

    property alias model: tableOfContentModel
    signal goTo(cfi: string)

    width: Kirigami.Units.gridUnit * 20
    edge: Qt.application.layoutDirection == Qt.RightToLeft ? Qt.LeftEdge : Qt.RightEdge
    handleClosedIcon.name: 'format-list-ordered'

    topPadding: 0
    leftPadding: 0
    rightPadding: 0

    Kirigami.Theme.colorSet: Kirigami.Theme.View

    contentItem: ColumnLayout {
        spacing: 0

        QQC2.ToolBar {
            Layout.fillWidth: true
            Layout.preferredHeight: applicationWindow().pageStack.globalToolBar.preferredHeight

            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.smallSpacing
            topPadding: Kirigami.Units.smallSpacing
            bottomPadding: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                text: i18n("Table of Contents")
            }
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Tree.TreeListView {
                model: TableOfContentModel {
                    id: tableOfContentModel
                }

                delegate: Tree.BasicTreeItem {
                    id: itemDelegate

                    text: model.title
                    width: ListView.view.width

                    onClicked: root.goTo(model.href)
                }
            }
        }
    }
}
