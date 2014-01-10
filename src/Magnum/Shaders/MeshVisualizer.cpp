/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "MeshVisualizer.h"

#include <Corrade/Utility/Resource.h>

#include "Context.h"
#include "Extensions.h"
#include "Shader.h"

namespace Magnum { namespace Shaders {

MeshVisualizer::MeshVisualizer(const Flags flags): flags(flags), transformationProjectionMatrixUniform(0), viewportSizeUniform(1), colorUniform(2), wireframeColorUniform(3), wireframeWidthUniform(4), smoothnessUniform(5) {
    #ifndef MAGNUM_TARGET_GLES
    if(flags & Flag::Wireframe && !(flags & Flag::NoGeometryShader)) {
        MAGNUM_ASSERT_VERSION_SUPPORTED(Version::GL320);
        MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::ARB::geometry_shader4);
    }
    #elif defined(MAGNUM_TARGET_GLES2)
    if(flags & Flag::Wireframe)
        MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::OES::standard_derivatives);
    #endif

    Utility::Resource rs("MagnumShaders");

    #ifndef MAGNUM_TARGET_GLES
    const Version version = Context::current()->supportedVersion({Version::GL320, Version::GL310, Version::GL300, Version::GL210});
    CORRADE_INTERNAL_ASSERT_OUTPUT(flags & Flag::NoGeometryShader || version >= Version::GL320);
    #else
    const Version version = Context::current()->supportedVersion({Version::GLES300, Version::GLES200});
    #endif

    Shader vert(version, Shader::Type::Vertex);
    vert.addSource(flags & Flag::Wireframe ? "#define WIREFRAME_RENDERING\n" : "")
        .addSource(flags & Flag::NoGeometryShader ? "#define NO_GEOMETRY_SHADER\n" : "")
        .addSource(rs.get("compatibility.glsl"))
        .addSource(rs.get("generic.glsl"))
        .addSource(rs.get("MeshVisualizer.vert"));
    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
    vert.compile();
    attachShader(vert);

    #ifndef MAGNUM_TARGET_GLES
    if(flags & Flag::Wireframe && !(flags & Flag::NoGeometryShader)) {
        Shader geom(version, Shader::Type::Geometry);
        geom.addSource(rs.get("compatibility.glsl"))
            .addSource(rs.get("MeshVisualizer.geom"));
        CORRADE_INTERNAL_ASSERT_OUTPUT(geom.compile());
        geom.compile();
        attachShader(geom);
    }
    #endif

    Shader frag(version, Shader::Type::Fragment);
    frag.addSource(flags & Flag::Wireframe ? "#define WIREFRAME_RENDERING\n" : "")
        .addSource(flags & Flag::NoGeometryShader ? "#define NO_GEOMETRY_SHADER\n" : "")
        .addSource(rs.get("compatibility.glsl"))
        .addSource(rs.get("MeshVisualizer.frag"));
    CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());
    frag.compile();
    attachShader(frag);

    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current()->isExtensionSupported<Extensions::GL::ARB::explicit_attrib_location>(version))
    #else
    if(!Context::current()->isVersionSupported(Version::GLES300))
    #endif
    {
        bindAttributeLocation(Position::Location, "position");

        #ifndef MAGNUM_TARGET_GLES
        if(!Context::current()->isVersionSupported(Version::GL310))
        #endif
        {
            bindAttributeLocation(VertexIndex::Location, "vertexIndex");
        }
    }

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());
    link();

    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current()->isExtensionSupported<Extensions::GL::ARB::explicit_uniform_location>(version))
    #endif
    {
        transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
        colorUniform = uniformLocation("color");
        if(flags & Flag::Wireframe) {
            wireframeColorUniform = uniformLocation("wireframeColor");
            wireframeWidthUniform = uniformLocation("wireframeWidth");
            smoothnessUniform = uniformLocation("smoothness");
            if(!(flags & Flag::NoGeometryShader))
                viewportSizeUniform = uniformLocation("viewportSize");
        }
    }

    /* Set defaults in OpenGL ES (for desktop they are set in shader code itself) */
    #ifdef MAGNUM_TARGET_GLES
    setColor(Color3(1.0f));
    if(flags & Flag::Wireframe) {
        setWireframeColor(Color3(0.0f));
        setWireframeWidth(1.0f);
        setSmoothness(2.0f);
    }
    #endif
}

}}
