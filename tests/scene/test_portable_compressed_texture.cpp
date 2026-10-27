/**************************************************************************/
/*  test_portable_compressed_texture.cpp                                  */
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

TEST_FORCE_LINK(test_portable_compressed_texture)

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/material.h"
#include "scene/resources/portable_compressed_texture.h"
#include "tests/test_utils.h"

namespace TestPortableCompressedTexture {

TEST_CASE("[SceneTree][PortableCompressedTexture2D] Preserve embedded data when duplicating and saving") {
	struct RestoreKeepAll {
		bool previous = PortableCompressedTexture2D::is_keeping_all_compressed_buffers();
		~RestoreKeepAll() { PortableCompressedTexture2D::set_keep_all_compressed_buffers(previous); }
	} restore_keep_all;

	Ref<PortableCompressedTexture2D> texture;
	texture.instantiate();
	SUBCASE("Per texture retention") {
		PortableCompressedTexture2D::set_keep_all_compressed_buffers(false);
		texture->set_keep_compressed_buffer(true);
	}
	SUBCASE("Global retention") {
		PortableCompressedTexture2D::set_keep_all_compressed_buffers(true);
	}

	Ref<Image> image = Image::create_empty(2, 1, false, Image::FORMAT_RGBA8);
	image->set_pixel(0, 0, Color(0.2, 0.4, 0.6, 1.0));
	image->set_pixel(1, 0, Color(0.8, 0.6, 0.4, 1.0));
	texture->create_from_image(image, PortableCompressedTexture2D::COMPRESSION_MODE_LOSSLESS);
	texture->set_size_override(Size2(8, 4));
	const PackedByteArray data = texture->get("_data");
	REQUIRE_FALSE(data.is_empty());

	Ref<Shader> shader;
	shader.instantiate();
	shader->set_code("shader_type spatial; uniform sampler2D albedo;");
	Ref<ShaderMaterial> material;
	material.instantiate();
	material->set_shader(shader);
	material->set_shader_parameter("albedo", texture);

	Ref<ShaderMaterial> copy = material->duplicate(true);
	REQUIRE(copy.is_valid());
	Ref<PortableCompressedTexture2D> copied_texture = copy->get_shader_parameter("albedo");
	REQUIRE(copied_texture.is_valid());
	CHECK(copied_texture != texture);
	CHECK(PackedByteArray(copied_texture->get("_data")) == data);
	CHECK(copied_texture->get_size_override() == Size2(8, 4));

	const String path = TestUtils::get_temp_path("portable_shader_material.res");
	REQUIRE(ResourceSaver::save(copy, path) == OK);
	Ref<ShaderMaterial> loaded = ResourceLoader::load(path, "", ResourceLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	Ref<PortableCompressedTexture2D> loaded_texture = loaded->get_shader_parameter("albedo");
	REQUIRE(loaded_texture.is_valid());
	CHECK(PackedByteArray(loaded_texture->get("_data")) == data);
	CHECK(loaded_texture->get_size_override() == Size2(8, 4));
}

} // namespace TestPortableCompressedTexture
