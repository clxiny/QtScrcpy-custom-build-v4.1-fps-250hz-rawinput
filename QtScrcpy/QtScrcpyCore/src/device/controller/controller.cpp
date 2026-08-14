#include <QApplication>
#include <QClipboard>
#include <QTimer>

#include "controller.h"
#include "controlmsg.h"
#include "inputconvertgame.h"
#include "receiver.h"
#include "videosocket.h"

namespace {

QSize clampAbsolutePointerRange(const QSize &frameSize)
{
    // The HID report carries unsigned 16-bit absolute coordinates. Real
    // Android displays are far below this limit, but keep the descriptor valid
    // if a malformed or synthetic video size is supplied.
    return QSize(qBound(2, frameSize.width(), 0x10000),
                 qBound(2, frameSize.height(), 0x10000));
}

QByteArray buildAbsolutePointerReportDescriptor(const QSize &frameSize)
{
    const QSize pointerRange = clampAbsolutePointerRange(frameSize);
    const quint16 xMax = static_cast<quint16>(pointerRange.width() - 1);
    const quint16 yMax = static_cast<quint16>(pointerRange.height() - 1);

    static const unsigned char beforeXMaximum[] = {
        0x05, 0x0d,       // Usage Page (Digitizers)
        0x09, 0x01,       // Usage (Digitizer)
        0xa1, 0x01,       // Collection (Application)
        0x09, 0x20,       //   Usage (Stylus)
        0xa1, 0x00,       //   Collection (Physical)
        0x09, 0x42,       //     Usage (Tip Switch)
        0x09, 0x32,       //     Usage (In Range)
        0x15, 0x00,       //     Logical Minimum (0)
        0x25, 0x01,       //     Logical Maximum (1)
        0x75, 0x01,       //     Report Size (1)
        0x95, 0x02,       //     Report Count (2)
        0x81, 0x02,       //     Input (Data, Variable, Absolute)
        0x75, 0x06,       //     Report Size (6)
        0x95, 0x01,       //     Report Count (1)
        0x81, 0x03,       //     Input (Constant, Variable, Absolute)
        0x05, 0x01,       //     Usage Page (Generic Desktop)
        0x09, 0x30,       //     Usage (X)
        0x15, 0x00,       //     Logical Minimum (0)
        0x26,             //     Logical Maximum (16-bit, value follows)
    };
    static const unsigned char betweenMaximums[] = {
        0x75, 0x10,       //     Report Size (16)
        0x95, 0x01,       //     Report Count (1)
        0x81, 0x02,       //     Input (Data, Variable, Absolute)
        0x09, 0x31,       //     Usage (Y)
        0x15, 0x00,       //     Logical Minimum (0)
        0x26,             //     Logical Maximum (16-bit, value follows)
    };
    static const unsigned char afterYMaximum[] = {
        0x75, 0x10,       //     Report Size (16)
        0x95, 0x01,       //     Report Count (1)
        0x81, 0x02,       //     Input (Data, Variable, Absolute)
        0xc0,             //   End Collection
        0x05, 0x01,       //   Usage Page (Generic Desktop)
        0x09, 0x01,       //   Usage (Pointer)
        0xa1, 0x00,       //   Collection (Physical)
        0x05, 0x09,       //     Usage Page (Buttons)
        0x19, 0x01,       //     Usage Minimum (1)
        0x29, 0x05,       //     Usage Maximum (5)
        0x15, 0x00,       //     Logical Minimum (0)
        0x25, 0x01,       //     Logical Maximum (1)
        0x75, 0x01,       //     Report Size (1)
        0x95, 0x05,       //     Report Count (5)
        0x81, 0x02,       //     Input (Data, Variable, Absolute)
        0x75, 0x03,       //     Report Size (3)
        0x95, 0x01,       //     Report Count (1)
        0x81, 0x03,       //     Input (Constant, Variable, Absolute)
        0x05, 0x01,       //     Usage Page (Generic Desktop)
        0x09, 0x38,       //     Usage (Wheel)
        0x15, 0x81,       //     Logical Minimum (-127)
        0x25, 0x7f,       //     Logical Maximum (127)
        0x75, 0x08,       //     Report Size (8)
        0x95, 0x01,       //     Report Count (1)
        0x81, 0x06,       //     Input (Data, Variable, Relative)
        0x05, 0x0c,       //     Usage Page (Consumer)
        0x0a, 0x38, 0x02, //     Usage (AC Pan)
        0x15, 0x81,       //     Logical Minimum (-127)
        0x25, 0x7f,       //     Logical Maximum (127)
        0x75, 0x08,       //     Report Size (8)
        0x95, 0x01,       //     Report Count (1)
        0x81, 0x06,       //     Input (Data, Variable, Relative)
        0xc0,             //   End Collection
        0xc0,             // End Collection
    };

    QByteArray descriptor;
    descriptor.reserve(static_cast<int>(sizeof(beforeXMaximum)
                                         + sizeof(betweenMaximums)
                                         + sizeof(afterYMaximum) + 4));
    descriptor.append(reinterpret_cast<const char *>(beforeXMaximum),
                      static_cast<int>(sizeof(beforeXMaximum)));
    descriptor.append(static_cast<char>(xMax & 0xff));
    descriptor.append(static_cast<char>((xMax >> 8) & 0xff));
    descriptor.append(reinterpret_cast<const char *>(betweenMaximums),
                      static_cast<int>(sizeof(betweenMaximums)));
    descriptor.append(static_cast<char>(yMax & 0xff));
    descriptor.append(static_cast<char>((yMax >> 8) & 0xff));
    descriptor.append(reinterpret_cast<const char *>(afterYMaximum),
                      static_cast<int>(sizeof(afterYMaximum)));
    return descriptor;
}

} // namespace

