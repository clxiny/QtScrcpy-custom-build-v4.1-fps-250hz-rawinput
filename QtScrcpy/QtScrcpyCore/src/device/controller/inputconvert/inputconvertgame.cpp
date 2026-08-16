#include <QDebug>
#include <QCursor>
#include <QGuiApplication>
#include <QTimer>
#include <QTime>
#include <QRandomGenerator>

#ifdef Q_OS_MACOS
#include <CoreGraphics/CoreGraphics.h>
#endif

#include "inputconvertgame.h"

#define CURSOR_POS_CHECK 50

namespace {
constexpr int FPS_TOUCH_INTERVAL_MS = 4;
constexpr int FPS_TOUCH_MAX_BOUNDARY_SPLITS = 16;
constexpr qreal FPS_TOUCH_SAFE_MIN = 0.05;
constexpr qreal FPS_TOUCH_SAFE_MAX = 0.95;
constexpr qreal FPS_TOUCH_DELTA_EPSILON = 0.0000001;

bool isTinyDelta(const QPointF &delta)
{
    return qAbs(delta.x()) < FPS_TOUCH_DELTA_EPSILON
            && qAbs(delta.y()) < FPS_TOUCH_DELTA_EPSILON;
}
}

InputConvertGame::InputConvertGame(Controller *controller) : InputConvertNormal(controller) {
    m_ctrlSteerWheel.delayData.timer = new QTimer(this);
    m_ctrlSteerWheel.delayData.timer->setSingleShot(true);
    connect(m_ctrlSteerWheel.delayData.timer, &QTimer::timeout, this, &InputConvertGame::onSteerWheelTimer);

    // A high-polling-rate mouse may report at 500/1000 Hz. Coalesce reports
    // into one newest Android MOVE every 4 ms so the camera receives up to
    // 250 updates/s without building an ever-growing Qt/TCP event backlog.
    m_ctrlMouseMove.flushTimer = new QTimer(this);
    m_ctrlMouseMove.flushTimer->setSingleShot(true);
    m_ctrlMouseMove.flushTimer->setTimerType(Qt::PreciseTimer);
    m_ctrlMouseMove.flushTimer->setInterval(FPS_TOUCH_INTERVAL_MS);
    connect(m_ctrlMouseMove.flushTimer, &QTimer::timeout,
            this, &InputConvertGame::flushPendingMouseMoveDelta);
}

InputConvertGame::~InputConvertGame() {}

void InputConvertGame::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    // 处理开关按键
    if (m_keyMap.isSwitchOnKeyboard() == false && m_keyMap.getSwitchKey() == static_cast<int>(from->button())) {
        if (from->type() != QEvent::MouseButtonPress) {
            return;
        }
        switchGameMap();
        return;
    }

    if (m_gameMap) {
        updateSize(frameSize, showSize);
        // mouse move
        if (m_keyMap.isValidMouseMoveMap()) {
            if (processMouseMove(from)) {
                return;
            }
        }
        // Preserve ordering between a final coalesced camera delta and a fire
        // button press/release that arrives inside the same 4 ms window.
        if (!isTinyDelta(m_ctrlMouseMove.pendingDelta)) {
            m_ctrlMouseMove.flushTimer->stop();
            flushPendingMouseMoveDelta();
        }
        // mouse click
        processMouseClick(from);

        // FPS mode is an exclusive, direct keymap path. Even an unmapped mouse
        // event is consumed here instead of falling through to normal touch or
        // the UHID pointer bridge.
        return;
    }
    InputConvertNormal::mouseEvent(from, frameSize, showSize);
}

void InputConvertGame::mouseMoveRelative(const QPointF &delta, const QSize &frameSize, const QSize &showSize)
{
    if (!m_gameMap || !m_processMouseMove
            || !m_keyMap.isValidMouseMoveMap() || isTinyDelta(delta)) {
        return;
    }

    updateSize(frameSize, showSize);
    queueMouseMoveDelta(delta);
}

void InputConvertGame::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_gameMap) {
        updateSize(frameSize, showSize);
    } else {
        InputConvertNormal::wheelEvent(from, frameSize, showSize);
    }
}

