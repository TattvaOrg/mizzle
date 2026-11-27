#include "shader_utils.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace lg {

namespace {

bool readFileText(const std::string& path, std::string& outText) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    outText = ss.str();
    return true;
}

std::string makeShaderHeader() {
    if (epoxy_is_desktop_gl()) {
        return "#version 330 core\n";
    }
    return "#version 300 es\nprecision highp float;\nprecision highp int;\n";
}

GLuint compileStage(GLenum type, const std::string& source, std::string& outError) {
    GLuint shader = glCreateShader(type);
    const char* srcPtr = source.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log(len > 1 ? static_cast<size_t>(len) : 1, '\0');
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        outError = log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

ShaderProgram::~ShaderProgram() {
    destroy();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : program_(other.program_) {
    other.program_ = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        destroy();
        program_ = other.program_;
        other.program_ = 0;
    }
    return *this;
}

void ShaderProgram::destroy() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

bool ShaderProgram::loadFromFiles(const std::string& vertPath, const std::string& fragPath, std::string& outError) {
    std::string vertBody;
    if (!readFileText(vertPath, vertBody)) {
        outError = "Failed to read vertex shader: " + vertPath;
        return false;
    }
    std::string fragBody;
    if (!readFileText(fragPath, fragBody)) {
        outError = "Failed to read fragment shader: " + fragPath;
        return false;
    }

    const std::string header = makeShaderHeader();
    std::string stageErr;
    GLuint vs = compileStage(GL_VERTEX_SHADER, header + vertBody, stageErr);
    if (!vs) {
        outError = "Vertex compile error (" + vertPath + "): " + stageErr;
        return false;
    }
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, header + fragBody, stageErr);
    if (!fs) {
        glDeleteShader(vs);
        outError = "Fragment compile error (" + fragPath + "): " + stageErr;
        return false;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len > 1 ? static_cast<size_t>(len) : 1, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        outError = "Program link error: " + log;
        glDeleteProgram(prog);
        return false;
    }

    destroy();
    program_ = prog;
    return true;
}

void ShaderProgram::use() const {
    glUseProgram(program_);
}

void ShaderProgram::setInt(const char* name, int v) const {
    GLint loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniform1i(loc, v);
}

void ShaderProgram::setFloat(const char* name, float v) const {
    GLint loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniform1f(loc, v);
}

void ShaderProgram::setVec2(const char* name, float x, float y) const {
    GLint loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniform2f(loc, x, y);
}

void ShaderProgram::setVec4(const char* name, float x, float y, float z, float w) const {
    GLint loc = glGetUniformLocation(program_, name);
    if (loc >= 0) glUniform4f(loc, x, y, z, w);
}

std::string resolveShaderPath(const std::string& filename) {
    namespace fs = std::filesystem;

    if (const char* envDir = std::getenv("MIZZLE_SHADER_DIR")) {
        fs::path envPath = fs::path(envDir) / filename;
        if (fs::exists(envPath)) return envPath.string();
    }

    fs::path local = fs::path("shaders") / filename;
    if (fs::exists(local)) return local.string();

    std::error_code ec;
    fs::path exePath = fs::read_symlink("/proc/self/exe", ec);
    if (!ec && exePath.has_parent_path()) {
        fs::path relShare = exePath.parent_path() / ".." / "share" / "mizzle" / "shaders" / filename;
        if (fs::exists(relShare, ec)) return relShare.lexically_normal().string();
        fs::path relLocal = exePath.parent_path() / "shaders" / filename;
        if (fs::exists(relLocal, ec)) return relLocal.lexically_normal().string();
    }

#ifdef LIQUID_GLASS_INSTALL_SHADER_DIR
    fs::path installed = fs::path(LIQUID_GLASS_INSTALL_SHADER_DIR) / filename;
    if (fs::exists(installed)) return installed.string();
#endif
#ifdef LIQUID_GLASS_SHADER_DIR
    fs::path compiled = fs::path(LIQUID_GLASS_SHADER_DIR) / filename;
    if (fs::exists(compiled)) return compiled.string();
#endif
    return local.string();
}

} // namespace lg
