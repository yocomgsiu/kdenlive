/***************************************************************************
 *   Copyright (C) 2010 by Till Theato (root@ttill.de)                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#include "geometrywidget.h"
#include "monitor.h"
#include "renderer.h"
#include "keyframehelper.h"
#include "timecodedisplay.h"
#include "monitorscene.h"
#include "monitoreditwidget.h"
#include "onmonitoritems/onmonitorrectitem.h"
#include "kdenlivesettings.h"
#include "dragvalue.h"

#include <QtCore>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMenu>



GeometryWidget::GeometryWidget(Monitor* monitor, Timecode timecode, int clipPos, bool isEffect, QWidget* parent):
    QWidget(parent),
    m_monitor(monitor),
    m_timePos(new TimecodeDisplay(timecode)),
    m_clipPos(clipPos),
    m_inPoint(0),
    m_outPoint(1),
    m_isEffect(isEffect),
    m_rect(NULL),
    m_geometry(NULL),
    m_showScene(true)
{
    m_ui.setupUi(this);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    MonitorEditWidget *edit = monitor->getEffectEdit();
    edit->showVisibilityButton(true);
    m_scene = edit->getScene();


    /*
        Setup of timeline and keyframe controls
    */

    ((QGridLayout *)(m_ui.widgetTimeWrapper->layout()))->addWidget(m_timePos, 1, 6);

    QVBoxLayout *layout = new QVBoxLayout(m_ui.frameTimeline);
    m_timeline = new KeyframeHelper(m_ui.frameTimeline);
    layout->addWidget(m_timeline);
    layout->setContentsMargins(0, 0, 0, 0);

    m_ui.buttonPrevious->setIcon(KIcon("media-skip-backward"));
    m_ui.buttonPrevious->setToolTip(i18n("Go to previous keyframe"));
    m_ui.buttonNext->setIcon(KIcon("media-skip-forward"));
    m_ui.buttonNext->setToolTip(i18n("Go to next keyframe"));
    m_ui.buttonAddDelete->setIcon(KIcon("document-new"));
    m_ui.buttonAddDelete->setToolTip(i18n("Add keyframe"));

    m_ui.buttonSync->setIcon(KIcon("insert-link"));
    m_ui.buttonSync->setToolTip(i18n("Synchronize with timeline cursor"));
    m_ui.buttonSync->setChecked(KdenliveSettings::transitionfollowcursor());

    connect(m_timeline, SIGNAL(positionChanged(int)), this, SLOT(slotPositionChanged(int)));
    connect(m_timeline, SIGNAL(keyframeMoved(int)),   this, SLOT(slotKeyframeMoved(int)));
    connect(m_timeline, SIGNAL(addKeyframe(int)),     this, SLOT(slotAddKeyframe(int)));
    connect(m_timeline, SIGNAL(removeKeyframe(int)),  this, SLOT(slotDeleteKeyframe(int)));
    connect(m_timePos, SIGNAL(editingFinished()), this, SLOT(slotPositionChanged()));
    connect(m_ui.buttonPrevious,  SIGNAL(clicked()), this, SLOT(slotPreviousKeyframe()));
    connect(m_ui.buttonNext,      SIGNAL(clicked()), this, SLOT(slotNextKeyframe()));
    connect(m_ui.buttonAddDelete, SIGNAL(clicked()), this, SLOT(slotAddDeleteKeyframe()));
    connect(m_ui.buttonSync,      SIGNAL(toggled(bool)), this, SLOT(slotSetSynchronize(bool)));

    m_spinX = new DragValue(i18n("X"), 0, -1, QString(), false, this);
    m_spinX->setRange(-10000, 10000);
    m_ui.horizontalLayout->addWidget(m_spinX);
    
    m_spinY = new DragValue(i18n("Y"), 0, -1, QString(), false, this);
    m_spinY->setRange(-10000, 10000);
    m_ui.horizontalLayout->addWidget(m_spinY);
    
    m_spinWidth = new DragValue(i18n("W"), m_monitor->render->frameRenderWidth(), -1, QString(), false, this);
    m_spinWidth->setRange(1, 10000);
    m_ui.horizontalLayout->addWidget(m_spinWidth);
    
    m_spinHeight = new DragValue(i18n("H"), m_monitor->render->renderHeight(), -1, QString(), false, this);
    m_spinHeight->setRange(1, 10000);
    m_ui.horizontalLayout->addWidget(m_spinHeight);
    m_ui.horizontalLayout->addStretch(10);
    
    m_spinSize = new DragValue(i18n("Size"), 100, -1, i18n("%"), false, this);
    m_spinSize->setRange(1, 10000);
    m_ui.horizontalLayout2->addWidget(m_spinSize);
    
    m_opacity = new DragValue(i18n("Opacity"), 100, -1, i18n("%"), true, this);
    m_ui.horizontalLayout2->addWidget(m_opacity);
    
    /*
        Setup of geometry controls
    */

    connect(m_spinX,            SIGNAL(valueChanged(int)), this, SLOT(slotSetX(int)));
    connect(m_spinY,            SIGNAL(valueChanged(int)), this, SLOT(slotSetY(int)));
    connect(m_spinWidth,        SIGNAL(valueChanged(int)), this, SLOT(slotSetWidth(int)));
    connect(m_spinHeight,       SIGNAL(valueChanged(int)), this, SLOT(slotSetHeight(int)));

    connect(m_spinSize,         SIGNAL(valueChanged(int)), this, SLOT(slotResize(int)));

    connect(m_opacity, SIGNAL(valueChanged(int)), this, SLOT(slotSetOpacity(int)));
    
    QMenu *menu = new QMenu(this);
    QAction *adjustSize = new QAction(i18n("Adjust to original size"), this);
    connect(adjustSize, SIGNAL(triggered()), this, SLOT(slotAdjustToFrameSize()));
    menu->addAction(adjustSize);
    QAction *fitToWidth = new QAction(i18n("Fit to width"), this);
    connect(fitToWidth, SIGNAL(triggered()), this, SLOT(slotFitToWidth()));
    menu->addAction(fitToWidth);
    QAction *fitToHeight = new QAction(i18n("Fit to height"), this);
    connect(fitToHeight, SIGNAL(triggered()), this, SLOT(slotFitToHeight()));
    menu->addAction(fitToHeight);
    menu->addSeparator();

    QAction *alignleft = new QAction(KIcon("kdenlive-align-left"), i18n("Align left"), this);
    connect(alignleft, SIGNAL(triggered()), this, SLOT(slotMoveLeft()));
    menu->addAction(alignleft);
    QAction *alignhcenter = new QAction(KIcon("kdenlive-align-hor"), i18n("Center horizontally"), this);
    connect(alignhcenter, SIGNAL(triggered()), this, SLOT(slotCenterH()));
    menu->addAction(alignhcenter);
    QAction *alignright = new QAction(KIcon("kdenlive-align-right"), i18n("Align right"), this);
    connect(alignright, SIGNAL(triggered()), this, SLOT(slotMoveRight()));
    menu->addAction(alignright);
    QAction *aligntop = new QAction(KIcon("kdenlive-align-top"), i18n("Align top"), this);
    connect(aligntop, SIGNAL(triggered()), this, SLOT(slotMoveTop()));
    menu->addAction(aligntop);    
    QAction *alignvcenter = new QAction(KIcon("kdenlive-align-vert"), i18n("Center vertically"), this);
    connect(alignvcenter, SIGNAL(triggered()), this, SLOT(slotCenterV()));
    menu->addAction(alignvcenter);
    QAction *alignbottom = new QAction(KIcon("kdenlive-align-bottom"), i18n("Align bottom"), this);
    connect(alignbottom, SIGNAL(triggered()), this, SLOT(slotMoveBottom()));
    menu->addAction(alignbottom);
    m_ui.buttonOptions->setMenu(menu);

    /*connect(m_ui.buttonMoveLeft,   SIGNAL(clicked()), this, SLOT(slotMoveLeft()));
    connect(m_ui.buttonCenterH,    SIGNAL(clicked()), this, SLOT(slotCenterH()));
    connect(m_ui.buttonMoveRight,  SIGNAL(clicked()), this, SLOT(slotMoveRight()));
    connect(m_ui.buttonMoveTop,    SIGNAL(clicked()), this, SLOT(slotMoveTop()));
    connect(m_ui.buttonCenterV,    SIGNAL(clicked()), this, SLOT(slotCenterV()));
    connect(m_ui.buttonMoveBottom, SIGNAL(clicked()), this, SLOT(slotMoveBottom()));*/


    /*
        Setup of configuration controls
    */

    connect(edit, SIGNAL(showEdit(bool)), this, SLOT(slotShowScene(bool)));

    connect(m_scene, SIGNAL(addKeyframe()),    this, SLOT(slotAddKeyframe()));
    connect(m_monitor, SIGNAL(renderPosition(int)), this, SLOT(slotCheckMonitorPosition(int)));
    connect(this, SIGNAL(parameterChanged()), this, SLOT(slotUpdateProperties()));
}