void InputConvertGame::keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize)
{
    // 处理开关按键
    if (m_keyMap.isSwitchOnKeyboard() && m_keyMap.getSwitchKey() == from->key()) {
        if (QEvent::KeyPress != from->type()) {
            return;
        }
        updateSize(frameSize, showSize);
        switchGameMap();
        return;
    }

    KeyMap::KeyMapNode node;
    bool haveLatchedNode = false;
    if (QEvent::KeyRelease == from->type()) {
        const auto pressedNode = m_pressedKeyNodes.constFind(from->key());
        if (pressedNode != m_pressedKeyNodes.constEnd()) {
            node = pressedNode.value();
            haveLatchedNode = true;
        }
    }
    if (!haveLatchedNode) {
        node = m_keyMap.getKeyMapNodeKey(from->key());
    }
    // Keep switchMap keys active while the Android UHID pointer is enabled so
    // the same binding can return directly to the FPS touch map.
    if (!m_gameMap && KeyMap::KMT_CLICK == node.type && node.data.click.switchMap) {
        updateSize(frameSize, showSize);
        processKeyClick(node.data.click.keyNode.pos, false, from);
        processAndroidKey(node.data.click.keyNode.androidKey, from);
        if (QEvent::KeyRelease == from->type()) {
            applyLayerAction(node);
            switchGameMap();
        }
        return;
    }

    if (m_gameMap) {
        updateSize(frameSize, showSize);
        if (!from || from->isAutoRepeat()) {
            return;
        }

        if (QEvent::KeyPress == from->type() && KeyMap::KMT_INVALID != node.type) {
            // Keep the mapping selected on DOWN until its matching UP. A layer
            // can legally remap the same physical key, but it must not move an
            // already-pressed touch to a different coordinate on release.
            m_pressedKeyNodes.insert(from->key(), node);
        }

        // small eyes
        if (m_keyMap.isValidMouseMoveMap() && from->key() == m_keyMap.getMouseMoveMap().data.mouseMove.smallEyes.key) {
            m_ctrlMouseMove.smallEyes = (QEvent::KeyPress == from->type());

            if (QEvent::KeyPress == from->type()) {
                m_ctrlMouseMove.flushTimer->stop();
                m_ctrlMouseMove.pendingDelta = QPointF();
                m_processMouseMove = false;
                scheduleMouseMoveTouchRestart(30);
                stopMouseMoveTimer();
            } else {
                // Cancel the delayed restart scheduled by KeyPress. Otherwise
                // a quick release can restart the camera touch again 30/60 ms
                // later and make the FPS response feel intermittent.
                ++m_mouseMoveRestartEpoch;
                mouseMoveStopTouch();
                mouseMoveStartTouch(nullptr, false);
                m_processMouseMove = true;
            }
            return;
        }

        switch (node.type) {
        // 处理方向盘
        case KeyMap::KMT_STEER_WHEEL:
            processSteerWheel(node, from);
            if (QEvent::KeyRelease == from->type()) {
                applyLayerAction(node);
                m_pressedKeyNodes.remove(from->key());
            }
            return;
        // 处理普通按键
        case KeyMap::KMT_CLICK:
            processKeyClick(node.data.click.keyNode.pos, false, from);
            processAndroidKey(node.data.click.keyNode.androidKey, from);
            if (QEvent::KeyRelease == from->type()) {
                applyLayerAction(node);
                m_pressedKeyNodes.remove(from->key());
                if (node.data.click.switchMap) {
                    switchGameMap();
                }
            }
            return;
        case KeyMap::KMT_CLICK_TWICE:
            processKeyClick(node.data.clickTwice.keyNode.pos, true, from);
            processAndroidKey(node.data.clickTwice.keyNode.androidKey, from);
            if (QEvent::KeyRelease == from->type()) {
                applyLayerAction(node);
                m_pressedKeyNodes.remove(from->key());
            }
            return;
        case KeyMap::KMT_CLICK_MULTI:
            processKeyClickMulti(node.data.clickMulti.keyNode.delayClickNodes,
                                 node.data.clickMulti.keyNode.delayClickNodesCount, from,
                                 m_layerEpoch);
            if (QEvent::KeyRelease == from->type()) {
                applyLayerAction(node);
                m_pressedKeyNodes.remove(from->key());
            }
            return;
        case KeyMap::KMT_DRAG:
            processKeyDrag(node.data.drag.keyNode.pos, node.data.drag.keyNode.extendPos,
                         node.data.drag.startDelay, node.data.drag.dragSpeed, from);
            if (QEvent::KeyRelease == from->type()) {
                applyLayerAction(node);
                m_pressedKeyNodes.remove(from->key());
            }
            return;
        case KeyMap::KMT_ANDROID_KEY:
            processAndroidKey(node.data.androidKey.keyNode.androidKey, from);
            if (QEvent::KeyRelease == from->type()) {
                applyLayerAction(node);
                m_pressedKeyNodes.remove(from->key());
            }
            return;
        default:
            break;
        }
    } else {
        InputConvertNormal::keyEvent(from, frameSize, showSize);
    }
}

bool InputConvertGame::isCurrentCustomKeymap()
{
    return m_gameMap;
}

void InputConvertGame::loadKeyMap(const QString &json)
{
    // Reloading changes the meaning of physical keys. Drop outstanding local
    // state and ask the server to cancel any old injected pointers first.
    resetInputState();
    requestAndroidInputStateReset();
    m_keyMap.loadKeyMap(json);
    m_keyMap.resetLayer();
}

void InputConvertGame::resetInputState()
{
    InputConvertNormal::resetInputState();

    // Invalidate every delayed callback before clearing the IDs it refers to.
    // Otherwise a delayed multi-click or small-eyes restart could recreate a
    // touch after the user has requested recovery.
    ++m_inputResetEpoch;
    ++m_mouseMoveRestartEpoch;
    ++m_layerEpoch;
    m_pressedKeyNodes.clear();
    m_pressedMouseNodes.clear();
    m_multiClickTouchPositions.clear();

    if (m_ctrlMouseMove.flushTimer) {
        m_ctrlMouseMove.flushTimer->stop();
    }
    m_ctrlMouseMove.pendingDelta = QPointF();
    stopMouseMoveTimer();
    m_ctrlMouseMove.touching = false;
    m_ctrlMouseMove.lastPos = QPointF();
    m_ctrlMouseMove.lastConverPos = QPointF();
    m_ctrlMouseMove.smallEyes = false;
    m_processMouseMove = true;

    if (m_ctrlSteerWheel.delayData.timer) {
        m_ctrlSteerWheel.delayData.timer->stop();
    }
    m_ctrlSteerWheel.pressedUp = false;
    m_ctrlSteerWheel.pressedDown = false;
    m_ctrlSteerWheel.pressedLeft = false;
    m_ctrlSteerWheel.pressedRight = false;
    m_ctrlSteerWheel.touchKey = Qt::Key_unknown;
    m_ctrlSteerWheel.delayData.currentPos = QPointF();
    m_ctrlSteerWheel.delayData.queuePos.clear();
    m_ctrlSteerWheel.delayData.queueTimer.clear();
    m_ctrlSteerWheel.delayData.pressedNum = 0;

    if (m_dragDelayData.timer) {
        m_dragDelayData.timer->stop();
        delete m_dragDelayData.timer;
        m_dragDelayData.timer = nullptr;
    }
    m_dragDelayData.currentPos = QPointF();
    m_dragDelayData.queuePos.clear();
    m_dragDelayData.queueTimer.clear();
    m_dragDelayData.pressKey = 0;

    for (int &touchKey : m_multiTouchID) {
        touchKey = 0;
    }
}

