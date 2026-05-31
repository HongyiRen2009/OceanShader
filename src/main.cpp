#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include "camera.h"
#include "stb.h"
#include "renderutils.h"
#include <chrono>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <numeric>
using namespace std;
using namespace glm;
constexpr int kSpectrumResolution = 256;
constexpr int kSpectrumLog2Resolution = 8;
constexpr float kOceanSize = 64.0f;
constexpr float kWindSpeed = 20.0f;
constexpr float kFetch = 20000.0f;
const vec2 kOceanMeshSize = vec2(1000.0f, 1000.0f);
vec2 screenSize(800.0f, 600.0f);
float aspectRatio = screenSize.x / screenSize.y;
Camera* camera;
unsigned int shaderProgram;
unsigned int oceanVAO;
vector<unsigned int> oceanIndices = vector<unsigned int>();
unsigned int cubeVAO;
unsigned int cubeIndexCount;
unsigned int environmentCubemap;
unsigned int equirectangularProgram;
unsigned int spectrumCreationProgram;
unsigned int spectrumTexture;
unsigned int gaussianNoiseTexture;
unsigned int spectrumProgram;
unsigned int fftProgram;
unsigned int twiddleFactorsTexture;
unsigned int twiddleFactorGenerationProgram;
unsigned int timeEvaluationProgram;
unsigned int pingpongTexture[2];
unsigned int pingpongdxTexture[2];
unsigned int pingpongdyTexture[2];
unsigned int pingpongdisplacementxTexture[2];
unsigned int pingpongdisplacementyTexture[2];
unsigned int fftOutputIndex = 0;
unsigned int fftLinearSampler = 0;
unsigned int postProcessProgram = 0;
unsigned int postProcessFbo = 0;
unsigned int postProcessColorTexture = 0;
unsigned int postProcessDepthTexture = 0;
unsigned int postProcessVAO = 0;
unsigned int postProcessVBO = 0;
const double PI = 3.14159265358979323846;

const std::filesystem::path programDir = std::filesystem::path(__FILE__).parent_path();
void updateSpectrumPipeline(float time);
void createPostProcessResources();
void createPostProcessQuad();

void updateFPSCounter(GLFWwindow* window) {
	static auto lastFPSTime = chrono::high_resolution_clock::now();
	static int frames = 0;

	frames++;
	auto now = chrono::high_resolution_clock::now();
	double elapsed = chrono::duration<double>(now - lastFPSTime).count();
	if (elapsed >= 1.0) {
		double fps = frames / elapsed;
		string title = "Ocean - FPS: " + to_string(static_cast<int>(fps));
		glfwSetWindowTitle(window, title.c_str());
		frames = 0;
		lastFPSTime = now;
	}

}
void createCubeMesh() {
	float vertices[] = {
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f
	};
	unsigned int indices[] = {
		0, 1, 2, 2, 3, 0,
		1, 5, 6, 6, 2, 1,
		5, 4, 7, 7, 6, 5,
		4, 0, 3, 3, 7, 4,
		3, 2, 6, 6, 7, 3,
		4, 5, 1, 1, 0, 4
	};
	cubeIndexCount = sizeof(indices) / sizeof(indices[0]);
	std::vector<attribute> attribs = {
		{"position", 0, 3, 3 * sizeof(float), GL_FLOAT}
	};
	cubeVAO = createInterleavedVAO(
		vertices,
		sizeof(vertices),
		indices,
		cubeIndexCount,
		attribs,
		equirectangularProgram
	);
}

unsigned int loadHDRTexture(const std::string& path) {
	int width = 0;
	int height = 0;
	int channels = 0;
	float* data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
	if (!data) {
		cout << "Failed to load HDR texture: " << path << endl;
		return 0;
	}
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	GLenum format = channels == 4 ? GL_RGBA : GL_RGB;
	GLenum internalFormat = channels == 4 ? GL_RGBA32F : GL_RGB32F;
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(data);
	return texture;
}

