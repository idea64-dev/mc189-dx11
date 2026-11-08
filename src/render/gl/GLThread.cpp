// The public GL:: calls of the client thread. The emulation itself (GLImpl.h) runs on a GL thread: each call
// becomes a command in a stream the GL thread executes in order, so the client thread only records them and
// goes on with the game. What the game reads back right away lives here as well (matrix stacks, current
// vertex attributes, viewport, fog colour); the few other queries wait for the GL thread.

#include "render/gl/GLImpl.h"

#include "core/Log.h"
#include "platform/Display.h"
#include "render/RenderDevice.h"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace mc::GL {

namespace {

// --- Command stream ---

constexpr size_t kBlockSize = size_t { 8 } << 20;
constexpr size_t kAlign = 16;
constexpr size_t kMaxCommand = 512;

constexpr size_t alignUp(size_t value)
{
    return (value + kAlign - 1) & ~(kAlign - 1);
}

// Commands and the data they point to, back to back: a header, then the command's callable (or raw data
// when `run` is null).
struct CommandHeader {
    void (*run)(void* payload);
    size_t size;
};
constexpr size_t kHeaderSize = alignUp(sizeof(CommandHeader));

struct Block {
    std::unique_ptr<std::byte[]> data;
    size_t capacity = 0;
    size_t used = 0;
};

struct Worker {
    std::thread thread;
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<Block> published;
    std::vector<Block> freeBlocks;
    bool stop = false;
    std::exception_ptr failure;
    uint64_t framesDone = 0;

    // Client thread only.
    bool running = false;
    RenderDevice* device = nullptr;
    Block current;
    uint64_t framesSubmitted = 0;
};

Worker& worker()
{
    static Worker instance;
    return instance;
}

void rethrowFailure()
{
    Worker& w = worker();
    std::lock_guard lock(w.mutex);
    if (w.failure)
        std::rethrow_exception(w.failure);
}

// Hands the commands recorded so far to the GL thread.
void publish()
{
    Worker& w = worker();
    if (w.current.used == 0)
        return;
    {
        std::lock_guard lock(w.mutex);
        w.published.push_back(std::move(w.current));
    }
    w.condition.notify_all();
    w.current = Block {};
}

void ensureRoom(size_t bytes)
{
    Worker& w = worker();
    if (w.current.data && w.current.used + bytes <= w.current.capacity)
        return;
    publish();
    {
        std::lock_guard lock(w.mutex);
        if (!w.freeBlocks.empty() && bytes <= kBlockSize) {
            w.current = std::move(w.freeBlocks.back());
            w.freeBlocks.pop_back();
        }
    }
    if (!w.current.data) {
        w.current.capacity = std::max(kBlockSize, alignUp(bytes));
        w.current.data.reset(new std::byte[w.current.capacity]);
    }
    w.current.used = 0;
}

std::byte* bump(size_t bytes)
{
    Worker& w = worker();
    std::byte* p = w.current.data.get() + w.current.used;
    w.current.used += alignUp(bytes);
    return p;
}

template <class F>
void enqueue(F&& f)
{
    using Fn = std::decay_t<F>;
    static_assert(std::is_trivially_copyable_v<Fn> && std::is_trivially_destructible_v<Fn>, "commands are copied as bytes");
    static_assert(kHeaderSize + sizeof(Fn) <= kMaxCommand);
    Worker& w = worker();
    if (!w.running) {
        f();
        return;
    }
    ensureRoom(kHeaderSize + alignUp(sizeof(Fn)));
    auto* header = reinterpret_cast<CommandHeader*>(bump(sizeof(CommandHeader)));
    void* payload = bump(sizeof(Fn));
    new (payload) Fn(std::forward<F>(f));
    header->run = [](void* p) { (*static_cast<Fn*>(p))(); };
    header->size = kHeaderSize + alignUp(sizeof(Fn));
}

// Copies data into the stream, next to the command that will read it (room for one is reserved).
template <class T>
const T* copyData(std::span<const T> data)
{
    Worker& w = worker();
    if (!w.running || data.empty())
        return data.data();
    ensureRoom(kHeaderSize + alignUp(data.size_bytes()) + kMaxCommand);
    auto* header = reinterpret_cast<CommandHeader*>(bump(sizeof(CommandHeader)));
    std::byte* p = bump(data.size_bytes());
    std::memcpy(p, data.data(), data.size_bytes());
    header->run = nullptr;
    header->size = kHeaderSize + alignUp(data.size_bytes());
    return reinterpret_cast<const T*>(p);
}

// Runs `f` on the GL thread after everything recorded before and returns its result.
template <class F>
auto call(F f) -> decltype(f())
{
    using R = decltype(f());
    Worker& w = worker();
    if (!w.running)
        return f();
    R result {};
    bool done = false;
    R* resultPtr = &result;
    bool* donePtr = &done;
    enqueue([f, resultPtr, donePtr] {
        *resultPtr = f();
        Worker& wk = worker();
        {
            std::lock_guard lock(wk.mutex);
            *donePtr = true;
        }
        wk.condition.notify_all();
    });
    publish();
    {
        std::unique_lock lock(w.mutex);
        w.condition.wait(lock, [&] { return done || w.failure; });
    }
    rethrowFailure();
    return result;
}

void workerMain()
{
    Log::setThreadName("GL thread");
    Worker& w = worker();
    bool failed = false;
    for (;;) {
        Block block;
        {
            std::unique_lock lock(w.mutex);
            w.condition.wait(lock, [&] { return w.stop || !w.published.empty(); });
            if (w.published.empty())
                return;
            block = std::move(w.published.front());
            w.published.pop_front();
        }
        if (!failed) {
            try {
                for (size_t offset = 0; offset < block.used;) {
                    auto* header = reinterpret_cast<CommandHeader*>(block.data.get() + offset);
                    if (header->run)
                        header->run(block.data.get() + offset + kHeaderSize);
                    offset += header->size;
                }
            } catch (...) {
                failed = true;
                Log::fatal("GL thread stopped by an exception");
                std::lock_guard lock(w.mutex);
                w.failure = std::current_exception();
            }
        }
        {
            std::lock_guard lock(w.mutex);
            if (block.capacity == kBlockSize) {
                block.used = 0;
                w.freeBlocks.push_back(std::move(block));
            }
        }
        w.condition.notify_all();
    }
}

// --- What the client thread reads back ---

struct ClientState {
    int matrixMode = GL_MODELVIEW;
    Matrix4f modelView = Matrix4f::identity();
    Matrix4f projection = Matrix4f::identity();
    std::array<Matrix4f, 2> texture { Matrix4f::identity(), Matrix4f::identity() };
    std::vector<Matrix4f> modelViewStack;
    std::vector<Matrix4f> projectionStack;
    std::array<std::vector<Matrix4f>, 2> textureStack;
    bool matricesDirty = true;
    int activeTexture = 0;

    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float normal[3] = { 0.0f, 0.0f, 1.0f };
    float texCoord[2][2] = {};
    int viewport[4] = {};
    float fogColor[4] = {};

