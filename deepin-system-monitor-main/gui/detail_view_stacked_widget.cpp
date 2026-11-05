// Copyright (C) 2019 - 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "detail_view_stacked_widget.h"
#include "cpu_detail_widget.h"
#include "gpu_detail_view_widget.h"
#include "mem_detail_view_widget.h"
#include "netif_detail_view_widget.h"
#include "block_dev_detail_view_widget.h"
#include "model/gpu_info_model.h"
#include "ddlog.h"

#include <DMenu>
#include <DApplication>
#include <QActionGroup>

using namespace DDLog;

namespace {

// 设备详情页的跳转消息码，如 MSG_GPU1 / MSG_NPU2；前缀固定 7 个字符，
// 后缀为从 1 开始的设备编号
int deviceIndexFromMsgCode(const QString &msgCode, bool *npu)
{
    const QString upper = msgCode.toUpper();
    *npu = upper.startsWith(QString("MSG_NPU"));
    if (!upper.startsWith(QString("MSG_GPU")) && !(*npu))
        return -1;

    const QString suffix = upper.mid(7);
    if (suffix.isEmpty())
        return 0;  // 兼容旧消息码 MSG_GPU/MSG_NPU，跳到第一块设备
    bool ok = false;
    const int index = suffix.toInt(&ok) - 1;
    return ok ? index : -1;
}

}

#define DELETE_PAGE(obj) if(obj) { delete obj; obj = nullptr; }

DetailViewStackedWidget::DetailViewStackedWidget(QWidget *parent) : AnimationStackedWidget(LR, parent)
{
    qCDebug(app) << "DetailViewStackedWidget constructor";
    connect(this, &AnimationStackedWidget::signalIsFinished, this, &DetailViewStackedWidget::onSwitchPageFinished);
}

DetailViewStackedWidget::~DetailViewStackedWidget()
{
    // qCDebug(app) << "DetailViewStackedWidget destructor";
    if (m_processWidget) {
        // qCDebug(app) << "Deleting process widget";
        delete m_processWidget;
        m_processWidget = nullptr;
    }
    if (m_cpudetailWidget) {
        // qCDebug(app) << "Deleting CPU detail widget";
        delete m_cpudetailWidget;
        m_cpudetailWidget = nullptr;
    }
    qDeleteAll(m_gpuDetailWidgets);
    m_gpuDetailWidgets.clear();
    qDeleteAll(m_npuDetailWidgets);
    m_npuDetailWidgets.clear();
    if (m_memDetailWidget) {
        // qCDebug(app) << "Deleting memory detail widget";
        delete m_memDetailWidget;
        m_memDetailWidget = nullptr;
    }
    if (m_netifDetailWidget) {
        // qCDebug(app) << "Deleting network interface detail widget";
        delete m_netifDetailWidget;
        m_netifDetailWidget = nullptr;
    }
    if (m_blockDevDetailWidget) {
        // qCDebug(app) << "Deleting block device detail widget";
        delete m_blockDevDetailWidget;
        m_blockDevDetailWidget = nullptr;
    }
}

void DetailViewStackedWidget::addProcessWidget(QWidget *processWidget)
{
    qCDebug(app) << "Adding process widget";
    m_processWidget = processWidget;
    this->insertWidget(0, processWidget);
}

void DetailViewStackedWidget::onSwitchPageFinished()
{
    qCDebug(app) << "onSwitchPageFinished";
    if (this->currentWidget() == m_processWidget) {
        qCDebug(app) << "Current widget is process widget, deleting detail pages";
        deleteDetailPage();
    } else {
        qCDebug(app) << "Current widget is a detail page, updating last detail widget name";
        QWidget *cur = this->currentWidget();
        m_lastDteailWidgetName = cur ? cur->objectName() : QString();
        // 设备详情页（GPU/NPU）记录对应设备，便于"上次的详情页"恢复
        const QVariant deviceIndex = cur ? cur->property("deviceIndex") : QVariant();
        if (deviceIndex.isValid()) {
            m_lastDeviceIndex = deviceIndex.toInt();
            m_lastDeviceIsNpu = cur->property("deviceIsNpu").toBool();
        } else {
            m_lastDeviceIndex = -1;
        }
    }
}