void InputConvertGame::updateSize(const QSize &frameSize, const QSize &showSize)
{
    if (showSize != m_showSize) {
        if (m_gameMap && m_keyMap.isValidMouseMoveMap()) {
#ifdef QT_NO_DEBUG
            // show size change, resize grab cursor
            emit grabCursor(true);
#endif
        }
    }
    m_frameSize = frameSize;
    m_showSize = showSize;
}

void InputConvertGame::sendTouchDownEvent(int id, QPointF pos, bool recoverIfFirst)
{
    if (id < 0 || id >= MULTI_TOUCH_MAX_NUM) {
        qWarning() << "Ignoring touch DOWN with invalid id:" << id;
        return;
    }
    if (recoverIfFirst && activeTouchCount() == 1) {
        // This is a new client-side gesture group. Clear any Android stream
        // which a physical finger canceled without telling the desktop. The
        // following DOWN is still the original direct CMT_INJECT_TOUCH path.
        requestAndroidInputStateReset();
    }
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_DOWN);
}

void InputConvertGame::sendTouchMoveEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_MOVE);
}

void InputConvertGame::sendTouchUpEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_UP);
}

void InputConvertGame::sendTouchEvent(int id, QPointF pos, AndroidMotioneventAction action)
{
    if (0 > id || MULTI_TOUCH_MAX_NUM - 1 < id) {
        qWarning() << "Ignoring touch event with invalid id:" << id
                   << "action:" << action;
        return;
    }
    //qDebug() << "id:" << id << " pos:" << pos << " action" << action;
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_TOUCH);
    if (!controlMsg) {
        return;
    }

    QPoint absolutePos = calcFrameAbsolutePos(pos).toPoint();
    static QPoint lastAbsolutePos = absolutePos;
    if (AMOTION_EVENT_ACTION_MOVE == action && lastAbsolutePos == absolutePos) {
        delete controlMsg;
        return;
    }
    lastAbsolutePos = absolutePos;

    controlMsg->setInjectTouchMsgData(
        static_cast<quint64>(id),
        action,
        static_cast<AndroidMotioneventButtons>(0),
        static_cast<AndroidMotioneventButtons>(0),
        QRect(absolutePos, m_frameSize),
        AMOTION_EVENT_ACTION_UP == action ? 0.0f : 1.0f);
    sendControlMsg(controlMsg);
}

void InputConvertGame::sendKeyEvent(AndroidKeyeventAction action, AndroidKeycode keyCode) {
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_KEYCODE);
    if (!controlMsg) {
        return;
    }

    controlMsg->setInjectKeycodeMsgData(action, keyCode, 0, AMETA_NONE);
    sendControlMsg(controlMsg);
}

QPointF InputConvertGame::calcFrameAbsolutePos(QPointF relativePos)
{
    QPointF absolutePos;
    absolutePos.setX(m_frameSize.width() * relativePos.x());
    absolutePos.setY(m_frameSize.height() * relativePos.y());
    return absolutePos;
}

QPointF InputConvertGame::calcScreenAbsolutePos(QPointF relativePos)
{
    QPointF absolutePos;
    absolutePos.setX(m_showSize.width() * relativePos.x());
    absolutePos.setY(m_showSize.height() * relativePos.y());
    return absolutePos;
}

int InputConvertGame::attachTouchID(int key)
{
    if (!key) {
        return -1;
    }
    const int existingId = getTouchID(key);
    if (existingId >= 0) {
        // Duplicate DOWN events must not consume a second Android pointer;
        // one later UP would otherwise leave the duplicate permanently held.
        return existingId;
    }
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (0 == m_multiTouchID[i]) {
            m_multiTouchID[i] = key;
            return i;
        }
    }
    return -1;
}

void InputConvertGame::detachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == m_multiTouchID[i]) {
            m_multiTouchID[i] = 0;
            return;
        }
    }
}

int InputConvertGame::getTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == m_multiTouchID[i]) {
            return i;
        }
    }
    return -1;
}

int InputConvertGame::activeTouchCount() const
{
    int count = 0;
    for (int touchKey : m_multiTouchID) {
        if (touchKey) {
            ++count;
        }
    }
    return count;
}

// -------- steer wheel event --------

