/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "render_widget_host_view_qt.h"

#include "backing_store_qt.h"
#include "web_event_factory.h"
#include "native_view_qt.h"

#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/common/gpu/gpu_messages.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScreenInfo.h"

#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScreen>
#include <QQuickWindow>

static void GetScreenInfoFromNativeWindow(QWindow* window, WebKit::WebScreenInfo* results)
{
    QScreen* screen = window->screen();

    WebKit::WebScreenInfo r;
    r.deviceScaleFactor = screen->devicePixelRatio();
    r.depthPerComponent = 8;
    r.depth = screen->depth();
    r.isMonochrome = (r.depth == 1);

    QRect virtualGeometry = screen->virtualGeometry();
    r.rect = WebKit::WebRect(virtualGeometry.x(), virtualGeometry.y(), virtualGeometry.width(), virtualGeometry.height());
    QRect available = screen->availableGeometry();
    r.availableRect = WebKit::WebRect(available.x(), available.y(), available.width(), available.height());
    *results = r;
}


namespace content {

RenderWidgetHostView* RenderWidgetHostView::CreateViewForWidget(RenderWidgetHost* widget) {
    // WebContentsViewQt should always take care of this directly.
    Q_ASSERT(false);
}

// static
void RenderWidgetHostViewPort::GetDefaultScreenInfo(WebKit::WebScreenInfo* results) {
    QWindow dummy;
    GetScreenInfoFromNativeWindow(&dummy, results);
}

RenderWidgetHostViewQt::RenderWidgetHostViewQt(content::RenderWidgetHost* widget)
    : m_host(content::RenderWidgetHostImpl::From(widget))
    , m_nativeView(0)
    , about_to_validate_and_paint_(false)
{
    m_host->SetView(this);
}

RenderWidgetHostViewQt::~RenderWidgetHostViewQt()
{
}

bool RenderWidgetHostViewQt::handleEvent(QEvent* event) {

    switch(event->type()) {
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        handleMouseEvent(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        handleKeyEvent(static_cast<QKeyEvent*>(event));
        break;
    case QEvent::Wheel:
        handleWheelEvent(static_cast<QWheelEvent*>(event));
        break;
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        handleFocusEvent(static_cast<QFocusEvent*>(event));
        break;
    default:
        return false;
    }
    return true;
}

void RenderWidgetHostViewQt::InitWithContainer(NativeViewQt* nativeView)
{
    m_nativeView = nativeView;
    m_nativeView->setRenderWidgetHostView(this);
    bool force_create = !m_host->empty();
    BackingStoreQt* backing_store = static_cast<BackingStoreQt*>(m_host->GetBackingStore(force_create));
    m_nativeView->setBackingStore(backing_store);
}

content::BackingStore *RenderWidgetHostViewQt::AllocBackingStore(const gfx::Size &size)
{
    if (m_nativeView)
        return new BackingStoreQt(m_host, size, new QWindow);
    return 0;
}

void RenderWidgetHostViewQt::InitAsChild(gfx::NativeView parent_view)
{
    Q_ASSERT(false);
}

void RenderWidgetHostViewQt::InitAsPopup(content::RenderWidgetHostView*, const gfx::Rect&)
{
}

void RenderWidgetHostViewQt::InitAsFullscreen(content::RenderWidgetHostView*)
{
}

content::RenderWidgetHost* RenderWidgetHostViewQt::GetRenderWidgetHost() const
{
    return m_host;
}

gfx::Size RenderWidgetHostViewQt::GetPhysicalBackingSize() const
{
    if (!m_nativeView)
        return m_requestedSize;
    return gfx::Size(m_nativeView->screenRect().width(), m_nativeView->screenRect().height());
}

void RenderWidgetHostViewQt::SetSize(const gfx::Size& size)
{
    int width = size.width();
    int height = size.height();
    // int width = std::min(size.width(), kMaxWindowWidth);
    // int height = std::min(size.height(), kMaxWindowHeight);
    // if (IsPopup())
        // m_nativeView->resize(width,height);

    if (m_requestedSize.width() != width ||
        m_requestedSize.height() != height) {
        m_requestedSize = gfx::Size(width, height);
        // m_host->SendScreenRects();
        m_host->WasResized();
    }
}

void RenderWidgetHostViewQt::SetBounds(const gfx::Rect& rect)
{
    // This is called when webkit has sent us a Move message.
    // if (IsPopup())
        // m_nativeView->setGeometry(rect.x(), rect.y(), rect.width(), rect.height());
    SetSize(rect.size());
}

// FIXME: Should this really return a QWindow pointer?
gfx::NativeView RenderWidgetHostViewQt::GetNativeView() const
{
    QT_NOT_YET_IMPLEMENTED
    return gfx::NativeView();
}

gfx::NativeViewId RenderWidgetHostViewQt::GetNativeViewId() const
{
    QT_NOT_YET_IMPLEMENTED
    return gfx::NativeViewId();
}

gfx::NativeViewAccessible RenderWidgetHostViewQt::GetNativeViewAccessible()
{
    NOTIMPLEMENTED();
    return NULL;
}

// Set focus to the associated View component.
void RenderWidgetHostViewQt::Focus()
{
    // m_nativeView->setFocus(Qt::MouseFocusReason);
}

bool RenderWidgetHostViewQt::HasFocus() const
{
    // return m_nativeView->hasFocus();
    return true;
}

bool RenderWidgetHostViewQt::IsSurfaceAvailableForCopy() const
{
    return true;
}

void RenderWidgetHostViewQt::Show()
{
    m_nativeView->show();
}

void RenderWidgetHostViewQt::Hide()
{
    m_nativeView->hide();
}

bool RenderWidgetHostViewQt::IsShowing()
{
    return m_nativeView->isVisible();
}

// Retrieve the bounds of the View, in screen coordinates.
gfx::Rect RenderWidgetHostViewQt::GetViewBounds() const
{
    QRectF p = m_nativeView->screenRect();
    return gfx::Rect(p.x(), p.y(), p.width(), p.height());
}

// Subclasses should override this method to do what is appropriate to set
// the custom background for their platform.
void RenderWidgetHostViewQt::SetBackground(const SkBitmap& background)
{
    RenderWidgetHostViewBase::SetBackground(background);
    // Send(new ViewMsg_SetBackground(m_host->GetRoutingID(), background));
}

// Return value indicates whether the mouse is locked successfully or not.
bool RenderWidgetHostViewQt::LockMouse()
{
    QT_NOT_YET_IMPLEMENTED
    return false;
}
void RenderWidgetHostViewQt::UnlockMouse()
{
    QT_NOT_YET_IMPLEMENTED
}

// Returns true if the mouse pointer is currently locked.
bool RenderWidgetHostViewQt::IsMouseLocked()
{
    QT_NOT_YET_IMPLEMENTED
    return false;
}

// FIXME: remove TOOLKIT_GTK related things.
#if defined(TOOLKIT_GTK)
// Gets the event for the last mouse down.
GdkEventButton* RenderWidgetHostViewQt::GetLastMouseDown()
{
    return 0;
}

gfx::NativeView RenderWidgetHostViewQt::BuildInputMethodsGtkMenu()
{
}
#endif  // defined(TOOLKIT_GTK)

void RenderWidgetHostViewQt::WasShown()
{
    if (m_nativeView->isVisible())
        return;

    m_host->WasShown();
}

void RenderWidgetHostViewQt::WasHidden()
{
    if (!m_nativeView->isVisible())
        return;

    m_host->WasHidden();
}

void RenderWidgetHostViewQt::MovePluginWindows(const gfx::Vector2d&, const std::vector<webkit::npapi::WebPluginGeometry>&)
{
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::Blur()
{
    m_host->Blur();
}

void RenderWidgetHostViewQt::UpdateCursor(const WebCursor&)
{
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::SetIsLoading(bool)
{
    QT_NOT_YET_IMPLEMENTED
    // Give visual feedback for loading process.
}

void RenderWidgetHostViewQt::TextInputStateChanged(const ViewHostMsg_TextInputState_Params&)
{
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::ImeCancelComposition()
{
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::ImeCompositionRangeChanged(const ui::Range&, const std::vector<gfx::Rect>&)
{
    // FIXME: not implemented?
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::DidUpdateBackingStore(const gfx::Rect& scroll_rect, const gfx::Vector2d& scroll_delta, const std::vector<gfx::Rect>& copy_rects)
{
    if (!m_nativeView || !m_nativeView->isVisible())
        return;

    if (about_to_validate_and_paint_)
        invalid_rect_.Union(scroll_rect);
    else
        Paint(scroll_rect);

    for (size_t i = 0; i < copy_rects.size(); ++i) {
        gfx::Rect rect = gfx::SubtractRects(copy_rects[i], scroll_rect);
        if (rect.IsEmpty())
            continue;

        if (about_to_validate_and_paint_)
            invalid_rect_.Union(rect);
        else
            Paint(rect);
    }
}

void RenderWidgetHostViewQt::RenderViewGone(base::TerminationStatus, int)
{
    Destroy();
}

void RenderWidgetHostViewQt::Destroy()
{
    m_nativeView = 0;
}

void RenderWidgetHostViewQt::SetTooltipText(const string16&)
{
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::SelectionBoundsChanged(const ViewHostMsg_SelectionBounds_Params&)
{
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::ScrollOffsetChanged()
{
    // FIXME: not implemented?
}

void RenderWidgetHostViewQt::CopyFromCompositingSurface(const gfx::Rect& src_subrect, const gfx::Size& /* dst_size */, const base::Callback<void(bool, const SkBitmap&)>& callback)
{
    // Grab the snapshot from the renderer as that's the only reliable way to
    // readback from the GPU for this platform right now.
    // FIXME: is this true?
    GetRenderWidgetHost()->GetSnapshotFromRenderer(src_subrect, callback);
}

void RenderWidgetHostViewQt::CopyFromCompositingSurfaceToVideoFrame(const gfx::Rect& src_subrect, const scoped_refptr<media::VideoFrame>& target, const base::Callback<void(bool)>& callback)
{
    NOTIMPLEMENTED();
    callback.Run(false);
}

bool RenderWidgetHostViewQt::CanCopyToVideoFrame() const
{
    return false;
}

void RenderWidgetHostViewQt::OnAcceleratedCompositingStateChange()
{
    // bool activated = m_host->is_accelerated_compositing_active();
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::AcceleratedSurfaceBuffersSwapped(const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params, int gpu_host_id)
{
    AcceleratedSurfaceMsg_BufferPresented_Params ack_params;
    ack_params.sync_point = 0;
    content::RenderWidgetHostImpl::AcknowledgeBufferPresent(params.route_id, gpu_host_id, ack_params);
}

void RenderWidgetHostViewQt::AcceleratedSurfacePostSubBuffer(const GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params& params, int gpu_host_id)
{
    AcceleratedSurfaceMsg_BufferPresented_Params ack_params;
    ack_params.sync_point = 0;
    content::RenderWidgetHostImpl::AcknowledgeBufferPresent(params.route_id, gpu_host_id, ack_params);
}

void RenderWidgetHostViewQt::AcceleratedSurfaceSuspend()
{
    //FIXME: not implemented?
}

void RenderWidgetHostViewQt::AcceleratedSurfaceRelease()
{
    //FIXME: not implemented?
}

bool RenderWidgetHostViewQt::HasAcceleratedSurface(const gfx::Size&)
{
    return false;
}

void RenderWidgetHostViewQt::GetScreenInfo(WebKit::WebScreenInfo* results)
{
    QWindow* window = m_nativeView->window();
    if (!window)
        return;
    GetScreenInfoFromNativeWindow(window, results);
}

gfx::Rect RenderWidgetHostViewQt::GetBoundsInRootWindow()
{
    if (!m_nativeView || !m_nativeView->window())
        return gfx::Rect();

    QRect r = m_nativeView->window()->frameGeometry();
    return gfx::Rect(r.x(), r.y(), r.width(), r.height());
}

gfx::GLSurfaceHandle RenderWidgetHostViewQt::GetCompositingSurface()
{
    QT_NOT_YET_IMPLEMENTED
    return gfx::GLSurfaceHandle();
}

void RenderWidgetHostViewQt::SetHasHorizontalScrollbar(bool) { }

void RenderWidgetHostViewQt::SetScrollOffsetPinning(bool, bool) { }

void RenderWidgetHostViewQt::OnAccessibilityNotifications(const std::vector<AccessibilityHostMsg_NotificationParams>&)
{
    QT_NOT_YET_IMPLEMENTED
}

void RenderWidgetHostViewQt::Paint(const gfx::Rect& damage_rect)
{
    DCHECK(!about_to_validate_and_paint_);

    invalid_rect_ = damage_rect;
    about_to_validate_and_paint_ = true;

    bool force_create = !m_host->empty();
    BackingStoreQt* backing_store = static_cast<BackingStoreQt*>(m_host->GetBackingStore(force_create));

    // Calling GetBackingStore maybe have changed |invalid_rect_|...
    about_to_validate_and_paint_ = false;

    if (backing_store && m_nativeView) {
        m_nativeView->setBackingStore(backing_store);
        QRect r(invalid_rect_.x(), invalid_rect_.y(), invalid_rect_.width(), invalid_rect_.height());
        m_nativeView->update(r);
    }
}

bool RenderWidgetHostViewQt::IsPopup() const
{
    return popup_type_ != WebKit::WebPopupTypeNone;
}

void RenderWidgetHostViewQt::handleMouseEvent(QMouseEvent* ev)
{
    m_host->ForwardMouseEvent(WebEventFactory::toWebMouseEvent(ev));
}

void RenderWidgetHostViewQt::handleKeyEvent(QKeyEvent *ev)
{
    m_host->ForwardKeyboardEvent(WebEventFactory::toWebKeyboardEvent(ev));
}

void RenderWidgetHostViewQt::handleWheelEvent(QWheelEvent *ev)
{
    m_host->ForwardWheelEvent(WebEventFactory::toWebWheelEvent(ev));
}

void RenderWidgetHostViewQt::handleFocusEvent(QFocusEvent *ev)
{
    if (ev->gotFocus()) {
        m_host->GotFocus();
        m_host->SetActive(true);
        ev->accept();
    } else if (ev->lostFocus()) {
        m_host->SetActive(false);
        m_host->Blur();
        ev->accept();
    }
}

}

