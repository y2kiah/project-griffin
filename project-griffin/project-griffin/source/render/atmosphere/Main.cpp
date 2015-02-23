/**
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Author: Eric Bruneton
 */

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>

#include <GL/glew.h>
#include <glut.h>

#include "tiffio.h"

#include "vec3.h"
#include "mat4.h"
#include "Main.h"

using namespace std;

// ----------------------------------------------------------------------------
// TOOLS
// ----------------------------------------------------------------------------

void loadTIFF(char *name, unsigned char *tex)
{
    tstrip_t strip = 0;
    tsize_t off = 0;
    tsize_t n = 0;
    TIFF* tf = TIFFOpen(name, "r");
    while ((n = TIFFReadEncodedStrip(tf, strip, tex + off, (tsize_t) -1)) > 0) {
    	strip += 1;
        off += n;
    };
    TIFFClose(tf);
}

string* loadFile(const string &fileName)
{
    string* result = new string();
    ifstream file(fileName.c_str());
    if (!file) {
        std::cerr << "Cannot open file " << fileName << endl;
        throw exception();
    }
    string line;
    while (getline(file, line)) {
        *result += line;
        *result += '\n';
    }
    file.close();
    return result;
}

void printShaderLog(int shaderId)
{
    int logLength;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        char *log = new char[logLength];
        glGetShaderInfoLog(shaderId, logLength, &logLength, log);
        cout << string(log);
        delete[] log;
    }
}

