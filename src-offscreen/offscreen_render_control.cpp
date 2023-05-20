#include <QtLogging>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QImage>
#include <QSize>
#include <QtGui/private/qrhi_p.h>
#include <QtQuick/private/qquickrendercontrol_p.h>
#include "offscreen_render_control.h"

OffscreenRenderControl::OffscreenRenderControl()
    : QQuickRenderControl()
{
}

OffscreenRenderControl::~OffscreenRenderControl()
{
}

bool OffscreenRenderControl::install(QQuickWindow *window)
{
    if (initialized)
        return true;
    if (!initialize()) {
        qCritical("Failed to initialize render control");
        return false;
    }

    QRhi* rhi = QQuickRenderControlPrivate::get(this)->rhi;

    const QSize size = window->size();
    texture.reset(rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!texture->create()) {
        qCritical("Failed to create texture");
        return false;
    }

    // depth-stencil is mandatory with RHI, although strictly speaking the
    // scenegraph could operate without one, but it has no means to figure out
    // the lack of a ds buffer, so just be nice and provide one.
    stencilBuffer.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size, 1));
    if (!stencilBuffer->create()) {
        qCritical("Failed to create render buffer");
        return false;
    }

    QRhiTextureRenderTargetDescription renderTargetDescription((QRhiColorAttachment(texture.data())));
    renderTargetDescription.setDepthStencilBuffer(stencilBuffer.data());
    textureRenderTarget.reset(rhi->newTextureRenderTarget(renderTargetDescription));
    renderPassDescriptor.reset(textureRenderTarget->newCompatibleRenderPassDescriptor());
    textureRenderTarget->setRenderPassDescriptor(renderPassDescriptor.data());
    if (!textureRenderTarget->create()) {
        qCritical("Failed to create render target");
        return false;
    }

    // redirect Qt Quick rendering into our texture
    window->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(textureRenderTarget.data()));

    initialized = true;
    return true;
}

QImage OffscreenRenderControl::renderImage()
{
    polishItems();
    beginFrame();
    sync();
    render();

    // RHI is private, we need to use it to avoid writing platform specific code for every platform
    // See https://bugreports.qt.io/browse/QTBUG-88876
    QQuickRenderControlPrivate* renderControlPrivate = QQuickRenderControlPrivate::get(this);
    QRhi* rhi = renderControlPrivate->rhi;

    QRhiReadbackResult readResult;
    QImage renderImage;
    readResult.completed = [&readResult, &rhi, &renderImage] {
        QImage sourceImage(reinterpret_cast<const uchar*>(readResult.data.constData()),
            readResult.pixelSize.width(), readResult.pixelSize.height(),
            QImage::Format_RGBA8888_Premultiplied);
        if (rhi->isYUpInFramebuffer()) {
            sourceImage.mirror();
        }
        renderImage = sourceImage;
    };
    QRhiResourceUpdateBatch* readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture(texture.data(), &readResult);
    renderControlPrivate->cb->resourceUpdate(readbackBatch);

    endFrame();

    return renderImage;
}