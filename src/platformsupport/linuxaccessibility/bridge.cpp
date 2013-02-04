/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "bridge_p.h"

#include <atspi/atspi-constants.h>
#include <qstring.h>

#include "atspiadaptor_p.h"

#include "cache_p.h"
#include "constant_mappings_p.h"
#include "dbusconnection_p.h"
#include "struct_marshallers_p.h"

#include "deviceeventcontroller_adaptor.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSpiAccessibleBridge
    \internal
*/

QSpiAccessibleBridge::QSpiAccessibleBridge()
    : cache(0), dec(0), dbusAdaptor(0), m_enabled(false)
{
    dbusConnection = new DBusConnection();
    connect(dbusConnection, SIGNAL(enabledChanged(bool)), this, SLOT(enabledChanged(bool)));
}

void QSpiAccessibleBridge::enabledChanged(bool enabled)
{
    m_enabled = enabled;
    updateStatus();
}

QSpiAccessibleBridge::~QSpiAccessibleBridge()
{
    delete dbusConnection;
} // Qt currently doesn't delete plugins.

QDBusConnection QSpiAccessibleBridge::dBusConnection() const
{
    return dbusConnection->connection();
}

void QSpiAccessibleBridge::updateStatus()
{
    // create the adaptor to handle everything if we are in enabled state
    if (!dbusAdaptor && m_enabled) {
        qSpiInitializeStructTypes();
        initializeConstantMappings();

        cache = new QSpiDBusCache(dbusConnection->connection(), this);
        dec = new DeviceEventControllerAdaptor(this);

        dbusConnection->connection().registerObject(QLatin1String(ATSPI_DBUS_PATH_DEC), this, QDBusConnection::ExportAdaptors);

        dbusAdaptor = new AtSpiAdaptor(dbusConnection, this);
        dbusConnection->connection().registerVirtualObject(QLatin1String(QSPI_OBJECT_PATH_ACCESSIBLE), dbusAdaptor, QDBusConnection::SubPath);
        dbusAdaptor->registerApplication();
    }
}

void QSpiAccessibleBridge::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (!dbusAdaptor)
        return;
    if (m_enabled)
        dbusAdaptor->notify(event);
}

struct RoleMapping {
    QAccessible::Role role;
    AtspiRole spiRole;
    const char *name;
};

