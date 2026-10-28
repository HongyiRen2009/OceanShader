#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderutils.h"
#include <filesystem>
#include <iostream>
#include <utility>
struct SpectrumSettings
{
	float oceanSize;
	float choppiness;
	float fetch;
	float foamBias;
	float frequency;
	float amplitude;
};
class SpectrumManager {
	public:
		SpectrumManager(unsigned int resolution, float windSpeed, vec2 windDir);
		~SpectrumManager();
		void initializeTextures(unsigned int spectrumProgram,unsigned int gaussianNoiseTexture, unsigned int twiddleFactorsTexture);
		void addSpectrum(std::initializer_list<SpectrumSettings> init);
		void tickSpectrums(unsigned int spectrumProgram, unsigned int timeEvaluationProgram, unsigned int fftProgram, unsigned int assemblyProgram, float time);
		unsigned int getDisplacementMap() const { return displacementMap; }
		unsigned int getNormalMap() const { return normalMap; }
		unsigned int getFoamTexture() const { return foamPingPongTexture[1 - foamIndex]; }
		int getNumSpectrums() const { return numSpectrums; }
		std::pair<std::vector<float>, std::vector<float>> getFrequencyAmplitudes() const {
			std::vector<float> frequencies;
			std::vector<float> amplitudes;
			for (const SpectrumSettings& spectrum : spectrums) {
				frequencies.push_back(spectrum.frequency);
				amplitudes.push_back(spectrum.amplitude);
			}
			return std::make_pair(frequencies, amplitudes);
		}
		void updateTextureArrays();
	private:
		std::vector<SpectrumSettings> spectrums;
		unsigned int resolution;
		unsigned int log2Resolution;
		unsigned int foamPingPongTexture[2] = {0,0};
		unsigned int normalMap = 0;
		unsigned int displacementMap = 0;
		unsigned int foamIndex = 0;
		unsigned int gaussianNoiseTexture = 0;
		unsigned int twiddleFactorsTexture = 0;
		unsigned int spectrumTextureArray;
		unsigned int pingpongHeightDxzTextureArray[2];
		unsigned int pingpongDxDyTextureArray[2];
		unsigned int pingpongDerivXXZZTextureArray[2];
		unsigned int pingpongDispXYTextureArray[2];
		int numSpectrums = 0;
		float windSpeed;
		vec2 windDir;
		void createAssembledTextures();
		void generateSpectrums(unsigned int spectrumProgram);
		void evaluateSpectrumsAtTime(unsigned int timeEvaluationProgram, float time);
		void runFFTPasses(unsigned int fftProgram, bool horizontal, bool& pingPong, unsigned int pingpongTextureArrays[2]);
		void assembleFinalMaps(unsigned int assemblyProgram);
};
