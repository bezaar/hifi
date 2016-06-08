//
//  BlurTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 6/7/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "BlurTask.h"

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "blurGaussianV_frag.h"
#include "blurGaussianH_frag.h"

using namespace render;

enum BlurShaderBufferSlots {
    BlurTask_ParamsSlot = 0,
};
enum BlurShaderMapSlots {
    BlurTask_SourceSlot = 0,
};

const float BLUR_NUM_SAMPLES = 7.0f;

BlurParams::BlurParams() {
    Params params;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Params), (const gpu::Byte*) &params));
}

void BlurParams::setWidthHeight(int width, int height) {
    auto resolutionInfo = _parametersBuffer.get<Params>().resolutionInfo;
    if (width != resolutionInfo.x || height != resolutionInfo.y) {
        _parametersBuffer.edit<Params>().resolutionInfo = glm::vec4((float) width, (float) height, 1.0f / (float) width, 1.0f / (float) height);
    }
}

void BlurParams::setFilterRadiusScale(float scale) {
    auto filterInfo = _parametersBuffer.get<Params>().filterInfo;
    if (scale != filterInfo.x) {
        _parametersBuffer.edit<Params>().filterInfo.x = scale;
        _parametersBuffer.edit<Params>().filterInfo.y = scale / BLUR_NUM_SAMPLES;
    }
}

BlurGaussian::BlurGaussian() {
    _parameters = std::make_shared<BlurParams>();
}

gpu::PipelinePointer BlurGaussian::getBlurVPipeline() {
    if (!_blurVPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(blurGaussianV_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("blurParamsBuffer"), BlurTask_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurVPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurVPipeline;
}

gpu::PipelinePointer BlurGaussian::getBlurHPipeline() {
    if (!_blurHPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(blurGaussianH_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("blurParamsBuffer"), BlurTask_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _blurHPipeline = gpu::Pipeline::create(program, state);
    }

    return _blurHPipeline;
}

bool BlurGaussian::updateBlurringResources(const gpu::FramebufferPointer& sourceFramebuffer, BlurringResources& blurringResources) {
    if (!sourceFramebuffer) {
        return false;
    }

    if (!_blurredFramebuffer) {
        _blurredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());

        // attach depthStencil if present in source
        if (sourceFramebuffer->hasDepthStencil()) {
            _blurredFramebuffer->setDepthStencilBuffer(sourceFramebuffer->getDepthStencilBuffer(), sourceFramebuffer->getDepthStencilBufferFormat());
        }
        auto blurringSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT);
        auto blurringTarget = gpu::TexturePointer(gpu::Texture::create2D(sourceFramebuffer->getRenderBuffer(0)->getTexelFormat(), sourceFramebuffer->getWidth(), sourceFramebuffer->getHeight(), blurringSampler));
        _blurredFramebuffer->setRenderBuffer(0, blurringTarget);
    }
    else {
        // it would be easier to just call resize on the bluredFramebuffer and let it work if needed but the source might loose it's depth buffer when doing so
        if ((_blurredFramebuffer->getWidth() != sourceFramebuffer->getWidth()) || (_blurredFramebuffer->getHeight() != sourceFramebuffer->getHeight())) {
            _blurredFramebuffer->resize(sourceFramebuffer->getWidth(), sourceFramebuffer->getHeight(), sourceFramebuffer->getNumSamples());
            if (sourceFramebuffer->hasDepthStencil()) {
                _blurredFramebuffer->setDepthStencilBuffer(sourceFramebuffer->getDepthStencilBuffer(), sourceFramebuffer->getDepthStencilBufferFormat());
            }
        }
    }

    blurringResources.sourceTexture = sourceFramebuffer->getRenderBuffer(0);
    blurringResources.blurringFramebuffer = _blurredFramebuffer;
    blurringResources.blurringTexture = _blurredFramebuffer->getRenderBuffer(0);
    blurringResources.finalFramebuffer = sourceFramebuffer;

    return true;
}

void BlurGaussian::configure(const Config& config) {
    _parameters->setFilterRadiusScale(config.filterScale);
}


void BlurGaussian::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;


    BlurringResources blurringResources;
    if (!updateBlurringResources(sourceFramebuffer, blurringResources)) {
        // early exit if no valid blurring resources
        return;
    }

    auto blurVPipeline = getBlurVPipeline();
    auto blurHPipeline = getBlurHPipeline();

    _parameters->setWidthHeight(args->_viewport.z, args->_viewport.w);

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);

        batch.setUniformBuffer(BlurTask_ParamsSlot, _parameters->_parametersBuffer);

        batch.setFramebuffer(blurringResources.blurringFramebuffer);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0));

        batch.setPipeline(blurVPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, blurringResources.sourceTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setFramebuffer(blurringResources.finalFramebuffer);
        batch.setPipeline(blurHPipeline);
        batch.setResourceTexture(BlurTask_SourceSlot, blurringResources.blurringTexture);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        batch.setResourceTexture(BlurTask_SourceSlot, nullptr);
        batch.setUniformBuffer(BlurTask_ParamsSlot, nullptr);
    });
}