unsigned int createEnvironmentCubemap(const std::string& hdrPath, int size) {
	unsigned int hdrTexture = loadHDRTexture(hdrPath);
	if (hdrTexture == 0) {
		return 0;
	}
	unsigned int captureFbo = 0;
	unsigned int captureRbo = 0;
	glGenFramebuffers(1, &captureFbo);
	glGenRenderbuffers(1, &captureRbo);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFbo);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRbo);

	unsigned int cubemap;
	glGenTextures(1, &cubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	mat4 captureProj = perspective(radians(90.0f), 1.0f, 0.1f, 10.0f);
	mat4 captureViews[] = {
		lookAt(vec3(0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),
		lookAt(vec3(0.0f), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),
		lookAt(vec3(0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
		lookAt(vec3(0.0f), vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)),
		lookAt(vec3(0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)),
		lookAt(vec3(0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f))
	};

	glUseProgram(equirectangularProgram);
	glUniformMatrix4fv(glGetUniformLocation(equirectangularProgram, "matProj"), 1, GL_FALSE, value_ptr(captureProj));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);
	glUniform1i(glGetUniformLocation(equirectangularProgram, "equirectangularMap"), 0);

	glViewport(0, 0, size, size);
	glBindVertexArray(cubeVAO);
	for (unsigned int i = 0; i < 6; ++i) {
		glUniformMatrix4fv(glGetUniformLocation(equirectangularProgram, "matView"), 1, GL_FALSE, value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cubeIndexCount), GL_UNSIGNED_INT, 0);
	}
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screenSize.x, screenSize.y);

	glDeleteFramebuffers(1, &captureFbo);
	glDeleteRenderbuffers(1, &captureRbo);
	glDeleteTextures(1, &hdrTexture);

	return cubemap;
}
void createSpectrumTextures() {
	if (spectrumTexture != 0) {
		glDeleteTextures(1, &spectrumTexture);
		spectrumTexture = 0;
	}
	spectrumTexture = createTexture2D(kSpectrumResolution, kSpectrumResolution, GL_RG32F, GL_RG, GL_FLOAT);
	string path = (programDir / "gaussian_noise.png").string();
	gaussianNoiseTexture = importTexture2D(path.c_str(), kSpectrumResolution, kSpectrumResolution, 2);
}
void createPingPongTextures() {
	for (int i = 0; i < 2; ++i) {
		if (pingpongTexture[i] != 0) {
			glDeleteTextures(1, &pingpongTexture[i]);
			pingpongTexture[i] = 0;
		}
		if (pingpongdxTexture[i] != 0) {
			glDeleteTextures(1, &pingpongdxTexture[i]);
			pingpongdxTexture[i] = 0;
		}
		if (pingpongdyTexture[i] != 0) {
			glDeleteTextures(1, &pingpongdyTexture[i]);
			pingpongdyTexture[i] = 0;
		}
		if (pingpongdisplacementxTexture[i] != 0) {
			glDeleteTextures(1, &pingpongdisplacementxTexture[i]);
			pingpongdisplacementxTexture[i] = 0;
		}
		if (pingpongdisplacementyTexture[i] != 0) {
			glDeleteTextures(1, &pingpongdisplacementyTexture[i]);
			pingpongdisplacementyTexture[i] = 0;
		}
		pingpongTexture[i] = createTexture2D(kSpectrumResolution, kSpectrumResolution, GL_RG32F, GL_RG, GL_FLOAT);
		pingpongdxTexture[i] = createTexture2D(kSpectrumResolution, kSpectrumResolution, GL_RG32F, GL_RG, GL_FLOAT);
		pingpongdyTexture[i] = createTexture2D(kSpectrumResolution, kSpectrumResolution, GL_RG32F, GL_RG, GL_FLOAT);
		pingpongdisplacementxTexture[i] = createTexture2D(kSpectrumResolution, kSpectrumResolution, GL_RG32F, GL_RG, GL_FLOAT);
		pingpongdisplacementyTexture[i] = createTexture2D(kSpectrumResolution, kSpectrumResolution, GL_RG32F, GL_RG, GL_FLOAT);
	}
}
void createTwiddleFactorsTexture(unsigned int twiddleGenerationProgram) {
	if (twiddleFactorsTexture != 0) {
		glDeleteTextures(1, &twiddleFactorsTexture);
		twiddleFactorsTexture = 0;
	}

	std::vector<glm::vec2> twiddles(kSpectrumResolution);
	twiddleFactorsTexture = createTexture2D(
		kSpectrumResolution,
		kSpectrumResolution,
		GL_RGBA32F,
		GL_RGBA,
		GL_FLOAT
	);
	glUseProgram(twiddleGenerationProgram);
	glUniform1i(glGetUniformLocation(twiddleGenerationProgram, "resolution"), kSpectrumResolution);
	glUniform1i(glGetUniformLocation(twiddleGenerationProgram, "log2Resolution"), kSpectrumLog2Resolution);
	glBindImageTexture(0, twiddleFactorsTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	const int groups = (kSpectrumResolution + 15) / 16;
	glDispatchCompute(kSpectrumLog2Resolution, groups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);

}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenSize = vec2(width, height);
	aspectRatio = float(width) / float(height);
	createPostProcessResources();
}