void InputConvertGame::getDelayQueue(const QPointF& start, const QPointF& end,
                                     const double& distanceStep, const double& posStepconst,
                                     quint32 lowestTimer, quint32 highestTimer,
                                     QQueue<QPointF>& queuePos, QQueue<quint32>& queueTimer) {
    double x1 = start.x();
    double y1 = start.y();
    double x2 = end.x();
    double y2 = end.y();

    double dx=x2-x1;
    double dy=y2-y1;
    double e=(fabs(dx)>fabs(dy))?fabs(dx):fabs(dy);
    e /= distanceStep;
    dx/=e;
    dy/=e;

    QQueue<QPointF> queue;
    QQueue<quint32> queue2;
    for(int i=1;i<=e;i++) {
        QPointF pos(x1+(QRandomGenerator::global()->bounded(posStepconst*2)-posStepconst), y1+(QRandomGenerator::global()->bounded(posStepconst*2)-posStepconst));
        queue.enqueue(pos);
        queue2.enqueue(QRandomGenerator::global()->bounded(lowestTimer, highestTimer));
        x1+=dx;
        y1+=dy;
    }

    queuePos = queue;
    queueTimer = queue2;
}

void InputConvertGame::onSteerWheelTimer() {
    if(m_ctrlSteerWheel.delayData.queuePos.empty()) {
        return;
    }
    int id = getTouchID(m_ctrlSteerWheel.touchKey);
    m_ctrlSteerWheel.delayData.currentPos = m_ctrlSteerWheel.delayData.queuePos.dequeue();
    sendTouchMoveEvent(id, m_ctrlSteerWheel.delayData.currentPos);

    if(m_ctrlSteerWheel.delayData.queuePos.empty() && m_ctrlSteerWheel.delayData.pressedNum == 0) {
        sendTouchUpEvent(id, m_ctrlSteerWheel.delayData.currentPos);
        detachTouchID(m_ctrlSteerWheel.touchKey);
        return;
    }

    if(!m_ctrlSteerWheel.delayData.queuePos.empty()) {
        m_ctrlSteerWheel.delayData.timer->start(m_ctrlSteerWheel.delayData.queueTimer.dequeue());
    }
}

void InputConvertGame::processSteerWheel(const KeyMap::KeyMapNode &node, const QKeyEvent *from)
{
    int key = from->key();
    bool flag = from->type() == QEvent::KeyPress;
    // identify keys
    if (key == node.data.steerWheel.up.key) {
        m_ctrlSteerWheel.pressedUp = flag;
    } else if (key == node.data.steerWheel.right.key) {
        m_ctrlSteerWheel.pressedRight = flag;
    } else if (key == node.data.steerWheel.down.key) {
        m_ctrlSteerWheel.pressedDown = flag;
    } else { // left
        m_ctrlSteerWheel.pressedLeft = flag;
    }

    // calc offset and pressed number
    QPointF offset(0.0, 0.0);
    int pressedNum = 0;
    if (m_ctrlSteerWheel.pressedUp) {
        ++pressedNum;
        offset.ry() -= node.data.steerWheel.up.extendOffset;
    }
    if (m_ctrlSteerWheel.pressedRight) {
        ++pressedNum;
        offset.rx() += node.data.steerWheel.right.extendOffset;
    }
    if (m_ctrlSteerWheel.pressedDown) {
        ++pressedNum;
        offset.ry() += node.data.steerWheel.down.extendOffset;
    }
    if (m_ctrlSteerWheel.pressedLeft) {
        ++pressedNum;
        offset.rx() -= node.data.steerWheel.left.extendOffset;
    }
    m_ctrlSteerWheel.delayData.pressedNum = pressedNum;

    // last key release and timer no active, active timer to detouch
    if (pressedNum == 0) {
        if (m_ctrlSteerWheel.delayData.timer->isActive()) {
            m_ctrlSteerWheel.delayData.timer->stop();
            m_ctrlSteerWheel.delayData.queueTimer.clear();
            m_ctrlSteerWheel.delayData.queuePos.clear();
        }

        const int touchId = getTouchID(m_ctrlSteerWheel.touchKey);
        if (touchId >= 0) {
            sendTouchUpEvent(touchId, m_ctrlSteerWheel.delayData.currentPos);
            detachTouchID(m_ctrlSteerWheel.touchKey);
        }
        return;
    }

    // process steer wheel key event
    m_ctrlSteerWheel.delayData.timer->stop();
    m_ctrlSteerWheel.delayData.queueTimer.clear();
    m_ctrlSteerWheel.delayData.queuePos.clear();

    // first press, get key and touch down
    if (pressedNum == 1 && flag) {
        m_ctrlSteerWheel.touchKey = from->key();
        int id = attachTouchID(m_ctrlSteerWheel.touchKey);
        sendTouchDownEvent(id, node.data.steerWheel.centerPos);

        getDelayQueue(node.data.steerWheel.centerPos, node.data.steerWheel.centerPos+offset,
                      0.01f, 0.002f, 2, 8,
                      m_ctrlSteerWheel.delayData.queuePos,
                      m_ctrlSteerWheel.delayData.queueTimer);
    } else {
        getDelayQueue(m_ctrlSteerWheel.delayData.currentPos, node.data.steerWheel.centerPos+offset,
                      0.01f, 0.002f, 2, 8,
                      m_ctrlSteerWheel.delayData.queuePos,
                      m_ctrlSteerWheel.delayData.queueTimer);
    }
    m_ctrlSteerWheel.delayData.timer->start();
    return;
}

// -------- key event --------

void InputConvertGame::processKeyClick(const QPointF &clickPos, bool clickTwice, const QKeyEvent *from)
{
    if (QEvent::KeyPress == from->type()) {
        int id = attachTouchID(from->key());
        sendTouchDownEvent(id, clickPos);
        if (clickTwice) {
            sendTouchUpEvent(getTouchID(from->key()), clickPos);
            detachTouchID(from->key());
        }
    } else if (QEvent::KeyRelease == from->type()) {
        if (clickTwice) {
            int id = attachTouchID(from->key());
            sendTouchDownEvent(id, clickPos);
        }
        sendTouchUpEvent(getTouchID(from->key()), clickPos);
        detachTouchID(from->key());
    }
}

