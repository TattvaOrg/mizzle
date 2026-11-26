#pragma once

#include <epoxy/gl.h>
#include <string>

namespace lg {

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath, std::string& outError);
    void use() const;
    GLuint id() const { return program_; }

    void setInt(const char* name, int v) const;
    void setFloat(const char* name, float v) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;

private:
    GLuint program_ = 0;
    void destroy();
};

std::string resolveShaderPath(const std::string& filename);

} // namespace lg
