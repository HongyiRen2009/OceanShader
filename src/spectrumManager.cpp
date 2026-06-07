#include "spectrumManager.h"
#include <glm/gtc/type_ptr.hpp>
SpectrumManager::SpectrumManager(unsigned int resolution, float windSpeed, vec2 windDir) {
	this->resolution = resolution;
	this->windSpeed = windSpeed;
	this->windDir = windDir;
	log2Resolution = static_cast<unsigned int>(std::log2(resolution));
}
SpectrumManager::~SpectrumManager() {
	if (displacementMap != 0) {
		glDeleteTextures(1, &displacementMap);
	}
	if (normalMap != 0) {
		glDeleteTextures(1, &normalMap);
	}
	for (int i = 0; i < 2; ++i) {
		if (foamPingPongTexture[i] != 0) {
			glDeleteTextures(1, &foamPingPongTexture[i]);
			foamPingPongTexture[i] = 0;
		}
	}
}
void SpectrumManager::addSpectrum(std::initializer_list<SpectrumSettings> init) {
	for (const SpectrumSettings& spectrum : init) {
		this->spectrums.push_back(spectrum);
	}
	numSpectrums = static_cast<int>(spectrums.size());
	updateTextureArrays();
	createAssembledTextures();
}
void SpectrumManager::initializeTextures(unsigned int spectrumProgram,unsigned int gaussianNoiseTexture, unsigned int twiddleFactorsTexture) {
	this->gaussianNoiseTexture = gaussianNoiseTexture;
	this->twiddleFactorsTexture = twiddleFactorsTexture;
}
void SpectrumManager::createAssembledTextures() {
	if (normalMap != 0) {
		glDeleteTextures(1, &normalMap);
		normalMap = 0;
	}
	if (displacementMap != 0) {
		glDeleteTextures(1, &displacementMap);
		displacementMap = 0;
	}
	normalMap = createTextureArray2D(resolution, resolution, numSpectrums, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	displacementMap = createTextureArray2D(resolution, resolution, numSpectrums, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	for (int i = 0;i < 2;++i) {
		if (foamPingPongTexture[i] != 0) {
			glDeleteTextures(1, &foamPingPongTexture[i]);
			foamPingPongTexture[i] = 0;
		}
		foamPingPongTexture[i] = createTextureArray2D(resolution, resolution, numSpectrums, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	}
}
void SpectrumManager::updateTextureArrays() {
	
	if (spectrumTextureArray != 0) {
		glDeleteTextures(1, &spectrumTextureArray);
		spectrumTextureArray = 0;
	}
	spectrumTextureArray = createTextureArray2D(resolution, resolution, numSpectrums, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	for (int i = 0;i < 2;++i) {
		if (pingpongHeightDxzTextureArray[i] != 0) {
			glDeleteTextures(1, &pingpongHeightDxzTextureArray[i]);
			pingpongHeightDxzTextureArray[i] = 0;
		}
		if (pingpongDxDyTextureArray[i] != 0) {
			glDeleteTextures(1, &pingpongDxDyTextureArray[i]);
			pingpongDxDyTextureArray[i] = 0;
		}
		if (pingpongDerivXXZZTextureArray[i] != 0) {
			glDeleteTextures(1, &pingpongDerivXXZZTextureArray[i]);
			pingpongDerivXXZZTextureArray[i] = 0;
		}
		if (pingpongDispXYTextureArray[i] != 0) {
			glDeleteTextures(1, &pingpongDispXYTextureArray[i]);
			pingpongDispXYTextureArray[i] = 0;
		}
		pingpongHeightDxzTextureArray[i] = createTextureArray2D(resolution, resolution, numSpectrums, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		pingpongDxDyTextureArray[i] = createTextureArray2D(resolution, resolution, numSpectrums, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		pingpongDerivXXZZTextureArray[i] = createTextureArray2D(resolution, resolution, numSpectrums, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		pingpongDispXYTextureArray[i] = createTextureArray2D(resolution, resolution, numSpectrums, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	}



}
void SpectrumManager::generateSpectrums(unsigned int spectrumProgram) {
	glUseProgram(spectrumProgram);
	
	glUniform1i(glGetUniformLocation(spectrumProgram, "resolution"), resolution);
	std::vector<float> oceanSizes = std::vector<float>(numSpectrums);
	std::vector<float> fetches = std::vector<float>(numSpectrums);
	for (int i = 0;i < numSpectrums	;++i) {
		oceanSizes[i] = spectrums[i].oceanSize;
		fetches[i] = spectrums[i].fetch;
	}

	glUniform1fv(glGetUniformLocation(spectrumProgram, "oceanSizes"), numSpectrums, oceanSizes.data());
	glUniform1f(glGetUniformLocation(spectrumProgram, "windSpeed"), windSpeed);
	glUniform1fv(glGetUniformLocation(spectrumProgram, "fetches"), numSpectrums, fetches.data());
	glUniform2fv(glGetUniformLocation(spectrumProgram, "windDir"),1, value_ptr(windDir));
	glUniform1i(glGetUniformLocation(spectrumProgram, "numSpectrums"), numSpectrums);
	glBindImageTexture(0, spectrumTextureArray, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	bindTexture(gaussianNoiseTexture,"gaussianNoiseTexture",0);
	glUniform1i(glGetUniformLocation(spectrumProgram, "gaussianNoise"), 0);
	const int groups = (resolution + 15) / 16;
	glDispatchCompute(groups, groups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}
void SpectrumManager::evaluateSpectrumsAtTime(unsigned int timeEvaluationProgram, float time) {
	glUseProgram(timeEvaluationProgram);
	
	glUniform1f(glGetUniformLocation(timeEvaluationProgram, "time"), time);
	glUniform1i(glGetUniformLocation(timeEvaluationProgram, "resolution"), resolution);
	glUniform1f(glGetUniformLocation(timeEvaluationProgram, "speed"), 4.0);
	glUniform1i(glGetUniformLocation(timeEvaluationProgram, "numSpectrums"), numSpectrums);

	std::vector<float> oceanSizes = std::vector<float>(numSpectrums);
	for (int i = 0;i < numSpectrums;i++) {
		oceanSizes[i] = spectrums[i].oceanSize;
	}
	glUniform1fv(glGetUniformLocation(timeEvaluationProgram, "oceanSizes"), numSpectrums, oceanSizes.data());
	glBindImageTexture(0, spectrumTextureArray, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, pingpongHeightDxzTextureArray[0], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, pingpongDxDyTextureArray[0], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(3, pingpongDerivXXZZTextureArray[0], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(4, pingpongDispXYTextureArray[0], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	const int groups = (resolution + 15) / 16;
	glDispatchCompute(groups, groups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}
void SpectrumManager::runFFTPasses(unsigned int fftProgram, bool horizontal, bool& pingPong, unsigned int pingpongTextureArrays[2]) {
	glUseProgram(fftProgram);
	
	for (int i = 0;i < log2Resolution;i++) {
		glUniform1i(glGetUniformLocation(fftProgram, "resolution"), resolution);
		glUniform1ui(glGetUniformLocation(fftProgram, "stage"), i);
		glUniform1i(glGetUniformLocation(fftProgram, "horizontal"), horizontal ? 1 : 0);
		bool isLastPass = (i == log2Resolution - 1) && !horizontal;
		glUniform1i(glGetUniformLocation(fftProgram, "normalization"), isLastPass);
		glUniform1i(glGetUniformLocation(fftProgram, "applyShift"), isLastPass);
		glUniform1i(glGetUniformLocation(fftProgram, "numSpectrums"), numSpectrums);
		glBindImageTexture(0, pingpongTextureArrays[pingPong ? 1 : 0], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, pingpongTextureArrays[pingPong ? 0 : 1], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(2, twiddleFactorsTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		const int groups = (resolution + 15) / 16;
		glDispatchCompute(groups, groups, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		pingPong = !pingPong;
	}
	glUseProgram(0);
}
void SpectrumManager::assembleFinalMaps(unsigned int assemblyProgram) {
	glUseProgram(assemblyProgram);
	std::vector<float> foamBiases = std::vector<float>(numSpectrums);
	for (int i = 0;i < numSpectrums;i++) {
		foamBiases[i] = spectrums[i].foamBias;
	}
	glUniform1i(glGetUniformLocation(assemblyProgram, "resolution"), resolution);
	glUniform1i(glGetUniformLocation(assemblyProgram, "numSpectrums"), numSpectrums);
	glUniform1fv(glGetUniformLocation(assemblyProgram, "foamBiases"), numSpectrums, foamBiases.data());
	glUniform1i(glGetUniformLocation(assemblyProgram, "resolution"), resolution);
	glUniform1f(glGetUniformLocation(assemblyProgram, "choppiness"), 1.5);
	glUniform1f(glGetUniformLocation(assemblyProgram, "foamDecayFactor"), 0.0175);

	glBindImageTexture(0, pingpongHeightDxzTextureArray[foamIndex], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, pingpongDerivXXZZTextureArray[foamIndex], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(2, pingpongDxDyTextureArray[foamIndex], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(3, pingpongDispXYTextureArray[foamIndex], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(4, foamPingPongTexture[1-foamIndex], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(5, normalMap, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(6, displacementMap, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(7, foamPingPongTexture[foamIndex], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	const int groups = (resolution + 15) / 16;
	glDispatchCompute(groups, groups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void SpectrumManager::tickSpectrums(unsigned int spectrumProgram, unsigned int timeEvaluationProgram, unsigned int fftProgram, unsigned int assemblyProgram, float time) {
	generateSpectrums(spectrumProgram);
	evaluateSpectrumsAtTime(timeEvaluationProgram, time);
	
	bool pingPong = false;
	runFFTPasses(fftProgram, true, pingPong, pingpongHeightDxzTextureArray);
	runFFTPasses(fftProgram, false, pingPong, pingpongHeightDxzTextureArray);
	runFFTPasses(fftProgram, true, pingPong, pingpongDxDyTextureArray);
	runFFTPasses(fftProgram, false, pingPong, pingpongDxDyTextureArray);
	runFFTPasses(fftProgram, true, pingPong, pingpongDerivXXZZTextureArray);
	runFFTPasses(fftProgram, false, pingPong, pingpongDerivXXZZTextureArray);
	runFFTPasses(fftProgram, true, pingPong, pingpongDispXYTextureArray);
	runFFTPasses(fftProgram, false, pingPong, pingpongDispXYTextureArray);
	assembleFinalMaps(assemblyProgram);
}