void DetailViewStackedWidget::showDeviceDetailPage(int index, bool npu)
{
    auto &pages = npu ? m_npuDetailWidgets : m_gpuDetailWidgets;
    QWidget *page = pages.value(index, nullptr);
    if (page == nullptr) {
        qCDebug(app) << "Device detail widget is null, creating new one, index:" << index << "npu:" << npu;
        page = new GpuDetailViewWidget(index, npu, this);
        pages.insert(index, page);
        // 设备页集中在 CPU 页之后，切换通过指针完成，位置仅影响内部顺序
        this->insertWidget(2, page);
    }
    this->setCurrent(page);
}

void DetailViewStackedWidget::onDbusSendMsgChangeDetailInfoWidget(QString msgCode)
{
    qCDebug(app) << "onDbusSendMsgChangeDetailInfoWidget with message code:" << msgCode;
    if (msgCode.compare(QString("MSG_CPU"), Qt::CaseInsensitive) == 0) {
        qCDebug(app) << "Switching to CPU page";
        if (m_cpudetailWidget == nullptr) {
            qCDebug(app) << "CPU detail widget is null, creating new one";
            m_cpudetailWidget = new CPUDetailWidget(this);
            this->insertWidget(1, m_cpudetailWidget);
        }
        qCDebug(app) << "Setting current widget to CPU detail widget";
        this->setCurrent(m_cpudetailWidget);
        return;
    }

    // MSG_GPU（兼容旧消息码，跳到第一块 GPU）、MSG_GPU1/2...、MSG_NPU1/2...
    bool isNpu = false;
    const int deviceIndex = deviceIndexFromMsgCode(msgCode, &isNpu);
    if (deviceIndex >= 0) {
        const int availableCount = isNpu ? GPUInfoModel::instance()->npuCount()
                                         : GPUInfoModel::instance()->gpuCount();
        if (deviceIndex >= availableCount) {
            qCDebug(app) << "Device unavailable, ignore message code:" << msgCode;
            return;
        }
        qCDebug(app) << "Switching to device detail page, index:" << deviceIndex << "npu:" << isNpu;
        showDeviceDetailPage(deviceIndex, isNpu);
        return;
    }

    if (msgCode.compare(QString("MSG_NET"), Qt::CaseInsensitive) == 0) {
        qCDebug(app) << "Switching to Network page";
        if (m_netifDetailWidget == nullptr) {
            qCDebug(app) << "Network detail widget is null, creating new one";
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->insertWidget(4, m_netifDetailWidget);
        }
        qCDebug(app) << "Setting current widget to Network detail widget";
        this->setCurrent(m_netifDetailWidget);
        return;
    }

    if (msgCode.compare(QString("MSG_MEM"), Qt::CaseInsensitive) == 0) {
        qCDebug(app) << "Switching to Memory page";
        if (m_memDetailWidget == nullptr) {
            qCDebug(app) << "Memory detail widget is null, creating new one";
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->insertWidget(3, m_memDetailWidget);
        }
        qCDebug(app) << "Setting current widget to Memory detail widget";
        this->setCurrent(m_memDetailWidget);
        return;
    }

    if (msgCode.compare(QString("MSG_DISK"), Qt::CaseInsensitive) == 0) {
        qCDebug(app) << "Switching to Disk page";
        if (m_blockDevDetailWidget == nullptr) {
            qCDebug(app) << "Disk detail widget is null, creating new one";
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->insertWidget(5, m_blockDevDetailWidget);
        }
        qCDebug(app) << "Setting current widget to Disk detail widget";
        this->setCurrent(m_blockDevDetailWidget);
        return;
    }
}

