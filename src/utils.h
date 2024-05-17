#include "stdio.h"
#include <glm/glm.hpp>
#include <sstream>
#include <Windows.h>
#include <glm/gtx/string_cast.hpp>
#include <string>

static void printMat4(const glm::mat4& matrix) {
    printf("MATRIX ========\n");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            printf("%.1f ", matrix[i][j]);
        }
        printf("\n");
    }

    OutputDebugStringA(glm::to_string(matrix).c_str());
}

static std::string mat4ToString(const glm::mat4& matrix) {
    std::string result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result += std::to_string(matrix[i][j]);
            result += " ";
        }
        result += "\n";
    }
    return result;
}