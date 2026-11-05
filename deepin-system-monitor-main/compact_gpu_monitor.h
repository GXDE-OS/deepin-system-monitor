// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COMPACTGPUMONITOR_H
#define COMPACTGPUMONITOR_H

#include <QWidget>
#include <QPainterPath>

class GPUInfoModel;

/**
 * @brief Compact GPU/NPU monitor widget for the compact view (bar chart style)
 */
class CompactGpuMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit CompactGpuMonitor(int deviceIndex, bool isNpu = false, QWidget *parent = nullptr);
    ~CompactGpuMonitor();

    // Message code used to jump to this device's detail page, e.g. "MSG_GPU1"
    QString msgCode() const;

    int deviceIndex() const { return m_deviceIndex; }
    bool isNpu() const { return m_isNpu; }

signals:
    void clicked(QString msgCode);

public slots:
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    void changeFont(const QFont &font);
    void getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxValue);

private:
    int m_deviceIndex {0};
    bool m_isNpu {false};

    QList<qreal> *gpuPercents;
    qreal m_gpuPercent {};

    int m_bulletSize = 6;

    QColor m_gpuColor {"#AA00FF"};

    QPainterPath gpuPath;

    int renderMaxHeight = 80;
    // 历史采样点数量；曲线横向间距在 paintEvent 中按实际图表宽度计算
    int pointsNumber = 25;

    QFont m_tagFont;
    QFont m_statFont;

    GPUInfoModel *m_gpuInfoModel {};
};

#endif // COMPACTGPUMONITOR_H