unsigned int loadProgram(const vector<string> &files)
{
    unsigned int programId = glCreateProgram();
    unsigned int vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    int n = files.size();
    string **strs = new string*[n];
    const char** lines = new const char*[n + 1];
    cout << "loading program " << files[n - 1] << "..." << endl;
    bool geo = false;
    for (int i = 0; i < n; ++i) {
        string* s = loadFile(files[i]);
        strs[i] = s;
        lines[i + 1] = s->c_str();
        if (strstr(lines[i + 1], "_GEOMETRY_") != NULL) {
            geo = true;
        }
    }

    lines[0] = "#define _VERTEX_\n";
    glShaderSource(vertexShaderId, n + 1, lines, NULL);
    glCompileShader(vertexShaderId);
    printShaderLog(vertexShaderId);

    if (geo) {
        unsigned geometryShaderId = glCreateShader(GL_GEOMETRY_SHADER_EXT);
        glAttachShader(programId, geometryShaderId);
        lines[0] = "#define _GEOMETRY_\n";
        glShaderSource(geometryShaderId, n + 1, lines, NULL);
        glCompileShader(geometryShaderId);
        printShaderLog(geometryShaderId);
        glProgramParameteriEXT(programId, GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
        glProgramParameteriEXT(programId, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
        glProgramParameteriEXT(programId, GL_GEOMETRY_VERTICES_OUT_EXT, 3);
    }

    lines[0] = "#define _FRAGMENT_\n";
    glShaderSource(fragmentShaderId, n + 1, lines, NULL);
    glCompileShader(fragmentShaderId);
    printShaderLog(fragmentShaderId);

    glLinkProgram(programId);

    for (int i = 0; i < n; ++i) {
        delete strs[i];
    }
    delete[] strs;
    delete[] lines;

    return programId;
}

void drawQuad()
{
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(-1.0, -1.0);
    glVertex2f(+1.0, -1.0);
    glVertex2f(-1.0, +1.0);
    glVertex2f(+1.0, +1.0);
    glEnd();
}

// ----------------------------------------------------------------------------
// PRECOMPUTATIONS
// ----------------------------------------------------------------------------

const int reflectanceUnit = 0;
const int transmittanceUnit = 1;
const int irradianceUnit = 2;
const int inscatterUnit = 3;
const int deltaEUnit = 4;
const int deltaSRUnit = 5;
const int deltaSMUnit = 6;
const int deltaJUnit = 7;

unsigned int reflectanceTexture;//unit 0, ground reflectance texture
unsigned int transmittanceTexture;//unit 1, T table
unsigned int irradianceTexture;//unit 2, E table
unsigned int inscatterTexture;//unit 3, S table
unsigned int deltaETexture;//unit 4, deltaE table
unsigned int deltaSRTexture;//unit 5, deltaS table (Rayleigh part)
unsigned int deltaSMTexture;//unit 6, deltaS table (Mie part)
unsigned int deltaJTexture;//unit 7, deltaJ table

unsigned int transmittanceProg;
unsigned int irradiance1Prog;
unsigned int inscatter1Prog;
unsigned int copyIrradianceProg;
unsigned int copyInscatter1Prog;
unsigned int jProg;
unsigned int irradianceNProg;
unsigned int inscatterNProg;
unsigned int copyInscatterNProg;

unsigned int fbo;

unsigned int drawProg;

void setLayer(unsigned int prog, int layer)
{
    double r = layer / (RES_R - 1.0);
    r = r * r;
    r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
    double dmin = Rt - r;
    double dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
    double dminp = r - Rg;
    double dmaxp = sqrt(r * r - Rg * Rg);
    glUniform1f(glGetUniformLocation(prog, "r"), float(r));
    glUniform4f(glGetUniformLocation(prog, "dhdH"), float(dmin), float(dmax), float(dminp), float(dmaxp));
    glUniform1i(glGetUniformLocation(prog, "layer"), layer);
}

void loadData()
{
    cout << "loading Earth texture..." << endl;
    glActiveTexture(GL_TEXTURE0 + reflectanceUnit);
    glGenTextures(1, &reflectanceTexture);
    glBindTexture(GL_TEXTURE_2D, reflectanceTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    unsigned char *tex = new unsigned char[2500 * 1250 * 4];
    loadTIFF("earth.tiff", tex);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2500, 1250, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    glGenerateMipmapEXT(GL_TEXTURE_2D);
    delete[] tex;
}

void precompute()
{
    glActiveTexture(GL_TEXTURE0 + transmittanceUnit);
    glGenTextures(1, &transmittanceTexture);
    glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, TRANSMITTANCE_W, TRANSMITTANCE_H, 0, GL_RGB, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + irradianceUnit);
    glGenTextures(1, &irradianceTexture);
    glBindTexture(GL_TEXTURE_2D, irradianceTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, SKY_W, SKY_H, 0, GL_RGB, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + inscatterUnit);
    glGenTextures(1, &inscatterTexture);
    glBindTexture(GL_TEXTURE_3D, inscatterTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F_ARB, RES_MU_S * RES_NU, RES_MU, RES_R, 0, GL_RGB, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + deltaEUnit);
    glGenTextures(1, &deltaETexture);
    glBindTexture(GL_TEXTURE_2D, deltaETexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, SKY_W, SKY_H, 0, GL_RGB, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + deltaSRUnit);
    glGenTextures(1, &deltaSRTexture);
    glBindTexture(GL_TEXTURE_3D, deltaSRTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB, RES_MU_S * RES_NU, RES_MU, RES_R, 0, GL_RGB, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + deltaSMUnit);
    glGenTextures(1, &deltaSMTexture);
    glBindTexture(GL_TEXTURE_3D, deltaSMTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB, RES_MU_S * RES_NU, RES_MU, RES_R, 0, GL_RGB, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + deltaJUnit);
    glGenTextures(1, &deltaJTexture);
    glBindTexture(GL_TEXTURE_3D, deltaJTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB, RES_MU_S * RES_NU, RES_MU, RES_R, 0, GL_RGB, GL_FLOAT, NULL);

    vector<string> files;
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("transmittance.glsl");
    transmittanceProg = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("irradiance1.glsl");
    irradiance1Prog = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("inscatter1.glsl");
    inscatter1Prog = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("copyIrradiance.glsl");
    copyIrradianceProg = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("copyInscatter1.glsl");
    copyInscatter1Prog = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("inscatterS.glsl");
    jProg = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("irradianceN.glsl");
    irradianceNProg = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("inscatterN.glsl");
    inscatterNProg = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("copyInscatterN.glsl");
    copyInscatterNProg = loadProgram(files);

    files.clear();
    files.push_back("Main.h");
    files.push_back("common.glsl");
    files.push_back("earth.glsl");
    drawProg = loadProgram(files);
    glUseProgram(drawProg);
    glUniform1i(glGetUniformLocation(drawProg, "reflectanceSampler"), reflectanceUnit);
    glUniform1i(glGetUniformLocation(drawProg, "transmittanceSampler"), transmittanceUnit);
    glUniform1i(glGetUniformLocation(drawProg, "irradianceSampler"), irradianceUnit);
    glUniform1i(glGetUniformLocation(drawProg, "inscatterSampler"), inscatterUnit);

    cout << "precomputations..." << endl;

    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

    // computes transmittance texture T (line 1 in algorithm 4.1)
    glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, transmittanceTexture, 0);
    glViewport(0, 0, TRANSMITTANCE_W, TRANSMITTANCE_H);
    glUseProgram(transmittanceProg);
    drawQuad();

    // computes irradiance texture deltaE (line 2 in algorithm 4.1)
    glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaETexture, 0);
    glViewport(0, 0, SKY_W, SKY_H);
    glUseProgram(irradiance1Prog);
    glUniform1i(glGetUniformLocation(irradiance1Prog, "transmittanceSampler"), transmittanceUnit);
    drawQuad();

    // computes single scattering texture deltaS (line 3 in algorithm 4.1)
    // Rayleigh and Mie separated in deltaSR + deltaSM
    glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaSRTexture, 0);
    glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, deltaSMTexture, 0);
    unsigned int bufs[2] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
    glDrawBuffers(2, bufs);
    glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
    glUseProgram(inscatter1Prog);
    glUniform1i(glGetUniformLocation(inscatter1Prog, "transmittanceSampler"), transmittanceUnit);
    for (int layer = 0; layer < RES_R; ++layer) {
        setLayer(inscatter1Prog, layer);
        drawQuad();
    }
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, 0, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

    // copies deltaE into irradiance texture E (line 4 in algorithm 4.1)
    glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, irradianceTexture, 0);
    glViewport(0, 0, SKY_W, SKY_H);
    glUseProgram(copyIrradianceProg);
    glUniform1f(glGetUniformLocation(copyIrradianceProg, "k"), 0.0);
    glUniform1i(glGetUniformLocation(copyIrradianceProg, "deltaESampler"), deltaEUnit);
    drawQuad();

    // copies deltaS into inscatter texture S (line 5 in algorithm 4.1)
    glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, inscatterTexture, 0);
    glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
    glUseProgram(copyInscatter1Prog);
    glUniform1i(glGetUniformLocation(copyInscatter1Prog, "deltaSRSampler"), deltaSRUnit);
    glUniform1i(glGetUniformLocation(copyInscatter1Prog, "deltaSMSampler"), deltaSMUnit);
    for (int layer = 0; layer < RES_R; ++layer) {
        setLayer(copyInscatter1Prog, layer);
        drawQuad();
    }

    // loop for each scattering order (line 6 in algorithm 4.1)
    for (int order = 2; order <= 4; ++order) {

        // computes deltaJ (line 7 in algorithm 4.1)
        glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaJTexture, 0);
        glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
        glUseProgram(jProg);
        glUniform1f(glGetUniformLocation(jProg, "first"), order == 2 ? 1.0 : 0.0);
        glUniform1i(glGetUniformLocation(jProg, "transmittanceSampler"), transmittanceUnit);
        glUniform1i(glGetUniformLocation(jProg, "deltaESampler"), deltaEUnit);
        glUniform1i(glGetUniformLocation(jProg, "deltaSRSampler"), deltaSRUnit);
        glUniform1i(glGetUniformLocation(jProg, "deltaSMSampler"), deltaSMUnit);
        for (int layer = 0; layer < RES_R; ++layer) {
            setLayer(jProg, layer);
            drawQuad();
        }

        // computes deltaE (line 8 in algorithm 4.1)
        glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaETexture, 0);
        glViewport(0, 0, SKY_W, SKY_H);
        glUseProgram(irradianceNProg);
        glUniform1f(glGetUniformLocation(irradianceNProg, "first"), order == 2 ? 1.0 : 0.0);
        glUniform1i(glGetUniformLocation(irradianceNProg, "transmittanceSampler"), transmittanceUnit);
        glUniform1i(glGetUniformLocation(irradianceNProg, "deltaSRSampler"), deltaSRUnit);
        glUniform1i(glGetUniformLocation(irradianceNProg, "deltaSMSampler"), deltaSMUnit);
        drawQuad();

        // computes deltaS (line 9 in algorithm 4.1)
        glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, deltaSRTexture, 0);
        glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
        glUseProgram(inscatterNProg);
        glUniform1f(glGetUniformLocation(inscatterNProg, "first"), order == 2 ? 1.0 : 0.0);
        glUniform1i(glGetUniformLocation(inscatterNProg, "transmittanceSampler"), transmittanceUnit);
        glUniform1i(glGetUniformLocation(inscatterNProg, "deltaJSampler"), deltaJUnit);
        for (int layer = 0; layer < RES_R; ++layer) {
            setLayer(inscatterNProg, layer);
            drawQuad();
        }

        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

        // adds deltaE into irradiance texture E (line 10 in algorithm 4.1)
        glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, irradianceTexture, 0);
        glViewport(0, 0, SKY_W, SKY_H);
        glUseProgram(copyIrradianceProg);
        glUniform1f(glGetUniformLocation(copyIrradianceProg, "k"), 1.0);
        glUniform1i(glGetUniformLocation(copyIrradianceProg, "deltaESampler"), deltaEUnit);
        drawQuad();

        // adds deltaS into inscatter texture S (line 11 in algorithm 4.1)
        glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, inscatterTexture, 0);
        glViewport(0, 0, RES_MU_S * RES_NU, RES_MU);
        glUseProgram(copyInscatterNProg);
        glUniform1i(glGetUniformLocation(copyInscatterNProg, "deltaSSampler"), deltaSRUnit);
        for (int layer = 0; layer < RES_R; ++layer) {
            setLayer(copyInscatterNProg, layer);
            drawQuad();
        }

        glDisable(GL_BLEND);
    }

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    glFinish();
    cout << "ready." << endl;
    glUseProgram(drawProg);
}