void DetailViewStackedWidget::onDetailInfoClicked()
{
    qCDebug(app) << "onDetailInfoClicked, last detail widget name was" << m_lastDteailWidgetName;
    if (m_lastDteailWidgetName == "GpuDetailViewWidget") {
        const int availableCount = m_lastDeviceIsNpu ? GPUInfoModel::instance()->npuCount()
                                                     : GPUInfoModel::instance()->gpuCount();
        if (m_lastDeviceIndex < 0 || m_lastDeviceIndex >= availableCount) {
            qCDebug(app) << "GPU unavailable, fallback to CPU page";
            m_lastDteailWidgetName.clear();
        } else {
            qCDebug(app) << "Switching to device detail page, index:" << m_lastDeviceIndex
                         << "npu:" << m_lastDeviceIsNpu;
            showDeviceDetailPage(m_lastDeviceIndex, m_lastDeviceIsNpu);
            return;
        }
    } else if (m_lastDteailWidgetName == "MemDetailViewWidget") {
        qCDebug(app) << "Switching to Memory page";
        if (m_memDetailWidget == nullptr) {
            qCDebug(app) << "Memory detail widget is null, creating new one";
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->insertWidget(3, m_memDetailWidget);
        }
        this->setCurrent(m_memDetailWidget);
    } else if (m_lastDteailWidgetName == "NetifDetailViewWidget") {
        qCDebug(app) << "Switching to Network page";
        if (m_netifDetailWidget == nullptr) {
            qCDebug(app) << "Network detail widget is null, creating new one";
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->insertWidget(4, m_netifDetailWidget);
        }
        this->setCurrent(m_netifDetailWidget);
    } else if (m_lastDteailWidgetName == "BlockDevDetailViewWidget") {
        qCDebug(app) << "Switching to Disk page";
        if (m_blockDevDetailWidget == nullptr) {
            qCDebug(app) << "Disk detail widget is null, creating new one";
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->insertWidget(5, m_blockDevDetailWidget);
        }
        this->setCurrent(m_blockDevDetailWidget);
    } else {
        qCDebug(app) << "Switching to CPU page by default";
        if (m_cpudetailWidget == nullptr) {
            qCDebug(app) << "CPU detail widget is null, creating new one";
            m_cpudetailWidget = new CPUDetailWidget(this);
            this->insertWidget(1, m_cpudetailWidget);
        }
        this->setCurrent(m_cpudetailWidget);
    }
}

void DetailViewStackedWidget::onSwitchProcessPage()
{
    qCDebug(app) << "Switching to process page";
    this->setCurrent(m_processWidget);
}

void DetailViewStackedWidget::refreshDeviceActions()
{
    if (m_menu == nullptr)
        return;

    // 移除旧的设备动作
    for (QAction *act : m_gpuActs) {
        m_menu->removeAction(act);
        delete act;
    }
    m_gpuActs.clear();
    for (QAction *act : m_npuActs) {
        m_menu->removeAction(act);
        delete act;
    }
    m_npuActs.clear();

    // 按当前设备数量重建，插在"内存"项之前，顺序为 GPU1..N、NPU1..M
    for (int i = 0; i < GPUInfoModel::instance()->gpuCount(); ++i) {
        QAction *act = new QAction(GPUInfoModel::deviceDisplayName(i, false), m_menu);
        act->setObjectName(QString("GpuAct%1").arg(i + 1));
        act->setCheckable(true);
        m_menu->insertAction(memAct, act);
        if (m_actionGroup)
            m_actionGroup->addAction(act);
        m_gpuActs.append(act);
    }
    for (int i = 0; i < GPUInfoModel::instance()->npuCount(); ++i) {
        QAction *act = new QAction(GPUInfoModel::deviceDisplayName(i, true), m_menu);
        act->setObjectName(QString("NpuAct%1").arg(i + 1));
        act->setCheckable(true);
        m_menu->insertAction(memAct, act);
        if (m_actionGroup)
            m_actionGroup->addAction(act);
        m_npuActs.append(act);
    }
}

