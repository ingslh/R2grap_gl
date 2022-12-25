#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <camera.h>
#include <shader_g.h>
#include "JsonReader.h"
#include "RenderContent.h"
#include "AniInfoManager.h"
#include "PathRenderData.h"

#include <iostream>
using namespace R2grap;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

Camera camera(glm::vec3(0.0f, 0.0f, 0.9f));

// timing
float input_deltaTime = 0.0f;	// time between current frame and last frame

int main()
{
  JsonReader reader("../assets/designers.json");
  unsigned int SCR_WIDTH = AniInfoManager::GetIns().GetWidth();
  unsigned int SCR_HEIGHT = AniInfoManager::GetIns().GetHeight();

  // glfw: initialize and configure
  // ------------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // glfw window creation
  // --------------------
  GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "R2grap", NULL, NULL);
  if (window == nullptr)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //disable the mouse
  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // build and compile shaders
  // -------------------------
  Shader shader("../src/shader/r2grap_m.vert", "../src/shader/r2grap_m.frag");

  // set up vertex data (and buffer(s)) and configure vertex attributes
  // ------------------------------------------------------------------

  auto layers_count = reader.getLayersCount();

  std::vector<std::shared_ptr<RenderContent>> contents;
  for (auto i = 0; i < layers_count; i++) {
    auto layer_info = reader.GetLayersInfo(i).get();
    contents.emplace_back(std::make_shared<RenderContent>(layer_info));
  }
	std::vector<RePathObj> objs;
	RenderContent::UpdateTransRenderData(contents,objs);

  auto paths_count = static_cast<int>(objs.size());

  unsigned int* VBOs = new unsigned int[paths_count];
  unsigned int* VAOs = new unsigned int[paths_count];
  unsigned int* EBOs = new unsigned int[paths_count];
  glGenBuffers(paths_count, VBOs);
  glGenBuffers(paths_count, EBOs);
  glGenVertexArrays(paths_count, VAOs);

	for(auto ind = 0; ind < objs.size() ; ind++){

		auto path_data = objs[ind].path;
		if(!path_data->has_keyframe){
			auto vert_array = path_data->verts;
			auto out_vert = new float[vert_array.size()];
			memcpy(out_vert, &vert_array[0], sizeof(float) * vert_array.size());

			glBindVertexArray(VAOs[ind]);
			glBindBuffer(GL_ARRAY_BUFFER, VBOs[ind]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vert_array.size(), out_vert, GL_STATIC_DRAW);

			if(path_data->closed){
				auto tri_array = path_data->tri_ind;
				auto out_tri = new unsigned int[tri_array.size()];
				memcpy(out_tri, &tri_array[0], tri_array.size() * sizeof(tri_array[0]));

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[ind]);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * tri_array.size(), out_tri, GL_STATIC_DRAW);
			}
		}
		else{
			auto max_verts_size = path_data->GetMaxVectorSize(PathData::PathVecContentType::t_Vertices);
			glBindVertexArray(VAOs[ind]);
			glBindBuffer(GL_ARRAY_BUFFER, VBOs[ind]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * max_verts_size, NULL, GL_DYNAMIC_DRAW);

			if(path_data->closed){
				auto max_tri_size = path_data->GetMaxVectorSize(PathData::PathVecContentType::t_TriangleIndex);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[ind]);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*max_tri_size, NULL, GL_DYNAMIC_DRAW);
			}
		}
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(0);
	}

  glfwSwapInterval(1);// open the vertical synchronization

  shader.use();

  static double limitFPS = 1.0 / AniInfoManager::GetIns().GetFrameRate();
  double lastTime = glfwGetTime(), timer = lastTime;
  double deltaTime = 0, nowTime = 0;
  int frames = 0, played = 0;

  auto frame_count = AniInfoManager::GetIns().GetDuration() * AniInfoManager::GetIns().GetFrameRate();

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window))
  {
    // - Measure time
    nowTime = glfwGetTime();
    input_deltaTime = nowTime - lastTime;
    deltaTime += input_deltaTime / limitFPS;
    lastTime = nowTime;

    //Input
    processInput(window);
    //Render
    while (deltaTime >= 1.0) { // render
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
      glm::mat4 view = camera.GetViewMatrix();
      glm::mat4 model = glm::mat4(1.0f);
      shader.setMat4("projection", projection);
      shader.setMat4("view", view);
      shader.setMat4("model", model);

			for(auto ind = 0; ind < objs.size(); ind++){
				auto obj = objs[ind];
				if(obj.in_pos > static_cast<float>(played) || obj.out_pos < static_cast<float>(played)) continue;
				if(!obj.keep_trans){
					shader.setMat4("transform",obj.trans[played]);
				}
				if(obj.fill){
					if(obj.fill->trans_color.empty())
						shader.setVec4("color", obj.fill->color);
					else
						shader.setVec4("color", obj.fill->trans_color[played]);
				}

				if(obj.stroke){
					if(obj.stroke->trans_color.empty())
						shader.setVec4("color", obj.stroke->color);
					else
						shader.setVec4("color",obj.stroke->trans_color[played]);
				}
				glBindVertexArray(VAOs[ind]);
				auto path = obj.path;
				if(!obj.path->has_keyframe){
					if(path->closed)
						glDrawElements(GL_TRIANGLES, path->verts.size(), GL_UNSIGNED_INT, 0);
					else
						glDrawArrays(GL_LINE_STRIP, 0, path->verts.size() / 3);
					glBindVertexArray(0);
				}else{
					auto vert_vec = path->trans_verts[played];
					if (!vert_vec.size()) continue;
					auto out_vert = new float[vert_vec.size()];
					memcpy(out_vert, &vert_vec[0], sizeof(float) * vert_vec.size());
					glBindBuffer(GL_ARRAY_BUFFER, VBOs[ind]);
					glBufferSubData(GL_ARRAY_BUFFER, 0 , sizeof(float) * vert_vec.size(), out_vert);
					if(path->closed){
						auto trig_vec = path->trans_tri_ind[played];
						auto out_trig = new unsigned int[trig_vec.size()];
						memcpy(out_trig, &trig_vec[0], sizeof(unsigned int) * trig_vec.size());
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[ind]);
						glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned int) * trig_vec.size(), out_trig);
						glDrawElements(GL_TRIANGLES, path->trans_verts[played].size(), GL_UNSIGNED_INT, 0);
						delete[] out_trig;
					}else
						glDrawArrays(GL_LINE_STRIP, 0, path->trans_verts[played].size() / 3);
					delete[] out_vert;
					glBindVertexArray(0);
				}
			}
      played = played > frame_count ? 0 : ++played;
      frames++;
      deltaTime--;
			glfwSwapBuffers(window);
      glfwPollEvents();
    }

    // - Reset after one second
    if (glfwGetTime() - timer > 1.0) {
      timer++;
      std::cout << "FPS: " << frames << std::endl;
      frames = 0;
    }
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
  }

  // optional: de-allocate all resources once they've outlived their purpose:
  // ------------------------------------------------------------------------
  glDeleteVertexArrays(paths_count, VAOs);
  glDeleteBuffers(paths_count, VBOs);
  glDeleteBuffers(paths_count, EBOs);

  glfwTerminate();
  return 0;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  // make sure the viewport matches the new window dimensions; note that width and 
  // height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
   if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, input_deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, input_deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, input_deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, input_deltaTime);
}