void recompute()
{
    glDeleteTextures(1, &transmittanceTexture);
    glDeleteTextures(1, &irradianceTexture);
    glDeleteTextures(1, &inscatterTexture);
    glDeleteTextures(1, &deltaETexture);
    glDeleteTextures(1, &deltaSRTexture);
    glDeleteTextures(1, &deltaSMTexture);
    glDeleteTextures(1, &deltaJTexture);
    glDeleteProgram(transmittanceProg);
    glDeleteProgram(irradiance1Prog);
    glDeleteProgram(inscatter1Prog);
    glDeleteProgram(copyIrradianceProg);
    glDeleteProgram(copyInscatter1Prog);
    glDeleteProgram(jProg);
    glDeleteProgram(irradianceNProg);
    glDeleteProgram(inscatterNProg);
    glDeleteProgram(copyInscatterNProg);
    glDeleteProgram(drawProg);
    glDeleteFramebuffersEXT(1, &fbo);
    precompute();
}

// ----------------------------------------------------------------------------
// RENDERING
// ----------------------------------------------------------------------------

int width, height;
int oldx, oldy;
int imove;

vec3f s(0.0, -1.0, 0.0);

double lon = 0.0;
double lat = 0.0;
double theta = 0.0;
double phi = 0.0;
double d = Rg;
vec3d position;
mat4d view;

