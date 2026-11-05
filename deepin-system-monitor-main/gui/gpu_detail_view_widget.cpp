// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gpu_detail_view_widget.h"

#include "base/base_detail_item_delegate.h"
#include "chart_view_widget.h"
#include "common/common.h"
#include "model/gpu_info_model.h"
#include "system/system_monitor.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif

#include <QAbstractTableModel>
#include <QApplication>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>

using namespace common::format;
using namespace core::system;

namespace {

class GPUSummaryTableModel : public QAbstractTableModel
{
public:
    explicit GPUSummaryTableModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    void setEntries(const QList<QPair<QString, QString>> &entries)
    {
        beginResetModel();
        m_entries = entries;
        endResetModel();
    }

    int rowCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;
        return (m_entries.size() + 1) / 2;
    }

    int columnCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;
        return 2;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        const int entryIndex = index.row() * 2 + index.column();
        if (entryIndex < 0 || entryIndex >= m_entries.size())
            return QVariant();

        if (role == Qt::DisplayRole)
            return m_entries.at(entryIndex).first;

        if (role == Qt::UserRole)
            return m_entries.at(entryIndex).second;

        if (
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            role == Qt::TextColorRole
#else
            role == Qt::ForegroundRole
#endif
        ) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            const auto &palette = DApplicationHelper::instance()->applicationPalette();
#else
            const auto &palette = DGuiApplicationHelper::instance()->applicationPalette();
#endif
            return palette.color(DPalette::Text);
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &) const override
    {
        return Qt::NoItemFlags;
    }

private:
    QList<QPair<QString, QString>> m_entries;
};

}

GpuDetailViewWidget::GpuDetailViewWidget(int deviceIndex, bool isNpu, QWidget *parent)
    : BaseDetailViewWidget(parent)
    , m_deviceIndex(deviceIndex)
    , m_isNpu(isNpu)
{
    this->setObjectName("GpuDetailViewWidget");
    // 记录设备信息，供 DetailViewStackedWidget 恢复"上次的详情页"
    this->setProperty("deviceIndex", deviceIndex);
    this->setProperty("deviceIsNpu", isNpu);
    setTitle(GPUInfoModel::deviceDisplayName(deviceIndex, isNpu));

    m_utilizationWidget = new QWidget(this);
    auto *utilizationLayout = new QGridLayout(m_utilizationWidget);
    utilizationLayout->setContentsMargins(0, 8, 0, 0);
    utilizationLayout->setHorizontalSpacing(12);
    utilizationLayout->setVerticalSpacing(6);

    const QString utilizationTitle = m_isNpu
        ? DApplication::translate("Process.Graph.View", "NPU Utilization")
        : DApplication::translate("Process.Graph.View", "GPU Utilization");
    m_utilizationTitleLabel = new QLabel(utilizationTitle, m_utilizationWidget);

    m_utilizationChartWidget = new ChartViewWidget(ChartViewWidget::ChartViewTypes::MEM_CHART, m_utilizationWidget);
    // NPU 页使用与原 NPU 图表一致的蓝色，GPU 页保持紫色
    m_utilizationChartWidget->setData1Color(m_isNpu ? QColor("#00A3FF") : QColor("#AA00FF"));
    m_utilizationChartWidget->setMinimumHeight(180);

    utilizationLayout->addWidget(m_utilizationTitleLabel, 0, 0);
    utilizationLayout->addWidget(m_utilizationChartWidget, 1, 0);
    utilizationLayout->setColumnStretch(0, 1);

    const QString memoryUtilizationTitle = m_isNpu
        ? DApplication::translate("Process.Graph.View", "NPU Memory Utilization")
        : DApplication::translate("Process.Graph.View", "GPU Memory Utilization");
    m_memoryTitleLabel = new QLabel(memoryUtilizationTitle, this);
    m_memoryChartWidget = new ChartViewWidget(ChartViewWidget::ChartViewTypes::MEM_CHART, this);
    m_memoryChartWidget->setData1Color(QColor("#00C5C0"));
    m_memoryChartWidget->setMinimumHeight(170);

    m_deviceTable = new QTableView(this);
    m_deviceModel = new GPUSummaryTableModel(this);
    m_deviceTable->setModel(m_deviceModel);
    m_deviceTable->setItemDelegate(new BaseDetailItemDelegate(this));
    m_deviceTable->horizontalHeader()->setVisible(false);
    m_deviceTable->verticalHeader()->setVisible(false);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_deviceTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_deviceTable->setGridStyle(Qt::NoPen);
    m_deviceTable->setFrameShape(QFrame::NoFrame);
    m_deviceTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->setFocusPolicy(Qt::NoFocus);
    m_deviceTable->setMinimumHeight(240);

    m_centralLayout->addWidget(m_utilizationWidget);
    m_centralLayout->addWidget(m_memoryTitleLabel);
    m_centralLayout->addWidget(m_memoryChartWidget);
    m_centralLayout->addWidget(m_deviceTable);

    detailFontChanged(DApplication::font());
    onModelUpdate();

    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &GpuDetailViewWidget::onModelUpdate);
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &GpuDetailViewWidget::detailFontChanged);
}