Controller::Controller(std::function<qint64(const QByteArray&)> sendData, QString gameScript, QObject *parent)
    : QObject(parent)
    , m_sendData(sendData)
{
    m_receiver = new Receiver(this);
    Q_ASSERT(m_receiver);

    updateScript(gameScript);
}

Controller::~Controller() {}

void Controller::postControlMsg(ControlMsg *controlMsg)
{
    if (!controlMsg) {
        return;
    }

    if (m_cameraMode) {
        const auto type = controlMsg->type();
        const bool isCameraControl = type == ControlMsg::CMT_CAMERA_SET_TORCH
                || type == ControlMsg::CMT_CAMERA_ZOOM_IN
                || type == ControlMsg::CMT_CAMERA_ZOOM_OUT
                || type == ControlMsg::CMT_SET_VIDEO_PAUSED;
        if (!isCameraControl) {
            qWarning() << "Ignoring display control message in camera mode:" << type;
            delete controlMsg;
            return;
        }
    }

    QCoreApplication::postEvent(this, controlMsg);
}

void Controller::setCameraMode(bool cameraMode)
{
    m_cameraMode = cameraMode;
}

void Controller::recvDeviceMsg(DeviceMsg *deviceMsg)
{
    if (!m_receiver) {
        return;
    }

    m_receiver->recvDeviceMsg(deviceMsg);
}

void Controller::test(QRect rc)
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_TOUCH);
    controlMsg->setInjectTouchMsgData(
        static_cast<quint64>(POINTER_ID_MOUSE), AMOTION_EVENT_ACTION_DOWN, AMOTION_EVENT_BUTTON_PRIMARY, AMOTION_EVENT_BUTTON_PRIMARY, rc, 1.0f);
    postControlMsg(controlMsg);
}

