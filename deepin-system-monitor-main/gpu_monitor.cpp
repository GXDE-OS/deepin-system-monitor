// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gpu_monitor.h"

#include "smooth_curve_generator.h"
#include "gui/ui_common.h"
#include "common/common.h"
#include "model/gpu_info_model.h"
#include "gui/base/base_commandlink_button.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <DPalette>
#include <DStyle>
#include <DFontSizeManager>

#include <QDebug>
#include <QPainter>
#include <QtMath>
#include <QPropertyAnimation>
#include <QPainterPath>
#include <QMouseEvent>
#include "ddlog.h"

DWIDGET_USE_NAMESPACE

using namespace common;
using namespace DDLog;

GpuMonitor::GpuMonitor(QWidget *parent)
    : QWidget(parent)
{
    qCDebug(app) << "GpuMonitor constructor";
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif

    int statusBarMaxWidth = common::getStatusBarMaxWidth();
    setFixedSize(statusBarMaxWidth, 240);
    waveformsRenderOffsetX = (statusBarMaxWidth - 140) / 2;

    gpuPercents = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        gpuPercents->append(0);
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &GpuMonitor::changeTheme);
#else
    connect(dAppHelper, &DGuiApplicationHelper::themeTypeChanged, this, &GpuMonitor::changeTheme);
#endif
    m_themeType = dAppHelper->themeType();
    changeTheme(m_themeType);

    m_animation = new QPropertyAnimation(this, "progress", this);
    m_animation->setDuration(20);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_animation, &QVariantAnimation::valueChanged, [ = ]() {
        update();
    });

    m_gpuInfoModel = GPUInfoModel::instance();
    connect(m_gpuInfoModel, &GPUInfoModel::modelUpdated, this, &GpuMonitor::updateStatus);

    m_detailButton = new BaseCommandLinkButton(tr("Details"), this);
    m_detailButton->setObjectName("GpuDetailButton");
    m_detailButton->setAccessibleName("GpuDetailButton");
    DFontSizeManager::instance()->bind(m_detailButton, DFontSizeManager::T8, QFont::Medium);
    connect(m_detailButton, &BaseCommandLinkButton::clicked, this, &GpuMonitor::onDetailInfoClicked);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &GpuMonitor::changeFont);
}

GpuMonitor::~GpuMonitor()
{
    delete gpuPercents;
}

void GpuMonitor::updateStatus()
{
    qCDebug(app) << "GpuMonitor updateStatus";
    qreal gpuPercent = m_gpuInfoModel->gpuAllUtilization();
    // 如果数据无效不更新
    if (qIsNaN(gpuPercent)) {
        qCWarning(app) << "gpuAllUtilization is nan";
        return;
    }

    gpuPercents->append(gpuPercent);

    if (gpuPercents->size() > pointsNumber) {
        gpuPercents->pop_front();
    }

    QList<QPointF> points;

    double gpuMaxHeight = 0;
    for (int i = 0; i < gpuPercents->size(); i++) {
        if (gpuPercents->at(i) > gpuMaxHeight) {
            gpuMaxHeight = gpuPercents->at(i);
        }
    }

    for (int i = 0; i < gpuPercents->size(); i++) {
        if (gpuMaxHeight < gpuRenderMaxHeight) {
            points.append(QPointF(i * 5 - 8, gpuPercents->at(i)));
        } else {
            points.append(
                QPointF(i * 5 - 8, gpuPercents->at(i) * gpuRenderMaxHeight / gpuMaxHeight));
        }
    }

    gpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    m_animation->start();
}

void GpuMonitor::onDetailInfoClicked()
{
    qCDebug(app) << "onDetailInfoClicked";
    setDetailButtonVisible(false);
    emit signalDetailInfoClicked();
}

void GpuMonitor::setDetailButtonVisible(bool visible)
{
    qCDebug(app) << "setDetailButtonVisible:" << visible;
    m_detailButton->setVisible(visible);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void GpuMonitor::changeTheme(DApplicationHelper::ColorType themeType)
#else
void GpuMonitor::changeTheme(DGuiApplicationHelper::ColorType themeType)
#endif
{
    qCDebug(app) << "GpuMonitor changeTheme, themeType:" << themeType;
    m_themeType = themeType;

    switch (m_themeType) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    case DApplicationHelper::LightType:
#else
    case DGuiApplicationHelper::LightType:
#endif
        ringBackgroundColor = "#000000";
        m_icon = QIcon(iconPathFromQrc("light/icon_gpu_light.svg"));
        break;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    case DApplicationHelper::DarkType:
#else
    case DGuiApplicationHelper::DarkType:
#endif
        ringBackgroundColor = "#FFFFFF";
        m_icon = QIcon(iconPathFromQrc("dark/icon_gpu_light.svg"));
        break;
    default:
        break;
    }

    // init colors
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif
    auto palette = dAppHelper->applicationPalette();

#ifndef THEME_FALLBACK_COLOR
    numberColor = palette.color(DPalette::TextTitle);
#else
    numberColor = palette.color(DPalette::Text);
#endif

    textColor = palette.color(DPalette::Text);

    update();
}