GeometryWidget::~GeometryWidget()
{
    m_scene->setEnabled(true);
    delete m_timePos;
    delete m_timeline;
    delete m_spinX;
    delete m_spinY;
    delete m_spinWidth;
    delete m_spinHeight;
    delete m_opacity;
    m_scene->removeItem(m_rect);
    delete m_geometry;
    if (m_monitor) {
        m_monitor->getEffectEdit()->showVisibilityButton(false);
        m_monitor->slotEffectScene(false);
    }
}

void GeometryWidget::updateTimecodeFormat()
{
    m_timePos->slotUpdateTimeCodeFormat();
}

QString GeometryWidget::getValue() const
{
    return m_geometry->serialise();
}

void GeometryWidget::setupParam(const QDomElement elem, int minframe, int maxframe)
{
    m_inPoint = minframe;
    m_outPoint = maxframe;

    if (m_geometry)
        m_geometry->parse(elem.attribute("value").toUtf8().data(), maxframe - minframe, m_monitor->render->frameRenderWidth(), m_monitor->render->renderHeight());
    else
        m_geometry = new Mlt::Geometry(elem.attribute("value").toUtf8().data(), maxframe - minframe, m_monitor->render->frameRenderWidth(), m_monitor->render->renderHeight());

    if (elem.attribute("fixed") == "1" || maxframe < minframe) {
        // Keyframes are disabled
        m_ui.widgetTimeWrapper->setHidden(true);
    } else {
        m_ui.widgetTimeWrapper->setHidden(false);
        m_timeline->setKeyGeometry(m_geometry, m_outPoint - m_inPoint - 1);
        m_timeline->update();
        m_timePos->setRange(0, m_outPoint - m_inPoint - 1);
    }

    // no opacity
    if (elem.attribute("opacity") == "false") {
        m_opacity->setHidden(true);
        m_ui.horizontalLayout2->addStretch(2);
    }

    Mlt::GeometryItem item;

    m_geometry->fetch(&item, 0);
    delete m_rect;
    m_rect = new OnMonitorRectItem(QRectF(0, 0, item.w(), item.h()), m_monitor->render->dar());
    m_rect->setPos(item.x(), item.y());
    m_rect->setZValue(0);
    m_scene->addItem(m_rect);
    connect(m_rect, SIGNAL(changed()), this, SLOT(slotUpdateGeometry()));

    slotPositionChanged(0, false);
    slotCheckMonitorPosition(m_monitor->render->seekFramePosition());
}