void GpuDetailViewWidget::detailFontChanged(const QFont &font)
{
    BaseDetailViewWidget::detailFontChanged(font);

    QFont contentFont = font;
    contentFont.setPointSizeF(contentFont.pointSizeF() - 1);

    m_utilizationWidget->setFont(contentFont);
    m_memoryTitleLabel->setFont(contentFont);
    m_deviceTable->setFont(contentFont);
}

void GpuDetailViewWidget::onModelUpdate()
{
    auto *gpuModel = GPUInfoModel::instance();
    auto device = gpuModel->device(m_deviceIndex, m_isNpu);

    const qreal utilization = device ? qreal(device->gpuUtilization) : 0.0;
    const qreal memoryUtilization = device ? qreal(device->memoryUtilization) : 0.0;

    m_utilizationChartWidget->addData1(utilization / 100.0);
    m_memoryChartWidget->addData1(memoryUtilization / 100.0);

    const QString utilizationTitle = m_isNpu
        ? DApplication::translate("Process.Graph.View", "NPU Utilization")
        : DApplication::translate("Process.Graph.View", "GPU Utilization");

    m_utilizationTitleLabel->setText(QString("%1 (%2%)")
                                            .arg(utilizationTitle)
                                            .arg(QString::number(utilization, 'f', 1)));

    setDetail(QString("%1 (%2%)")
                  .arg(utilizationTitle)
                  .arg(QString::number(utilization, 'f', 1)));

    QList<QPair<QString, QString>> tableEntries;

    auto addEntry = [&tableEntries](const QString &field, const QString &value) {
        tableEntries.append(qMakePair(field, value));
    };

    if (device) {
        const QString indexName = GPUInfoModel::deviceDisplayName(m_deviceIndex, m_isNpu);

        const QString name = device->name.isEmpty() ? "-" : device->name;
        const QString vendor = device->vendor.isEmpty() ? "-" : device->vendor;

        QString deviceClass;
        if (device->isNpu) {
            deviceClass = DApplication::translate("Process.Graph.View", "NPU");
        } else if (device->isIntegrated) {
            deviceClass = DApplication::translate("Process.Graph.View", "Embedded in CPU");
        } else {
            deviceClass = DApplication::translate("Process.Graph.View", "External");
        }

        addEntry(DApplication::translate("Process.Graph.View", "Device"), QString("%1 (%2)").arg(indexName).arg(deviceClass));

        QString memory {"-"};
        if (device->totalMemory > 0) {
            memory = QString("%1 / %2")
                         .arg(formatUnit_memory_disk(device->usedMemory, B, 1))
                         .arg(formatUnit_memory_disk(device->totalMemory, B, 1));
        }

        QString temp {"-"};
        if (device->temperature > 0.0f)
            temp = QString("%1 C").arg(QString::number(device->temperature, 'f', 1));

        QString sharedMemory {"-"};
        if (!device->isNpu && device->isIntegrated) {
            if (device->totalMemory > 0)
                sharedMemory = formatUnit_memory_disk(device->totalMemory, B, 1);
            else
                sharedMemory = DApplication::translate("Process.Graph.View", "System RAM (dynamic)");
        }

        QString dedicatedMemory {"-"};
        if (!device->isNpu && !device->isIntegrated && device->totalMemory > 0)
            dedicatedMemory = formatUnit_memory_disk(device->totalMemory, B, 1);

        addEntry(DApplication::translate("Process.Graph.View", "Model"), name);
        addEntry(DApplication::translate("Process.Graph.View", "Vendor"), vendor);
        addEntry(DApplication::translate("Process.Graph.View", "Bus Interface"), device->busInfo.isEmpty() ? "-" : device->busInfo);
        addEntry(DApplication::translate("Process.Graph.View", "Active Time"), QString("%1%").arg(QString::number(device->gpuUtilization, 'f', 1)));

        QString engineValue {"-"};
        if (!device->engineNames.isEmpty() && device->engineNames.size() == device->engineLoads.size()) {
            QStringList enginePairs;
            for (int i = 0; i < device->engineNames.size(); ++i) {
                enginePairs << QString("%1 %2%")
                                   .arg(device->engineNames.at(i))
                                   .arg(QString::number(device->engineLoads.at(i), 'f', 1));
            }
            engineValue = enginePairs.join("; ");
        }
        addEntry(DApplication::translate("Process.Graph.View", "Engines"), engineValue);

        addEntry(DApplication::translate("Process.Graph.View", "Temperature"), temp);
        addEntry(DApplication::translate("Process.Graph.View", "Memory"), memory);
        addEntry(DApplication::translate("Process.Graph.View", "Shared Memory"), sharedMemory);
        addEntry(DApplication::translate("Process.Graph.View", "Dedicated Memory"), dedicatedMemory);
    }

    static_cast<GPUSummaryTableModel *>(m_deviceModel)->setEntries(tableEntries);
}
