//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2023 LucKey Productions.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../../Precompiled.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/ShaderProgram.h"
#include "../../Graphics/ShaderVariation.h"
#include "../../IO/Log.h"

#include "../../DebugNew.h"

namespace Dry
{

const char* ShaderVariation::elementSemanticNames[] =
{
    "POS",
    "NORMAL",
    "BINORMAL",
    "TANGENT",
    "TEXCOORD",
    "COLOR",
    "BLENDWEIGHT",
    "BLENDINDICES",
    "OBJECTINDEX"
};

void ShaderVariation::OnDeviceLost()
{
    if (objectName_ && !graphics_->IsDeviceLost())
        glDeleteShader(objectName_);

    GPUObject::OnDeviceLost();

    compilerOutput_.Clear();
}

void ShaderVariation::Release()
{
    if (objectName_)
    {
        if (!graphics_)
            return;

        if (!graphics_->IsDeviceLost())
        {
            if (type_ == VS)
            {
                if (graphics_->GetVertexShader() == this)
                    graphics_->SetShaders(nullptr, nullptr);
            }
            else
            {
                if (graphics_->GetPixelShader() == this)
                    graphics_->SetShaders(nullptr, nullptr);
            }

            glDeleteShader(objectName_);
        }

        objectName_ = 0;
        graphics_->CleanupShaderPrograms(this);
    }

    compilerOutput_.Clear();
}

bool ShaderVariation::Create()
{
    Release();

    if (!owner_)
    {
        compilerOutput_ = "Owner shader has expired";
        return false;
    }

    objectName_ = glCreateShader(type_ == VS ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    if (!objectName_)
    {
        compilerOutput_ = "Could not create shader object";
        return false;
    }

    const String& originalShaderCode = owner_->GetSourceCode(type_);
    String shaderCode;

    // Check if the shader code contains a version define
    unsigned verStart = originalShaderCode.Find('#');
    unsigned verEnd = 0;
    if (verStart != String::NPOS)
    {
        if (originalShaderCode.Substring(verStart + 1, 7) == "version")
        {
            verEnd = verStart + 9;
            while (verEnd < originalShaderCode.Length())
            {
                if (IsDigit((unsigned)originalShaderCode[verEnd]))
                    ++verEnd;
                else
                    break;
            }
            // If version define found, insert it first
            String versionDefine = originalShaderCode.Substring(verStart, verEnd - verStart);
            shaderCode += versionDefine + "\n";
        }
    }
    // Force GLSL version 150 if no version define and GL3 is being used
    if (!verEnd && Graphics::GetGL3Support())
        shaderCode += "#version 150\n";

    // Distinguish between VS and PS compile in case the shader code wants to include/omit different things
    shaderCode += type_ == VS ? "#define COMPILEVS\n" : "#define COMPILEPS\n";

    // Add define for the maximum number of supported bones
    shaderCode += "#define MAXBONES " + String(Graphics::GetMaxBones()) + "\n";

    // Prepend the defines to the shader code
    Vector<String> defineVec = defines_.Split(' ');
    for (unsigned i{ 0 }; i < defineVec.Size(); ++i)
    {
        // Add extra space for the checking code below
        String defineString = "#define " + defineVec[i].Replaced('=', ' ') + " \n";
        shaderCode += defineString;

        // In debug mode, check that all defines are referenced by the shader code
#ifdef _DEBUG
        String defineCheck = defineString.Substring(8, defineString.Find(' ', 8) - 8);
        if (originalShaderCode.Find(defineCheck) == String::NPOS)
            DRY_LOGWARNING("Shader " + GetFullName() + " does not use the define " + defineCheck);
#endif
    }

#ifdef RPI
    if (type_ == VS)
        shaderCode += "#define RPI\n";
#endif
#ifdef __EMSCRIPTEN__
    shaderCode += "#define WEBGL\n";
#endif
    if (Graphics::GetGL3Support())
        shaderCode += "#define GL3\n";

    // When version define found, do not insert it a second time
    if (verEnd > 0)
        shaderCode += (originalShaderCode.CString() + verEnd);
    else
        shaderCode += originalShaderCode;

    const char* shaderCStr = shaderCode.CString();
    glShaderSource(objectName_, 1, &shaderCStr, nullptr);
    glCompileShader(objectName_);

    int compiled, length;
    glGetShaderiv(objectName_, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        glGetShaderiv(objectName_, GL_INFO_LOG_LENGTH, &length);
        compilerOutput_.Resize((unsigned)length);
        int outLength;
        glGetShaderInfoLog(objectName_, length, &outLength, &compilerOutput_[0]);
        glDeleteShader(objectName_);
        objectName_ = 0;
    }
    else
        compilerOutput_.Clear();

    return objectName_ != 0;
}

void ShaderVariation::SetDefines(const String& defines)
{
    defines_ = defines;
}

// These methods are no-ops for OpenGL
bool ShaderVariation::LoadByteCode(const String& binaryShaderName) { return false; }
bool ShaderVariation::Compile() { return false; }
void ShaderVariation::ParseParameters(unsigned char* bufData, unsigned bufSize) {}
void ShaderVariation::SaveByteCode(const String& binaryShaderName) {}
void ShaderVariation::CalculateConstantBufferSizes() {}

}
