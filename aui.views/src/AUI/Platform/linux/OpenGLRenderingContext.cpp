// AUI Framework - Declarative UI toolkit for modern C++20
// Copyright (C) 2020-2023 Alex2772
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library. If not, see <http://www.gnu.org/licenses/>.

//
// Created by Alex2772 on 12/7/2021.
//

#include <AUI/GL/gl.h>
#include <AUI/Platform/OpenGLRenderingContext.h>
#include <AUI/Util/ARandom.h>
#include <AUI/Logging/ALogger.h>
#include <AUI/Platform/AMessageBox.h>
#include <AUI/GL/GLDebug.h>

#include <AUI/GL/OpenGLRenderer.h>
#include <AUI/GL/State.h>
#include <GL/glx.h>


/* Typedef for the GL 3.0 context creation function */
typedef GLXContext (*PFNGLXCREATECONTEXTATTRIBSARBPROC)(Display *dpy,
                                                        GLXFBConfig config,
                                                        GLXContext
                                                        share_context,
                                                        Bool direct,
                                                        const int
                                                        *attrib_list);
static PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = nullptr;


GLXContext OpenGLRenderingContext::ourContext = nullptr;

OpenGLRenderingContext::~OpenGLRenderingContext() {
}

void OpenGLRenderingContext::init(const Init& init) {
    CommonRenderingContext::init(init);
    static XSetWindowAttributes swa;
    static XVisualInfo* vi;
    if (ourContext == nullptr) {
        glXCreateContextAttribsARB = reinterpret_cast<PFNGLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB")));

        GLint att[] = {GLX_X_RENDERABLE, True, // 1
                       GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, // 3
                       GLX_RENDER_TYPE, GLX_RGBA_BIT, // 5
                       GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, // 7
                       GLX_RED_SIZE, 8, // 9
                       GLX_GREEN_SIZE, 8, // 11
                       GLX_BLUE_SIZE, 8, // 13
                       GLX_ALPHA_SIZE, 8, // 15
                       GLX_DEPTH_SIZE, 24,
                       GLX_STENCIL_SIZE, 8,
                       GLX_DOUBLEBUFFER, true,
                       GLX_STENCIL_SIZE, 8,
                       GLX_SAMPLE_BUFFERS  , 0,
                       GLX_SAMPLES         , 0,
                       None};

        int fbcount;
        GLXFBConfig* fbc = glXChooseFBConfig(ourDisplay, DefaultScreen(ourDisplay), att, &fbcount);

        if (fbc == nullptr || fbcount <= 0) {
            // try to reduce system requirements
            size_t indexToReduce = std::size(att) - 2;
            do {
                ALogger::warn("[OpenGL compatibility] Reduced OpenGL requirements: pass {}"_format((std::size(att) - indexToReduce) / 2 - 1));
                att[indexToReduce] = 0;
                indexToReduce -= 2;
                fbc = glXChooseFBConfig(ourDisplay, DefaultScreen(ourDisplay), att, &fbcount);
            } while ((fbc == nullptr || fbcount <= 0) && indexToReduce > 13); // up to GLX_BLUE_SIZE

            if (fbc == nullptr || fbcount <= 0) {
                // try to disable rgba.
                att[5] = 0;
                ALogger::warn("[OpenGL compatibility] Disabled RGBA");
                fbc = glXChooseFBConfig(ourDisplay, DefaultScreen(ourDisplay), att, &fbcount);

                if (fbc == nullptr || fbcount <= 0) {
                    // use default attribs
                    ALogger::warn("[OpenGL compatibility] Using default attribs");
                    glXChooseFBConfig(ourDisplay, DefaultScreen(ourDisplay), nullptr, &fbcount);
                    if (fbc == nullptr || fbcount <= 0) {
                        // giving up.
                        ALogger::err("[OpenGL compatibility] System hardware is not supported. Giving up.");
                        exit(-1);
                    }
                }
            }
        }

        // Pick the FB config/visual with the most samples per pixel
        int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = std::numeric_limits<int>::max();

        int i;


        for (i = 0; i < fbcount; ++i) {
            vi = glXGetVisualFromFBConfig(ourDisplay, fbc[i]);
            if (vi) {
                int samp_buf, samples;
                glXGetFBConfigAttrib(ourDisplay, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
                glXGetFBConfigAttrib(ourDisplay, fbc[i], GLX_SAMPLES, &samples);

                if (best_fbc < 0 || samp_buf && samples > best_num_samp)
                    best_fbc = i, best_num_samp = samples;
                if (worst_fbc < 0 || !samp_buf || samples < worst_num_samp)
                    worst_fbc = i, worst_num_samp = samples;
            }
            XFree(vi);
        }

        GLXFBConfig bestFbc = fbc[best_fbc];

        // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
        XFree(fbc);

        // Get a visual
        vi = glXGetVisualFromFBConfig(ourDisplay, bestFbc);
        auto cmap = XCreateColormap(ourDisplay, ourScreen->root, vi->visual, AllocNone);
        swa.colormap = cmap;
        swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask
                         | PointerMotionMask | StructureNotifyMask | PropertyChangeMask | StructureNotifyMask;
        if (glXCreateContextAttribsARB) {
            int attribList[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 3,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                None,
            };
            ourContext = glXCreateContextAttribsARB(ourDisplay, bestFbc, nullptr, true, attribList);
        } else {
            ALogger::warn("OpenGLRenderingContext") << "glXCreateContextAttribsARB is not available";
            ourContext = glXCreateContext(ourDisplay, vi, nullptr, true);
        }
    }
    initX11Window(init, swa, vi);
    glXMakeCurrent(ourDisplay, init.window.mHandle, ourContext);

    if (!glewExperimental) {
        ALogger::info((const char*) glGetString(GL_VERSION));
        ALogger::info((const char*) glGetString(GL_VENDOR));
        ALogger::info((const char*) glGetString(GL_RENDERER));
        ALogger::info((const char*) glGetString(GL_EXTENSIONS));
        glewExperimental = true;
        if (glewInit() != GLEW_OK) {
            throw AException("glewInit failed");
        }
        ALogger::info("OpenGL context is ready");
        ARender::setRenderer(mRenderer = _new<OpenGLRenderer>());
    }

    if (init.parent) {
        XSetTransientForHint(ourDisplay, init.window.mHandle, init.parent->mHandle);
    }

#if defined(_DEBUG)
    gl::setupDebug();
#endif
    //assert(glGetError() == 0);

}

void OpenGLRenderingContext::destroyNativeWindow(ABaseWindow& window) {
    CommonRenderingContext::destroyNativeWindow(window);
    if (auto w = dynamic_cast<AWindow*>(&window)) {
        XDestroyWindow(ourDisplay, w->mHandle);
    }
}

void OpenGLRenderingContext::beginPaint(ABaseWindow& window) {
    CommonRenderingContext::beginPaint(window);

    if (auto w = dynamic_cast<AWindow*>(&window)) {
        glXMakeCurrent(ourDisplay, w->mHandle, ourContext);
    }
    mRenderer->beginPaint(window.getSize());
}

void OpenGLRenderingContext::endPaint(ABaseWindow& window) {
    CommonRenderingContext::endPaint(window);

    if (auto w = dynamic_cast<AWindow*>(&window)) {
        glXSwapBuffers(ourDisplay, w->mHandle);
    }
    mRenderer->endPaint();
}

void OpenGLRenderingContext::beginResize(ABaseWindow& window) {
    if (auto w = dynamic_cast<AWindow*>(&window)) {
        glXMakeCurrent(ourDisplay, w->mHandle, ourContext);
    }
}

void OpenGLRenderingContext::endResize(ABaseWindow& window) {

}

AImage OpenGLRenderingContext::makeScreenshot() {
    return AImage();
}
