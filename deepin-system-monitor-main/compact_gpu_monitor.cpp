// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "compact_gpu_monitor.h"

#include "smooth_curve_generator.h"
#include "common/common.h"
#include "model/gpu_info_model.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif
#include <DPalette>
#include <DStyleHelper>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QMouseEvent>

DWIDGET_USE_NAMESPACE
using namespace common;
using namespace DDLog;

CompactGpuMonitor::CompactGpuMonitor(int deviceIndex, bool isNpu, QWidget *parent)
    : QWidget(parent)
    , m_deviceIndex(deviceIndex)
    , m_isNpu(isNpu)
{
    qCDebug(app) << "CompactGpuMonitor constructor, device:" << deviceIndex << "isNpu:" << isNpu;
    int statusBarMaxWidth = common::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);
    setFixedHeight(160);

    pointsNumber = int(statusBarMaxWidth / 10);

    gpuPercents = new QList<qreal>();
    for (int i = 0; i < pointsNumber; i++) {
        gpuPercents->append(0);
    }

    // NPU 使用与 NPU 详情图表一致的蓝色，GPU 保持紫色
    m_gpuColor = isNpu ? QColor("#00A3FF") : QColor("#AA00FF");

    m_gpuInfoModel = GPUInfoModel::instance();
    connect(m_gpuInfoModel, &GPUInfoModel::modelUpdated, this, &CompactGpuMonitor::updateStatus);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CompactGpuMonitor::changeFont);
}

CompactGpuMonitor::~CompactGpuMonitor()
{
    delete gpuPercents;
}

QString CompactGpuMonitor::msgCode() const
{
    return GPUInfoModel::deviceMsgCode(m_deviceIndex, m_isNpu);
}

void CompactGpuMonitor::updateStatus()
{
    qCDebug(app) << "CompactGpuMonitor updateStatus";
    auto device = m_gpuInfoModel->device(m_deviceIndex, m_isNpu);
    m_gpuPercent = device ? qreal(device->gpuUtilization) : 0.0;

    gpuPercents->append(m_gpuPercent);

    if (gpuPercents->size() > pointsNumber) {
        gpuPercents->pop_front();
    }

    // 曲线在 paintEvent 中按实际图表宽度生成，避免固定点距导致只画出半幅
    update();
}

void CompactGpuMonitor::changeFont(const QFont &font)
{
    m_tagFont = font;
    m_tagFont.setWeight(QFont::Medium);
    m_tagFont.setPointSizeF(m_tagFont.pointSizeF() - 1);
    m_statFont = font;
    m_statFont.setPointSizeF(m_statFont.pointSizeF() - 1);
}

void CompactGpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto *dAppHelper = DApplicationHelper::instance();
#else
    auto *dAppHelper = DGuiApplicationHelper::instance();
#endif

    auto palette = dAppHelper->applicationPalette();
    QColor frameColor = palette.color(DPalette::TextTips);
    frameColor.setAlphaF(0.3);

#ifndef THEME_FALLBACK_COLOR
    QColor tagColor = palette.color(DPalette::TextTitle);
#else
    QColor tagColor = palette.color(DPalette::Text);
#endif

    QColor statColor = palette.color(DPalette::TextTips);
    QColor sectionColor = m_gpuColor;

    int spacing = 4;

    // Draw tag line
    QFontMetrics fm(m_tagFont);
    QFontMetrics fmStat(m_statFont);

    const QString gpuText = GPUInfoModel::deviceDisplayName(m_deviceIndex, m_isNpu);
    const QString gpuStatText = QString::number(m_gpuPercent, 'f', 1).append('%');

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const int gpuTextWidth = fm.width(gpuText);
    const int statTextWidth = fmStat.width(gpuStatText);
#else
    const int gpuTextWidth = fm.horizontalAdvance(gpuText);
    const int statTextWidth = fmStat.horizontalAdvance(gpuStatText);
