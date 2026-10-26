// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GPU_MONITOR_H
#define GPU_MONITOR_H

#include <QIcon>
#include <QList>
#include <QWidget>
#include <QPainterPath>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#endif

class QPropertyAnimation;
class GPUInfoModel;
class BaseCommandLinkButton;

/**
 * @brief GPU monitor widget for the expand view.
 *        Renders the same style as CpuMonitor (title + icon, big percent,
 *        loading ring and a utilization waveform), showing only the total
 *        GPU utilization.
 */
class GpuMonitor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit GpuMonitor(QWidget *parent = nullptr);
    ~GpuMonitor();

signals:
    void clicked(QString msgCode);
    void signalDetailInfoClicked();

public slots:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void changeTheme(DApplicationHelper::ColorType themeType);
#else
    void changeTheme(DGuiApplicationHelper::ColorType themeType);
#endif
    void updateStatus();
    void onDetailInfoClicked();

    void setDetailButtonVisible(bool visible);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    qreal progress() const { return m_progress; }
    void setProgress(qreal p) { m_progress = p; }

private:
    void changeFont(const QFont &font);
    void resizeItemWidgetRect();

private:
    QIcon m_icon;

    QList<double> *gpuPercents;
    QPainterPath gpuPath;
    QColor numberColor;
    QColor ringBackgroundColor;
    QColor ringForegroundColor {"#AA00FF"};
    QColor textColor;
    double ringBackgroundOpacity = 0.1;
    double ringForegroundOpacity = 1;
    int gpuRenderMaxHeight = 45;
    int percentRenderOffsetY = 131;
    int pointsNumber = 28;
    int ringRadius = 90;
    int ringRenderOffsetY = 86;
    int ringWidth = 8;
    int titleRenderOffsetY = 161;
    int waveformsRenderOffsetX;
    int waveformsRenderOffsetY = 96;

    QFont m_gpuUsageFont;
    QFont m_gpuDisplayFont;

    qreal m_progress {};
    QPropertyAnimation *m_animation {};

    GPUInfoModel *m_gpuInfoModel {};

    BaseCommandLinkButton *m_detailButton {};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DApplicationHelper::ColorType m_themeType;
#else
    DGuiApplicationHelper::ColorType m_themeType;
#endif
};

#endif // GPU_MONITOR_H
