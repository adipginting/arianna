// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-only or LGPL-3.0-only or LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtWebEngine 1.4
import QtWebChannel 1.4
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import org.kde.kirigami 2.13 as Kirigami
import org.kde.arianna 1.0

Kirigami.ScrollablePage {
    id: root

    property BookListModel bookListModel

    title: i18n("Library")

    actions.main: Kirigami.Action {
        text: i18nc("@action:button", "Add Book")
        icon.name: "list-add"
        onTriggered: {
            const fileDialog = openFileDialog.createObject(QQC2.ApplicationWindow.overlay)
            fileDialog.accepted.connect(() => {
                const file = fileDialog.file;
                if (!file) {
                    return;
                }
                contentList.addFile(file)
            })
            fileDialog.open();
        }
    }

    Component {
        id: openFileDialog

        FileDialog {
            id: root
            title: i18n("Please choose a file")
        }
    }

    GridView {
        id: contentDirectoryView

        leftMargin: Kirigami.Units.smallSpacing
        rightMargin: Kirigami.Units.smallSpacing
        topMargin: Kirigami.Units.smallSpacing
        bottomMargin: Kirigami.Units.smallSpacing

        model: root.bookListModel

        cellWidth: {
            const viewWidth = contentDirectoryView.width - Kirigami.Units.smallSpacing * 2;
            let columns = Math.max(Math.floor(viewWidth / 170), 2);
            return Math.floor(viewWidth / columns);
        }
        cellHeight: {
            if (Kirigami.Settings.isMobile) {
                return cellWidth + Kirigami.Units.gridUnit * 2 + Kirigami.Units.largeSpacing;
            } else {
                return 170 + Kirigami.Units.gridUnit * 2 + Kirigami.Units.largeSpacing
            }
        }
        currentIndex: Kirigami.Settings.isMobile ? 0 : -1
        reuseItems: true
        activeFocusOnTab: true
        keyNavigationEnabled: true

        delegate: GridBrowserDelegate {
            id: bookDelegate

            required property string thumbnail
            required property string title
            required property string filename
            required property var author
            required property var currentLocation

            width: Kirigami.Settings.isMobile ? contentDirectoryView.cellWidth : 170
            height: contentDirectoryView.cellHeight
            focus: true

            imageUrl: 'file://' + bookDelegate.thumbnail
            mainText: bookDelegate.title
            secondaryText: bookDelegate.author.join(', ')

            onOpen: Navigation.openBook(bookDelegate.filename, bookDelegate.locations, bookDelegate.currentLocation)
        }
    }
}
