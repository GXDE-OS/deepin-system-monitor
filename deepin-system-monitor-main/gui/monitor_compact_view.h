// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MONITOR_COMPACT_VIEW_H
#define MONITOR_COMPACT_VIEW_H

#include <DFrame>

DWIDGET_USE_NAMESPACE

class CompactCpuMonitor;
class CompactGpuMonitor;
class CompactMemoryMonitor;
class CompactNetworkMonitor;
class CompactDiskMonitor;

/**
 * @brief Compact view frame
 */
class MonitorCompactView : public DFrame
{
    Q_OBJECT

public:
    /**
     * @brief Compact view frame constructor
     * @param parent Parent object
     */
    explicit MonitorCompactView(QWidget *parent = nullptr);
    /**
     * @brief Default destructor
     */
    ~MonitorCompactView() = default;

signals:
    void signalDetailInfoByDbus(QString);

private:
    // Compact cpu view instance (full-width row)
    CompactCpuMonitor *m_cpuMonitor {};
    // Row holding one chart per detected GPU/NPU device (GPU1..N, NPU1..M)
    QWidget *m_deviceRowWidget {};
    QList<CompactGpuMonitor *> m_deviceMonitors;
    // Compact memory view instance
    CompactMemoryMonitor *m_memoryMonitor {};
    // Compact network view instance
    CompactNetworkMonitor *m_networkMonitor {};
    // Compact disk view instance
    CompactDiskMonitor *m_diskMonitor {};
};

#endif  // MONITOR_COMPACT_VIEW_H
