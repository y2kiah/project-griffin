#pragma once
#ifndef RENDER_H
#define RENDER_H

#include <string>

void initRenderData();
void renderFrame();
unsigned int loadShaders(std::string vertexFilePath, std::string fragmentFilePath);

#endif