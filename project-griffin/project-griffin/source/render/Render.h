#pragma once
#ifndef _RENDER_H
#define _RENDER_H

#include <string>

void initRenderData();
void renderFrame();
unsigned int loadShaders(std::string vertexFilePath, std::string fragmentFilePath);

#endif