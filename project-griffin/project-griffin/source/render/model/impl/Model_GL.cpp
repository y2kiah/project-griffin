/**
* @file Model_GL.cpp
* @author Jeff Kiah
*/
#include <render/model/Model_GL.h>
#include <application/Engine.h>
//#include <GL/glew.h>
//#include <utility>
//#include <cassert>
//#include <render/ShaderProgramLayouts_GL.h>
#include <render/Render.h>
//#include <render/RenderResources.h>
//#include <resource/ResourceLoader.h>
//#include <utility/container/vector_queue.h>
//#include <glm/mat4x4.hpp>
//#include <glm/gtc/matrix_transform.hpp>
#include <scene/Scene.h>


namespace griffin {
	namespace render {

		void Model_GL::render(Id_T entityId, scene::Scene& scene, uint8_t viewport, Engine& engine)
		{
			//m_mesh.render(engine, viewport );
		}

		void Model_GL::draw(Id_T entityId, int drawSetIndex)
		{
			m_mesh.drawMesh(drawSetIndex);
		}

	}
}