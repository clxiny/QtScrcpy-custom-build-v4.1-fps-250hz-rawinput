#ifndef INPUTCONVERT_H
#define INPUTCONVERT_H

#include <QTimer>

#include "inputconvertbase.h"

class InputConvertNormal : public InputConvertBase
{
    Q_OBJECT
public:
    InputConvertNormal(Controller *controller);
    virtual ~InputConvertNormal();

    virtual void mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize);
    virtual void wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize);
    virtual void keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize);
    void setMousePassthrough(bool enabled, const QSize &pointerRange = QSize()) override;

private:
    void sendUhidPointerInput(quint8 buttonBits, const QPoint &pos, const QSize &frameSize,
                              int vScroll, int hScroll, bool inRange = true);
    void sendUhidTouch(AndroidMotioneventAction action, const QPoint &pos, const QSize &frameSize);
    void sendPendingUhidTouchDown();
    void sendPendingUhidTouchUp();
    QPoint mapToVideoFrame(const QPoint &pos, const QSize &frameSize, const QSize &showSize) const;
    quint8 convertUhidMouseButton(Qt::MouseButton button) const;
    AndroidMotioneventButtons convertMouseButtons(Qt::MouseButtons buttonState);
    AndroidMotioneventButtons convertMouseButton(Qt::MouseButton button);
    AndroidKeycode convertKeyCode(int key, Qt::KeyboardModifiers modifiers);
    AndroidMetastate convertMetastate(Qt::KeyboardModifiers modifiers);

    bool m_mousePassthrough = false;
    QSize m_uhidPointerRange;
    quint8 m_uhidButtonState = 0;
    bool m_uhidTouching = false;
    bool m_uhidTouchPressPending = false;
    bool m_uhidTouchReleasePending = false;
    QTimer m_uhidTouchStartTimer;
    QTimer m_uhidTouchReleaseTimer;
    QPoint m_uhidTouchDownPos;
    QSize m_uhidTouchDownFrameSize;
    QPoint m_uhidLastTouchPos;
    QSize m_uhidLastFrameSize;
};

#endif // INPUTCONVERT_H
