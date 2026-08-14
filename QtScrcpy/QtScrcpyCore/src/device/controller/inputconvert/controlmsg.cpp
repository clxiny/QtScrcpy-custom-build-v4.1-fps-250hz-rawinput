#include <QDebug>

#include "bufferutil.h"
#include "controlmsg.h"

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define CLAMP(V, X, Y) MIN(MAX((V), (X)), (Y))

ControlMsg::ControlMsg(ControlMsgType controlMsgType) : QScrcpyEvent(Control)
{
    m_data.type = controlMsgType;
}

ControlMsg::~ControlMsg()
{
    if (CMT_SET_CLIPBOARD == m_data.type && Q_NULLPTR != m_data.setClipboard.text) {
        delete[] m_data.setClipboard.text;
        m_data.setClipboard.text = Q_NULLPTR;
    } else if (CMT_INJECT_TEXT == m_data.type && Q_NULLPTR != m_data.injectText.text) {
        delete[] m_data.injectText.text;
        m_data.injectText.text = Q_NULLPTR;
    } else if (CMT_START_APP == m_data.type && Q_NULLPTR != m_data.startApp.name) {
        delete[] m_data.startApp.name;
        m_data.startApp.name = Q_NULLPTR;
    } else if (CMT_SCAN_FILE == m_data.type && Q_NULLPTR != m_data.scanFile.path) {
        delete[] m_data.scanFile.path;
        m_data.scanFile.path = Q_NULLPTR;
    }
}

void ControlMsg::setInjectKeycodeMsgData(AndroidKeyeventAction action, AndroidKeycode keycode, quint32 repeat, AndroidMetastate metastate)
{
    m_data.injectKeycode.action = action;
    m_data.injectKeycode.keycode = keycode;
    m_data.injectKeycode.repeat = repeat;
    m_data.injectKeycode.metastate = metastate;
}

void ControlMsg::setInjectTextMsgData(QString &text)
{
    // write length (2 byte) + string (non nul-terminated)
    if (CONTROL_MSG_INJECT_TEXT_MAX_LENGTH < text.length()) {
        // injecting a text takes time, so limit the text length
        text = text.left(CONTROL_MSG_INJECT_TEXT_MAX_LENGTH);
    }
    QByteArray tmp = text.toUtf8();
    m_data.injectText.text = new char[tmp.length() + 1];
    memcpy(m_data.injectText.text, tmp.data(), tmp.length());
    m_data.injectText.text[tmp.length()] = '\0';
}

void ControlMsg::setInjectTouchMsgData(
    quint64 id,
    AndroidMotioneventAction action,
    AndroidMotioneventButtons actionButtons,
    AndroidMotioneventButtons buttons,
    QRect position,
    float pressure)
{
    m_data.injectTouch.id = id;
    m_data.injectTouch.action = action;
    m_data.injectTouch.actionButtons = actionButtons;
    m_data.injectTouch.buttons = buttons;
    m_data.injectTouch.position = position;
    m_data.injectTouch.pressure = pressure;
}

void ControlMsg::setInjectScrollMsgData(QRect position, float hScroll, float vScroll, AndroidMotioneventButtons buttons)
{
    m_data.injectScroll.position = position;
    m_data.injectScroll.hScroll = hScroll;
    m_data.injectScroll.vScroll = vScroll;
    m_data.injectScroll.buttons = buttons;
}

void ControlMsg::setGetClipboardMsgData(ControlMsg::GetClipboardCopyKey copyKey) 
{
    m_data.getClipboard.copyKey = copyKey;
}

void ControlMsg::setSetClipboardMsgData(QString &text, bool paste)
{
    if (text.isEmpty()) {
        m_data.setClipboard.text = Q_NULLPTR;
        return;
    }
    if (CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH < text.length()) {
        text = text.left(CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH);
    }

    QByteArray tmp = text.toUtf8();
    m_data.setClipboard.text = new char[tmp.length() + 1];
    memcpy(m_data.setClipboard.text, tmp.data(), tmp.length());
    m_data.setClipboard.text[tmp.length()] = '\0';
    m_data.setClipboard.paste = paste;
    m_data.setClipboard.sequence = 0;
}

void ControlMsg::setDisplayPowerData(bool on)
{
    m_data.setDisplayPower.on = on;
}

void ControlMsg::setVideoPausedData(bool paused)
{
    m_data.setVideoPaused.paused = paused;
}