void DetailViewStackedWidget::onShowPerformMenu(QPoint pos)
{
    qCDebug(app) << "onShowPerformMenu";
    if (m_menu == nullptr) {
        qCDebug(app) << "Menu is null, creating new one";
        m_menu = new DMenu(this);
        m_menu->setObjectName("Menu");
        m_menu->setAccessibleName("Menu");
        cpuAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "CPU"));
        cpuAct->setObjectName("CpuAct");
        memAct = m_menu->addAction(DApplication::translate("Process.Graph.Title", "Memory"));
        memAct->setObjectName("MemAct");
        netifAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "Network"));
        netifAct->setObjectName("NetifAct");
        blockDevAct = m_menu->addAction(DApplication::translate("Process.Graph.View", "Disks"));
        blockDevAct->setObjectName("BlockDevAct");

        m_actionGroup = new QActionGroup(m_menu);
        m_actionGroup->addAction(cpuAct);
        m_actionGroup->addAction(memAct);
        m_actionGroup->addAction(netifAct);
        m_actionGroup->addAction(blockDevAct);

        cpuAct->setCheckable(true);
        memAct->setCheckable(true);
        netifAct->setCheckable(true);
        blockDevAct->setCheckable(true);
    }

    // 按当前设备数量重建 GPU1..N / NPU1..M 菜单项
    refreshDeviceActions();

    if (this->currentWidget() == m_cpudetailWidget) {
        qCDebug(app) << "Current widget is CPU, checking CPU action";
        cpuAct->setChecked(true);
    } else if (this->currentWidget() == m_memDetailWidget) {
        qCDebug(app) << "Current widget is Memory, checking Memory action";
        memAct->setChecked(true);
    } else if (this->currentWidget() == m_netifDetailWidget) {
        qCDebug(app) << "Current widget is Network, checking Network action";
        netifAct->setChecked(true);
    } else if (this->currentWidget() == m_blockDevDetailWidget) {
        qCDebug(app) << "Current widget is Disk, checking Disk action";
        blockDevAct->setChecked(true);
    }

    // 设备详情页对应菜单项打勾
    for (int i = 0; i < m_gpuActs.size(); ++i)
        m_gpuActs.at(i)->setChecked(this->currentWidget() == m_gpuDetailWidgets.value(i));
    for (int i = 0; i < m_npuActs.size(); ++i)
        m_npuActs.at(i)->setChecked(this->currentWidget() == m_npuDetailWidgets.value(i));

    QAction *resAct = m_menu->exec(pos);
    if (resAct == nullptr)
        return;
    if (resAct == cpuAct) {
        qCDebug(app) << "CPU action triggered";
        if (m_cpudetailWidget == nullptr) {
            qCDebug(app) << "CPU detail widget is null, creating new one";
            m_cpudetailWidget = new CPUDetailWidget(this);
            this->insertWidget(1, m_cpudetailWidget);
        }
        this->setCurrent(m_cpudetailWidget);
    } else if (m_gpuActs.contains(resAct)) {
        const int index = m_gpuActs.indexOf(resAct);
        if (index < GPUInfoModel::instance()->gpuCount()) {
            qCDebug(app) << "GPU action triggered, index:" << index;
            showDeviceDetailPage(index, false);
        }
    } else if (m_npuActs.contains(resAct)) {
        const int index = m_npuActs.indexOf(resAct);
        if (index < GPUInfoModel::instance()->npuCount()) {
            qCDebug(app) << "NPU action triggered, index:" << index;
            showDeviceDetailPage(index, true);
        }
    } else if (resAct == memAct) {
        qCDebug(app) << "Memory action triggered";
        if (m_memDetailWidget == nullptr) {
            qCDebug(app) << "Memory detail widget is null, creating new one";
            m_memDetailWidget = new MemDetailViewWidget(this);
            this->insertWidget(3, m_memDetailWidget);
        }
        this->setCurrent(m_memDetailWidget);
    } else if (resAct == netifAct) {
        qCDebug(app) << "Network action triggered";
        if (m_netifDetailWidget == nullptr) {
            qCDebug(app) << "Network detail widget is null, creating new one";
            m_netifDetailWidget = new NetifDetailViewWidget(this);
            this->insertWidget(4, m_netifDetailWidget);
        }
        this->setCurrent(m_netifDetailWidget);
    } else if (resAct == blockDevAct) {
        qCDebug(app) << "Disk action triggered";
        if (m_blockDevDetailWidget == nullptr) {
            qCDebug(app) << "Disk detail widget is null, creating new one";
            m_blockDevDetailWidget = new BlockDevDetailViewWidget(this);
            this->insertWidget(5, m_blockDevDetailWidget);
        }
        this->setCurrent(m_blockDevDetailWidget);
    }
}

void DetailViewStackedWidget::deleteDetailPage()
{
    qCDebug(app) << "Deleting all detail pages";
    DELETE_PAGE(m_cpudetailWidget);
    qDeleteAll(m_gpuDetailWidgets);
    m_gpuDetailWidgets.clear();
    qDeleteAll(m_npuDetailWidgets);
    m_npuDetailWidgets.clear();
    DELETE_PAGE(m_memDetailWidget);
    DELETE_PAGE(m_netifDetailWidget);
    DELETE_PAGE(m_blockDevDetailWidget);
    // 注意：不清除 m_lastDteailWidgetName/m_lastDeviceIndex，
    // "上次的详情页"记忆需要跨页存活
}