void GeometryWidget::slotSyncPosition(int relTimelinePos)
{
    // do only sync if this effect is keyframable
    if (m_timePos->maximum() > 0 && KdenliveSettings::transitionfollowcursor()) {
        relTimelinePos = qBound(0, relTimelinePos, m_timePos->maximum());
        if (relTimelinePos != m_timePos->getValue())
            slotPositionChanged(relTimelinePos, false);
    }
}


void GeometryWidget::slotPositionChanged(int pos, bool seek)
{
    if (pos == -1)
        pos = m_timePos->getValue();
    else
        m_timePos->setValue(pos);

    m_timeline->blockSignals(true);
    m_timeline->setValue(pos);
    m_timeline->blockSignals(false);

    Mlt::GeometryItem item;
    if (m_geometry->fetch(&item, pos) || item.key() == false) {
        // no keyframe
        m_rect->setEnabled(false);
        m_scene->setEnabled(false);
        m_ui.widgetGeometry->setEnabled(false);
        m_ui.buttonAddDelete->setIcon(KIcon("document-new"));
        m_ui.buttonAddDelete->setToolTip(i18n("Add keyframe"));
    } else {
        // keyframe
        m_rect->setEnabled(true);
        m_scene->setEnabled(true);
        m_ui.widgetGeometry->setEnabled(true);
        m_ui.buttonAddDelete->setIcon(KIcon("edit-delete"));
        m_ui.buttonAddDelete->setToolTip(i18n("Delete keyframe"));
    }

    m_rect->setPos(item.x(), item.y());
    m_rect->setRect(0, 0, item.w(), item.h());

    m_opacity->blockSignals(true);
    m_opacity->setValue(item.mix());
    m_opacity->blockSignals(false);

    slotUpdateProperties();

    if (seek && KdenliveSettings::transitionfollowcursor())
        emit seekToPos(m_clipPos + pos);
}