static RoleMapping map[] = {
    { QAccessible::NoRole, ATSPI_ROLE_INVALID, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "invalid role") },
    { QAccessible::TitleBar, ATSPI_ROLE_TEXT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "title bar") },
    { QAccessible::MenuBar, ATSPI_ROLE_MENU_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "menu bar") },
    { QAccessible::ScrollBar, ATSPI_ROLE_SCROLL_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "scroll bar") },
    { QAccessible::Grip, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "grip") },
    { QAccessible::Sound, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "sound") },
    { QAccessible::Cursor, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "cursor") },
    { QAccessible::Caret, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "cursor") },
    { QAccessible::AlertMessage, ATSPI_ROLE_ALERT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "alert message") },
    { QAccessible::Window, ATSPI_ROLE_WINDOW, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "window") },
    { QAccessible::Client, ATSPI_ROLE_FILLER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "filler") },
    { QAccessible::PopupMenu, ATSPI_ROLE_POPUP_MENU, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "popup menu") },
    { QAccessible::MenuItem, ATSPI_ROLE_MENU_ITEM, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "menu item") },
    { QAccessible::ToolTip, ATSPI_ROLE_TOOL_TIP, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "tool tip") },
    { QAccessible::Application, ATSPI_ROLE_APPLICATION, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "application") },
    { QAccessible::Document, ATSPI_ROLE_DOCUMENT_FRAME, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "document") },
    { QAccessible::Pane, ATSPI_ROLE_PANEL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "panel") },
    { QAccessible::Chart, ATSPI_ROLE_CHART, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "chart") },
    { QAccessible::Dialog, ATSPI_ROLE_DIALOG, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "dialog") },
    { QAccessible::Border, ATSPI_ROLE_FRAME, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "frame") },
    { QAccessible::Grouping, ATSPI_ROLE_PANEL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "panel") },
    { QAccessible::Separator, ATSPI_ROLE_SEPARATOR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "separator") },
    { QAccessible::ToolBar, ATSPI_ROLE_TOOL_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "tool bar") },
    { QAccessible::StatusBar, ATSPI_ROLE_STATUS_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "status bar") },
    { QAccessible::Table, ATSPI_ROLE_TABLE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "table") },
    { QAccessible::ColumnHeader, ATSPI_ROLE_TABLE_COLUMN_HEADER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "column header") },
    { QAccessible::RowHeader, ATSPI_ROLE_TABLE_ROW_HEADER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "row header") },
    { QAccessible::Column, ATSPI_ROLE_TABLE_CELL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "column") },
    { QAccessible::Row, ATSPI_ROLE_TABLE_ROW, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "row") },
    { QAccessible::Cell, ATSPI_ROLE_TABLE_CELL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "cell") },
    { QAccessible::Link, ATSPI_ROLE_LINK, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "link") },
    { QAccessible::HelpBalloon, ATSPI_ROLE_DIALOG, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "help balloon") },
    { QAccessible::Assistant, ATSPI_ROLE_DIALOG, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "assistant") },
    { QAccessible::List, ATSPI_ROLE_LIST, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "list") },
    { QAccessible::ListItem, ATSPI_ROLE_LIST_ITEM, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "list item") },
    { QAccessible::Tree, ATSPI_ROLE_TREE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "tree") },
    { QAccessible::TreeItem, ATSPI_ROLE_TABLE_CELL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "tree item") },
    { QAccessible::PageTab, ATSPI_ROLE_PAGE_TAB, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "page tab") },
    { QAccessible::PropertyPage, ATSPI_ROLE_PAGE_TAB, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "property page") },
    { QAccessible::Indicator, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "indicator") },
    { QAccessible::Graphic, ATSPI_ROLE_IMAGE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "graphic") },
    { QAccessible::StaticText, ATSPI_ROLE_LABEL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "label") },
    { QAccessible::EditableText, ATSPI_ROLE_TEXT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "text") },
    { QAccessible::PushButton, ATSPI_ROLE_PUSH_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "push button") },
    { QAccessible::CheckBox, ATSPI_ROLE_CHECK_BOX, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "check box") },
    { QAccessible::RadioButton, ATSPI_ROLE_RADIO_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "radio button") },
    { QAccessible::ComboBox, ATSPI_ROLE_COMBO_BOX, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "combo box") },
    { QAccessible::ProgressBar, ATSPI_ROLE_PROGRESS_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "progress bar") },
    { QAccessible::Dial, ATSPI_ROLE_DIAL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "dial") },
    { QAccessible::HotkeyField, ATSPI_ROLE_TEXT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "hotkey field") },
    { QAccessible::Slider, ATSPI_ROLE_SLIDER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "slider") },
    { QAccessible::SpinBox, ATSPI_ROLE_SPIN_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "spin box") },
    { QAccessible::Canvas, ATSPI_ROLE_CANVAS, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "canvas") },
    { QAccessible::Animation, ATSPI_ROLE_ANIMATION, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "animation") },
    { QAccessible::Equation, ATSPI_ROLE_TEXT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "equation") },
    { QAccessible::ButtonDropDown, ATSPI_ROLE_PUSH_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "button drop down") },
    { QAccessible::ButtonMenu, ATSPI_ROLE_PUSH_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "button menu") },
    { QAccessible::ButtonDropGrid, ATSPI_ROLE_PUSH_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "button drop grid") },
    { QAccessible::Whitespace, ATSPI_ROLE_FILLER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "whitespace") },
    { QAccessible::PageTabList, ATSPI_ROLE_PAGE_TAB_LIST, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "page tab list") },
    { QAccessible::Clock, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "clock") },
    { QAccessible::Splitter, ATSPI_ROLE_SPLIT_PANE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "splitter") },
    { QAccessible::LayeredPane, ATSPI_ROLE_LAYERED_PANE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "layered pane") },
    { QAccessible::UserRole, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "unknown") }
};

void QSpiAccessibleBridge::initializeConstantMappings()
{
    for (uint i = 0; i < sizeof(map) / sizeof(RoleMapping); ++i)
        qSpiRoleMapping.insert(map[i].role, RoleNames(map[i].spiRole, QLatin1String(map[i].name), tr(map[i].name)));
}

QT_END_NAMESPACE
