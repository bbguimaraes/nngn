#include "compute_test.h"

#include "compute/compute.h"
#include "compute/opencl.h"
#include "utils/scoped.h"
#include "utils/utils.h"

using nngn::i32, nngn::u32, nngn::Compute;

void ComputeTest::execute_kernel() {
    auto c = Compute::create(Compute::Backend::OPENCL_BACKEND);
    QVERIFY(c->init());
    const auto prog = c->create_program(
        "__kernel void f(\n"
        "    uchar b, int i, uint u, float f,"
        "    __global float *dst\n"
        ") {\n"
        "    *dst = b + i + u + f;\n"
        "}",
        "-Werror");
    QVERIFY(prog);
    const auto dst = c->create_buffer(
        Compute::MemFlag::WRITE_ONLY, sizeof(float), nullptr);
    QVERIFY(dst);
    const auto kernel = c->create_kernel(
        prog, "f", {}, std::byte{1}, i32{2}, u32{3}, 4.0f, dst);
    QVERIFY(kernel);
    constexpr std::size_t size = 1;
    QVERIFY(c->execute(
        kernel, Compute::ExecFlag::BLOCKING, 1, &size, &size, {}));
    float ret = {};
    QVERIFY(c->read_buffer(dst, 0, sizeof(ret), nngn::as_bytes(&ret), {}));
    QCOMPARE(ret, 10.0f);
}

void ComputeTest::execute() {
    auto c = Compute::create(Compute::Backend::OPENCL_BACKEND);
    QVERIFY(c->init());
    const auto prog = c->create_program(
        "__kernel void f(uint src, __global uint *dst) { *dst = src; }",
        "-Werror");
    QVERIFY(prog);
    const auto dst = c->create_buffer(
        Compute::MemFlag::WRITE_ONLY, sizeof(u32), nullptr);
    QVERIFY(dst);
    constexpr u32 cmp = 1;
    constexpr std::size_t size = 1;
    QVERIFY(c->execute(
        prog, "f", Compute::ExecFlag::BLOCKING,
        1, &size, &size, {}, cmp, dst));
    u32 ret = {};
    QVERIFY(c->read_buffer(dst, 0, sizeof(ret), nngn::as_bytes(&ret), {}));
    QCOMPARE(ret, cmp);
}

void ComputeTest::execute_args() {
    auto c = Compute::create(Compute::Backend::OPENCL_BACKEND);
    QVERIFY(c->init());
    const auto prog = c->create_program(
        "__kernel void f(\n"
        "    uchar b, int i, uint u, float f,"
        "    __global const uchar *bv,\n"
        "    __global const int *iv,\n"
        "    __global const uint *uv,\n"
        "    __global const float *fv,\n"
        "    __global float *dst\n"
        ") {\n"
        "    *dst = b + i + u + f\n"
        "        + bv[0] + bv[1] + bv[2]\n"
        "        + iv[0] + iv[1] + iv[2]\n"
        "        + uv[0] + uv[1] + uv[2]\n"
        "        + fv[0] + fv[1] + fv[2];\n"
        "}",
        "-Werror");
    QVERIFY(prog);
    const auto dst = c->create_buffer(
        Compute::MemFlag::WRITE_ONLY, sizeof(float), nullptr);
    QVERIFY(dst);
    constexpr std::size_t size = 1;
    constexpr std::array b = {std::byte{5}, std::byte{6}, std::byte{7}};
    constexpr auto i = std::to_array<u32>({8, 9, 10});
    constexpr auto u = std::to_array<u32>({11, 12, 13});
    constexpr std::array f = {14.0f, 15.0f, 16.0f};
    QVERIFY(c->execute(
        prog, "f", Compute::ExecFlag::BLOCKING, 1, &size, &size, {},
        std::byte{1}, i32{2}, u32{3}, 4.0f, b, i, u, f, dst));
    float ret = {};
    QVERIFY(c->read_buffer(dst, 0, sizeof(ret), nngn::as_bytes(&ret), {}));
    QCOMPARE(ret, 136.0f);
}

void ComputeTest::events() {
    auto c = Compute::create(Compute::Backend::OPENCL_BACKEND);
    QVERIFY(c->init());
    const auto prog = c->create_program(
        "__kernel void f(uint i) {}", "-Werror");
    QVERIFY(prog);
    auto events = nngn::scoped(
        std::array<Compute::Event*, 3>{},
        [&c](auto &v) { for(auto x : v) if(x) c->release_events(1, &x); });
    const auto exec = [&c, prog](Compute::Event **w, Compute::Event **e) {
        constexpr std::size_t size = 1;
        return c->execute(
            prog, "f", Compute::ExecFlag::BLOCKING,
            1, &size, &size, {!!w, w, e}, 0u);
    };
    QVERIFY(exec(nullptr, &(*events)[0]));
    if(!(*events)[0])
        QCOMPARE((*events)[0], nullptr);
    cl_command_type type = {};
    const auto get_type = [](auto e, auto *p) {
        return clGetEventInfo(
            nngn::chain_cast<cl_event, void*>(e), CL_EVENT_COMMAND_TYPE,
            sizeof(*p), p, nullptr) == CL_SUCCESS;
    };
    QVERIFY(get_type((*events)[0], &type));
    QCOMPARE(type, CL_COMMAND_NDRANGE_KERNEL);
    QVERIFY(exec(&(*events)[0], &(*events)[1]));
    if(!(*events)[1])
        QCOMPARE((*events)[1], nullptr);
    if(!(*events)[2])
        QCOMPARE((*events)[2], nullptr);
    QVERIFY(get_type((*events)[1], &type));
    QCOMPARE(type, CL_COMMAND_BARRIER);
    QVERIFY(get_type((*events)[2], &type));
    QCOMPARE(type, CL_COMMAND_NDRANGE_KERNEL);
}

void ComputeTest::write_struct() {
    auto c = Compute::create(Compute::Backend::OPENCL_BACKEND);
    QVERIFY(c->init());
    const auto prog = c->create_program(
        "struct __attribute__((packed)) S { uint u; float f; float4 v; };\n"
        "__kernel void f(__global struct S *s, __global uint *dst) {\n"
        "    *dst = s->u + s->f + s->v.s0 + s->v.s1 + s->v.s2 + s->v.s3;\n"
        "}\n", "-Werror");
    QVERIFY(prog);
    struct {
        u32 u; float f; std::array<float, 4> v;
    } s = {1, 2, {3, 4, 5, 6}};
    const auto src = c->create_buffer(
        Compute::MemFlag::READ_ONLY, sizeof(s), nullptr);
    QVERIFY(src);
    const auto dst = c->create_buffer(
        Compute::MemFlag::WRITE_ONLY, sizeof(u32), nullptr);
    QVERIFY(dst);
    QVERIFY(c->write_struct(src, {}, s.u, s.f, s.v));
    constexpr std::size_t size = 1;
    QVERIFY(c->execute(
        prog, "f", Compute::ExecFlag::BLOCKING,
        1, &size, &size, {}, src, dst));
    u32 ret = {};
    QVERIFY(c->read_buffer(dst, 0, sizeof(ret), nngn::as_bytes(&ret), {}));
    QCOMPARE(ret, 21);
}

QTEST_MAIN(ComputeTest)