void GeometryWidget::slotKeyframeMoved(int pos)
{
    slotPositionChanged(pos);
    slotUpdateGeometry();
    QTimer::singleShot(100, this, SIGNAL(parameterChanged()));
}

void GeometryWidget::slotAddKeyframe(int pos)
{
    Mlt::GeometryItem item;
    if (pos == -1)
        pos = m_timePos->getValue();
    item.frame(pos);
    QRectF r = m_rect->rect().normalized();
    QPointF rectpos = m_rect->pos();
    item.x(rectpos.x());
    item.y(rectpos.y());
    item.w(r.width());
    item.h(r.height());
    item.mix(m_opacity->value());
    m_geometry->insert(item);

    m_timeline->update();
    slotPositionChanged(pos, false);
    emit parameterChanged();
}

void GeometryWidget::slotDeleteKeyframe(int pos)
{
    Mlt::GeometryItem item;
    if (pos == -1)
        pos = m_timePos->getValue();
    // check there is more than one keyframe, do not allow to delete last one
    if (m_geometry->next_key(&item, pos + 1)) {
        if (m_geometry->prev_key(&item, pos - 1) || item.frame() == pos)
            return;
    }
    m_geometry->remove(pos);

    m_timeline->update();
    slotPositionChanged(pos, false);
    emit parameterChanged();
}

void GeometryWidget::slotPreviousKeyframe()
{
    Mlt::GeometryItem item;
    // Go to start if no keyframe is found
    int currentPos = m_timePos->getValue();
    int pos = 0;
    if (!m_geometry->prev_key(&item, currentPos - 1) && item.frame() < currentPos)
        pos = item.frame();

    slotPositionChanged(pos);
}

void GeometryWidget::slotNextKeyframe()
{
    Mlt::GeometryItem item;
    // Go to end if no keyframe is found
    int pos = m_timeline->frameLength;
    if (!m_geometry->next_key(&item, m_timeline->value() + 1))
        pos = item.frame();

    slotPositionChanged(pos);
}

void GeometryWidget::slotAddDeleteKeyframe()
{
    Mlt::GeometryItem item;
    if (m_geometry->fetch(&item, m_timePos->getValue()) || item.key() == false)
        slotAddKeyframe();
    else
        slotDeleteKeyframe();
}


void GeometryWidget::slotCheckMonitorPosition(int renderPos)
{
    if (m_showScene) {
        /*
            We do only get the position in timeline if this geometry belongs to a transition,
            therefore we need two ways here.
        */
        if (m_isEffect) {
            emit checkMonitorPosition(renderPos);
        } else {
            if (renderPos >= m_clipPos && renderPos <= m_clipPos + m_outPoint - m_inPoint) {
                if (!m_scene->views().at(0)->isVisible())
                    m_monitor->slotEffectScene(true);
            } else {
                m_monitor->slotEffectScene(false);
            }
        }
    }
}


void GeometryWidget::slotUpdateGeometry()
{
    Mlt::GeometryItem item;
    int pos = m_timePos->getValue();

    // get keyframe and make sure it is the correct one
    if (m_geometry->next_key(&item, pos) || item.frame() != pos)
        return;

    QRectF rectSize = m_rect->rect().normalized();
    QPointF rectPos = m_rect->pos();
    item.x(rectPos.x());
    item.y(rectPos.y());
    item.w(rectSize.width());
    item.h(rectSize.height());
    m_geometry->insert(item);
    emit parameterChanged();
}

void GeometryWidget::slotUpdateProperties()
{
    QRectF rectSize = m_rect->rect().normalized();
    QPointF rectPos = m_rect->pos();
    int size;
    if (rectSize.width() / m_monitor->render->dar() < rectSize.height())
        size = (int)((rectSize.width() * 100.0 / m_monitor->render->frameRenderWidth()) + 0.5);
    else
        size = (int)((rectSize.height() * 100.0 / m_monitor->render->renderHeight()) + 0.5);

    m_spinX->blockSignals(true);
    m_spinY->blockSignals(true);
    m_spinWidth->blockSignals(true);
    m_spinHeight->blockSignals(true);
    m_spinSize->blockSignals(true);

    m_spinX->setValue(rectPos.x());
    m_spinY->setValue(rectPos.y());
    m_spinWidth->setValue(rectSize.width());
    m_spinHeight->setValue(rectSize.height());
    m_spinSize->setValue(size);

    m_spinX->blockSignals(false);
    m_spinY->blockSignals(false);
    m_spinWidth->blockSignals(false);
    m_spinHeight->blockSignals(false);
    m_spinSize->blockSignals(false);
}