void ControlMsg::setUhidCreateData(quint16 id, quint16 vendorId, quint16 productId,
                                   const QByteArray &name, const QByteArray &reportDesc)
{
    m_data.uhidCreate.id = id;
    m_data.uhidCreate.vendorId = vendorId;
    m_data.uhidCreate.productId = productId;
    m_data.uhidCreate.nameSize = static_cast<quint8>(qMin(name.size(), 127));
    memcpy(m_data.uhidCreate.name, name.constData(), m_data.uhidCreate.nameSize);
    m_data.uhidCreate.reportDescSize = static_cast<quint16>(qMin(reportDesc.size(), 256));
    memcpy(m_data.uhidCreate.reportDesc, reportDesc.constData(), m_data.uhidCreate.reportDescSize);
}

void ControlMsg::setUhidInputData(quint16 id, const QByteArray &data)
{
    m_data.uhidInput.id = id;
    m_data.uhidInput.size = static_cast<quint16>(qMin(data.size(), 64));
    memcpy(m_data.uhidInput.data, data.constData(), m_data.uhidInput.size);
}

void ControlMsg::setUhidDestroyData(quint16 id)
{
    m_data.uhidDestroy.id = id;
}

void ControlMsg::setCameraTorchData(bool on)
{
    m_data.cameraTorch.on = on;
}

void ControlMsg::setBackOrScreenOnData(bool down)
{
    m_data.backOrScreenOn.action = down ? AKEY_EVENT_ACTION_DOWN : AKEY_EVENT_ACTION_UP;
}

void ControlMsg::setStartAppData(const QString &name)
{
    QByteArray utf8 = name.toUtf8().left(CONTROL_MSG_START_APP_MAX_LENGTH);
    m_data.startApp.name = new char[utf8.size() + 1];
    memcpy(m_data.startApp.name, utf8.constData(), utf8.size());
    m_data.startApp.name[utf8.size()] = '\0';
}

void ControlMsg::setScanFileData(const QString &path)
{
    QByteArray utf8 = path.toUtf8().left(CONTROL_MSG_SCAN_FILE_PATH_MAX_LENGTH);
    m_data.scanFile.path = new char[utf8.size() + 1];
    memcpy(m_data.scanFile.path, utf8.constData(), utf8.size());
    m_data.scanFile.path[utf8.size()] = '\0';
}

void ControlMsg::setResizeDisplayData(const QSize &size)
{
    m_data.resizeDisplay.width = static_cast<quint16>(qBound(1, size.width(), 0xffff));
    m_data.resizeDisplay.height = static_cast<quint16>(qBound(1, size.height(), 0xffff));
}

void ControlMsg::writePosition(QBuffer &buffer, const QRect &value)
{
    BufferUtil::write32(buffer, value.left());
    BufferUtil::write32(buffer, value.top());
    BufferUtil::write16(buffer, value.width());
    BufferUtil::write16(buffer, value.height());
}

quint16 ControlMsg::flostToU16fp(float f)
{
    Q_ASSERT(f >= 0.0f && f <= 1.0f);
    quint32 u = f * 0x1p16f; // 2^16
    if (u >= 0xffff) {
        u = 0xffff;
    }
    return (quint16)u;
}

qint16 ControlMsg::flostToI16fp(float f)
{
    Q_ASSERT(f >= -1.0f && f <= 1.0f);
    qint32 i = f * 0x1p15f; // 2^15
    Q_ASSERT(i >= -0x8000);
    if (i >= 0x7fff) {
        Q_ASSERT(i == 0x8000); // for f == 1.0f
        i = 0x7fff;
    }
    return (qint16)i;
}