void Controller::updateScript(QString gameScript)
{
    if (m_inputConvert) {
        delete m_inputConvert;
    }
    if (!gameScript.isEmpty()) {
        InputConvertGame *convertgame = new InputConvertGame(this);
        convertgame->loadKeyMap(gameScript);
        m_inputConvert = convertgame;
    } else {
        m_inputConvert = new InputConvertNormal(this);
    }
    Q_ASSERT(m_inputConvert);
    m_inputConvert->setMousePassthrough(
        m_mousePassthrough,
        m_mousePassthroughDeviceCreated
            ? clampAbsolutePointerRange(m_mousePassthroughFrameSize)
            : QSize());
    connect(m_inputConvert, &InputConvertBase::grabCursor, this, &Controller::grabCursor);
}

bool Controller::isCurrentCustomKeymap()
{
    if (!m_inputConvert) {
        return false;
    }

    return m_inputConvert->isCurrentCustomKeymap();
}

void Controller::setMousePassthrough(bool enabled, bool keepDevice)
{
    if (m_mousePassthrough == enabled) {
        // A fast pointer -> FPS switch may have kept the idle UHID device. A
        // later explicit disable/close must still be able to destroy it.
        if (!enabled && !keepDevice) {
            destroyMousePassthroughDevice();
        }
        return;
    }

    m_mousePassthrough = enabled;
    if (!enabled) {
        if (m_inputConvert) {
            m_inputConvert->setMousePassthrough(false);
        }
        if (keepDevice) {
            // The descriptor stays registered, but HOVER_EXIT above makes it
            // inactive. This avoids an Android InputReader rescan on every '~'
            // while FPS mouse movement immediately uses CMT_INJECT_TOUCH.
            qInfo() << "UHID pointer suspended for direct FPS input";
        } else {
            destroyMousePassthroughDevice();
        }
    } else if (m_inputConvert) {
        // The real frame size arrives with the explicit center MouseMove sent
        // by VideoForm. Create the UHID descriptor lazily from that size so its
        // raw X:Y aspect ratio matches Android's display viewport.
        m_inputConvert->setMousePassthrough(
            true,
            m_mousePassthroughDeviceCreated
                ? clampAbsolutePointerRange(m_mousePassthroughFrameSize)
                : QSize());
    }
}

void Controller::ensureMousePassthroughDevice(const QSize &frameSize)
{
    if (!m_mousePassthrough || frameSize.width() <= 1 || frameSize.height() <= 1) {
        return;
    }
    if (m_mousePassthroughDeviceCreated && m_mousePassthroughFrameSize == frameSize) {
        return;
    }

    // Android's drawing-tablet POINTER mode deliberately uses one fixed scale
    // for both axes (max(xScale, yScale)). A square 32767x32767 descriptor on a
    // rectangular tablet therefore cannot align with touchscreen coordinates.
    // Recreate the UHID axes with the current video aspect ratio. The same path
    // runs automatically after rotation or a display/video resize.
    if (m_mousePassthroughDeviceCreated) {
        if (m_inputConvert) {
            m_inputConvert->setMousePassthrough(false);
        }
        destroyMousePassthroughDevice();
    }

    const QSize pointerRange = clampAbsolutePointerRange(frameSize);
    ControlMsg *create = new ControlMsg(ControlMsg::CMT_UHID_CREATE);
    create->setUhidCreateData(
        2, 0, 0, QByteArray("QtScrcpy Absolute Game Pointer"),
        buildAbsolutePointerReportDescriptor(pointerRange));
    postControlMsg(create);
    m_mousePassthroughDeviceCreated = true;
    m_mousePassthroughFrameSize = frameSize;
    qInfo() << "UHID absolute pointer calibrated to video frame:" << frameSize
            << "axis range:" << pointerRange;

    if (m_inputConvert) {
        m_inputConvert->setMousePassthrough(true, pointerRange);
    }
}

void Controller::destroyMousePassthroughDevice()
{
    if (!m_mousePassthroughDeviceCreated) {
        m_mousePassthroughFrameSize = QSize();
        return;
    }

    // Any touch release/proximity exit has already been queued by the input
    // converter. Posted control events preserve order before this destroy.
    ControlMsg *destroy = new ControlMsg(ControlMsg::CMT_UHID_DESTROY);
    destroy->setUhidDestroyData(2);
    postControlMsg(destroy);
    m_mousePassthroughDeviceCreated = false;
    m_mousePassthroughFrameSize = QSize();
}