void GeometryWidget::slotSetX(int value)
{
    m_rect->setPos(value, m_spinY->value());
    slotUpdateGeometry();
}

void GeometryWidget::slotSetY(int value)
{
    m_rect->setPos(m_spinX->value(), value);
    slotUpdateGeometry();
}

void GeometryWidget::slotSetWidth(int value)
{
    m_rect->setRect(0, 0, value, m_spinHeight->value());
    slotUpdateGeometry();
}

void GeometryWidget::slotSetHeight(int value)
{
    m_rect->setRect(0, 0, m_spinWidth->value(), value);
    slotUpdateGeometry();
}


void GeometryWidget::slotResize(int value)
{
    m_rect->setRect(0, 0,
                    (int)((m_monitor->render->frameRenderWidth() * value / 100.0) + 0.5),
                    (int)((m_monitor->render->renderHeight() * value / 100.0) + 0.5));
    slotUpdateGeometry();
}


void GeometryWidget::slotSetOpacity(int value)
{
    int pos = m_timePos->getValue();
    Mlt::GeometryItem item;
    if (m_geometry->fetch(&item, pos) || item.key() == false)
        return;
    item.mix(value);
    m_geometry->insert(item);
    emit parameterChanged();
}


void GeometryWidget::slotMoveLeft()
{
    m_rect->setPos(0, m_rect->pos().y());
    slotUpdateGeometry();
}

void GeometryWidget::slotCenterH()
{
    m_rect->setPos((m_monitor->render->frameRenderWidth() - m_rect->rect().width()) / 2, m_rect->pos().y());
    slotUpdateGeometry();
}

void GeometryWidget::slotMoveRight()
{
    m_rect->setPos(m_monitor->render->frameRenderWidth() - m_rect->rect().width(), m_rect->pos().y());
    slotUpdateGeometry();
}

void GeometryWidget::slotMoveTop()
{
    m_rect->setPos(m_rect->pos().x(), 0);
    slotUpdateGeometry();
}

void GeometryWidget::slotCenterV()
{
    m_rect->setPos(m_rect->pos().x(), (m_monitor->render->renderHeight() - m_rect->rect().height()) / 2);
    slotUpdateGeometry();
}

void GeometryWidget::slotMoveBottom()
{
    m_rect->setPos(m_rect->pos().x(), m_monitor->render->renderHeight() - m_rect->rect().height());
    slotUpdateGeometry();
}


void GeometryWidget::slotSetSynchronize(bool sync)
{
    KdenliveSettings::setTransitionfollowcursor(sync);
    if (sync)
        emit seekToPos(m_clipPos + m_timePos->getValue());
}

void GeometryWidget::slotShowScene(bool show)
{
    m_showScene = show;
    if (!m_showScene)
        m_monitor->slotEffectScene(false);
    else
        slotCheckMonitorPosition(m_monitor->render->seekFramePosition());
}

void GeometryWidget::setFrameSize(QPoint size)
{
    m_frameSize = size;
}

void GeometryWidget::slotAdjustToFrameSize()
{
    if (m_frameSize == QPoint()) m_frameSize = QPoint(m_monitor->render->frameRenderWidth(), m_monitor->render->renderHeight());
    m_spinWidth->setValue(m_frameSize.x());
    m_spinHeight->setValue(m_frameSize.y());
}

void GeometryWidget::slotFitToWidth()
{
    if (m_frameSize == QPoint()) m_frameSize = QPoint(m_monitor->render->frameRenderWidth(), m_monitor->render->renderHeight());
    double factor = 100.0 * m_monitor->render->frameRenderWidth() / m_frameSize.x() + 0.5;
    m_spinSize->setValue(factor);
}

void GeometryWidget::slotFitToHeight()
{
    if (m_frameSize == QPoint()) m_frameSize = QPoint(m_monitor->render->frameRenderWidth(), m_monitor->render->renderHeight());
    double factor = 100.0 * m_monitor->render->renderHeight() / m_frameSize.y() + 0.5;
    m_spinSize->setValue(factor);
}

#include "geometrywidget.moc"