    bool inBeginEnd = false;
    int beginMode = GL_TRIANGLES;
    std::vector<GLVertex> immediate;

    int nextName = 1;
    int windowWidth = 0;
    int windowHeight = 0;
};

ClientState& client()
{
    static ClientState instance;
    return instance;
}

Matrix4f& activeMatrix()
{
    ClientState& c = client();
    switch (c.matrixMode) {
    case GL_PROJECTION: return c.projection;
    case GL_TEXTURE: return c.texture[static_cast<size_t>(c.activeTexture)];
    default: return c.modelView;
    }
}

std::vector<Matrix4f>& activeStack()
{
    ClientState& c = client();
    switch (c.matrixMode) {
    case GL_PROJECTION: return c.projectionStack;
    case GL_TEXTURE: return c.textureStack[static_cast<size_t>(c.activeTexture)];
    default: return c.modelViewStack;
    }
}

Matrix4f& changeMatrix()
{
    client().matricesDirty = true;
    return activeMatrix();
}

// The GL thread gets the matrices before anything that reads them.
void syncMatrices()
{
    ClientState& c = client();
    if (!c.matricesDirty)
        return;
    c.matricesDirty = false;
    struct Matrices {
        Matrix4f modelView, projection, texture0, texture1;
    };
    const Matrices m { c.modelView, c.projection, c.texture[0], c.texture[1] };
    enqueue([m] { impl::setMatrices(m.modelView, m.projection, m.texture0, m.texture1); });
}

// The default framebuffer follows the window, whose size the client thread knows.
void syncWindow()
{
    ClientState& c = client();
    const int width = Display::getWidth();
    const int height = Display::getHeight();
    if (width == c.windowWidth && height == c.windowHeight)
        return;
    c.windowWidth = width;
    c.windowHeight = height;
    enqueue([width, height] { impl::setWindowSize(width, height); });
}

void endFrame(bool vsync)
{
    Worker& w = worker();
    syncWindow();
    enqueue([vsync] {
        impl::endFrame(vsync);
        Worker& wk = worker();
        {
            std::lock_guard lock(wk.mutex);
            ++wk.framesDone;
        }
        wk.condition.notify_all();
    });
    publish();
    ++w.framesSubmitted;
    // At most one frame waits for the GL thread.
    {
        std::unique_lock lock(w.mutex);
        w.condition.wait(lock, [&] { return w.framesSubmitted - w.framesDone <= 1 || w.failure; });
    }
    rethrowFailure();
}

std::array<float, 4> copy4(const float* params, bool four)
{
    return { params[0], four ? params[1] : 0.0f, four ? params[2] : 0.0f, four ? params[3] : 0.0f };
}

} // namespace