void Controller::resetInputState()
{
    if (m_inputConvert) {
        m_inputConvert->resetInputState();
    }

    // Re-registering the retained drawing-tablet device also clears any
    // InputReader hover/button state. It is recreated lazily on the next
    // pointer move when passthrough remains enabled.
    destroyMousePassthroughDevice();
    postControlMsg(new ControlMsg(ControlMsg::CMT_RESET_INPUT_STATE));
    qInfo() << "Input state recovery requested";
}

void Controller::postBackOrScreenOn(bool down)
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_BACK_OR_SCREEN_ON);
    controlMsg->setBackOrScreenOnData(down);
    if (!controlMsg) {
        return;
    }
    postControlMsg(controlMsg);
}

void Controller::postGoHome()
{
    postKeyCodeClick(AKEYCODE_HOME);
}

void Controller::postGoMenu()
{
    postKeyCodeClick(AKEYCODE_MENU);
}

void Controller::postGoBack()
{
    postKeyCodeClick(AKEYCODE_BACK);
}

void Controller::postAppSwitch()
{
    postKeyCodeClick(AKEYCODE_APP_SWITCH);
}

void Controller::postPower()
{
    postKeyCodeClick(AKEYCODE_POWER);
}

void Controller::postVolumeUp()
{
    postKeyCodeClick(AKEYCODE_VOLUME_UP);
}

void Controller::postVolumeDown()
{
    postKeyCodeClick(AKEYCODE_VOLUME_DOWN);
}

void Controller::copy()
{
    postKeyCodeClick(AKEYCODE_COPY);
}

void Controller::cut()
{
    postKeyCodeClick(AKEYCODE_CUT);
}

void Controller::expandNotificationPanel()
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_EXPAND_NOTIFICATION_PANEL);
    if (!controlMsg) {
        return;
    }
    postControlMsg(controlMsg);
}

void Controller::expandSettingsPanel()
{
    postControlMsg(new ControlMsg(ControlMsg::CMT_EXPAND_SETTINGS_PANEL));
}

void Controller::collapsePanel()
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_COLLAPSE_PANELS);
    if (!controlMsg) {
        return;
    }
    postControlMsg(controlMsg);
}

void Controller::rotateDevice()
{
    postControlMsg(new ControlMsg(ControlMsg::CMT_ROTATE_DEVICE));
}

void Controller::startApp(const QString &name)
{
    if (name.isEmpty()) {
        return;
    }
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_START_APP);
    controlMsg->setStartAppData(name);
    postControlMsg(controlMsg);
}

void Controller::scanFile(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_SCAN_FILE);
    controlMsg->setScanFileData(path);
    postControlMsg(controlMsg);
}

void Controller::resizeDisplay(const QSize &size)
{
    if (size.width() <= 0 || size.height() <= 0) {
        return;
    }
    m_pendingResize = size;
    if (m_resizeQueued) {
        return;
    }
    m_resizeQueued = true;
    QTimer::singleShot(0, this, &Controller::sendPendingResize);
}

void Controller::sendPendingResize()
{
    m_resizeQueued = false;
    if (m_pendingResize.isEmpty()) {
        return;
    }
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_RESIZE_DISPLAY);
    controlMsg->setResizeDisplayData(m_pendingResize);
    m_pendingResize = QSize();
    postControlMsg(controlMsg);
}

void Controller::requestDeviceClipboard()
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_GET_CLIPBOARD);
    if (!controlMsg) {
        return;
    }
    postControlMsg(controlMsg);
}

void Controller::getDeviceClipboard(bool cut)
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_GET_CLIPBOARD);
    if (!controlMsg) {
        return;
    }
    ControlMsg::GetClipboardCopyKey copyKey = cut ? ControlMsg::GCCK_CUT : ControlMsg::GCCK_COPY;
    controlMsg->setGetClipboardMsgData(copyKey);
    postControlMsg(controlMsg);
}