void InputConvertGame::processKeyClickMulti(const KeyMap::DelayClickNode *nodes, const int count,
                                            const QKeyEvent *from, quint64 layerEpoch)
{
    if (QEvent::KeyPress != from->type()) {
        return;
    }

    int key = from->key();
    int delay = 0;
    QPointF clickPos;
    const quint64 resetEpoch = m_inputResetEpoch;

    for (int i = 0; i < count; i++) {
        delay += nodes[i].delay;
        clickPos = nodes[i].pos;
        QTimer::singleShot(delay, this, [this, key, clickPos, resetEpoch, layerEpoch]() {
            if (resetEpoch != m_inputResetEpoch || layerEpoch != m_layerEpoch || !m_gameMap) {
                return;
            }
            int id = attachTouchID(key);
            sendTouchDownEvent(id, clickPos);
            if (id >= 0) {
                m_multiClickTouchPositions.insert(key, clickPos);
            }
        });

        // Don't up it too fast
        delay += 20;
        QTimer::singleShot(delay, this, [this, key, clickPos, resetEpoch, layerEpoch]() {
            if (resetEpoch != m_inputResetEpoch || layerEpoch != m_layerEpoch || !m_gameMap) {
                return;
            }
            const auto active = m_multiClickTouchPositions.constFind(key);
            if (active != m_multiClickTouchPositions.constEnd()) {
                int id = getTouchID(key);
                sendTouchUpEvent(id, active.value());
                detachTouchID(key);
                m_multiClickTouchPositions.remove(key);
            }
        });
    }
}

void InputConvertGame::onDragTimer() {
    if(m_dragDelayData.queuePos.empty()) {
        return;
    }
    int id = getTouchID(m_dragDelayData.pressKey);
    m_dragDelayData.currentPos = m_dragDelayData.queuePos.dequeue();
    sendTouchMoveEvent(id, m_dragDelayData.currentPos);

    if(m_dragDelayData.queuePos.empty()) {
        delete m_dragDelayData.timer;
        m_dragDelayData.timer = nullptr;

        sendTouchUpEvent(id, m_dragDelayData.currentPos);
        detachTouchID(m_dragDelayData.pressKey);

        m_dragDelayData.currentPos = QPointF();
        m_dragDelayData.pressKey = 0;
        return;
    }

    if(!m_dragDelayData.queuePos.empty()) {
        m_dragDelayData.timer->start(m_dragDelayData.queueTimer.dequeue());
    }
}

void InputConvertGame::processKeyDrag(const QPointF &startPos, QPointF endPos, quint32 startDelay, float dragSpeed, const QKeyEvent *from)
{
    if (QEvent::KeyPress == from->type()) {
        // stop last
        if (m_dragDelayData.timer && m_dragDelayData.timer->isActive()) {
            m_dragDelayData.timer->stop();
            delete m_dragDelayData.timer;
            m_dragDelayData.timer = nullptr;
            m_dragDelayData.queuePos.clear();
            m_dragDelayData.queueTimer.clear();

            sendTouchUpEvent(getTouchID(m_dragDelayData.pressKey), m_dragDelayData.currentPos);
            detachTouchID(m_dragDelayData.pressKey);

            m_dragDelayData.currentPos = QPointF();
            m_dragDelayData.pressKey = 0;
        }

        // start this
        int id = attachTouchID(from->key());
        sendTouchDownEvent(id, startPos);

        m_dragDelayData.timer = new QTimer(this);
        m_dragDelayData.timer->setSingleShot(true);
        connect(m_dragDelayData.timer, &QTimer::timeout, this, &InputConvertGame::onDragTimer);
        m_dragDelayData.pressKey = from->key();
        m_dragDelayData.currentPos = startPos;
        m_dragDelayData.queuePos.clear();
        m_dragDelayData.queueTimer.clear();

        // Clamp dragSpeed to 0-1 range
        const float speed = qBound(0.0f, static_cast<float>(dragSpeed), 1.0f);
        
        // Calculate delays based on dragSpeed
        // dragSpeed = 1 -> minDelay = 1, maxDelay = 2 (fastest)
        // dragSpeed = 0 -> minDelay = 30, maxDelay = 40 (slowest)
        const quint32 minDelay = static_cast<quint32>(1 + (1.0f - speed) * 29);  // 1 to 30
        const quint32 maxDelay = minDelay + static_cast<quint32>((1.0f - speed) * 9) + 1;  // // min + (0 to 9) + 1

        getDelayQueue(startPos, endPos,
                      0.01f, 0.0005f,
                      minDelay,
                      maxDelay,
                      m_dragDelayData.queuePos,
                      m_dragDelayData.queueTimer);

        m_dragDelayData.timer->start(startDelay);
    }
}

void InputConvertGame::processAndroidKey(AndroidKeycode androidKey, const QKeyEvent *from)
{
    if (AKEYCODE_UNKNOWN == androidKey) {
        return;
    }

    AndroidKeyeventAction action;
    switch (from->type()) {
    case QEvent::KeyPress:
        action = AKEY_EVENT_ACTION_DOWN;
        break;
    case QEvent::KeyRelease:
        action = AKEY_EVENT_ACTION_UP;
        break;
    default:
        return;
    }

    sendKeyEvent(action, androidKey);
}

// -------- mouse event --------

