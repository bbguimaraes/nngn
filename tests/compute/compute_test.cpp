#include <CL/cl.h>

#include "compute/compute.h"
#include "utils/scoped.h"
#include "utils/utils.h"

#include "compute_test.h"

void ComputeTest::execute() {
    auto c = nngn::Compute::create(nngn::Compute::Backend::OPENCL_BACKEND);
    QVERIFY(c->init());
    const auto prog = c->create_program(
        "__kernel void f(uint in, __global uint *dst) { *dst = in; }",
        "-Werror");
    QVERIFY(prog);
    const auto dst = c->create_buffer(
        nngn::Compute::MemFlag::WRITE_ONLY, sizeof(std::uint32_t), nullptr);
    QVERIFY(dst);
    constexpr std::uint32_t cmp = 1;
    constexpr std::size_t size = 1;
    QVERIFY(c->execute(
        prog, "f", nngn::Compute::ExecFlag::BLOCKING,
        1, &size, &size, {}, cmp, dst));
    std::uint32_t ret = {};
    QVERIFY(c->read_buffer(dst, 0, sizeof(ret), nngn::as_bytes(&ret), {}));
    QCOMPARE(ret, cmp);
}

void ComputeTest::execute_args() {
    auto c = nngn::Compute::create(nngn::Compute::Backend::OPENCL_BACKEND);
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
        nngn::Compute::MemFlag::WRITE_ONLY, sizeof(float), nullptr);
    QVERIFY(dst);
    constexpr std::size_t size = 1;
    constexpr std::array b = {std::byte{5}, std::byte{6}, std::byte{7}};
    constexpr auto i = std::to_array<std::int32_t>({8, 9, 10});
    constexpr auto u = std::to_array<std::uint32_t>({11, 12, 13});
    constexpr std::array f = {14.0f, 15.0f, 16.0f};
    QVERIFY(c->execute(
        prog, "f", nngn::Compute::ExecFlag::BLOCKING,
        1, &size, &size, {},
        std::byte{1}, std::uint32_t{2}, std::uint32_t{3}, 4.0f,
        b, i, u, f, dst));
    float ret = {};
    QVERIFY(c->read_buffer(dst, 0, sizeof(ret), nngn::as_bytes(&ret), {}));
    QCOMPARE(ret, 136.0f);
}

void ComputeTest::events() {
    auto c = nngn::Compute::create(nngn::Compute::Backend::OPENCL_BACKEND);
    QVERIFY(c->init());
    const auto prog = c->create_program(
        "__kernel void f(uint i) {}", "-Werror");
    QVERIFY(prog);
    auto events = nngn::scoped(
        std::array<nngn::Compute::Event, 3>{},
        [&c](auto &v) { for(auto x : v) if(x.p) c->release_events(1, &x); });
    const auto exec = [&c, prog](
            nngn::Compute::Event *w, nngn::Compute::Event *e) {
        constexpr std::size_t size = 1;
        return c->execute(
            prog, "f", nngn::Compute::ExecFlag::BLOCKING,
            1, &size, &size, {!!w, w, e}, 0u);
    };
    QVERIFY(exec(nullptr, &(*events)[0]));
    if(!(*events)[0].p)
        QCOMPARE((*events)[0].p, nullptr);
    cl_command_type type = {};
    const auto get_type = [](auto e, auto *p) {
        return clGetEventInfo(
            static_cast<cl_event>(e.p), CL_EVENT_COMMAND_TYPE,
            sizeof(*p), p, nullptr) == CL_SUCCESS;
    };
    QVERIFY(get_type((*events)[0], &type));
    QCOMPARE(type, CL_COMMAND_NDRANGE_KERNEL);
    QVERIFY(exec(&(*events)[0], &(*events)[1]));
    if(!(*events)[1].p)
        QCOMPARE((*events)[1].p, nullptr);
    if(!(*events)[2].p)
        QCOMPARE((*events)[2].p, nullptr);
    QVERIFY(get_type((*events)[1], &type));
    QCOMPARE(type, CL_COMMAND_BARRIER);
    QVERIFY(get_type((*events)[2], &type));
    QCOMPARE(type, CL_COMMAND_NDRANGE_KERNEL);
}

void ComputeTest::write_struct() {
    auto c = nngn::Compute::create(nngn::Compute::Backend::OPENCL_BACKEND);
    QVERIFY(c->init());
    const auto prog = c->create_program(
        "struct __attribute__((packed)) S { uint u; float f; float4 v; };\n"
        "__kernel void f(__global struct S *s, __global uint *dst) {\n"
        "    *dst = s->u + s->f + s->v.s0 + s->v.s1 + s->v.s2 + s->v.s3;\n"
        "}\n", "-Werror");
    QVERIFY(prog);
    struct {
        std::uint32_t u; float f; std::array<float, 4> v;
    } s = {1, 2, {3, 4, 5, 6}};
    const auto src = c->create_buffer(
        nngn::Compute::MemFlag::READ_ONLY, sizeof(s), nullptr);
    QVERIFY(src);
    const auto dst = c->create_buffer(
        nngn::Compute::MemFlag::WRITE_ONLY, sizeof(std::uint32_t), nullptr);
    QVERIFY(dst);
    QVERIFY(c->write_struct(src, {}, s.u, s.f, s.v));
    constexpr std::size_t size = 1;
    QVERIFY(c->execute(
        prog, "f", nngn::Compute::ExecFlag::BLOCKING,
        1, &size, &size, {}, src, dst));
    std::uint32_t ret = {};
    QVERIFY(c->read_buffer(dst, 0, sizeof(ret), nngn::as_bytes(&ret), {}));
    QCOMPARE(ret, 21);
}

QTEST_MAIN(ComputeTest)