#endif

    const int tagX = m_bulletSize * 2 + spacing - 2;
    // 多个图表共占一行时宽度会变小，标签与百分比按可用空间裁剪
    const int availableWidth = qMax(0, rect().width() - tagX - 2);
    const int maxTagWidth = qMin(gpuTextWidth, qMax(0, (availableWidth - spacing) / 2));
    const int maxStatWidth = qMax(0, availableWidth - maxTagWidth - spacing);

    QRect tagRect(tagX, 0, maxTagWidth, fm.height() + 4);
    QRect sectionRect(0, tagRect.y() + qCeil((tagRect.height() - m_bulletSize) / 2.),
                      m_bulletSize, m_bulletSize);
    QRect statRect(tagRect.x() + tagRect.width() + spacing, tagRect.y(),
                   qMin(statTextWidth, maxStatWidth), fmStat.height() + 4);

    // draw section bullet
    painter.setPen(sectionColor);
    QPainterPath path;
    path.addRoundedRect(sectionRect, m_bulletSize, m_bulletSize);
    painter.fillPath(path, sectionColor);

    // draw GPU label
    painter.setPen(tagColor);
    painter.setFont(m_tagFont);
    painter.drawText(tagRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fm.elidedText(gpuText, Qt::ElideRight, tagRect.width()));

    // draw GPU stat
    painter.setPen(statColor);
    painter.setFont(m_statFont);
    painter.drawText(statRect, Qt::AlignLeft | Qt::AlignVCenter,
                     fmStat.elidedText(gpuStatText, Qt::ElideRight, statRect.width()));

    // draw grid frame
    QPen framePen;
    int penSize = 1;
    framePen.setColor(frameColor);
    framePen.setWidth(penSize);
    painter.setPen(framePen);

    int gridSize = 10;
    int gridX = rect().x() + penSize;
    int gridY = tagRect.y() + tagRect.height() + 10;
    int gridWidth =
        rect().width() - 3 - ((rect().width() - 3 - penSize) % (gridSize + penSize)) - penSize;
    int gridHeight = renderMaxHeight + 8 * penSize;

    painter.setRenderHint(QPainter::Antialiasing, false);
    QPainterPath framePath;
    QRect gridFrame(gridX, gridY, gridWidth, gridHeight);
    framePath.addRect(gridFrame);
    painter.drawPath(framePath);

    // Draw grid lines
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 2;
    dashes << 2 << space;
    gridPen.setDashPattern(dashes);
    gridPen.setColor(frameColor);
    gridPen.setWidth(0);
    painter.setPen(gridPen);

    int gridLineX = gridX;
    while (gridLineX + gridSize + penSize < gridX + gridWidth) {
        gridLineX += gridSize + penSize;
        painter.drawLine(gridLineX, gridY + 1, gridLineX, gridY + gridHeight - 1);
    }
    int gridLineY = gridY;
    while (gridLineY + gridSize + penSize < gridY + gridHeight) {
        gridLineY += gridSize + penSize;
        painter.drawLine(gridX + 1, gridLineY, gridX + gridWidth - 1, gridLineY);
    }

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Clip to grid and draw curve
    QPainterPath clip;
    clip.addRect(gridFrame);
    painter.setClipPath(clip);

    // 按当前网格宽度重算曲线，固定 5px 点距只会画出半幅图表
    QList<QPointF> points;
    const int dataCount = gpuPercents->size();
    const qreal drawWidth = qMax(qreal(1), qreal(gridFrame.width() - 2 * penSize));
    const qreal deltaX = dataCount > 1 ? drawWidth / (dataCount - 1) : 0;
    for (int i = 0; i < dataCount; ++i) {
        qreal val = gpuPercents->at(i);
        if (val > renderMaxHeight)
            val = renderMaxHeight;
        points.append(QPointF(i * deltaX, val));
    }
    gpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    painter.translate(gridFrame.x() + penSize, gridFrame.y() + gridFrame.height() - penSize);
    painter.scale(1, -1);

    qreal strokeWidth = 1.2;
    painter.setPen(QPen(m_gpuColor, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(gpuPath);

    setFixedHeight(gridFrame.y() + gridFrame.height() + penSize);
}

void CompactGpuMonitor::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        emit clicked(msgCode());
}

void CompactGpuMonitor::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    return;
}