void Controller::setDeviceClipboard(bool pause)
{
    QClipboard *board = QApplication::clipboard();
    QString text = board->text();
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_SET_CLIPBOARD);
    if (!controlMsg) {
        return;
    }
    controlMsg->setSetClipboardMsgData(text, pause);
    postControlMsg(controlMsg);
}

void Controller::clipboardPaste()
{
    QClipboard *board = QApplication::clipboard();
    QString text = board->text();
    postTextInput(text);
}

void Controller::postTextInput(QString &text)
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_TEXT);
    if (!controlMsg) {
        return;
    }
    controlMsg->setInjectTextMsgData(text);
    postControlMsg(controlMsg);
}

void Controller::setDisplayPower(bool on)
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_SET_DISPLAY_POWER);
    if (!controlMsg) {
        return;
    }
    controlMsg->setDisplayPowerData(on);
    postControlMsg(controlMsg);
}

void Controller::setVideoPaused(bool paused)
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_SET_VIDEO_PAUSED);
    controlMsg->setVideoPausedData(paused);
    postControlMsg(controlMsg);
}

void Controller::setCameraTorch(bool on)
{
    ControlMsg *controlMsg = new ControlMsg(ControlMsg::CMT_CAMERA_SET_TORCH);
    controlMsg->setCameraTorchData(on);
    postControlMsg(controlMsg);
}

void Controller::cameraZoomIn()
{
    postControlMsg(new ControlMsg(ControlMsg::CMT_CAMERA_ZOOM_IN));
}

void Controller::cameraZoomOut()
{
    postControlMsg(new ControlMsg(ControlMsg::CMT_CAMERA_ZOOM_OUT));
}

void Controller::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_mousePassthrough) {
        ensureMousePassthroughDevice(frameSize);
    }
    if (m_inputConvert) {
        m_inputConvert->mouseEvent(from, frameSize, showSize);
    }
}

void Controller::mouseMoveRelative(const QPointF &delta, const QSize &frameSize, const QSize &showSize)
{
    if (m_inputConvert) {
        m_inputConvert->mouseMoveRelative(delta, frameSize, showSize);
    }
}

void Controller::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_mousePassthrough) {
        ensureMousePassthroughDevice(frameSize);
    }
    if (m_inputConvert) {
        m_inputConvert->wheelEvent(from, frameSize, showSize);
    }
}

void Controller::keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_inputConvert) {
        m_inputConvert->keyEvent(from, frameSize, showSize);
    }
}

bool Controller::event(QEvent *event)
{
    if (event && static_cast<ControlMsg::Type>(event->type()) == ControlMsg::Control) {
        ControlMsg *controlMsg = dynamic_cast<ControlMsg *>(event);
        if (controlMsg) {
            sendControl(controlMsg->serializeData());
        }
        return true;
    }
    return QObject::event(event);
}

bool Controller::sendControl(const QByteArray &buffer)
{
    if (buffer.isEmpty()) {
        return false;
    }
    qint32 len = 0;
    if (m_sendData) {
        len = static_cast<qint32>(m_sendData(buffer));
    }
    return len == buffer.length() ? true : false;
}

void Controller::postKeyCodeClick(AndroidKeycode keycode)
{
    ControlMsg *controlEventDown = new ControlMsg(ControlMsg::CMT_INJECT_KEYCODE);
    if (!controlEventDown) {
        return;
    }
    controlEventDown->setInjectKeycodeMsgData(AKEY_EVENT_ACTION_DOWN, keycode, 0, AMETA_NONE);
    postControlMsg(controlEventDown);

    ControlMsg *controlEventUp = new ControlMsg(ControlMsg::CMT_INJECT_KEYCODE);
    if (!controlEventUp) {
        return;
    }
    controlEventUp->setInjectKeycodeMsgData(AKEY_EVENT_ACTION_UP, keycode, 0, AMETA_NONE);
    postControlMsg(controlEventUp);
}
