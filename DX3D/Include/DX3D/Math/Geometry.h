#pragma once
#include <DX3D/Core/Core.h>
#include <vector>
#include <d3d11.h>
namespace dx3d {
    struct MeshDimensions {
        f32 width{ 0 }, height{ 0 }, depth{ 0 }; // depth=0 for 2D quads
    };
    enum class Primitive {
        Triangles,
        Lines
    };
	class Rect
	{
	public:
		Rect() = default;
		Rect(i32 width, i32 height) : left(0), top(0), width(width), height(height) {}
		Rect(i32 left, i32 top, i32 width, i32 height) : left(left), top(top), width(width), height(height) {}
	public:
		i32 left{}, top{}, width{}, height{};
	};
	struct Vec2 { f32 x{}, y{}; };
	class Vec3
	{
	public:
		Vec3() = default;
		Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

		f32 x{}, y{}, z{};
	};
	class Vec4
	{
	public:
		Vec4() = default;
		Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}

		f32 x{}, y{}, z{}, w{};
	};
	struct Vertex
	{
		Vec3 pos;
		Vec3 normal;
		Vec2 uv;
	};
}