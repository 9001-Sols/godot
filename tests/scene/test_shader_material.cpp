/**************************************************************************/
/*  test_shader_material.cpp                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_shader_material)

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "tests/test_utils.h"

namespace TestShaderMaterial {

TEST_CASE("[SceneTree][ShaderMaterial] Preserve source color uniforms when duplicating and saving") {
	Ref<Shader> shader;
	shader.instantiate();
	shader->set_code("shader_type spatial; uniform vec3 tint : source_color = vec3(1.0); uniform sampler2D albedo;");

	Ref<Image> image = Image::create_empty(2, 1, false, Image::FORMAT_RGBA8);
	image->set_pixel(0, 0, Color(0.2, 0.4, 0.6, 1.0));
	image->set_pixel(1, 0, Color(0.8, 0.6, 0.4, 1.0));
	Ref<ImageTexture> texture = ImageTexture::create_from_image(image);

	Variant tint;
	SUBCASE("Vector3") {
		tint = Vector3(0.17, 0.39, 0.73);
	}
	SUBCASE("Vector4") {
		shader->set_code("shader_type spatial; uniform vec4 tint : source_color = vec4(1.0); uniform sampler2D albedo;");
		tint = Vector4(0.17, 0.39, 0.73, 0.51);
	}
	SUBCASE("Color") {
		tint = Color(0.17, 0.39, 0.73, 1.0);
	}

	Ref<ShaderMaterial> material;
	material.instantiate();
	material->set_shader(shader);
	material->set_shader_parameter("tint", tint);
	material->set_shader_parameter("albedo", texture);

	List<PropertyInfo> properties;
	material->get_property_list(&properties);
	CHECK(material->get_shader_parameter("tint") == tint);

	Ref<ShaderMaterial> copy = material->duplicate();
	REQUIRE(copy.is_valid());
	CHECK(material->get_shader_parameter("tint") == tint);
	CHECK(copy->get_shader_parameter("tint") == tint);
	CHECK(copy->get_shader_parameter("albedo") == texture);

	const String path = TestUtils::get_temp_path("shader_material.res");
	REQUIRE(ResourceSaver::save(copy, path) == OK);
	Ref<ShaderMaterial> loaded = ResourceLoader::load(path, "", ResourceLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	CHECK(loaded->get_shader_parameter("tint") == tint);
	Ref<ImageTexture> loaded_texture = loaded->get_shader_parameter("albedo");
	REQUIRE(loaded_texture.is_valid());
	CHECK(loaded_texture->get_image()->get_data() == image->get_data());
}

TEST_CASE("[SceneTree][ShaderMaterial] Reset uniforms whose shader type changed") {
	Ref<Shader> shader;
	shader.instantiate();
	shader->set_code("shader_type spatial; uniform float value = 0.5;");
	Ref<ShaderMaterial> material;
	material.instantiate();
	material->set_shader(shader);
	material->set_shader_parameter("value", Vector3(0.2, 0.4, 0.6));
	List<PropertyInfo> properties;
	material->get_property_list(&properties);
	CHECK(material->get_shader_parameter("value").get_type() != Variant::VECTOR3);
}

} // namespace TestShaderMaterial
