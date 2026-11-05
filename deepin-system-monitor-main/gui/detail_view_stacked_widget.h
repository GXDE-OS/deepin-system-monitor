// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DETAIL_VIEW_STACKED_WIDGET_H
#define DETAIL_VIEW_STACKED_WIDGET_H

#include "animation_stackedwidget.h"
#include <DMenu>
#include <QList>
#include <QMap>

/**
 * @brief Detail view stacked widget (main content stacked area)
 */
class MemDetailViewWidget;
class GpuDetailViewWidget;
class NetifDetailViewWidget;
class BlockDevDetailViewWidget;
class QActionGroup;

DWIDGET_USE_NAMESPACE
class DetailViewStackedWidget : public AnimationStackedWidget
{
public:
    explicit DetailViewStackedWidget(QWidget *parent);

    virtual ~DetailViewStackedWidget();

    void addProcessWidget(QWidget *processWidget);

    void deleteDetailPage();

public slots:
    void onShowPerformMenu(QPoint pos);
    void onDetailInfoClicked();
    void onSwitchProcessPage();
    void onSwitchPageFinished();

    void onDbusSendMsgChangeDetailInfoWidget(QString msgCode);

private:
    // 创建（如未创建）并切换到第 index 块 GPU/NPU 设备的详情页
    void showDeviceDetailPage(int index, bool npu);
    // 按当前设备数量重建菜单中的 GPU1..N / NPU1..M 项
    void refreshDeviceActions();

private:
    QString m_lastDteailWidgetName;
    // "上次的详情页"为设备页时记录对应设备，返回详情模式时恢复
    int m_lastDeviceIndex = -1;
    bool m_lastDeviceIsNpu = false;

    QWidget *m_processWidget = nullptr;
    QWidget *m_cpudetailWidget = nullptr;
    // 每块 GPU/NPU 一页，键为设备索引（0 起）
    QMap<int, QWidget *> m_gpuDetailWidgets;
    QMap<int, QWidget *> m_npuDetailWidgets;
    QWidget *m_memDetailWidget = nullptr;
    QWidget *m_netifDetailWidget = nullptr;
    QWidget *m_blockDevDetailWidget = nullptr;

    QAction *cpuAct = nullptr;
    QList<QAction *> m_gpuActs;
    QList<QAction *> m_npuActs;
    QAction *memAct = nullptr;
    QAction *netifAct = nullptr;
    QAction *blockDevAct = nullptr;

    QActionGroup *m_actionGroup = nullptr;

    DMenu *m_menu = nullptr;
};

#endif // DETAIL_VIEW_STACKED_WIDGET_H