bool InputConvertGame::processMouseClick(const QMouseEvent *from)
{
    if (QEvent::MouseButtonPress == from->type() || QEvent::MouseButtonDblClick == from->type()) {
        const KeyMap::KeyMapNode &node = m_keyMap.getKeyMapNodeMouse(from->button());
        if (KeyMap::KMT_CLICK != node.type) {
            return false;
        }
        m_pressedMouseNodes.insert(from->button(), node);
        int id = attachTouchID(from->button());
        sendTouchDownEvent(id, node.data.click.keyNode.pos);
        return true;
    }
    if (QEvent::MouseButtonRelease == from->type()) {
        const auto pressedNode = m_pressedMouseNodes.constFind(from->button());
        if (pressedNode == m_pressedMouseNodes.constEnd()) {
            return false;
        }
        const KeyMap::KeyMapNode &node = pressedNode.value();
        int id = getTouchID(from->button());
        sendTouchUpEvent(id, node.data.click.keyNode.pos);
        detachTouchID(from->button());
        applyLayerAction(node);
        m_pressedMouseNodes.remove(from->button());
        return true;
    }
    return false;
}

bool InputConvertGame::processMouseMove(const QMouseEvent *from)
{
    if (QEvent::MouseMove != from->type()) {
        return false;
    }

    if (checkCursorPos(from)) {
        m_ctrlMouseMove.lastPos = QPointF(0.0, 0.0);
        return true;
    }

    auto lastPos = m_ctrlMouseMove.lastPos;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    m_ctrlMouseMove.lastPos = from->localPos();
#else
    m_ctrlMouseMove.lastPos = from->position();
#endif

    if (!lastPos.isNull() && m_processMouseMove) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF distance_raw{from->localPos() - lastPos};
#else
        QPointF distance_raw{from->position() - lastPos};
#endif
        queueMouseMoveDelta(distance_raw);
    }

    return true;
}

void InputConvertGame::queueMouseMoveDelta(const QPointF &delta)
{
    if (!m_ctrlMouseMove.flushTimer || isTinyDelta(delta)) {
        return;
    }

    m_ctrlMouseMove.pendingDelta += delta;
    if (!m_ctrlMouseMove.flushTimer->isActive()) {
        // Send the first report immediately for minimum input-to-camera
        // latency, then rate-limit only the following reports in this burst.
        const QPointF immediateDelta = m_ctrlMouseMove.pendingDelta;
        m_ctrlMouseMove.pendingDelta = QPointF();
        applyMouseMoveDelta(immediateDelta);
        m_ctrlMouseMove.flushTimer->start();
    }
}

void InputConvertGame::flushPendingMouseMoveDelta()
{
    if (!m_gameMap || !m_processMouseMove) {
        m_ctrlMouseMove.pendingDelta = QPointF();
        return;
    }

    if (isTinyDelta(m_ctrlMouseMove.pendingDelta)) {
        m_ctrlMouseMove.pendingDelta = QPointF();
        return;
    }

    const QPointF delta = m_ctrlMouseMove.pendingDelta;
    m_ctrlMouseMove.pendingDelta = QPointF();
    applyMouseMoveDelta(delta);
    m_ctrlMouseMove.flushTimer->start();
}

void InputConvertGame::applyMouseMoveDelta(const QPointF &delta)
{
    if (!m_processMouseMove || m_showSize.width() <= 0 || m_showSize.height() <= 0) {
        return;
    }

    const QPointF speedRatio = m_keyMap.getMouseMoveMap().data.mouseMove.speedRatio;
    if (qAbs(speedRatio.x()) < FPS_TOUCH_DELTA_EPSILON
            || qAbs(speedRatio.y()) < FPS_TOUCH_DELTA_EPSILON) {
        return;
    }

    mouseMoveStartTouch(nullptr);
    startMouseMoveTimer();

    const QPointF normalizedDelta(delta.x() / speedRatio.x() / m_showSize.width(),
                                  delta.y() / speedRatio.y() / m_showSize.height());
    applyNormalizedMouseMoveDelta(normalizedDelta);
}