// --- Lifetime ---

void initialize(RenderDevice& device)
{
    ClientState& c = client();
    c = ClientState {};
    c.windowWidth = Display::getWidth();
    c.windowHeight = Display::getHeight();
    impl::setWindowSize(c.windowWidth, c.windowHeight);
    impl::initialize(device);
    impl::glGetIntegerv(GL_VIEWPORT, c.viewport);

    Worker& w = worker();
    w.stop = false;
    w.failure = nullptr;
    w.framesDone = 0;
    w.framesSubmitted = 0;
    w.thread = std::thread(workerMain);
    w.running = true;
    w.device = &device;
    device.setPresentHandler([](bool vsync) { endFrame(vsync); });
}

void shutdown()
{
    Worker& w = worker();
    if (w.running) {
        w.device->setPresentHandler(nullptr);
        publish();
        {
            std::lock_guard lock(w.mutex);
            w.stop = true;
        }
        w.condition.notify_all();
        w.thread.join();
        w.running = false;
        w.published.clear();
        w.freeBlocks.clear();
        w.current = Block {};
    }
    impl::shutdown();
}

std::string glGetString(int name)
{
    // Adapter information only, fixed once the device exists.
    return impl::glGetString(name);
}

// --- Capabilities ---

void glEnable(int capability)
{
    enqueue([capability] { impl::glEnable(capability); });
}

void glDisable(int capability)
{
    enqueue([capability] { impl::glDisable(capability); });
}

bool glIsEnabled(int capability)
{
    return call([capability] { return impl::glIsEnabled(capability); });
}

void glPushAttrib(int mask)
{
    enqueue([mask] { impl::glPushAttrib(mask); });
}

void glPopAttrib()
{
    enqueue([] { impl::glPopAttrib(); });
}

// --- Fragment operations ---

void glAlphaFunc(int func, float ref)
{
    enqueue([func, ref] { impl::glAlphaFunc(func, ref); });
}

void glBlendFunc(int srcFactor, int dstFactor)
{
    enqueue([srcFactor, dstFactor] { impl::glBlendFunc(srcFactor, dstFactor); });
}

void glBlendFuncSeparate(int srcRgb, int dstRgb, int srcAlpha, int dstAlpha)
{
    enqueue([srcRgb, dstRgb, srcAlpha, dstAlpha] { impl::glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha); });
}

void glDepthFunc(int func)
{
    enqueue([func] { impl::glDepthFunc(func); });
}

void glDepthMask(bool flag)
{
    enqueue([flag] { impl::glDepthMask(flag); });
}

void glColorMask(bool red, bool green, bool blue, bool alpha)
{
    enqueue([red, green, blue, alpha] { impl::glColorMask(red, green, blue, alpha); });
}

void glCullFace(int mode)
{
    enqueue([mode] { impl::glCullFace(mode); });
}

void glFrontFace(int mode)
{
    enqueue([mode] { impl::glFrontFace(mode); });
}

void glPolygonOffset(float factor, float units)
{
    enqueue([factor, units] { impl::glPolygonOffset(factor, units); });
}

void glShadeModel(int mode)
{
    enqueue([mode] { impl::glShadeModel(mode); });
}

void glLineWidth(float width)
{
    enqueue([width] { impl::glLineWidth(width); });
}

void glLogicOp(int opcode)
{
    enqueue([opcode] { impl::glLogicOp(opcode); });
}

// --- Fog and lighting ---

void glFogi(int pname, int param)
{
    enqueue([pname, param] { impl::glFogi(pname, param); });
}

void glFogf(int pname, float param)
{
    enqueue([pname, param] { impl::glFogf(pname, param); });
}

