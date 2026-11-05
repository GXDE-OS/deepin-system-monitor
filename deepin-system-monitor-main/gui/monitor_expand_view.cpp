// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "monitor_expand_view.h"

#include "cpu_monitor.h"
#include "gpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "detailwidgetmanager.h"
#include "ddlog.h"
#include "common/common.h"
#include "model/gpu_info_model.h"

#include <DPalette>
#include <DStyle>
#include <DButtonBox>

#include <QHBoxLayout>
#include <QVBoxLayout>

using namespace DDLog;

// constructor
MonitorExpandView::MonitorExpandView(QWidget *parent)
    : DFrame(parent)
{
    qCDebug(app) << "MonitorExpandView constructor";
    // disable auto fill frame background
    setAutoFillBackground(false);
    // set background role
    setBackgroundRole(DPalette::Window);
    // frameless style
    setFrameStyle(DFrame::NoFrame);

    // cpu monitor view
    m_cpuMonitor = new CpuMonitor(this);
    // memory monitor view
    m_memoryMonitor = new MemoryMonitor(this);
    // network monitor view
    m_networkMonitor = new NetworkMonitor(this);

    const int totalWidth = common::getStatusBarMaxWidth();

    // monitor view layout
    QVBoxLayout *layout = new QVBoxLayout(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    layout->setMargin(0);
#else
    layout->setContentsMargins(0, 0, 0, 0);
#endif
    layout->setSpacing(0);

    // CPU / GPU1 / GPU2 / ... tab switcher: switch between the CPU and
    // per-device GPU/NPU usage charts inside the same area. Only one chart is
    // shown at a time so they no longer overlap in the expand view, and each
    // chart gets the full available width. Device tabs are (re)built in
    // refreshGpuVisibility below according to the detected devices.
    m_cpuGpuTabBox = new DButtonBox(this);
    m_cpuGpuTabBox->setObjectName("CpuGpuTabBox");
    m_cpuGpuTabBox->setAccessibleName("CpuGpuTabBox");
    m_cpuTabBtn = new DButtonBoxButton("CPU", m_cpuGpuTabBox);
    QList<DButtonBoxButton *> tabBtns;
    tabBtns << m_cpuTabBtn;
    m_cpuGpuTabBox->setButtonList(tabBtns, true);
    m_cpuTabBtn->setChecked(true);

    QWidget *cpuGpuRow = new QWidget(this);
    QVBoxLayout *cpuGpuLayout = new QVBoxLayout(cpuGpuRow);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    cpuGpuLayout->setMargin(0);
#else
    cpuGpuLayout->setContentsMargins(0, 0, 0, 0);
#endif
    cpuGpuLayout->setSpacing(8);
    cpuGpuLayout->addWidget(m_cpuGpuTabBox, 0, Qt::AlignLeft);
    cpuGpuLayout->addWidget(m_cpuMonitor);
    cpuGpuRow->setFixedWidth(totalWidth);

    // show CPU chart by default
    m_cpuMonitor->setVisible(true);

    connect(m_cpuTabBtn, &DButtonBoxButton::toggled, this, [=](bool checked) {
        if (checked) {
            m_cpuMonitor->setVisible(true);
            for (auto *monitor : m_deviceMonitors)
                monitor->setVisible(false);
        }
    });

    auto refreshGpuVisibility = [=]() {
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
        if (!needRebuild)
            return;

        // 清理旧的设备标签与监控视图
        qDeleteAll(m_deviceMonitors);
        m_deviceMonitors.clear();
        for (auto *btn : m_deviceTabBtns)
            btn->deleteLater();
        m_deviceTabBtns.clear();

        if (devices.isEmpty()) {
            // 无 GPU/NPU 设备时回到 CPU 图表
            m_cpuTabBtn->setChecked(true);
            m_cpuMonitor->setVisible(true);
            return;
        }

        QList<DButtonBoxButton *> tabBtns;
        tabBtns << m_cpuTabBtn;
        for (const auto &dev : devices) {
            auto *monitor = new GpuMonitor(dev.first, dev.second, this);
            monitor->setVisible(false);
            connect(monitor, &GpuMonitor::clicked,
                    &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
            connect(monitor, &GpuMonitor::signalDetailInfoClicked, this, [this](const QString &msgCode) {
                emit signalDetailInfoByDbus(msgCode);
            });
            cpuGpuLayout->addWidget(monitor);
            m_deviceMonitors.append(monitor);

            auto *btn = new DButtonBoxButton(GPUInfoModel::deviceDisplayName(dev.first, dev.second),
                                             m_cpuGpuTabBox);
            connect(btn, &DButtonBoxButton::toggled, this, [=](bool checked) {
                if (checked) {
                    m_cpuMonitor->setVisible(false);
                    for (auto *m : m_deviceMonitors)
                        m->setVisible(m == monitor);
                }
            });
            tabBtns << btn;
            m_deviceTabBtns.append(btn);
        }
        m_cpuGpuTabBox->setButtonList(tabBtns, true);

        // 重建后默认回到 CPU 图表
        m_cpuTabBtn->setChecked(true);
        m_cpuMonitor->setVisible(true);
    };
    refreshGpuVisibility();
    connect(GPUInfoModel::instance(), &GPUInfoModel::modelUpdated, this, refreshGpuVisibility);

    layout->addStretch(1);
    layout->addWidget(cpuGpuRow, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(1);
    // set monitor view layout
    setLayout(layout);

    // CPU/GPU详情按钮都通过MSG_XXX直接跳转对应详情页，与点击图表区域的行为一致，
    // 避免"打开上次详情页"导致切换不到对应页面
    connect(m_cpuMonitor, &CpuMonitor::signalDetailInfoClicked, this, [this]() {
        emit signalDetailInfoByDbus("MSG_CPU");
    });
    // 连接dbus发来的信号
    connect(&DetailWidgetManager::getInstance(), &DetailWidgetManager::sigJumpToDetailWidget, this, [=](QString msgCode) {
        qCDebug(app) << "Received signal from DBus to show detail info:" << msgCode;
        emit signalDetailInfoByDbus(msgCode);
    });

    //点击左侧区域触发跳转
    connect(m_cpuMonitor, &CpuMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_memoryMonitor, &MemoryMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_networkMonitor, &NetworkMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
}

void MonitorExpandView::setDetailButtonVisible(bool visible)
{
    qCDebug(app) << "Setting detail button visibility to:" << visible;
    m_cpuMonitor->setDetailButtonVisible(visible);
    for (auto *monitor : m_deviceMonitors)
        monitor->setDetailButtonVisible(visible);
}
