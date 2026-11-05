// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GPU_DETAIL_VIEW_WIDGET_H
#define GPU_DETAIL_VIEW_WIDGET_H

#include "base/base_detail_view_widget.h"

class ChartViewWidget;
class QLabel;
class QTableView;
class QAbstractTableModel;

class GpuDetailViewWidget : public BaseDetailViewWidget
{
    Q_OBJECT

public:
    explicit GpuDetailViewWidget(int deviceIndex, bool isNpu = false, QWidget *parent = nullptr);

public slots:
    void detailFontChanged(const QFont &font);
    void onModelUpdate();

private:
    int m_deviceIndex {0};
    bool m_isNpu {false};

    QWidget *m_utilizationWidget {};
    QLabel *m_utilizationTitleLabel {};
    ChartViewWidget *m_utilizationChartWidget {};
    QLabel *m_memoryTitleLabel {};
    ChartViewWidget *m_memoryChartWidget {};
    QTableView *m_deviceTable {};
    QAbstractTableModel *m_deviceModel {};
};

#endif // GPU_DETAIL_VIEW_WIDGET_H
