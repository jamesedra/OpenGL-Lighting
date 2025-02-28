#pragma once
#include <glm/glm.hpp>

struct PointLightData {
    glm::vec4 positionAndConstant;
    glm::vec4 ambientAndLinear;
    glm::vec4 diffuseAndQuadratic;
    glm::vec4 specular;

    PointLightData()
        : positionAndConstant(0.0f),
        ambientAndLinear(0.0f),
        diffuseAndQuadratic(0.0f),
        specular(0.0f)
    {}

    PointLightData(const glm::vec4& posConst,
        const glm::vec4& ambLin,
        const glm::vec4& diffQuad,
        const glm::vec4& spec)
        : positionAndConstant(posConst),
        ambientAndLinear(ambLin),
        diffuseAndQuadratic(diffQuad),
        specular(spec)
    {}
};

constexpr int MAX_POINT_LIGHTS = 4;

struct PointLightsBlock {
    PointLightData pointLights[MAX_POINT_LIGHTS];
    int numPointLights = 0;

    PointLightsBlock(const std::vector<PointLightData>& pointLights) {
        numPointLights = static_cast<int>(pointLights.size());
        if (numPointLights > MAX_POINT_LIGHTS) numPointLights = MAX_POINT_LIGHTS;

        for (int i = 0; i < numPointLights; i++) {
            this->pointLights[i] = pointLights[i];
        }
    }
};