void GpuMonitor::changeFont(const QFont &font)
{
    qCDebug(app) << "GpuMonitor changeFont";
    m_gpuUsageFont = font;
    m_gpuUsageFont.setBold(true);
    m_gpuUsageFont.setPointSizeF(m_gpuUsageFont.pointSizeF() + 3);

    m_gpuDisplayFont = font;
    m_gpuDisplayFont.setPointSizeF(m_gpuDisplayFont.pointSizeF() + 12);

    resizeItemWidgetRect();
    update();
}

void GpuMonitor::resizeItemWidgetRect()
{
    // qCDebug(app) << "resizeItemWidgetRect";
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_detailButton->setFixedSize(m_detailButton->fontMetrics().width(m_detailButton->text()) + 16, m_detailButton->fontMetrics().height() + 4);
#else
    m_detailButton->setFixedSize(m_detailButton->fontMetrics().horizontalAdvance(m_detailButton->text()) + 16, m_detailButton->fontMetrics().height() + 4);
#endif
    const QSize &detailtextSize =  m_detailButton->size();
    m_detailButton->setGeometry(this->width() / 2 - detailtextSize.width() / 2 - 4, this->height() - detailtextSize.height() + 3, detailtextSize.width(), detailtextSize.height());
}

void GpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int margin = 10;

    QFontMetrics fm(m_gpuDisplayFont);
    int titleWidth =
        fm.size(Qt::TextSingleLine, DApplication::translate("Process.Graph.View", "GPU")).width();

    int iconSize = 24;

    painter.setFont(m_gpuDisplayFont);
    painter.setPen(QPen(textColor));
    QRect gpuDisplayRect(((rect().x() + rect().width() - titleWidth) / 2),
                         rect().y() + titleRenderOffsetY, titleWidth, fm.height());
    painter.drawText(gpuDisplayRect, Qt::AlignCenter,
                     DApplication::translate("Process.Graph.View", "GPU"));

    QRect iconRect(gpuDisplayRect.x() - margin - iconSize + 6,
                   gpuDisplayRect.y() + qCeil((gpuDisplayRect.height() - iconSize) / 2.) + 2,
                   iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    auto gdiff = gpuPercents->last() - gpuPercents->at(gpuPercents->size() - 2);
    double percent = (gpuPercents->at(gpuPercents->size() - 2) + m_progress * gdiff);

    painter.setFont(m_gpuUsageFont);
    painter.setPen(QPen(numberColor));
    painter.drawText(
        QRect(rect().x(), rect().y() + percentRenderOffsetY, rect().width(), 30),
        Qt::AlignCenter, QString("%1%").arg(QString::number(percent, 'f', 1)));

    int centerX = rect().x() + rect().width() / 2;
    int centerY = rect().y() + ringRenderOffsetY;
    drawLoadingRing(painter, centerX, centerY, ringRadius, ringWidth, 270, 135, ringForegroundColor,
                    ringForegroundOpacity, ringBackgroundColor, ringBackgroundOpacity,
                    percent / 100);

    // clip internal area of the ring
    QRect clipRect(centerX - ringRadius + ringWidth, centerY - ringRadius + ringWidth,
                   (ringRadius - ringWidth) * 2, (ringRadius - ringWidth) * 2);
    QPainterPath clip;
    clipRect = clipRect.marginsRemoved({ringWidth, ringWidth, ringWidth, ringWidth});
    clip.addRoundedRect(clipRect, clipRect.width(), clipRect.height());
    painter.setClipPath(clip);

    painter.translate(waveformsRenderOffsetX, waveformsRenderOffsetY);
    painter.scale(1, -1);
    painter.setPen(QPen(ringForegroundColor, 1.5));
    painter.drawPath(gpuPath);

    setFixedHeight(gpuDisplayRect.y() + gpuDisplayRect.height() + 1 + m_detailButton->fontMetrics().height());
}

void GpuMonitor::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        emit clicked("MSG_GPU");
    }
}

void GpuMonitor::resizeEvent(QResizeEvent *event)
{
    // qCDebug(app) << "resizeEvent";
    QWidget::resizeEvent(event);
    resizeItemWidgetRect();
}

void GpuMonitor::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    return;
}