double exposure = 0.4;

void updateView()
{
	double co = cos(lon);
	double so = sin(lon);
	double ca = cos(lat);
	double sa = sin(lat);
	vec3d po = vec3d(co*ca, so*ca, sa) * Rg;
	vec3d px = vec3d(-so, co, 0);
    vec3d py = vec3d(-co*sa, -so*sa, ca);
    vec3d pz = vec3d(co*ca, so*ca, sa);

    double ct = cos(theta);
    double st = sin(theta);
    double cp = cos(phi);
    double sp = sin(phi);
    vec3d cx = px * cp + py * sp;
    vec3d cy = -px * sp*ct + py * cp*ct + pz * st;
    vec3d cz = px * sp*st - py * cp*st + pz * ct;
    position = po + cz * d;

    if (position.length() < Rg + 0.01) {
    	position.normalize(Rg + 0.01);
    }

    view = mat4d(cx.x, cx.y, cx.z, 0,
            cy.x, cy.y, cy.z, 0,
            cz.x, cz.y, cz.z, 0,
            0, 0, 0, 1);
    view = view * mat4d::translate(-position);
}

void redisplayFunc()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float h = position.length() - Rg;
    float vfov = 2.0 * atan(float(height) / float(width) * tan(80.0 / 180 * M_PI / 2.0)) / M_PI * 180;
    mat4f proj = mat4f::perspectiveProjection(vfov, float(width) / float(height), 0.1 * h, 1e5 * h);

    mat4f iproj = proj.inverse();
    mat4d iview = view.inverse();
    vec3d c = iview * vec3d(0.0, 0.0, 0.0);

    mat4f iviewf = mat4f(iview[0][0], iview[0][1], iview[0][2], iview[0][3],
        iview[1][0], iview[1][1], iview[1][2], iview[1][3],
        iview[2][0], iview[2][1], iview[2][2], iview[2][3],
        iview[3][0], iview[3][1], iview[3][2], iview[3][3]);

    glUniform3f(glGetUniformLocation(drawProg, "c"), c.x, c.y, c.z);
    glUniform3f(glGetUniformLocation(drawProg, "s"), s.x, s.y, s.z);
    glUniformMatrix4fv(glGetUniformLocation(drawProg, "projInverse"), 1, true, iproj.coefficients());
    glUniformMatrix4fv(glGetUniformLocation(drawProg, "viewInverse"), 1, true, iviewf.coefficients());
    glUniform1f(glGetUniformLocation(drawProg, "exposure"), exposure);
    drawQuad();

    glutSwapBuffers();
}