void glFogfv(int pname, const float* params)
{
    const bool color = pname == GL_FOG_COLOR;
    const std::array<float, 4> values = copy4(params, color);
    if (color)
        std::copy(values.begin(), values.end(), client().fogColor);
    enqueue([pname, values] { impl::glFogfv(pname, values.data()); });
}

void glLightfv(int light, int pname, const float* params)
{
    // GL_POSITION is transformed by the modelview matrix of this call.
    if (pname == GL_POSITION)
        syncMatrices();
    const std::array<float, 4> values = copy4(params, true);
    enqueue([light, pname, values] { impl::glLightfv(light, pname, values.data()); });
}

void glLightModelfv(int pname, const float* params)
{
    const std::array<float, 4> values = copy4(params, true);
    enqueue([pname, values] { impl::glLightModelfv(pname, values.data()); });
}

void glColorMaterial(int face, int mode)
{
    enqueue([face, mode] { impl::glColorMaterial(face, mode); });
}

// --- Current attributes and immediate mode ---

void glColor4f(float red, float green, float blue, float alpha)
{
    ClientState& c = client();
    c.color[0] = red;
    c.color[1] = green;
    c.color[2] = blue;
    c.color[3] = alpha;
    enqueue([red, green, blue, alpha] { impl::glColor4f(red, green, blue, alpha); });
}

void glNormal3f(float x, float y, float z)
{
    ClientState& c = client();
    c.normal[0] = x;
    c.normal[1] = y;
    c.normal[2] = z;
    enqueue([x, y, z] { impl::glNormal3f(x, y, z); });
}

void glTexCoord2f(float u, float v)
{
    ClientState& c = client();
    c.texCoord[0][0] = u;
    c.texCoord[0][1] = v;
    enqueue([u, v] { impl::glTexCoord2f(u, v); });
}

void glMultiTexCoord2f(int textureUnit, float u, float v)
{
    const int unit = textureUnit - GL_TEXTURE0;
    if (unit >= 0 && unit < 2) {
        client().texCoord[unit][0] = u;
        client().texCoord[unit][1] = v;
    }
    enqueue([textureUnit, u, v] { impl::glMultiTexCoord2f(textureUnit, u, v); });
}

const float* currentColor()
{
    return client().color;
}

const float* currentNormal()
{
    return client().normal;
}

const float* currentTexCoord(int unit)
{
    return client().texCoord[std::clamp(unit, 0, 1)];
}

void glBegin(int mode)
{
    ClientState& c = client();
    c.inBeginEnd = true;
    c.beginMode = mode;
    c.immediate.clear();
}

void glVertex3f(float x, float y, float z)
{
    ClientState& c = client();
    GLVertex v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.r = c.color[0];
    v.g = c.color[1];
    v.b = c.color[2];
    v.a = c.color[3];
    v.u0 = c.texCoord[0][0];
    v.v0 = c.texCoord[0][1];
    v.u1 = c.texCoord[1][0];
    v.v1 = c.texCoord[1][1];
    v.nx = c.normal[0];
    v.ny = c.normal[1];
    v.nz = c.normal[2];
    c.immediate.push_back(v);
}

void glVertex2f(float x, float y)
{
    glVertex3f(x, y, 0.0f);
}

void glEnd()
{
    ClientState& c = client();
    c.inBeginEnd = false;
    drawVertices(c.beginMode, c.immediate);
    c.immediate.clear();
}

void drawVertices(int mode, std::span<const GLVertex> vertices)
{
    if (vertices.empty())
        return;
    syncWindow();
    syncMatrices();
    const GLVertex* data = copyData(vertices);
    const size_t count = vertices.size();
    enqueue([mode, data, count] { impl::drawVertices(mode, std::span<const GLVertex>(data, count)); });
}

// --- Buffer objects ---

int glGenBuffers()
{
    const int name = client().nextName++;
    enqueue([] { impl::glGenBuffers(); });
    return name;
}

void glDeleteBuffers(int buffer)
{
    enqueue([buffer] { impl::glDeleteBuffers(buffer); });
}

void bufferVertices(int buffer, int mode, std::span<const GLVertex> vertices)
{
    const GLVertex* data = copyData(vertices);
    const size_t count = vertices.size();
    enqueue([buffer, mode, data, count] { impl::bufferVertices(buffer, mode, std::span<const GLVertex>(data, count)); });
}

