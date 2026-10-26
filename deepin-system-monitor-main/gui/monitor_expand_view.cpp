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
    // gpu monitor view
    m_gpuMonitor = new GpuMonitor(this);
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

    // CPU / GPU tab switcher: switch between the CPU and GPU usage chart inside
    // the same area. Only one chart is shown at a time so they no longer
    // overlap in the expand view, and each chart gets the full available width.
    m_cpuGpuTabBox = new DButtonBox(this);
    m_cpuGpuTabBox->setObjectName("CpuGpuTabBox");
    m_cpuGpuTabBox->setAccessibleName("CpuGpuTabBox");
    m_cpuTabBtn = new DButtonBoxButton("CPU", m_cpuGpuTabBox);
    m_gpuTabBtn = new DButtonBoxButton("GPU", m_cpuGpuTabBox);
    QList<DButtonBoxButton *> tabBtns;
    tabBtns << m_cpuTabBtn << m_gpuTabBtn;
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
    cpuGpuLayout->addWidget(m_gpuMonitor);
    cpuGpuRow->setFixedWidth(totalWidth);

    // show CPU chart by default, hide GPU chart
    m_cpuMonitor->setVisible(true);
    m_gpuMonitor->setVisible(false);

    connect(m_cpuTabBtn, &DButtonBoxButton::toggled, this, [=](bool checked) {
        if (checked) {
            m_cpuMonitor->setVisible(true);
            m_gpuMonitor->setVisible(false);
        }
    });
    connect(m_gpuTabBtn, &DButtonBoxButton::toggled, this, [=](bool checked) {
        if (checked) {
            m_cpuMonitor->setVisible(false);
            m_gpuMonitor->setVisible(true);
        }
    });

    auto refreshGpuVisibility = [=]() {
        const bool hasGpu = GPUInfoModel::instance()->gpuCount() > 0;
        m_gpuTabBtn->setVisible(hasGpu);
        if (!hasGpu) {
            m_cpuTabBtn->setChecked(true);
            m_cpuMonitor->setVisible(true);
            m_gpuMonitor->setVisible(false);
        }
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

    connect(m_cpuMonitor, &CpuMonitor::signalDetailInfoClicked, this, &MonitorExpandView::signalDetailInfoClicked);
    // The GPU detail link should open the GPU detail page directly. Route it
    // through the same MSG_GPU path used by clicking the GPU chart, instead of
    // the last-selected-page logic in DetailViewStackedWidget::onDetailInfoClicked.
    connect(m_gpuMonitor, &GpuMonitor::signalDetailInfoClicked, this, [this]() {
        emit signalDetailInfoByDbus("MSG_GPU");
    });
    // 连接dbus发来的信号
    connect(&DetailWidgetManager::getInstance(), &DetailWidgetManager::sigJumpToDetailWidget, this, [=](QString msgCode) {
        qCDebug(app) << "Received signal from DBus to show detail info:" << msgCode;
        emit signalDetailInfoByDbus(msgCode);
    });

    //点击左侧区域触发跳转
    connect(m_cpuMonitor, &CpuMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_gpuMonitor, &GpuMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_memoryMonitor, &MemoryMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
    connect(m_networkMonitor, &NetworkMonitor::clicked, &DetailWidgetManager::getInstance(), &DetailWidgetManager::jumpDetailWidget);
}

void MonitorExpandView::setDetailButtonVisible(bool visible)
{
    qCDebug(app) << "Setting detail button visibility to:" << visible;
    m_cpuMonitor->setDetailButtonVisible(visible);
    m_gpuMonitor->setDetailButtonVisible(visible);
}