GLFWwindow* initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6); // version 4.6 of OpenGL
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	screenSize = vec2(mode->width, mode->height);
	aspectRatio = (float)mode->width / (float)mode->height;
	GLFWwindow* window = glfwCreateWindow(screenSize.x, screenSize.y, "Ocean", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return NULL;
	}

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return NULL;
	}
	glViewport(0, 0, screenSize.x, screenSize.y);
	glfwSwapInterval(0);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	return window;
}
void createOceanMesh(ivec2 size, int stepSize) {
	stepSize = std::max(stepSize, 1);

	int cellsX = std::max(size.x / stepSize, 1);
	int cellsZ = std::max(size.y / stepSize, 1);

	float halfX = size.x * 0.5f;
	float halfZ = size.y * 0.5f;

	vector<float> vertices;
	vertices.reserve((cellsX + 1) * (cellsZ + 1) * 3);

	for (int z = 0; z <= cellsZ; ++z) {
		for (int x = 0; x <= cellsX; ++x) {
			float xpos = float(x * stepSize) - halfX;
			float zpos = float(z * stepSize) - halfZ;
			vertices.push_back(xpos);
			vertices.push_back(0.0f);
			vertices.push_back(zpos);
		}
	}

	oceanIndices.clear();
	oceanIndices.reserve(cellsX * cellsZ * 4);

	for (int z = 0; z < cellsZ; ++z) {
		for (int x = 0; x < cellsX; ++x) {
			int i0 = z * (cellsX + 1) + x;
			int i1 = i0 + 1;
			int i2 = i0 + (cellsX + 1);
			int i3 = i2 + 1;

			// bottom-left, bottom-right, top-right, top-left
			oceanIndices.push_back(i0);
			oceanIndices.push_back(i1);
			oceanIndices.push_back(i3);
			oceanIndices.push_back(i2);
		}
	}

	std::vector<attribute> attribs = {
		{"position", 0, 3, 3 * sizeof(float), GL_FLOAT}
	};
	cout << "Created ocean mesh with " << vertices.size() / 3 << " vertices and " << oceanIndices.size() / 4 << " quads." << endl;
	oceanVAO = createInterleavedVAO(
		vertices.data(),
		vertices.size() * sizeof(float),
		oceanIndices.data(),
		oceanIndices.size(),
		attribs,
		shaderProgram
	);
}
void createPostProcessQuad() {
	if (postProcessVAO != 0) {
		glDeleteVertexArrays(1, &postProcessVAO);
		postProcessVAO = 0;
	}
	if (postProcessVBO != 0) {
		glDeleteBuffers(1, &postProcessVBO);
		postProcessVBO = 0;
	}

	float quadVertices[] = {
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f,
		-1.0f,  1.0f,  0.0f, 1.0f
	};

	glGenVertexArrays(1, &postProcessVAO);
	glGenBuffers(1, &postProcessVBO);
	glBindVertexArray(postProcessVAO);
	glBindBuffer(GL_ARRAY_BUFFER, postProcessVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void createPostProcessResources() {
	const int width = static_cast<int>(screenSize.x);
	const int height = static_cast<int>(screenSize.y);
	if (postProcessFbo == 0) {
		glGenFramebuffers(1, &postProcessFbo);
	}

	if (postProcessColorTexture != 0) {
		glDeleteTextures(1, &postProcessColorTexture);
		postProcessColorTexture = 0;
	}
	if (postProcessDepthTexture != 0) {
		glDeleteTextures(1, &postProcessDepthTexture);
		postProcessDepthTexture = 0;
	}

	postProcessColorTexture = createTexture2D(
		width,
		height,
		GL_RGBA16F,
		GL_RGBA,
		GL_FLOAT,
		GL_LINEAR,
		GL_LINEAR,
		GL_CLAMP_TO_EDGE,
		GL_CLAMP_TO_EDGE
	);
	postProcessDepthTexture = createDepthTexture(width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessColorTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, postProcessDepthTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		cout << "Post-process framebuffer is not complete." << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render() {
	updateSpectrumPipeline((float)glfwGetTime());
	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFbo);
	glViewport(0, 0, screenSize.x, screenSize.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(shaderProgram);
	mat4 view = camera->getViewMatrix();
	mat4 proj = camera->getProjectionMatrix(aspectRatio);
	vec3 cameraPos = camera->position;
	vec3 cameraDir = camera->front;
	vec3 lightDir = normalize(vec3(1.0, 1.0, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "matProj"), 1, GL_FALSE, value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "matView"), 1, GL_FALSE, value_ptr(view));
	glUniform3fv(glGetUniformLocation(shaderProgram, "cameraPos"), 1, value_ptr(cameraPos));
	glUniform3fv(glGetUniformLocation(shaderProgram, "cameraDir"), 1, value_ptr(cameraDir));
	glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, value_ptr(lightDir));
	glUniform1f(glGetUniformLocation(shaderProgram, "time"), (float)glfwGetTime());
	glUniform2fv(glGetUniformLocation(shaderProgram, "oceanMeshSize"), 1, value_ptr(kOceanMeshSize));
	glUniform1f(glGetUniformLocation(shaderProgram, "meshResolution"), 15.0);
	glUniform1f(glGetUniformLocation(shaderProgram, "displacementScale"), 1.0);
	glUniform1f(glGetUniformLocation(shaderProgram, "frequency"), kOceanMeshSize.x/10.0);
	glUniform1f(glGetUniformLocation(shaderProgram, "choppiness"), 1.5f);
	bindTexture(pingpongTexture[fftOutputIndex], "heightmap", 3);
	bindTexture(pingpongdxTexture[fftOutputIndex], "dxTexture", 4);
	bindTexture(pingpongdyTexture[fftOutputIndex], "dyTexture", 5);
	bindTexture(pingpongdisplacementxTexture[fftOutputIndex], "displacementxTexture", 6);
	bindTexture(pingpongdisplacementyTexture[fftOutputIndex], "displacementyTexture", 7);
	bindTexture(environmentCubemap, "environmentMap", 8, GL_TEXTURE_CUBE_MAP);
	glBindSampler(3, fftLinearSampler);
	glBindSampler(4, fftLinearSampler);
	glBindSampler(5, fftLinearSampler);
	glBindSampler(6, fftLinearSampler);
	glBindSampler(7, fftLinearSampler);
	glBindVertexArray(oceanVAO);
	glPatchParameteri(GL_PATCH_VERTICES, 4);
	glDrawElements(GL_PATCHES, static_cast<GLsizei>(oceanIndices.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glBindSampler(3, 0);
	glBindSampler(4, 0);
	glBindSampler(5, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screenSize.x, screenSize.y);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(postProcessProgram);
	mat4 invProj = inverse(proj);
	mat4 invView = inverse(view);
	glUniformMatrix4fv(glGetUniformLocation(postProcessProgram, "invProj"), 1, GL_FALSE, value_ptr(invProj));
	glUniformMatrix4fv(glGetUniformLocation(postProcessProgram, "invView"), 1, GL_FALSE, value_ptr(invView));
	bindTexture(postProcessColorTexture, "sceneColor", 0);
	bindTexture(postProcessDepthTexture, "sceneDepth", 1);
	bindTexture(environmentCubemap, "environmentMap", 2, GL_TEXTURE_CUBE_MAP);
	glBindVertexArray(postProcessVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
}

void generateSpectrum() {
	glUseProgram(spectrumProgram);
	glUniform1i(glGetUniformLocation(spectrumProgram, "resolution"), kSpectrumResolution);
	glUniform1f(glGetUniformLocation(spectrumProgram, "oceanSize"), kOceanSize);
	glUniform1f(glGetUniformLocation(spectrumProgram, "windSpeed"), kWindSpeed);
	glUniform1f(glGetUniformLocation(spectrumProgram, "fetch"), kFetch);
	glUniform2fv(glGetUniformLocation(spectrumProgram, "windDir"), 1, value_ptr(normalize(vec2(1.0f, 1.0f))));
	glBindImageTexture(0, spectrumTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gaussianNoiseTexture);
	glUniform1i(glGetUniformLocation(spectrumProgram, "gaussianNoise"), 0);
	const int groups = (kSpectrumResolution + 15) / 16;
	glDispatchCompute(groups, groups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}
void evaluateSpectrumAtTime(float time) {
	glUseProgram(timeEvaluationProgram);
	glUniform1f(glGetUniformLocation(timeEvaluationProgram, "time"), time);
	glUniform1i(glGetUniformLocation(timeEvaluationProgram, "resolution"), kSpectrumResolution);
	glUniform1i(glGetUniformLocation(timeEvaluationProgram, "oceanSize"), static_cast<int>(kOceanSize));
	glUniform1f(glGetUniformLocation(timeEvaluationProgram, "speed"), 4.0);
	glBindImageTexture(0, spectrumTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
	glBindImageTexture(1, pingpongTexture[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
	glBindImageTexture(2, pingpongdxTexture[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
	glBindImageTexture(3, pingpongdyTexture[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
	glBindImageTexture(4, pingpongdisplacementxTexture[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
	glBindImageTexture(5, pingpongdisplacementyTexture[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
	const int groups = (kSpectrumResolution + 15) / 16;
	glDispatchCompute(groups, groups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}
void runFFTPasses(bool horizontal, bool& pingPong, unsigned int inputPingPongTextures[2]) {
	glUseProgram(fftProgram);
	for (int i = 0;i < kSpectrumLog2Resolution;i++) {

		glUniform1i(glGetUniformLocation(fftProgram, "resolution"), kSpectrumResolution);
		glUniform1ui(glGetUniformLocation(fftProgram, "stage"), i);
		glUniform1i(glGetUniformLocation(fftProgram, "horizontal"), horizontal ? 1 : 0);
		bool isLastPass = (i == kSpectrumLog2Resolution - 1) && !horizontal;
		glUniform1i(glGetUniformLocation(fftProgram, "normalization"), isLastPass);
		glUniform1i(glGetUniformLocation(fftProgram, "applyShift"), isLastPass);
		glBindImageTexture(0, inputPingPongTextures[pingPong ? 1 : 0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, inputPingPongTextures[pingPong ? 0 : 1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(2, twiddleFactorsTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		const int groups = (kSpectrumResolution + 15) / 16;
		glDispatchCompute(groups, groups, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		pingPong = !pingPong;

	}
	glUseProgram(0);
}
void updateSpectrumPipeline(float time) {
	evaluateSpectrumAtTime(time);
	bool pingPong = false;
	runFFTPasses(true, pingPong, pingpongTexture);
	runFFTPasses(false, pingPong, pingpongTexture);
	runFFTPasses(true, pingPong, pingpongdxTexture);
	runFFTPasses(false, pingPong, pingpongdxTexture);
	runFFTPasses(true, pingPong, pingpongdyTexture);
	runFFTPasses(false, pingPong, pingpongdyTexture);
	runFFTPasses(true, pingPong, pingpongdisplacementxTexture);
	runFFTPasses(false, pingPong, pingpongdisplacementxTexture);
	runFFTPasses(true, pingPong, pingpongdisplacementyTexture);
	runFFTPasses(false, pingPong, pingpongdisplacementyTexture);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}
void processInput(GLFWwindow* window, Camera* camera)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	camera->update();
}
void createFftSampler() {
	if (fftLinearSampler != 0) {
		glDeleteSamplers(1, &fftLinearSampler);
	}
	glGenSamplers(1, &fftLinearSampler);
	glSamplerParameteri(fftLinearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(fftLinearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(fftLinearSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glSamplerParameteri(fftLinearSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
int main(void)
{
	stbi_set_flip_vertically_on_load(true);
	GLFWwindow* window = initWindow();
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (window == NULL) {
		return -1;
	}
	camera = new Camera(window);

	shaderProgram = createShaderProgram(
		(programDir / "ocean.vert").string(),
		(programDir / "ocean.frag").string(),
		(programDir / "ocean.tesc").string(),
		(programDir / "ocean.tese").string()
	);
	equirectangularProgram = createShaderProgram(
		(programDir / "equirectangular_to_cubemap.vert").string(),
		(programDir / "equirectangular_to_cubemap.frag").string()
	);
	postProcessProgram = createShaderProgram(
		(programDir / "postprocess.vert").string(),
		(programDir / "postprocess.frag").string()
	);
	spectrumProgram = createComputeShaderProgram((programDir / "spectrum.comp").string());
	fftProgram = createComputeShaderProgram((programDir / "fft.comp").string());
	twiddleFactorGenerationProgram = createComputeShaderProgram((programDir / "precomputeTwiddleFactor.comp").string());
	timeEvaluationProgram = createComputeShaderProgram((programDir / "timeEvaluation.comp").string());
	glEnable(GL_DEPTH_TEST);
	createSpectrumTextures();
	createTwiddleFactorsTexture(twiddleFactorGenerationProgram);
	createPingPongTextures();
	createOceanMesh(ivec2(static_cast<int>(kOceanMeshSize.x), static_cast<int>(kOceanMeshSize.y)),10.0);
	createCubeMesh();
	createFftSampler();
	createPostProcessQuad();
	createPostProcessResources();
	generateSpectrum();
	environmentCubemap = createEnvironmentCubemap((programDir / "skyboxes/CasualDay4K.hdr").string(), 512);

	while (!glfwWindowShouldClose(window))
	{
		updateFPSCounter(window);
		render();
		processInput(window, camera);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}