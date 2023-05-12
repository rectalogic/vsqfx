#pragma once

#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QImage>
#include <QScopedPointer>
#include <QtGui/private/qrhi_p.h>

class OffscreenRenderControl : public QQuickRenderControl {
    Q_OBJECT
public:
    OffscreenRenderControl();
    bool install(QQuickWindow* window);
    QImage render();

private:
    QScopedPointer<QRhiTexture> texture;
    QScopedPointer<QRhiRenderBuffer> stencilBuffer;
    QScopedPointer<QRhiTextureRenderTarget> textureRenderTarget;
    QScopedPointer<QRhiRenderPassDescriptor> renderPassDescriptor;
    bool initialized;
};