void drawBuffer(int buffer)
{
    syncWindow();
    syncMatrices();
    enqueue([buffer] { impl::drawBuffer(buffer); });
}

void bufferBlockVertices(int buffer, std::span<const uint8_t> bytes, uint32_t vertexCount)
{
    const uint8_t* data = copyData(bytes);
    const size_t size = bytes.size();
    enqueue([buffer, data, size, vertexCount] { impl::bufferBlockVertices(buffer, std::span<const uint8_t>(data, size), vertexCount); });
}

void drawBlockBuffers(std::span<const int> buffers, std::span<const Matrix4f> modelViews)
{
    if (buffers.empty())
        return;
    syncWindow();
    syncMatrices();
    // Both copies and the command fit in one block.
    ensureRoom(2 * kHeaderSize + alignUp(buffers.size_bytes()) + alignUp(modelViews.size_bytes()) + 2 * kMaxCommand);
    const int* bufferData = copyData(buffers);
    const Matrix4f* matrixData = copyData(modelViews);
    const size_t bufferCount = buffers.size();
    const size_t matrixCount = modelViews.size();
    enqueue([bufferData, bufferCount, matrixData, matrixCount] {
        impl::drawBlockBuffers(std::span<const int>(bufferData, bufferCount), std::span<const Matrix4f>(matrixData, matrixCount));
    });
}

// --- Matrices (the stacks live on the client thread) ---

void glMatrixMode(int mode)
{
    client().matrixMode = mode;
}

void glLoadIdentity()
{
    changeMatrix() = Matrix4f::identity();
}

void glPushMatrix()
{
    activeStack().push_back(activeMatrix());
}

void glPopMatrix()
{
    auto& stack = activeStack();
    if (stack.empty()) {
        Log::warn("[GL emulation] matrix stack underflow");
        return;
    }
    changeMatrix() = stack.back();
    stack.pop_back();
}

void glLoadMatrixf(const float* matrix)
{
    std::memcpy(changeMatrix().m.data(), matrix, sizeof(float) * 16);
}

void glMultMatrixf(const float* matrix)
{
    Matrix4f other;
    std::memcpy(other.m.data(), matrix, sizeof(float) * 16);
    Matrix4f& m = changeMatrix();
    m = m * other;
}

void glTranslatef(float x, float y, float z)
{
    changeMatrix().translate(x, y, z);
}

