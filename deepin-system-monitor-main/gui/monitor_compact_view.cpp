// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "monitor_compact_view.h"

#include "compact_cpu_monitor.h"
#include "compact_gpu_monitor.h"
#include "compact_disk_monitor.h"
#include "compact_memory_monitor.h"
#include "compact_network_monitor.h"
#include "detailwidgetmanager.h"
#include "ddlog.h"
#include "common/common.h"
#include "model/gpu_info_model.h"

#include <DPalette>
#include <DStyle>

#include <QHBoxLayout>
#include <QVBoxLayout>

using namespace DDLog;

// constructor
MonitorCompactView::MonitorCompactView(QWidget *parent)
    : DFrame(parent)
{
    qCDebug(app) << "MonitorCompactView constructor";
    // disable auto fill frame background
    setAutoFillBackground(false);
    // set frame background role
    setBackgroundRole(DPalette::Window);
    // set no frame borders
    setFrameStyle(DFrame::NoFrame);

    // cpu monitor view instance (full-width row)
    m_cpuMonitor = new CompactCpuMonitor(this);
    // memory monitor view instance
    m_memoryMonitor = new CompactMemoryMonitor(this);
    // network monitor view instance
    m_networkMonitor = new CompactNetworkMonitor(this);
    // disk monitor view instance
    m_diskMonitor = new CompactDiskMonitor(this);

    const int totalWidth = common::getStatusBarMaxWidth();
    const int deviceSpacing = 8;
    m_cpuMonitor->setFixedWidth(totalWidth);

    // vertical layout to hold monitor instances
    QVBoxLayout *layout = new QVBoxLayout(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    layout->setMargin(0);
#else
    layout->setContentsMargins(0, 0, 0, 0);
#endif
    layout->setSpacing(10);

    // CPU 独占一行，所有 GPU/NPU 图表共用下一行并平分该行宽度
    m_deviceRowWidget = new QWidget(this);
    QHBoxLayout *deviceRowLayout = new QHBoxLayout(m_deviceRowWidget);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    deviceRowLayout->setMargin(0);
#else
    deviceRowLayout->setContentsMargins(0, 0, 0, 0);
#endif
    deviceRowLayout->setSpacing(deviceSpacing);
    m_deviceRowWidget->setFixedWidth(totalWidth);

    auto refreshDeviceRow = [this, deviceRowLayout, totalWidth, deviceSpacing]() {
        // 期望显示的设备列表：GPU1..N，随后 NPU1..M
        QList<QPair<int, bool>> devices;
        for (int i = 0; i < GPUInfoModel::instance()->gpuCount(); ++i)
            devices.append(qMakePair(i, false));
        for (int i = 0; i < GPUInfoModel::instance()->npuCount(); ++i)
            devices.append(qMakePair(i, true));

        bool needRebuild = devices.size() != m_deviceMonitors.size();
        if (!needRebuild) {
            for (int i = 0; i < devices.size(); ++i) {
                if (m_deviceMonitors.at(i)->deviceIndex() != devices.at(i).first
                        || m_deviceMonitors.at(i)->isNpu() != devices.at(i).second) {
                    needRebuild = true;
                    break;
                }
            }
        }
        if (needRebuild) {
            while (QLayoutItem *item = deviceRowLayout->takeAt(0))
                delete item;
            qDeleteAll(m_deviceMonitors);
            m_deviceMonitors.clear();
            for (const auto &dev : devices) {
                auto *monitor = new CompactGpuMonitor(dev.first, dev.second, m_deviceRowWidget);
                connect(monitor, &CompactGpuMonitor::clicked,
                        &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
                deviceRowLayout->addWidget(monitor);
                m_deviceMonitors.append(monitor);
            }
        }
        m_deviceRowWidget->setVisible(!m_deviceMonitors.isEmpty());

        // 按设备数平分整行宽度，除不尽的余量给最后一个图表
        const int count = m_deviceMonitors.size();
        if (count > 0) {
            const int width = (totalWidth - (count - 1) * deviceSpacing) / count;
            for (int i = 0; i < count; ++i) {
                const int w = (i == count - 1)
                                  ? totalWidth - (count - 1) * (width + deviceSpacing)
                                  : width;
                m_deviceMonitors.at(i)->setFixedWidth(w);
            }
        }
    };
    refreshDeviceRow();
    connect(GPUInfoModel::instance(), &GPUInfoModel::modelUpdated, this, refreshDeviceRow);

    layout->addStretch(1);
    layout->addWidget(m_cpuMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(m_deviceRowWidget, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_diskMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(1);

    // set frame layout
    setLayout(layout);

    // 连接dbus发来的信号
    connect(&DetailWidgetManager::getInstance(), &DetailWidgetManager::sigJumpToDetailWidget, this, [=](QString msgCode) {
        qCDebug(app) << "Received signal from DBus to show detail info:" << msgCode;
        emit signalDetailInfoByDbus(msgCode);
    });

    //点击左侧区域触发跳转
    connect(m_cpuMonitor, &CompactCpuMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_memoryMonitor, &CompactMemoryMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_networkMonitor, &CompactNetworkMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_diskMonitor, &CompactDiskMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
}