QByteArray ControlMsg::serializeData()
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QBuffer::WriteOnly);
    buffer.putChar(m_data.type);

    switch (m_data.type) {
    case CMT_INJECT_KEYCODE:
        buffer.putChar(m_data.injectKeycode.action);
        BufferUtil::write32(buffer, m_data.injectKeycode.keycode);
        BufferUtil::write32(buffer, m_data.injectKeycode.repeat);
        BufferUtil::write32(buffer, m_data.injectKeycode.metastate);
        break;
    case CMT_INJECT_TEXT:
        BufferUtil::write32(buffer, static_cast<quint32>(strlen(m_data.injectText.text)));
        buffer.write(m_data.injectText.text, strlen(m_data.injectText.text));
        break;
    case CMT_INJECT_TOUCH: {
        buffer.putChar(m_data.injectTouch.action);
        BufferUtil::write64(buffer, m_data.injectTouch.id);
        writePosition(buffer, m_data.injectTouch.position);
        quint16 pressure = flostToU16fp(m_data.injectTouch.pressure);
        BufferUtil::write16(buffer, pressure);
        BufferUtil::write32(buffer, m_data.injectTouch.actionButtons);
        BufferUtil::write32(buffer, m_data.injectTouch.buttons);
    } break;
    case CMT_INJECT_SCROLL: {
        writePosition(buffer, m_data.injectScroll.position);
        // Accept values in the range [-16, 16].
        // Normalize to [-1, 1] in order to use sc_float_to_i16fp().
        float hscrollNorm = m_data.injectScroll.hScroll / 16;
        hscrollNorm = CLAMP(hscrollNorm, -1, 1);
        float vscrollNorm = m_data.injectScroll.vScroll / 16;
        vscrollNorm = CLAMP(vscrollNorm, -1, 1);
        qint16 hScroll = flostToI16fp(hscrollNorm);
        qint16 vScroll = flostToI16fp(vscrollNorm);
        BufferUtil::write16(buffer, (quint16)hScroll);
        BufferUtil::write16(buffer, (quint16)vScroll);
        BufferUtil::write32(buffer, m_data.injectScroll.buttons);
    } break;
    case CMT_BACK_OR_SCREEN_ON:
        buffer.putChar(m_data.backOrScreenOn.action);
        break;
    case CMT_GET_CLIPBOARD:
        buffer.putChar(m_data.getClipboard.copyKey);
        break;
    case CMT_SET_CLIPBOARD:
        BufferUtil::write64(buffer, m_data.setClipboard.sequence);
        buffer.putChar(!!m_data.setClipboard.paste);
        if (m_data.setClipboard.text != Q_NULLPTR) {
            BufferUtil::write32(buffer, static_cast<quint32>(strlen(m_data.setClipboard.text)));
            buffer.write(m_data.setClipboard.text, strlen(m_data.setClipboard.text));
        } else {
            BufferUtil::write32(buffer, 0);
            buffer.write(m_data.setClipboard.text, 0);
        }
        break;
    case CMT_SET_DISPLAY_POWER:
        buffer.putChar(m_data.setDisplayPower.on);
        break;
    case CMT_SET_VIDEO_PAUSED:
        buffer.putChar(m_data.setVideoPaused.paused);
        break;
    case CMT_UHID_CREATE:
        BufferUtil::write16(buffer, m_data.uhidCreate.id);
        BufferUtil::write16(buffer, m_data.uhidCreate.vendorId);
        BufferUtil::write16(buffer, m_data.uhidCreate.productId);
        buffer.putChar(m_data.uhidCreate.nameSize);
        buffer.write(m_data.uhidCreate.name, m_data.uhidCreate.nameSize);
        BufferUtil::write16(buffer, m_data.uhidCreate.reportDescSize);
        buffer.write(m_data.uhidCreate.reportDesc, m_data.uhidCreate.reportDescSize);
        break;
    case CMT_UHID_INPUT:
        BufferUtil::write16(buffer, m_data.uhidInput.id);
        BufferUtil::write16(buffer, m_data.uhidInput.size);
        buffer.write(m_data.uhidInput.data, m_data.uhidInput.size);
        break;
    case CMT_UHID_DESTROY:
        BufferUtil::write16(buffer, m_data.uhidDestroy.id);
        break;
    case CMT_CAMERA_SET_TORCH:
        buffer.putChar(m_data.cameraTorch.on);
        break;
    case CMT_START_APP: {
        const quint8 length = m_data.startApp.name ? static_cast<quint8>(strlen(m_data.startApp.name)) : 0;
        buffer.putChar(length);
        if (length) {
            buffer.write(m_data.startApp.name, length);
        }
    } break;
    case CMT_RESIZE_DISPLAY:
        BufferUtil::write16(buffer, m_data.resizeDisplay.width);
        BufferUtil::write16(buffer, m_data.resizeDisplay.height);
        break;
    case CMT_SCAN_FILE: {
        const quint32 length = m_data.scanFile.path ? static_cast<quint32>(strlen(m_data.scanFile.path)) : 0;
        BufferUtil::write32(buffer, length);
        if (length) {
            buffer.write(m_data.scanFile.path, length);
        }
    } break;
    case CMT_EXPAND_NOTIFICATION_PANEL:
    case CMT_EXPAND_SETTINGS_PANEL:
    case CMT_COLLAPSE_PANELS:
    case CMT_ROTATE_DEVICE:
    case CMT_OPEN_HARD_KEYBOARD_SETTINGS:
    case CMT_RESET_VIDEO:
    case CMT_CAMERA_ZOOM_IN:
    case CMT_CAMERA_ZOOM_OUT:
    case CMT_RESET_INPUT_STATE:
        break;
    default:
        qDebug() << "Unknown event type:" << m_data.type;
        break;
    }
    buffer.close();
    return byteArray;
}