void glTranslated(double x, double y, double z)
{
    changeMatrix().translate(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

void glScalef(float x, float y, float z)
{
    changeMatrix().scale(x, y, z);
}

void glScaled(double x, double y, double z)
{
    changeMatrix().scale(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

void glRotatef(float angle, float x, float y, float z)
{
    changeMatrix().rotate(angle, x, y, z);
}

void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar)
{
    changeMatrix().ortho(left, right, bottom, top, zNear, zFar);
}

void glGetFloatv(int pname, float* params)
{
    const ClientState& c = client();
    if (pname == GL_MODELVIEW_MATRIX)
        std::memcpy(params, c.modelView.m.data(), sizeof(float) * 16);
    else if (pname == GL_PROJECTION_MATRIX)
        std::memcpy(params, c.projection.m.data(), sizeof(float) * 16);
    else if (pname == GL_FOG_COLOR)
        std::memcpy(params, c.fogColor, sizeof(float) * 4);
    else
        Log::warn("[GL emulation] glGetFloatv({}) is not emulated", pname);
}

void glGetIntegerv(int pname, int* params)
{
    if (pname == GL_VIEWPORT)
        std::memcpy(params, client().viewport, sizeof(int) * 4);
    else
        Log::warn("[GL emulation] glGetIntegerv({}) is not emulated", pname);
}

const Matrix4f& currentMatrix(int mode)
{
    const ClientState& c = client();
    return mode == GL_PROJECTION ? c.projection : (mode == GL_TEXTURE ? c.texture[static_cast<size_t>(c.activeTexture)] : c.modelView);
}

// --- Viewport, scissor, clear ---

void glViewport(int x, int y, int width, int height)
{
    int* viewport = client().viewport;
    viewport[0] = x;
    viewport[1] = y;
    viewport[2] = width;
    viewport[3] = height;
    enqueue([x, y, width, height] { impl::glViewport(x, y, width, height); });
}

void glScissor(int x, int y, int width, int height)
{
    enqueue([x, y, width, height] { impl::glScissor(x, y, width, height); });
}

void glClearColor(float red, float green, float blue, float alpha)
{
    enqueue([red, green, blue, alpha] { impl::glClearColor(red, green, blue, alpha); });
}

void glClearDepth(double depth)
{
    enqueue([depth] { impl::glClearDepth(depth); });
}

void glClear(int mask)
{
    syncWindow();
    enqueue([mask] { impl::glClear(mask); });
}

// --- Textures ---

void glActiveTexture(int texture)
{
    const int unit = texture - GL_TEXTURE0;
    if (unit >= 0 && unit < 2)
        client().activeTexture = unit;
    enqueue([texture] { impl::glActiveTexture(texture); });
}

int glGenTextures()
{
    const int name = client().nextName++;
    enqueue([] { impl::glGenTextures(); });
    return name;
}

void glDeleteTextures(int texture)
{
    enqueue([texture] { impl::glDeleteTextures(texture); });
}

void glBindTexture(int target, int texture)
{
    enqueue([target, texture] { impl::glBindTexture(target, texture); });
}

void glTexParameteri(int target, int pname, int param)
{
    enqueue([target, pname, param] { impl::glTexParameteri(target, pname, param); });
}

void glTexParameterf(int target, int pname, float param)
{
    enqueue([target, pname, param] { impl::glTexParameterf(target, pname, param); });
}

void glTexEnvi(int target, int pname, int param)
{
    enqueue([target, pname, param] { impl::glTexEnvi(target, pname, param); });
}

void glTexImage2D(int target, int level, int width, int height, const uint32_t* pixels)
{
    const uint32_t* data = pixels
        ? copyData(std::span<const uint32_t>(pixels, static_cast<size_t>(std::max(0, width)) * static_cast<size_t>(std::max(0, height))))
        : nullptr;
    enqueue([target, level, width, height, data] { impl::glTexImage2D(target, level, width, height, data); });
}

void glTexSubImage2D(int target, int level, int x, int y, int width, int height, const uint32_t* pixels)
{
    if (!pixels || width <= 0 || height <= 0)
        return;
    const uint32_t* data = copyData(std::span<const uint32_t>(pixels, static_cast<size_t>(width) * static_cast<size_t>(height)));
    enqueue([target, level, x, y, width, height, data] { impl::glTexSubImage2D(target, level, x, y, width, height, data); });
}

void glCopyTexSubImage2D(int target, int level, int xOffset, int yOffset, int x, int y, int width, int height)
{
    syncWindow();
    enqueue([target, level, xOffset, yOffset, x, y, width, height] { impl::glCopyTexSubImage2D(target, level, xOffset, yOffset, x, y, width, height); });
}

std::vector<uint32_t> glGetTexImage(int target, int level)
{
    return call([target, level] { return impl::glGetTexImage(target, level); });
}

int textureWidth(int texture)
{
    return call([texture] { return impl::textureWidth(texture); });
}

int textureHeight(int texture)
{
    return call([texture] { return impl::textureHeight(texture); });
}

// --- Framebuffer objects ---

int glGenFramebuffers()
{
    const int name = client().nextName++;
    enqueue([] { impl::glGenFramebuffers(); });
    return name;
}

void glDeleteFramebuffers(int framebuffer)
{
    enqueue([framebuffer] { impl::glDeleteFramebuffers(framebuffer); });
}

void glBindFramebuffer(int framebuffer)
{
    enqueue([framebuffer] { impl::glBindFramebuffer(framebuffer); });
}

void glFramebufferTexture2D(int texture)
{
    enqueue([texture] { impl::glFramebufferTexture2D(texture); });
}

int glGenRenderbuffers()
{
    const int name = client().nextName++;
    enqueue([] { impl::glGenRenderbuffers(); });
    return name;
}

void glDeleteRenderbuffers(int renderbuffer)
{
    enqueue([renderbuffer] { impl::glDeleteRenderbuffers(renderbuffer); });
}

void glRenderbufferStorage(int renderbuffer, int width, int height)
{
    enqueue([renderbuffer, width, height] { impl::glRenderbufferStorage(renderbuffer, width, height); });
}

void glFramebufferRenderbuffer(int renderbuffer)
{
    enqueue([renderbuffer] { impl::glFramebufferRenderbuffer(renderbuffer); });
}

std::vector<uint32_t> glReadPixels(int x, int y, int width, int height)
{
    syncWindow();
    return call([x, y, width, height] { return impl::glReadPixels(x, y, width, height); });
}

int drawCallsLastFrame()
{
    return impl::drawCallsLastFrame();
}

} // namespace mc::GL
