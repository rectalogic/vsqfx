#pragma once

#include <QImage>
#include <QQuickRenderControl>
#include <QScopedPointer>

class QQuickWindow;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;

class OffscreenRenderControl : public QQuickRenderControl {
    Q_OBJECT
public:
    OffscreenRenderControl();
    ~OffscreenRenderControl();
    bool install(QQuickWindow* window);
    QImage renderImage();

private:
    QScopedPointer<QRhiTexture> texture;
    QScopedPointer<QRhiRenderBuffer> stencilBuffer;
    QScopedPointer<QRhiTextureRenderTarget> textureRenderTarget;
    QScopedPointer<QRhiRenderPassDescriptor> renderPassDescriptor;
    bool initialized;
};
