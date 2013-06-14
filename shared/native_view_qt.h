#ifndef NATIVE_VIEW_QT_H
#define NATIVE_VIEW_QT_H

class BackingStoreQt;
class QWindow;

namespace content {
	class RenderWidgetHostViewQt;
}

class NativeViewQt {
public:
    virtual ~NativeViewQt() {}
    virtual void setRenderWidgetHostView(content::RenderWidgetHostViewQt* rwhv) = 0;
    virtual void setBackingStore(BackingStoreQt* backingStore) = 0;
    virtual QRectF screenRect() const = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual bool isVisible() const = 0;
    virtual QWindow* window() const = 0;
    virtual void update(const QRect& rect = QRect()) = 0;
};

#endif