void InputConvertGame::applyNormalizedMouseMoveDelta(QPointF delta)
{
    // Preserve the whole relative delta when the simulated finger reaches a
    // safe screen edge. The old code released the finger and discarded the
    // current report plus the next five reports, which produced a visible
    // pause during fast 180/360-degree turns.
    for (int split = 0; split < FPS_TOUCH_MAX_BOUNDARY_SPLITS && !isTinyDelta(delta); ++split) {
        const QPointF current = m_ctrlMouseMove.lastConverPos;
        const QPointF target = current + delta;
        const bool targetInside = target.x() >= FPS_TOUCH_SAFE_MIN
                && target.x() <= FPS_TOUCH_SAFE_MAX
                && target.y() >= FPS_TOUCH_SAFE_MIN
                && target.y() <= FPS_TOUCH_SAFE_MAX;

        if (targetInside) {
            m_ctrlMouseMove.lastConverPos = target;
            sendTouchMoveEvent(getTouchID(Qt::ExtraButton24), target);
            return;
        }

        qreal fraction = 1.0;
        if (delta.x() > 0.0 && target.x() > FPS_TOUCH_SAFE_MAX) {
            fraction = qMin(fraction, (FPS_TOUCH_SAFE_MAX - current.x()) / delta.x());
        } else if (delta.x() < 0.0 && target.x() < FPS_TOUCH_SAFE_MIN) {
            fraction = qMin(fraction, (FPS_TOUCH_SAFE_MIN - current.x()) / delta.x());
        }
        if (delta.y() > 0.0 && target.y() > FPS_TOUCH_SAFE_MAX) {
            fraction = qMin(fraction, (FPS_TOUCH_SAFE_MAX - current.y()) / delta.y());
        } else if (delta.y() < 0.0 && target.y() < FPS_TOUCH_SAFE_MIN) {
            fraction = qMin(fraction, (FPS_TOUCH_SAFE_MIN - current.y()) / delta.y());
        }
        fraction = qBound<qreal>(0.0, fraction, 1.0);

        QPointF edge = current + delta * fraction;
        edge.setX(qBound<qreal>(FPS_TOUCH_SAFE_MIN, edge.x(), FPS_TOUCH_SAFE_MAX));
        edge.setY(qBound<qreal>(FPS_TOUCH_SAFE_MIN, edge.y(), FPS_TOUCH_SAFE_MAX));
        m_ctrlMouseMove.lastConverPos = edge;
        sendTouchMoveEvent(getTouchID(Qt::ExtraButton24), edge);

        if (m_ctrlMouseMove.smallEyes) {
            // Keep the keymap's original small-eyes release/restart gesture;
            // only normal FPS camera movement uses the immediate handoff.
            m_ctrlMouseMove.pendingDelta = QPointF();
            m_processMouseMove = false;
            scheduleMouseMoveTouchRestart(30);
            return;
        }

        delta *= (1.0 - fraction);
        mouseMoveStopTouch();
        if (isTinyDelta(delta)) {
            return;
        }

        // Start a fresh finger at the configured camera origin, then consume
        // the unprocessed remainder in the same input update.
        mouseMoveStartTouch(nullptr, false);
    }

    if (!isTinyDelta(delta)) {
        QPointF finalPos = m_ctrlMouseMove.lastConverPos + delta;
        finalPos.setX(qBound<qreal>(FPS_TOUCH_SAFE_MIN, finalPos.x(), FPS_TOUCH_SAFE_MAX));
        finalPos.setY(qBound<qreal>(FPS_TOUCH_SAFE_MIN, finalPos.y(), FPS_TOUCH_SAFE_MAX));
        m_ctrlMouseMove.lastConverPos = finalPos;
        sendTouchMoveEvent(getTouchID(Qt::ExtraButton24), finalPos);
    }
}

