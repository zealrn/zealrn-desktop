// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalbridge.h"

#include <QTimer>

#include <utility>

namespace Zeal::WidgetUi {

TerminalBridge::TerminalBridge(QObject *parent)
    : QObject(parent)
{
}

void TerminalBridge::enqueueOutput(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }

    m_pendingOutput.append(data);
    if (m_pendingOutput.size() > MaxPendingBytes) {
        m_pendingOutput = m_pendingOutput.right(MaxPendingBytes);
        emit errorOccurred(tr("Terminal output exceeded the pending buffer; older output was discarded."));
    }
    scheduleFlush();
}

void TerminalBridge::frontendReady()
{
    m_frontendReady = true;
    scheduleFlush();
}

void TerminalBridge::sendInput(const QString &encodedData)
{
    const QByteArray encoded = encodedData.toLatin1();
    const QByteArray decoded = QByteArray::fromBase64(encoded, QByteArray::AbortOnBase64DecodingErrors);
    if (!encoded.isEmpty() && decoded.isEmpty()) {
        emit errorOccurred(tr("The terminal frontend sent invalid input."));
        return;
    }
    emit inputReceived(decoded);
}

void TerminalBridge::resizeTerminal(int columns, int rows)
{
    if (columns > 0 && rows > 0) {
        emit terminalResizeRequested(QSize(columns, rows));
    }
}

void TerminalBridge::startSession(const QString &shellId, const QString &workingDirectory)
{
    emit sessionStartRequested(shellId, workingDirectory);
}

void TerminalBridge::interruptSession()
{
    emit interruptRequested();
}

void TerminalBridge::terminateSession()
{
    emit terminateRequested();
}

void TerminalBridge::copySelection(const QString &text)
{
    emit selectionCopyRequested(text);
}

void TerminalBridge::requestPaste()
{
    emit pasteRequested();
}

void TerminalBridge::scheduleFlush()
{
    if (!m_frontendReady || m_pendingOutput.isEmpty() || m_flushScheduled) {
        return;
    }
    m_flushScheduled = true;
    QTimer::singleShot(0, this, &TerminalBridge::flushOutput);
}

void TerminalBridge::flushOutput()
{
    m_flushScheduled = false;
    if (!m_frontendReady || m_pendingOutput.isEmpty()) {
        return;
    }

    const QByteArray output = std::exchange(m_pendingOutput, {});
    emit outputReceived(QString::fromLatin1(output.toBase64()));
}

} // namespace Zeal::WidgetUi