// ----------------------------------------------------------------------------
// USER INTERFACE
// ----------------------------------------------------------------------------

void reshapeFunc(int x, int y)
{
    width = x;
    height = y;
    glViewport(0, 0, x, y);
    glutPostRedisplay();
}

void mouseClickFunc(int button, int state, int x, int y)
{
    oldx = x;
    oldy = y;
    int modifiers = glutGetModifiers();
    bool ctrl = (modifiers & GLUT_ACTIVE_CTRL) != 0;
    bool shift = (modifiers & GLUT_ACTIVE_SHIFT) != 0;
    if (ctrl) {
    	imove = 0;
    } else if (shift) {
        imove = 1;
    } else {
    	imove = 2;
    }
}

void mouseMotionFunc(int x, int y)
{
    if (imove == 0) {
    	phi += (oldx - x) / 500.0;
    	theta += (oldy - y) / 500.0;
		theta = std::max(0.0, std::min(M_PI, theta));
        updateView();
        oldx = x;
        oldy = y;
    } else if (imove == 1) {
    	double factor = position.length() - Rg;
    	factor = factor / Rg;
    	lon += (oldx - x) / 400.0 * factor;
    	lat -= (oldy - y) / 400.0 * factor;
        lat = std::max(-M_PI / 2.0, std::min(M_PI / 2.0, lat));
        updateView();
        oldx = x;
        oldy = y;
    } else if (imove == 2) {
    	float vangle = asin(s.z);
    	float hangle = atan2(s.y, s.x);
    	vangle += (oldy - y) / 180.0 * M_PI / 4;
    	hangle += (oldx - x) / 180.0 * M_PI / 4;
    	s.x = cos(vangle) * cos(hangle);
    	s.y = cos(vangle) * sin(hangle);
    	s.z = sin(vangle);
        oldx = x;
        oldy = y;
    }
}

void specialKeyFunc(int c, int x, int y)
{
    switch (c) {
    case GLUT_KEY_PAGE_UP:
    	d = d * 1.05;
        updateView();
        break;
    case GLUT_KEY_PAGE_DOWN:
    	d = d / 1.05;
        updateView();
        break;
    case GLUT_KEY_F5:
	    recompute();
        glViewport(0, 0, width, height);
        break;
    }
}

void keyboardFunc(unsigned char c, int x, int y)
{
    if (c == 27) {
        ::exit(0);
    } else if (c == '+') {
		exposure *= 1.1;
	} else if (c == '-') {
		exposure /= 1.1;
	}
}

void idleFunc()
{
    glutPostRedisplay();
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1778, 1000);
    glutCreateWindow("Precomputed Atmospheric Scattering");
    glutCreateMenu(NULL);
    glutDisplayFunc(redisplayFunc);
    glutReshapeFunc(reshapeFunc);
    glutMouseFunc(mouseClickFunc);
    glutMotionFunc(mouseMotionFunc);
    glutSpecialFunc(specialKeyFunc);
    glutKeyboardFunc(keyboardFunc);
    glutIdleFunc(idleFunc);
    glewInit();

    loadData();
    precompute();
    updateView();
    glutMainLoop();
}