bool InputConvertGame::checkCursorPos(const QMouseEvent *from)
{
    bool moveCursor = false;
    QPoint pos = from->pos();
    if (pos.x() < CURSOR_POS_CHECK) {
        pos.setX(m_showSize.width() - CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.x() > m_showSize.width() - CURSOR_POS_CHECK) {
        pos.setX(CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.y() < CURSOR_POS_CHECK) {
        pos.setY(m_showSize.height() - CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.y() > m_showSize.height() - CURSOR_POS_CHECK) {
        pos.setY(CURSOR_POS_CHECK);
        moveCursor = true;
    }

    if (moveCursor) {
        moveCursorTo(from, pos);
    }

    return moveCursor;
}

void InputConvertGame::moveCursorTo(const QMouseEvent *from, const QPoint &localPosPixel)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QPoint posOffset = from->pos() - localPosPixel;
    QPoint globalPos = from->globalPos();
#else
    QPoint posOffset = from->position().toPoint() - localPosPixel;
    QPoint globalPos = from->globalPosition().toPoint();
#endif
    globalPos -= posOffset;
    //qDebug()<<"move cursor to "<<globalPos<<" offset "<<posOffset;
#ifdef Q_OS_MACOS
    // On macOS, QCursor::setPos() posts a synthetic mouse-moved event (CGEventPost)
    // and does NOT re-associate the hardware mouse with the on-screen cursor, so the
    // warp does not "stick": the next hardware delta snaps the cursor back. That makes
    // mouseMoveMap re-centering a no-op, and the camera can no longer pan past the edge.
    // CGWarpMouseCursorPosition performs a real warp, and
    // CGAssociateMouseAndMouseCursorPosition(true) immediately restores the
    // mouse<->cursor association so subsequent hardware deltas keep flowing. This
    // mirrors the approach already used in util/mousetap/cocoamousetap.mm.
    CGWarpMouseCursorPosition(CGPointMake(globalPos.x(), globalPos.y()));
    CGAssociateMouseAndMouseCursorPosition(true);
#else
    QCursor::setPos(globalPos);
#endif
}

void InputConvertGame::mouseMoveStartTouch(const QMouseEvent *from, bool recoverInputState)
{
    Q_UNUSED(from)
    if (!m_ctrlMouseMove.touching) {
        QPointF mouseMoveStartPos
            = m_ctrlMouseMove.smallEyes ? m_keyMap.getMouseMoveMap().data.mouseMove.smallEyes.pos : m_keyMap.getMouseMoveMap().data.mouseMove.startPos;
        int id = attachTouchID(Qt::ExtraButton24);
        if (id < 0) {
            return;
        }
        sendTouchDownEvent(id, mouseMoveStartPos, recoverInputState);
        m_ctrlMouseMove.lastConverPos = mouseMoveStartPos;
        m_ctrlMouseMove.touching = true;
    }
}

void InputConvertGame::mouseMoveStopTouch()
{
    if (m_ctrlMouseMove.touching) {
        sendTouchUpEvent(getTouchID(Qt::ExtraButton24), m_ctrlMouseMove.lastConverPos);
        detachTouchID(Qt::ExtraButton24);
        m_ctrlMouseMove.touching = false;
    }
}

void InputConvertGame::startMouseMoveTimer()
{
    stopMouseMoveTimer();
    m_ctrlMouseMove.timer = startTimer(500);
}

void InputConvertGame::stopMouseMoveTimer()
{
    if (0 != m_ctrlMouseMove.timer) {
        killTimer(m_ctrlMouseMove.timer);
        m_ctrlMouseMove.timer = 0;
    }
}

void InputConvertGame::scheduleMouseMoveTouchRestart(int delayMs)
{
    const quint64 epoch = ++m_mouseMoveRestartEpoch;
    QTimer::singleShot(delayMs, this, [this, epoch]() {
        if (m_gameMap && epoch == m_mouseMoveRestartEpoch) {
            mouseMoveStopTouch();
        }
    });
    QTimer::singleShot(delayMs * 2, this, [this, epoch]() {
        if (m_gameMap && epoch == m_mouseMoveRestartEpoch) {
            mouseMoveStartTouch(nullptr, false);
            m_processMouseMove = true;
        }
    });
}

void InputConvertGame::applyLayerAction(const KeyMap::KeyMapNode &node)
{
    if (node.switchLayer.isEmpty() && !node.toggleLayer) {
        return;
    }

    const QString previousLayer = m_keyMap.currentLayer();
    bool switched = false;
    if (!node.switchLayer.isEmpty()) {
        switched = m_keyMap.switchLayer(node.switchLayer);
    } else {
        switched = m_keyMap.toggleLayer();
    }

    if (switched && previousLayer != m_keyMap.currentLayer()) {
        // Layer changes are intentionally narrower than switchGameMap(): FPS
        // raw mouse capture and its 250 Hz camera path stay untouched, while
        // queued map-specific gestures from the old layer are canceled.
        cancelLayerDelayedActions();
    }
}

void InputConvertGame::cancelLayerDelayedActions()
{
    ++m_layerEpoch;

    if (m_ctrlSteerWheel.delayData.timer) {
        m_ctrlSteerWheel.delayData.timer->stop();
    }
    const int steerTouchId = getTouchID(m_ctrlSteerWheel.touchKey);
    if (steerTouchId >= 0) {
        sendTouchUpEvent(steerTouchId, m_ctrlSteerWheel.delayData.currentPos);
        detachTouchID(m_ctrlSteerWheel.touchKey);
    }
    m_ctrlSteerWheel.pressedUp = false;
    m_ctrlSteerWheel.pressedDown = false;
    m_ctrlSteerWheel.pressedLeft = false;
    m_ctrlSteerWheel.pressedRight = false;
    m_ctrlSteerWheel.touchKey = Qt::Key_unknown;
    m_ctrlSteerWheel.delayData.currentPos = QPointF();
    m_ctrlSteerWheel.delayData.queuePos.clear();
    m_ctrlSteerWheel.delayData.queueTimer.clear();
    m_ctrlSteerWheel.delayData.pressedNum = 0;

    for (auto it = m_multiClickTouchPositions.constBegin();
         it != m_multiClickTouchPositions.constEnd(); ++it) {
        const int touchId = getTouchID(it.key());
        if (touchId >= 0) {
            sendTouchUpEvent(touchId, it.value());
            detachTouchID(it.key());
        }
    }
    m_multiClickTouchPositions.clear();

    if (m_dragDelayData.timer) {
        m_dragDelayData.timer->stop();
        const int dragTouchId = getTouchID(m_dragDelayData.pressKey);
        if (dragTouchId >= 0) {
            sendTouchUpEvent(dragTouchId, m_dragDelayData.currentPos);
            detachTouchID(m_dragDelayData.pressKey);
        }
        delete m_dragDelayData.timer;
        m_dragDelayData.timer = nullptr;
    }
    m_dragDelayData.currentPos = QPointF();
    m_dragDelayData.queuePos.clear();
    m_dragDelayData.queueTimer.clear();
    m_dragDelayData.pressKey = 0;
}

void InputConvertGame::resetMouseMoveForModeSwitch()
{
    // A mode transition is also a hard input boundary: keys may still be
    // physically held and Android may already have canceled their gestures.
    // Clear both local bookkeeping and the matching server state instead of
    // trying to finish unknown pointers one by one.
    resetInputState();
    requestAndroidInputStateReset();
}

bool InputConvertGame::switchGameMap()
{
    m_gameMap = !m_gameMap;
    resetMouseMoveForModeSwitch();
    if (m_gameMap) {
        m_keyMap.resetLayer();
    }
    qInfo() << QString("current keymap mode: %1").arg(m_gameMap ? "custom" : "normal");
    if (m_gameMap) {
        qInfo() << "FPS mouse route: direct keymap touch injection (UHID bypassed)";
    }

    if (!m_keyMap.isValidMouseMoveMap()) {
        return m_gameMap;
    }
#ifdef QT_NO_DEBUG
    // grab cursor and set cursor only mouse move map
    emit grabCursor(m_gameMap);
#endif
    hideMouseCursor(m_gameMap);

    return m_gameMap;
}

void InputConvertGame::hideMouseCursor(bool hide)
{
    if (hide) {
#ifdef QT_NO_DEBUG
        QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
#else
        QGuiApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
#endif
    } else {
        QGuiApplication::restoreOverrideCursor();
    }
}

void InputConvertGame::timerEvent(QTimerEvent *event)
{
    if (m_ctrlMouseMove.timer == event->timerId()) {
        stopMouseMoveTimer();
        // Don't auto-reset view when smallEyes mode is active
        if (!m_ctrlMouseMove.smallEyes) {
            mouseMoveStopTouch();
        }
    }
}
